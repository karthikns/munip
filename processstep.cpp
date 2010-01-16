#include "processstep.h"
#include "staff.h"

#include "imagewidget.h"
#include "horizontalrunlengthimage.h"
#include "mainwindow.h"
#include "tools.h"
#include "DataWarehouse.h"

#include <QAction>
#include <QInputDialog>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QRgb>
#include <QSet>
#include <QStack>
#include <QVector>
#include <iostream>

#include <cmath>

namespace Munip
{
        ProcessStep::ProcessStep(const QImage& originalImage, ProcessQueue *processQueue) :
        m_originalImage(originalImage),
        m_processedImage(originalImage),
        m_processQueue(processQueue),
        m_processCompleted(false)
    {
        if (!m_processQueue.isNull())
            m_processQueue->enqueue(this);

        connect(this, SIGNAL(started()), SLOT(slotStarted()));
        connect(this, SIGNAL(ended()), SLOT(slotEnded()));
    }

    ProcessStep::~ProcessStep()
    {
    }

    QImage ProcessStep::originalImage() const
    {
        return m_originalImage;
    }

    QImage ProcessStep::processedImage() const
    {
        if (!m_processCompleted)
            return QImage();
        return m_processedImage;
    }

    ProcessStep* ProcessStep::previousStep() const
    {
        if (!m_processQueue.isNull()) {
            ProcessStep *that = const_cast<ProcessStep*>(this);
            int prevIndex = m_processQueue->indexOf(that)-1;
            if (prevIndex >= 0 && prevIndex < m_processQueue->size())
                return m_processQueue->value(prevIndex);
        }
        return 0;
    }

    ProcessStep* ProcessStep::nextStep() const
    {
        if (!m_processQueue.isNull()) {
            ProcessStep *that = const_cast<ProcessStep*>(this);
            int nextIndex = m_processQueue->indexOf(that)+1;
            if (nextIndex >= 0 && nextIndex < m_processQueue->size())
                return m_processQueue->value(nextIndex);
        }
        return 0;
    }

    ProcessQueue* ProcessStep::processQueue() const
    {
        return m_processQueue;
    }

    void ProcessStep::slotStarted()
    {
        m_processCompleted = false;
    }

    void ProcessStep::slotEnded()
    {
        m_processCompleted = true;
    }

    ProcessStepAction::ProcessStepAction(const QByteArray& className, const QIcon& icon, const QString& caption, QObject *parent) :
        QAction(icon, caption, parent),
        m_className(className)
    {
        connect(this, SIGNAL(triggered()), this, SLOT(execute()));
    }

    void ProcessStepAction::execute()
    {
        MainWindow *main = MainWindow::instance();
        if (!main)
            return;
        ImageWidget *imgWidget = main->activeImageWidget();
        if (!imgWidget)
            return;
        ProcessStep *step = ProcessStepFactory::create(m_className, imgWidget->image());
        if (!step)
            return;

        step->process();

        ImageWidget *processed = new ImageWidget(step->processedImage());
        processed->setWidgetID(IDGenerator::gen());
        processed->setProcessorWidget(imgWidget);

        main->addSubWindow(processed);
    }

    ProcessStep* ProcessStepFactory::create(const QByteArray& className, const QImage& originalImage, ProcessQueue *queue)
    {
        ProcessStep *step = 0;

        if (className == QByteArray("GrayScaleConversion"))
            step = new GrayScaleConversion(originalImage, queue);
        else if (className == QByteArray("MonoChromeConversion"))
            step = new MonoChromeConversion(originalImage, queue);
        else if (className == QByteArray("SkewCorrection"))
            step = new SkewCorrection(originalImage, queue);
        else if (className == QByteArray("StaffLineDetect"))
            step = new StaffLineDetect(originalImage, queue);
        else if (className == QByteArray("StaffLineRemoval"))
            step = new StaffLineRemoval(originalImage, queue);
        else if (className == QByteArray("ImageRotation"))
            step = new ImageRotation(originalImage, queue);

        return step;
    }

    GrayScaleConversion::GrayScaleConversion(const QImage& originalImage, ProcessQueue *queue) : ProcessStep(originalImage, queue)
    {
    }

    void GrayScaleConversion::process()
    {
        emit started();

        do {
            // Do nothing, if the image is invalid or a monochrome.
            if (m_processedImage.format() == QImage::Format_Invalid ||
                m_processedImage.format() == QImage::Format_Mono ||
                m_processedImage.format() == QImage::Format_MonoLSB)
            {
                break;
            }

            // Modify color table if the image is color table
            // base. Otherwise use the slow process of manipulating
            // each and every pixel.
            QVector<QRgb> colorTable = m_processedImage.colorTable();
            if (colorTable.isEmpty()) {
                int pixels = m_processedImage.width() * m_processedImage.height();
                unsigned int *data = reinterpret_cast<unsigned int *>(m_processedImage.bits());
                for (int i = 0; i < pixels; ++i) {
                    int val = qGray(data[i]);
                    data[i] = qRgba(val, val, val, qAlpha(data[i]));
                }
            }
            else {
                for(int i = 0; i < colorTable.size(); ++i) {
                    int gray = qGray(colorTable.at(i));
                    colorTable[i] = qRgb(gray, gray, gray);
                }
            }
        } while(false);

        emit ended();
    }

    MonoChromeConversion::MonoChromeConversion(const QImage& originalImage, ProcessQueue *queue) :
        ProcessStep(originalImage, queue),
        m_threshold(240)
    {
    }

    void MonoChromeConversion::process()
    {
        emit started();
        m_processedImage = Munip::convertToMonochrome(m_originalImage, m_threshold);
        emit ended();
    }

    SkewCorrection::SkewCorrection(const QImage& originalImage, ProcessQueue *queue) :
        ProcessStep(originalImage, queue),
        m_workImage(originalImage),
        m_lineSliceSize(20)//(int)originalImage.width()*0.05)
    {
        Q_ASSERT(m_originalImage.format() == QImage::Format_Mono);
        //m_lineSliceSize = (int)originalImage.width()*0.05;
    }

    void SkewCorrection::process()
    {
        emit started();
        double theta = std::atan(detectSkew());
        mDebug() << Q_FUNC_INFO << theta
                 << theta*(180.0/M_PI) << endl;
        if(theta == 0.0) {
            emit ended();
            return;
        }

        QTransform transform, trueTransform;
        double angle = -180.0/M_PI * theta;
        emit angleCalculated(-angle);
        mDebug() << "                  " << Q_FUNC_INFO << -angle << endl;
        transform.rotate(angle);
        // Find out the true tranformation used (automatically adjusted
        // by QImage::transformed method)
        trueTransform = m_processedImage.trueMatrix(transform, m_processedImage.width(),
                                                    m_processedImage.height());

        // Processed image will have the transformed image rotated by
        // staff skew after following operation. Apart from that it also
        // has black triangular corners produced due to bounding rect
        // extentsion.
        m_processedImage = m_processedImage.transformed(transform, Qt::FastTransformation);
        m_processedImage = Munip::convertToMonochrome(m_processedImage, 240);


        // Calculate the black triangular areas as single polygon.
        const QPolygonF oldImageTransformedRect = trueTransform.map(QPolygonF(QRectF(m_originalImage.rect())));
        const QPolygonF newImageRect = QPolygonF(QRectF(m_processedImage.rect()));
        const QPolygonF remainingBlackTriangularAreas = newImageRect.subtracted(oldImageTransformedRect);

        // Now simply fill the above obtained polygon with white to
        // eliminate the black corner triangle.
        //
        // NOTE: Pen width = 2 ensures there is no faint line garbage
        //       left behind.
        QPainter painter(&m_processedImage);
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(QBrush(Qt::white));
        painter.drawPolygon(remainingBlackTriangularAreas);
        painter.end();

        emit ended();
    }

    double SkewCorrection::detectSkew()
    {
        int x = 0, y = 0;
        const int Black = m_workImage.color(0) == 0xffffffff ? 1 : 0;
        for(x = 0;  x < m_workImage.width(); x++) {
            for(y = 0; y < m_workImage.height(); y++) {
                if(m_originalImage.pixelIndex(x,y) == Black) {
                    QList<QPoint> t;
                    dfs(x, y, t);
                }
            }
        }

        // Computation of the skew with highest frequency
        qSort(m_skewList.begin(), m_skewList.end());

        mDebug() << endl << Q_FUNC_INFO << "Display the list";
        foreach(double skew, m_skewList) {
            mDebug() << skew;
        }
        mDebug() << endl;


        int i = 0, n = m_skewList.size();
        if (n == 0) {
            mWarning() << "Empty skew list encountered";
            return 0.0;
        }

        int modefrequency = 0;
        int maxstartindex = -1, maxendindex = -1;
        while( i <= n-1)
        {
                int runlength = 1;
                double t = m_skewList[i];
                int runvalue = (int) (t * 100);
                while( i + runlength <= n-1 && (int)(m_skewList[i+runlength]*100) == runvalue) {
                    runlength++;
                }
                if(runlength > modefrequency)
                {
                    modefrequency = runlength;
                    maxstartindex = i;
                    maxendindex = i + runlength - 1;
                }
                i += runlength;
        }

        double skew = 0;
        for(int i = maxstartindex; i <= maxendindex; i++)
            skew += m_skewList[i];
        skew /= modefrequency;

        return skew;
    }


    void SkewCorrection::dfs(int x,int y, QList<QPoint> points)
    {
        const int Black = m_workImage.color(0) == 0xffffffff ? 1 : 0;
        const int White = 1 - Black;

        m_workImage.setPixel(x, y, White);

        const bool xPlus1Valid = (x+1 >= 0 && x+1 < m_workImage.width());
        const bool yMinus1Valid = (y-1 >= 0 && y-1 < m_workImage.height());
        const bool yPlus1Valid = (y+1 >= 0 && y+1 < m_workImage.height());


        bool noBlacks = (!xPlus1Valid) ||
                         ((m_workImage.pixelIndex(x+1, y) == White) &&
                          (!yMinus1Valid || m_workImage.pixelIndex(x+1, y-1) == White) &&
                          (!yPlus1Valid || m_workImage.pixelIndex(x+1, y+1) == White));
        if(noBlacks && points.size() >= m_lineSliceSize )
        {
              double skew = findSkew(points);
              m_skewList.push_back(skew);
              points.pop_back();
        }

        if (xPlus1Valid) {
            if(yPlus1Valid && m_workImage.pixelIndex(x+1, y+1) == Black)
            {
                points.push_back(QPoint(x,y));
                dfs(x+1, y+1, points);
            }
            if(m_workImage.pixelIndex(x+1, y) == Black)
            {
                points.push_back(QPoint(x,y));
                dfs(x+1, y, points);
            }
            if(yMinus1Valid && m_workImage.pixelIndex(x+1, y-1))
            {
                points.push_back(QPoint(x,y));
                dfs(x+1, y-1, points);
            }
        }
    }

    double SkewCorrection::findSkew(QList<QPoint>& points)
    {
        QPointF mean = Munip::meanOfPoints(points);
        QList<double> covmat = Munip::covariance(points, mean);
        if(covmat[1] == 0)
            return 0;
        Q_ASSERT(covmat[1] != 0);
        double eigenvalue = Munip::highestEigenValue(covmat);
        double slope = (eigenvalue - covmat[0]) / (covmat[1]);
        return slope;
    }

    StaffLineDetect::StaffLineDetect(const QImage& originalImage, ProcessQueue *queue) :
        ProcessStep(originalImage, queue)
    {
        Q_ASSERT(originalImage.format() == QImage::Format_Mono);
        m_connectedComponentID = 1;
        memset(m_imageMap,0,sizeof(m_imageMap));
    }

    void StaffLineDetect::process()
    {
        emit started();

        m_lineRemovedTracker = QPixmap(m_processedImage.size());
        m_lineRemovedTracker.fill(QColor(Qt::white));
        detectLines();
        //removeStaffLines();
        //constructStaff();

        m_processedImage = m_lineRemovedTracker.toImage();

        emit ended();
    }

    bool StaffLineDetect::checkDiscontinuity(int countWhite )
    {
        if( m_processedImage.width() <= 500 && countWhite >= 5 )
            return true;
        if( m_processedImage.width() > 500 && countWhite >= (int) (0.005*m_processedImage.width()) )
            return true;
        return false;
    }

    bool StaffLineDetect::isLine( int countBlack )
    {
        if( countBlack >= (int) (0.1*m_processedImage.width()) )
            return true;
        return false;
    }

    bool StaffLineDetect::isStaff( int countStaffLines )
    {
        if( countStaffLines == 5 ) //TODO Put this in DataWarehouse
            return true;
        return false;
    }


    void StaffLineDetect::detectLines()
    {
        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
        const int Black = 1 - White;
        int countWhite = 0;
        QPoint start,end;

        for(int y = 0; y < m_processedImage.height(); y++)
        {
            int x = 0;
            while(x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White )
                x++;

            start = QPoint(x,y);
            countWhite = 0;
            while( x < m_processedImage.width() )
            {
                while(x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == Black )
                        x++;

                while( x+countWhite < m_processedImage.width() && m_processedImage.pixelIndex(x+countWhite,y) == White )
                        countWhite++;
                if(checkDiscontinuity(countWhite))
                    end = QPoint(x-1,y);
                else {
                    x += countWhite;
                    continue;
                }

                x+= countWhite;

                Segment segment = Segment(start,end);
                m_segments[y].push_back(segment);


                countWhite = 0;
                start = QPoint(x,y);
            }
        }
       findPaths();

    }

    void StaffLineDetect::findPaths()
    {


       for(int y = 0; y <m_processedImage.height() ; y++)
          for(int i = 0; i < m_segments[y].size(); i++)
          {
               findMaxPath(m_segments[y][i]);
               m_connectedComponentID++;
          }

        QList<Segment> segmentList = m_lookUpTable.uniqueKeys();
        QVector<Segment> paths;
        foreach(Segment p,segmentList)
           paths.push_back(p);

        qSort( paths.begin(),paths.end(),segmentSortByWeight );
        for(int i = 0; i < paths.size(); i++)
            mDebug()<<paths[i].startPos()<<paths[i].endPos()<<m_lookUpTable.value(paths[i]).startPos()<<m_lookUpTable.value(paths[i]).endPos()<<paths[i].destinationPos();

        Segment maxWeightPath = paths[0];

        //prune the segments to find the ones with maximum weight
        int i = 0;
        while( i < paths.size() && paths[i].weight() >= (int)(0.9*maxWeightPath.weight() ) )
            i++;

        paths.remove(i,paths.size()-i);

        // Now construct the lines from optimal segments

        const int size = paths.size();
        mDebug() << Q_FUNC_INFO << " Size = " << size;

        qSort( paths.begin(),paths.end(),segmentSortByConnectedComponentID);
        QSet<int> done;

        i = 0;
        while( i < paths.size() )
        {
            int k = 0;
            if( !done.contains(paths[i].connectedComponentID()))
            {
                int ID = paths[i].connectedComponentID();
                int weight = paths[i].weight();
                StaffLine line(paths[i].startPos(),paths[i].destinationPos(),1);

                while(i+k < paths.size() && paths[i+k].connectedComponentID() == ID && paths[i+k].weight() == weight )
                {
                    line.addSegment(paths[i+k]);

                    Segment s = m_lookUpTable.value(paths[i+k]);

                    while(s.isValid())
                    {
                        line.addSegment(s);
                        s = m_lookUpTable.value(s);
                    }


                    k++;
                }
                done.insert(ID);
                if(m_lineList.isEmpty()||!m_lineList.last().aggregate(line))
                    m_lineList.push_back(line);
            }
            k = (k== 0)?1:k;
            i+=k;
        }
        qSort(m_lineList.begin(),m_lineList.end(),staffLineSort);

        for(int i = 0; i < m_lineList.size(); i++)
                    mDebug() <<Q_FUNC_INFO<< m_lineList[i].startPos() << m_lineList[i].endPos()<<m_lineList[i].boundingBox();
                //m_lineList[i].displaySegments();
           // }

         drawDetectedLines();
         //removeLines();
    }

    Segment StaffLineDetect::findMaxPath(Segment segment)
    {
       if( !segment.isValid()) {
            return segment;
       }
       //mDebug() << Q_FUNC_INFO << segment.startPos() << segment.endPos();

       if( m_lookUpTable.contains(segment) )
        {

           QHash<Segment,Segment> ::iterator i;
           i = m_lookUpTable.find(segment);
           segment = i.key();

           return segment;
        }

        QVector<Segment> segments;
        /*
        segments.push_back(segment.getSegment(QPoint(segment.endPos().x()+1,segment.endPos().y()+1),m_segments[segment.endPos().y()+1]));
        segments.push_back(segment.getSegment(QPoint(segment.endPos().x()+1,segment.endPos().y()-1),m_segments[segment.endPos().y()-1]));
        */
        const int yPlus1 = segment.endPos().y() + 1;
        const QVector<Segment> yPlus1Segments = m_segments[yPlus1];
        const int yMinus1 = segment.endPos().y() - 1;
        const QVector<Segment> yMinus1Segments = m_segments[yMinus1];
        const int startX = segment.endPos().x() + 1;

        const int White = m_originalImage.color(0) == 0xffffffff ? 0 : 1;
        const QPoint InvalidPoint(-1, -1);
        Segment seg(InvalidPoint, InvalidPoint);
        if (yPlus1 < m_processedImage.height()) {
            for (int whiteCount = 0; !checkDiscontinuity(whiteCount); ++whiteCount) {
                if ((startX + whiteCount) >= m_processedImage.width()) break;
                if (m_originalImage.pixelIndex(startX + whiteCount, yPlus1) == White) {
                    continue;
                }
                seg = segment.getSegment(QPoint(startX + whiteCount, yPlus1), yPlus1Segments);
                if (seg.isValid()) break;
            }
        }
        segments.push_back(seg);

        seg = Segment(InvalidPoint, InvalidPoint); // invalidate
        if (yMinus1 >= 0) {
            for (int whiteCount = 0; !checkDiscontinuity(whiteCount); ++whiteCount) {
                if ((startX + whiteCount) >= m_processedImage.width()) break;
                if (m_originalImage.pixelIndex(startX + whiteCount, yMinus1) == White) {
                    continue;
                }
                seg = segment.getSegment(QPoint(startX + whiteCount, yMinus1), yMinus1Segments);
                if (seg.isValid()) break;
            }
        }
        segments.push_back(seg);

        Segment path = findMaxPath(segments[0]);

        int i = 1;
        while( i < segments.size() )
        {
            path = path.maxPath(findMaxPath(segments[i]));
            i++;
        }


       if( path.isValid())
        {
           segment.setDestinationPos(path.destinationPos());
           segment.setConnectedComponentID(path.connectedComponentID());
        }
       else
           segment.setConnectedComponentID(m_connectedComponentID);

        m_lookUpTable.insert(segment,path);
        return segment;
    }



    void StaffLineDetect::constructStaff()
    {

        int i = 0;
        while( i < m_lineList.size())
        {
            Staff s;
            for(int count = 0; count < 5; count++)
                s.addStaffLine(m_lineList[i+count]);
            DataWarehouse ::instance()->appendStaff(s);
            i+=5;
        }

    }

    void StaffLineDetect ::drawDetectedLines()
    {
         QPainter p(&m_lineRemovedTracker);

         int i = 0;

         QSet<Segment> visited;
         while( i< m_lineList.size() )
         {
              p.setPen(QColor(qrand() % 255, qrand()%255, 100+qrand()%155));
             foreach(Segment s,m_lineList[i].segments())
                 while(s.isValid() && !visited.contains(s))
                 {
               // p.drawLine(m_lineList[i].startPos(),m_lineList[i].endPos());
                    visited.insert(s);
                    p.drawLine(s.startPos(),s.endPos());
                    s = m_lookUpTable.value(s);
                }
            i++;
         }

     }

    void StaffLineDetect ::removeLines()
    {
        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;


        QSet<Segment> noisySegmentList;

        for(int i = 0; i < m_lineList.size(); i++)
        {
            QVector<Segment> segmentList = m_lineList[i].segments();
            for(int j = 0; j < segmentList.size();j++)
            {
                Segment s = segmentList[j];
                for(int x = s.startPos().x();x <=s.endPos().x();x++)
                    {
                        QPoint above(x,s.endPos().y()-1);
                        QPoint below(x,s.endPos().y()+1);
                        Segment a,b;

                        if( above.y() >= 0)
                           a = s.getSegment(above,m_segments[above.y()]);

                        if( below.y() < m_processedImage.height())
                            b = s.getSegment(below,m_segments[below.y()]);


                        if(a.isValid() && !noisySegmentList.contains(a))
                        {
                            noisySegmentList.insert(a);
                            setImageMap(a,1,true);
                        }
                        if( b.isValid() && !noisySegmentList.contains(b))
                        {
                            noisySegmentList.insert(b);
                            setImageMap(b,1,true);
                        }
                    }
                    setImageMap(s,1,true);
            }
        }
        for(int y =0; y <m_processedImage.height();y++)
            for(int i = 0; i < m_segments[y].size();i++)
                setImageMap(m_segments[y][i],2,false);

        QList<Segment> list = noisySegmentList.toList();

        foreach(Segment t,list)
            convertSymbol(t);

        for(int i = 0; i < m_lineList.size(); i++)
        {
            QVector<Segment> segmentList = m_lineList[i].segments();
            for(int j = 0; j < segmentList.size();j++)
            {
                Segment s = segmentList[j];
                convertSymbol(s);
            }

        }

        for(int i = 0; i < m_lineList.size();i++)
        {
            QVector<Segment> segmentList = m_lineList[i].segments();
            for(int j = 0; j < segmentList.size();j++)
            {
                Segment s = segmentList[j];
                for(int x=s.startPos().x();x<=s.endPos().x();x++)
                       if( m_imageMap[x][s.endPos().y()] == 1 )
                            m_processedImage.setPixel(x,s.endPos().y(),White);
            }
        }
    }

    void StaffLineDetect::convertSymbol(Segment s)
    {
        int y = s.endPos().y();
        for(int x = s.startPos().x();x<=s.endPos().x();x++)
        {

            if(m_imageMap[x-1][y-1] == 2 || m_imageMap[x][y-1] == 2 || m_imageMap[x+1][y-1] == 2)
                m_imageMap[x][y] = 2;
            else if( m_imageMap[x-1][y+1] == 2 || m_imageMap[x-1][y+1] == 2 || m_imageMap[x+1][y+1] == 2)
                m_imageMap[x][y] = 2;

        }

    }


    void StaffLineDetect::setImageMap(Segment &s,int value,bool replace)
    {
        //if replace flag is false it sets only if the corresponding index is 0
        if(replace)
            for(int x = s.startPos().x();x <= s.endPos().x();x++)
                m_imageMap[x][s.endPos().y()] = value;
        else
             for(int x = s.startPos().x();x <= s.endPos().x();x++)
                 if(!m_imageMap[x][s.endPos().y()])
                    m_imageMap[x][s.endPos().y()] = value;

    }







    StaffLineRemoval::StaffLineRemoval(const QImage& originalImage, ProcessQueue *queue) :
        ProcessStep(originalImage, queue)
    {
        for(int y = 0; y < m_processedImage.height(); y++)
            m_isLine.append(false);
        m_upperLimit = 5;
        m_lineWidthLimit = 0.4;

        Q_ASSERT(originalImage.format() == QImage::Format_Mono);
    }

    void StaffLineRemoval::process()
    {
        emit started();

        detectLines();
        removeStaffLines();
        m_processedImage = m_lineRemovedTracker.toImage();

        emit ended();
    }


    void StaffLineRemoval::detectLines()
    {
        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
        const int Black = 1 - White;
        bool done = false;
        QPoint p1,p2;
        for(int x = 0; x < m_processedImage.width() && !done; x++)
        {
            for(int y = 0; y < m_processedImage.height() && !done; y++)
            {
                if(m_processedImage.pixelIndex(x,y) == Black)
                {
                    p1.setX(x);
                    p1.setY(y);
                    done = 1;
                }
            }
        }

        done = false;

        for(int x = m_processedImage.width()-1; x > 0 && !done; x--)
        {
            for(int y = m_processedImage.height() -1; y > 0 && !done; y--)
            {
                if(m_processedImage.pixelIndex(x,y) == Black)
                {
                    p2.setX(x);
                    p2.setY(y);
                    done = true;
                }
            }
        }
        mDebug()<<Q_FUNC_INFO<<p1.x()<<p2.x();
        int width = p2.x() - p1.x();
        for(int y = 0; y < m_processedImage.height(); y++)
        {
            bool done = false;
            for(int x = 0; x < m_processedImage.width() && !done; x++)
            {
                if(m_processedImage.pixelIndex(x,y) == Black)
                {
                    QPoint start(x,y);
                    int count = 0;
                    QPoint p = start;
                    followLine(p,count);
                    if(count >= m_lineWidthLimit * width)
                    {
                        QPoint end = p;
                        end.setY(start.y());
                        m_lineLocation.push_back(start);
                        m_lineLocation.push_back(end);
                        m_isLine[y] = 1;
                        mDebug()<<start<<end<<count<<width;
                    }
                    done = true;

                }
            }
        }
    }

    void StaffLineRemoval::followLine(QPoint& p,int& count)
    {

        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
        const int Black = 1 - White;

        bool done = false;
        int x = p.x();
        int y = p.y();
        int  nextjump = 0, countabove = 0, countbelow = 0;
        while(!done && x < m_processedImage.width())
        {
            while(x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == Black)
            {
                count++;
                x++;
                p.setX(x);
            }
            int whiterun = 0;
            while( x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White && whiterun < 5 )
            {
                whiterun++;
                if(count >= 160 && m_processedImage.pixelIndex(x,y+1) == Black)
                {
                    count++;
                    countabove++;
                }

                else if(count >= 160 && m_processedImage.pixelIndex(x,y-1) == Black)
                {
                    count++;
                    countbelow++;

                }
                x++;
                p.setX(x);
            }
            if(whiterun >= 5)
                done = true;
            else
            {
                countabove = 0;
                countbelow = 0;
            }

        }

        if((countabove > countbelow) && (countabove || countbelow))
        {
            nextjump = 1;
        }
        else if(countabove || countbelow)
        {
            nextjump = -1;
        }

        if(nextjump && x < m_processedImage.width())
        {
            p.setY(y+nextjump);
            followLine(p,count);
        }

    }

    QVector<Staff> StaffLineRemoval::fillDataStructures()
    {
        QVector<StaffLine> staves;
        QVector<Staff> staffList;
        int thickness = 0;
        int countAddedLines = 0;

        m_lineRemovedTracker = QPixmap(m_processedImage.size());
        m_lineRemovedTracker.fill(QColor(Qt::white));
        QPainter p(&m_lineRemovedTracker);
#if 1
        if (!m_lineLocation.isEmpty() && m_lineLocation.size()%2 != 0) {
            m_lineLocation.append(m_lineLocation[m_lineLocation.size()-1]);
        }
        for (int i=0; i<m_lineLocation.size(); i += 2) {
            p.setPen(QColor(qrand() % 255, qrand()%255, 100+qrand()%155));
            p.drawLine(m_lineLocation[i], m_lineLocation[i+1]);
        }
#endif
        /*
           Parallel StaffLines Functionality to be implemented
           */
        for(int y = 0; y < m_processedImage.height(); y++)
        {
            if(m_isLine[y])
            {
                int temp = y;
                QPoint start = m_lineLocation[0];
                QPoint end = m_lineLocation[1];

                while(m_isLine[temp])
                {
                    temp++;
                    thickness++;
                    if( start.x() > m_lineLocation[0].x())
                        start.setX(m_lineLocation[0].x());
                    if(end.x() < m_lineLocation[1].x())
                        end.setX(m_lineLocation[1].x());

                    m_lineLocation.erase(m_lineLocation.begin());
                    m_lineLocation.erase(m_lineLocation.begin());

                }
                y = temp;
                StaffLine st(start,end,thickness);
                thickness = 0;
                staves.append(st);
                countAddedLines++;
                if(countAddedLines == 5)
                {
                    Staff s(staves[0].startPos(),staves[4].startPos());
                    countAddedLines = 0;
                    s.addStaffLineList(staves);
                    staffList.append(s);
                    staves.clear();
                }
            }
        }

#if 0
        for (int i=0; i<staffList.size(); ++i) {
            QList<StaffLine> staffLines = staffList[i].staffLines();
            p.setPen(QColor(qrand() % 255, qrand()%255, 100+qrand()%155));

            for (int j=0; j<staffLines.size(); ++j) {
                p.drawLine(staffLines[j].startPos(), staffLines[j].endPos());
            }
        }
#endif

        p.end();

        return staffList;
    }

    bool StaffLineRemoval::canBeRemoved(QPoint& p)
    {
        int x = p.x();
        int y = p.y();

        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
        const int Black = 1 - White;

        if(m_processedImage.pixelIndex(x-1,y-1) == Black ||m_processedImage.pixelIndex(x,y-1) == Black || m_processedImage.pixelIndex(x+1,y-1) == Black)
            return false;
        return true;
    }

    void StaffLineRemoval::removeStaffLines()
    {
        QVector<Staff> pageStaffList = fillDataStructures();
        for(int i = 0; i < pageStaffList.size(); i++)
        {
            QVector<StaffLine> staves = pageStaffList[i].staffLines();
            for(int j = 0; j < staves.size(); j++)
            {
                StaffLine staffline = staves[j];
                QPoint s = staffline.startPos();
                QPoint e = staffline.endPos();
                int t = staffline.lineWidth();
                removeFirstLine(s,e);
                s.setY(s.y()+t-1);
                e.setY(s.y()+t-1);
                removeLastLine(s,e);
                s = staffline.startPos();
                e = staffline.endPos();
                for(int k = 1; k < t-1; k++)
                {
                    s.setY(s.y()+1);
                    e.setY(s.y()+1);
                    removeLine(s,e);
                }


            }

        }
    }

    void StaffLineRemoval::removeFirstLine(QPoint& start,QPoint& end)
    {
        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
        const int Black = 1 - White;
        int y = start.y();
        for(int x = start.x(); x <= end.x();x++)
        {
            if(m_processedImage.pixelIndex(x,y) == Black)
            {
                int temp = y-1;

                if(m_processedImage.pixelIndex(x,temp) == Black)
                {
                    if(m_processedImage.pixelIndex(x-1,temp-1) == White && m_processedImage.pixelIndex(x,temp-1) == White && m_processedImage.pixelIndex(x+1,temp-1) == White)
                        m_processedImage.setPixel(x,temp,White);
                }
                if(m_processedImage.pixelIndex(x-1,temp) == White && m_processedImage.pixelIndex(x,temp) == White && m_processedImage.pixelIndex(x+1,temp) == White)
                    m_processedImage.setPixel(x,y,White);
            }
            else if(m_processedImage.pixelIndex(x,y) == White)
            {
                int whiterun = 0,nextjump = 0,countabove = 0,countbelow = 0;
                while( x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White && whiterun < 10)
                {
                    whiterun++;
                    if(m_processedImage.pixelIndex(x,y+1) == Black)
                    {
                        countabove++;
                    }
                    else if(m_processedImage.pixelIndex(x,y-1) == Black )
                    {
                        countbelow++;
                    }
                    x++;
                }
                if(countabove > countbelow &&(countabove||countbelow))
                    nextjump = 1;
                else if(countabove || countbelow)
                    nextjump = -1;

                if(whiterun>= 10 && nextjump && x < m_processedImage.width())
                {
                    y += nextjump;
                }
            }


        }
    }

    void StaffLineRemoval::removeLastLine(QPoint& start,QPoint& end)
    {
        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
        const int Black = 1 - White;
        int y = start.y();
        for(int x = start.x(); x <= end.x();x++)
        {
            if(m_processedImage.pixelIndex(x,y) == Black)
            {
                int temp = y+1;

                if(m_processedImage.pixelIndex(x,temp) == Black)
                {
                    if(m_processedImage.pixelIndex(x-1,temp+1) == White && m_processedImage.pixelIndex(x,temp+1) == White && m_processedImage.pixelIndex(x+1,temp+1) == White)
                        m_processedImage.setPixel(x,temp,White);
                }
                if(m_processedImage.pixelIndex(x-1,temp) == White && m_processedImage.pixelIndex(x,temp) == White && m_processedImage.pixelIndex(x+1,temp) == White)
                    m_processedImage.setPixel(x,y,White);
            }
            else if(m_processedImage.pixelIndex(x,y) == White)
            {
                int whiterun = 0,nextjump = 0,countabove = 0,countbelow = 0;
                while( x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White && whiterun < 10)
                {
                    whiterun++;
                    if(m_processedImage.pixelIndex(x,y+1) == Black)
                    {
                        countabove++;
                    }
                    else if(m_processedImage.pixelIndex(x,y-1) == Black )
                    {
                        countbelow++;
                    }
                    x++;
                }
                if(countabove > countbelow &&(countabove||countbelow))
                    nextjump = 1;
                else if(countabove || countbelow)
                    nextjump = -1;

                if(whiterun>= 10 && nextjump && x < m_processedImage.width())
                {
                    y += nextjump;
                }
            }


        }
    }


    void StaffLineRemoval::removeLine(QPoint& start,QPoint& end)
    {

        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
        const int Black = 1 - White;
        m_upperLimit = abs(end.y() - start.y());

        int y = start.y();
        for(int x = start.x(); x <= end.x();x++)
        {
            QPoint p(x,y);
            if(m_processedImage.pixelIndex(x,y) == Black && canBeRemoved(p))
                m_processedImage.setPixel(x,y,White);
            else if(m_processedImage.pixelIndex(x,y) == White)
            {
                int whiterun = 0,nextjump = 0,countabove = 0,countbelow = 0;
                while( x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White && whiterun < 5)
                {
                    whiterun++;
                    if(m_processedImage.pixelIndex(x,y+1) == Black)
                    {
                        countabove++;
                    }
                    else if(m_processedImage.pixelIndex(x,y-1) == Black )
                    {
                        countbelow++;
                    }
                    x++;
                }
                if(countabove > countbelow &&(countabove||countbelow))
                    nextjump = 1;
                else if(countabove || countbelow)
                    nextjump = -1;

                if(whiterun>= 5 && nextjump && x < m_processedImage.width())
                {
                    y += nextjump;
                }
            }
        }


    }

    const qreal ImageRotation::InvalidAngle = -753;

    ImageRotation::ImageRotation(const QImage& image, ProcessQueue *queue) :
        ProcessStep(image, queue), m_angle(ImageRotation::InvalidAngle)
    {
    }

    ImageRotation::ImageRotation(const QImage& image, qreal _angle, ProcessQueue *queue) :
        ProcessStep(image, queue), m_angle(_angle)
    {
    }

    void ImageRotation::process()
    {
        emit started();
        QImage::Format destFormat = m_originalImage.format();

        if (qFuzzyCompare(m_angle, ImageRotation::InvalidAngle)) {
            bool ok;
            qreal angle = QInputDialog::getDouble(0, tr("Angle"), tr("Enter rotation angle in degrees for image rotation"),
                                         5, -45, 45, 1, &ok);
            if (!ok) {
                angle = 5;
            }

            m_angle = angle;
        }

        QTransform transform;
        transform.rotate(m_angle);
        transform = m_originalImage.trueMatrix(transform, m_originalImage.width(), m_originalImage.height());

        m_processedImage = m_processedImage.transformed(transform, Qt::SmoothTransformation);
        if (destFormat == QImage::Format_Mono) {
            m_processedImage = Munip::convertToMonochrome(m_processedImage, 240);
        } else {
            m_processedImage = m_processedImage.convertToFormat(destFormat);
        }


        // Calculate the black triangular areas as single polygon.
        const QPolygonF oldImageTransformedRect = transform.map(QPolygonF(QRectF(m_originalImage.rect())));
        const QPolygonF newImageRect = QPolygonF(QRectF(m_processedImage.rect()));
        const QPolygonF remainingBlackTriangularAreas = newImageRect.subtracted(oldImageTransformedRect);

        // Now simply fill the above obtained polygon with white to
        // eliminate the black corner triangle.
        //
        // NOTE: Pen width = 2 ensures there is no faint line garbage
        //       left behind.
        QPainter painter(&m_processedImage);
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(QBrush(Qt::white));
        painter.drawPolygon(remainingBlackTriangularAreas);
        painter.end();

        emit ended();
    }
}
