#include "processstep.h"
#include "staff.h"

#include "cluster.h"
#include "datawarehouse.h"
#include "horizontalrunlengthimage.h"
#include "imagewidget.h"
#include "mainwindow.h"
#include "projection.h"
#include "tools.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QProcess>
#include <QRgb>
#include <QSet>
#include <QStack>
#include <QTextStream>
#include <QList>

#include <iostream>
#include <cmath>

namespace Munip
{
    QColor randColor()
    {
        static QList<int> possible;
        if (possible.isEmpty()) {
            for (int i = 0; i < 19; ++i) {
                if (i != Qt::black || i != Qt::white) {
                    possible << i;
                }
            }
        }

        return QColor(Qt::GlobalColor(possible[qrand() % possible.size()]));
    }

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
        if (caption.isEmpty()) {
            setText(QString(className));
        }
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
        QScopedPointer<ProcessStep> step(
                ProcessStepFactory::create(m_className, imgWidget->image()));
        if (step.isNull()) {
            return;
        }

        DataWarehouse::instance()->setWorkImage(imgWidget->image());
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
        else if (className == QByteArray("StaffParamExtraction"))
            step = new StaffParamExtraction(originalImage, queue);
        else if (className == QByteArray("ImageRotation"))
            step = new ImageRotation(originalImage, queue);
        else if (className == QByteArray("ImageCluster"))
            step = new ImageCluster(originalImage, queue);
        else if (className == QByteArray("SymbolAreaExtraction"))
            step = new SymbolAreaExtraction(originalImage, queue);

        return step;
    }

    QList<ProcessStepAction*> ProcessStepFactory::actions(QObject *parent)
    {
        QList<ProcessStepAction*> actions;
        QByteArray classes[] =
        {
            "GrayScaleConversion", "MonoChromeConversion", "SkewCorrection",
            "StaffLineDetect", "StaffLineRemoval", "StaffParamExtraction",
            "ImageRotation", "ImageCluster", "SymbolAreaExtraction"
        };

        int size = (int)((sizeof(classes))/(sizeof(QByteArray)));
        for (int i = 0; i < size; ++i) {
            ProcessStepAction *newAction = new ProcessStepAction(classes[i]);
            newAction->setParent(parent);
            actions << newAction;
        }

        return actions;
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
        } while (false);

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
        if (theta == 0.0) {
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
                if (m_originalImage.pixelIndex(x,y) == Black) {
                    QList<QPoint> t;
                    dfs(x, y, t);
                }
            }
        }

        // Computation of the skew with highest frequency
        qSort(m_skewList.begin(), m_skewList.end());

/*        mDebug() << endl << Q_FUNC_INFO << "Display the list";
        foreach(double skew, m_skewList) {
            mDebug() << skew;
        }
        mDebug() << endl;
*/

        int i = 0, n = m_skewList.size();
        if (n == 0) {
            mWarning() << "Empty skew list encountered";
            return 0.0;
        }

        int modefrequency = 0;
        int maxstartindex = -1, maxendindex = -1;
        while (i <= n-1)
        {
            int runlength = 1;
            double t = m_skewList[i];
            int runvalue = (int) (t * 100);
            while (i + runlength <= n-1 && (int)(m_skewList[i+runlength]*100) == runvalue) {
                runlength++;
            }
            if (runlength > modefrequency)
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
        if (noBlacks && points.size() >= m_lineSliceSize)
        {
            double skew = findSkew(points);
            m_skewList.push_back(skew);
            points.pop_back();
        }

        if (xPlus1Valid) {
            if (yPlus1Valid && m_workImage.pixelIndex(x+1, y+1) == Black)
            {
                points.push_back(QPoint(x,y));
                dfs(x+1, y+1, points);
            }
            if (m_workImage.pixelIndex(x+1, y) == Black)
            {
                points.push_back(QPoint(x,y));
                dfs(x+1, y, points);
            }
            if (yMinus1Valid && m_workImage.pixelIndex(x+1, y-1))
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
        if (covmat[1] == 0)
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
        //memset(m_imageMap,0,sizeof(m_imageMap));
    }

    void StaffLineDetect::process()
    {
        emit started();

        m_lineRemovedTracker = QPixmap(m_processedImage.size());
        m_rectTracker = QPixmap(m_processedImage.size());
        m_lineRemovedTracker.fill(QColor(Qt::white));
        m_rectTracker.fill(QColor(Qt::white));
        m_symbolMap = m_processedImage;
        detectLines();
        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
        const int Black = 1 - White;

        //removeStaffLines();
        constructStaff();

       // m_processedImage = m_lineRemovedTracker.toImage();
        QPainter p(&m_rectTracker);
        for(int x = 0; x < m_symbolMap.width(); x++)
            for(int y = 0; y< m_symbolMap.height();y++)
                if(m_symbolMap.pixelIndex(x,y) == Black)
                    p.drawPoint(x,y);
        MainWindow::instance()->addSubWindow(new ImageWidget(m_rectTracker.toImage()));
        MainWindow::instance()->addSubWindow(new ImageWidget(m_symbolMap));

        emit ended();
    }

    bool StaffLineDetect::checkDiscontinuity(int countWhite)
    {
        if ( m_processedImage.width() <= 500 && countWhite >= 5)
            return true;
        if ( m_processedImage.width() > 500 && countWhite >= (int) (0.005*m_processedImage.width()))
            return true;
        return false;
    }

    bool StaffLineDetect::isLine( int countBlack)
    {
        if ( countBlack >= (int) (0.1*m_processedImage.width()))
            return true;
        return false;
    }

    bool StaffLineDetect::isStaff( int countStaffLines)
    {
        if ( countStaffLines == 5) //TODO Put this in DataWarehouse
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
            while (x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White)
                x++;

            start = QPoint(x,y);
            countWhite = 0;
            while (x < m_processedImage.width())
            {
                while (x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == Black)
                    x++;

                while (x+countWhite < m_processedImage.width() && m_processedImage.pixelIndex(x+countWhite,y) == White)
                    countWhite++;
                if (checkDiscontinuity(countWhite))
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
        QList<Segment> paths;
        foreach(Segment p,segmentList)
            paths.push_back(p);

        qSort( paths.begin(),paths.end(),segmentSortByWeight);
        for(int i = 0; i < paths.size(); i++)
            mDebug()<<paths[i].startPos()<<paths[i].endPos()<<m_lookUpTable.value(paths[i]).startPos()<<m_lookUpTable.value(paths[i]).endPos()<<paths[i].destinationPos();

        Segment maxWeightPath = paths[0];

        //prune the segments to find the ones with maximum weight
        int i = 0;
        while (i < paths.size() && paths[i].weight() >= (int)(0.9*maxWeightPath.weight()))
            i++;

        paths.erase(paths.begin() + i, paths.end());

        // Now construct the lines from optimal segments

        const int size = paths.size();
        mDebug() << Q_FUNC_INFO << " Size = " << size;

        qSort( paths.begin(),paths.end(),segmentSortByConnectedComponentID);
        QSet<int> done;

        i = 0;
        while (i < paths.size())
        {
            int k = 0;
            if ( !done.contains(paths[i].connectedComponentID()))
            {
                int ID = paths[i].connectedComponentID();
                int weight = paths[i].weight();
                StaffLine line(paths[i].startPos(),paths[i].destinationPos(),1);

                while (i+k < paths.size() && paths[i+k].connectedComponentID() == ID && paths[i+k].weight() == weight)
                {
                    line.addSegment(paths[i+k]);

                    Segment s = m_lookUpTable.value(paths[i+k]);

                    while (s.isValid())
                    {
                        line.addSegment(s);
                        s = m_lookUpTable.value(s);
                    }


                    k++;
                }
                done.insert(ID);
                if (m_lineList.isEmpty()||!m_lineList.last().aggregate(line))
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
    if ( !segment.isValid()) {
        return segment;
    }
    //mDebug() << Q_FUNC_INFO << segment.startPos() << segment.endPos();

    if ( m_lookUpTable.contains(segment))
    {

        QHash<Segment,Segment>::iterator i;
        i = m_lookUpTable.find(segment);
        segment = i.key();

        return segment;
    }

    QList<Segment> segments;
    /*
       segments.push_back(segment.getSegment(QPoint(segment.endPos().x()+1,segment.endPos().y()+1),m_segments[segment.endPos().y()+1]));
       segments.push_back(segment.getSegment(QPoint(segment.endPos().x()+1,segment.endPos().y()-1),m_segments[segment.endPos().y()-1]));
       */
    const int yPlus1 = segment.endPos().y() + 1;
    const QList<Segment> yPlus1Segments = m_segments[yPlus1];
    const int yMinus1 = segment.endPos().y() - 1;
    const QList<Segment> yMinus1Segments = m_segments[yMinus1];
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
    while (i < segments.size())
    {
        path = path.maxPath(findMaxPath(segments[i]));
        i++;
    }


    if ( path.isValid())
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
    DataWarehouse::instance()->clearStaff();
    while (i < m_lineList.size())
    {
        Staff s;

        for(int count = 0; count < 5 && i+count < m_lineList.size(); count++)
        {
            m_lineList[i+count].sortSegments();
            s.addStaffLine(m_lineList[i+count]);
        }

        s.setStartPos(s.staffLines()[0].startPos());
        s.setEndPos(s.staffLines()[s.staffLines().size()-1].endPos());
        s.setBoundingRect(findStaffBoundingRect(s));

        DataWarehouse ::instance()->appendStaff(s);
        //mDebug() <<Q_FUNC_INFO<<s.boundingRect().topLeft()<<s.boundingRect().bottomRight()<<s.boundingRect();
        identifySymbolRegions(s);
        i+=5;


    }

}


QRect StaffLineDetect::findStaffBoundingRect(const Staff& s)
{
    QImage workImage = m_processedImage;
    StaffLine firstLine =  s.staffLines()[0];
    StaffLine lastLine = s.staffLines()[s.staffLines().size()-1];

    int maxTopHeight = firstLine.startPos().y();
    int maxBottomHeight = lastLine.endPos().y();

    foreach(Segment s,firstLine.segments())
    {

        for(int x = s.startPos().x(); x <= s.endPos().x();x++)
        {
            int topHeight = findTopHeight(QPoint(x,s.startPos().y()),workImage);
            if(topHeight < maxTopHeight)
                maxTopHeight = topHeight;
        }
    }

    foreach(Segment s,lastLine.segments())
    {
        for(int x = s.startPos().x();x<=s.endPos().x();x++)
        {
            int bottomHeight = findBottomHeight(QPoint(x,s.startPos().y()),workImage);
            if(bottomHeight > maxBottomHeight)
                maxBottomHeight = bottomHeight;
        }
    }


    return QRect(QPoint(firstLine.startPos().x(),maxTopHeight),QPoint(lastLine.endPos().x(),maxBottomHeight));

}

int StaffLineDetect::findTopHeight(QPoint pos,QImage& workImage)
{
    const int White = workImage.color(0) == 0xffffffff ? 0 : 1;
    const int Black = 1 - White;
    int t1,t2,t3;
    t1 = t2 = t3 = 1<<30;

    if(workImage.pixelIndex(pos) == White)
        return pos.y();


    workImage.setPixel(pos,White);
    bool flag = false;

    if( pos.y() - 1 > 0)
    {
        if( workImage.pixelIndex(pos.x(),pos.y()-1)== Black)
        {
            flag = true;
            t2 = findTopHeight(QPoint(pos.x(),pos.y()-1),workImage);
        }

        if(pos.x()-1 > 0 && workImage.pixelIndex(pos.x()-1,pos.y()-1) == Black)
        {
            flag = true;
            t1= findTopHeight(QPoint(pos.x()-1,pos.y()-1),workImage);
        }

        if( pos.x()+1 < workImage.width() && workImage.pixelIndex(pos.x()+1,pos.y()-1) == Black)
        {
            flag = true;
            t3 = findTopHeight(QPoint(pos.x()+1,pos.y()-1),workImage);
        }

    }
    if(!flag)
        return pos.y();

    return qMin(t1,qMin(t2,t3));
}

int StaffLineDetect::findBottomHeight(QPoint pos,QImage& workImage)
{
    const int White = workImage.color(0) == 0xffffffff ? 0 : 1;
    const int Black = 1 - White;
    int t1,t2,t3;

    t1 = t2 = t3 = -1;

    if(workImage.pixelIndex(pos) == White)
        return pos.y();

    workImage.setPixel(pos,White);
    bool flag = false;

    if( pos.y()+1 < workImage.height() )
    {
        if( workImage.pixelIndex(pos.x(),pos.y()+1) == Black)
        {
            flag = true;
            t3 = findBottomHeight(QPoint(pos.x(),pos.y()+1),workImage);
        }

        if(pos.x()+1 < workImage.width() && workImage.pixelIndex(pos.x()+1,pos.y()+1) == Black)
        {
            flag = true;
            t1 = findBottomHeight(QPoint(pos.x()+1,pos.y()+1),workImage);
        }
        if(pos.x()-1 > 0 && workImage.pixelIndex(pos.x()-1,pos.y()+1) == Black)
        {
            flag = true;
            t2 = findBottomHeight(QPoint(pos.x()-1,pos.y()+1),workImage);
        }

    }
    if(!flag)
        return pos.y();

    return qMax(t1,qMax(t2,t3));
}

void StaffLineDetect::drawDetectedLines()
{
    const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
    QPainter p(&m_lineRemovedTracker);

    int i = 0;


    QSet<Segment> visited;
    while (i< m_lineList.size())
    {
        p.setPen(QColor(qrand() % 255, qrand()%255, 100+qrand()%155));
        foreach(Segment s,m_lineList[i].segments())
            while (s.isValid() && !visited.contains(s))
            {

                visited.insert(s);
                p.drawLine(s.startPos(),s.endPos());

                for(int x = s.startPos().x(); x <= s.endPos().x();x++)
                    m_symbolMap.setPixel(x,s.startPos().y(),White);
                s = m_lookUpTable.value(s);
            }
        i++;
    }

    m_lineMap = m_lineRemovedTracker.toImage();
    ProcessStep *step = ProcessStepFactory::create("MonoChromeConversion",m_lineMap,0);
    step->process();
    m_lineMap = step->processedImage();
}



QRect StaffLineDetect::aggregateAdjacentRegions(QRect symbolRect1,QRect symbolRect2)
{
    if(symbolRect1.intersects(symbolRect2) )
    {
        mDebug()<<"hii"<<symbolRect1<<symbolRect2;
        QRect aggregatedRect = symbolRect1;
        if(aggregatedRect.left() > symbolRect2.left())
           aggregatedRect.setLeft(symbolRect2.left());


       if(aggregatedRect.right() < symbolRect2.right())
           aggregatedRect.setRight(symbolRect2.right());


       if(aggregatedRect.top() > symbolRect2.top())
           aggregatedRect.setTop(symbolRect2.top());


       if(aggregatedRect.bottom() < symbolRect2.bottom())
           aggregatedRect.setBottom(symbolRect2.bottom());
       return aggregatedRect;


    }
    if(symbolRect1.contains(symbolRect2))
        return symbolRect1;
    if(symbolRect2.contains(symbolRect1))
        return symbolRect2;

    return QRect(QPoint(-1,-1),QPoint(-1,-1));

}

QRect StaffLineDetect::aggregateSymbolRects( QRect symbolRect1, QRect symbolRect2)
{
    const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
    if(symbolRect1.bottom() > symbolRect2.bottom())
    {
        QRect temp = symbolRect1;
        symbolRect1 = symbolRect2;
        symbolRect2 = temp;
    }

    QRect aggregatedRect = symbolRect1;
    int y = symbolRect1.bottom();
    int x = symbolRect1.left();
    y++;
    bool isLine = true;
    int vicinity = 0;
    while(isLine)
    {
        while(x <= symbolRect1.right() && y <= m_processedImage.height() && m_lineMap.pixelIndex(x,y) == White)
            x++;
        if(x > symbolRect1.right())
            isLine = false;
        else
        {
            aggregatedRect.setBottom(y);
            y++;
            x = symbolRect1.left();
            vicinity++;
        }
    }
    aggregatedRect.setBottom(y);


    if(aggregatedRect.bottom() >= symbolRect2.top() && symbolRect2.left() >= aggregatedRect.left()-vicinity && symbolRect2.left() <= aggregatedRect.right()+vicinity)
    {
         if(aggregatedRect.left() > symbolRect2.left())
            aggregatedRect.setLeft(symbolRect2.left());


        if(aggregatedRect.right() < symbolRect2.right())
            aggregatedRect.setRight(symbolRect2.right());


        if(aggregatedRect.top() > symbolRect2.top())
            aggregatedRect.setTop(symbolRect2.top());


        if(aggregatedRect.bottom() < symbolRect2.bottom())
            aggregatedRect.setBottom(symbolRect2.bottom());

          //mDebug()<<true<<aggregatedRect.topLeft()<<aggregatedRect.bottomRight()<<aggregatedRect;//<<symbolRect2.topLeft()<<symbolRect2.bottomRight()<<symbolRect2;

        return aggregatedRect;

    }

    return QRect(QPoint(-1,-1),QPoint(-1,-1));
}

void StaffLineDetect::aggregateSymbolRegion()
{

    QList<QRect> symbolRegions = m_symbolRegions;
    for(int i = 0; i < symbolRegions.size()-1 ; i++)
    {
        if(symbolRegions[i] == QRect(QPoint(-1,-1),QPoint(-1,-1)))
            continue;

        int count = 1;
        while(i+count < symbolRegions.size() && symbolRegions[i+count].left() >= symbolRegions[i].left() && symbolRegions[i+count].left() <= symbolRegions[i].right())
        {
           QRect aggregatedRect = aggregateSymbolRects(symbolRegions[i],symbolRegions[i+count]);
           if(aggregatedRect.topLeft() == QPoint(-1,-1) && aggregatedRect.bottomRight() == QPoint(-1,-1))
           {
               count++;
               continue;
           }

           symbolRegions[i] = aggregatedRect;
           symbolRegions[i+count] = QRect(QPoint(-1,-1),QPoint(-1,-1));
           count++;

        }
    }

    qSort(symbolRegions.begin(),symbolRegions.end(),symbolRectSort);

    for(int i = 0; i < symbolRegions.size()-1;i++)
    {
        if(symbolRegions[i] == QRect(QPoint(-1,-1),QPoint(-1,-1)))
            continue;

        int count = 1;
        while(i+count < symbolRegions.size() && symbolRegions[i+count].left() >= symbolRegions[i].left() && symbolRegions[i+count].left() <= symbolRegions[i].right())
        {
           QRect aggregatedRect = aggregateAdjacentRegions(symbolRegions[i],symbolRegions[i+count]);
           if(aggregatedRect.topLeft() == QPoint(-1,-1) && aggregatedRect.bottomRight() == QPoint(-1,-1))
           {
               count++;
               continue;
           }

           symbolRegions[i] = aggregatedRect;
           symbolRegions[i+count] = QRect(QPoint(-1,-1),QPoint(-1,-1));
           count++;

        }
    }
    m_symbolRegions.clear();
    foreach(QRect symbolRegion,symbolRegions)
    {
        if(symbolRegion.topLeft() == QPoint(-1,-1) && symbolRegion.bottomRight() == QPoint(-1,-1))
            continue;
        m_symbolRegions.push_back(symbolRegion);
        mDebug()<<Q_FUNC_INFO<<symbolRegion<<symbolRegion.topLeft()<<symbolRegion.bottomRight();
    }

}



void StaffLineDetect::identifySymbolRegions(const Staff &staff)
{
    QRect rect = staff.boundingRect();
    QPoint topLeft = rect.topLeft();
    QPoint bottomRight = rect.bottomRight();
    QQueue<Segment> segmentQueue;
    QSet<Segment> segmentSet;
    QPainter p(&m_rectTracker);

    StaffCleanUp(staff);
    QImage workImage = m_symbolMap;
    const int White = workImage.color(0) == 0xffffffff ? 0 : 1;
    const int Black = 1 - White;

    for(int y = topLeft.y(); y <= bottomRight.y(); y++)
        for(int x = topLeft.x(); x <= bottomRight.x(); x++)

            if(workImage.pixelIndex(x,y) == Black )
            {
                Segment segment;
                segment.setStartPos(QPoint(x,y));
                while(x < bottomRight.x() && workImage.pixelIndex(x,y) == Black)
                    x++;
                segment.setEndPos(QPoint(x-1,y));
                segmentQueue.push_back(segment);
                QRect symbolRect(segment.startPos(),segment.endPos());

                while(!segmentQueue.empty())
                {
                    Segment segment = segmentQueue.front();

                    segmentQueue.pop_front();
                    if(segmentSet.contains(segment))
                        continue;

                    segmentSet.insert(segment);
                    for(int x = segment.startPos().x(); x <= segment.endPos().x();x++)
                        workImage.setPixel(x,segment.startPos().y(),White);

                    if(segment.startPos().y() > symbolRect.bottom())
                        symbolRect.setBottom(segment.startPos().y());

                    if(segment.startPos().y() < symbolRect.top())
                        symbolRect.setTop(segment.startPos().y());

                    if(segment.startPos().x() < symbolRect.left())
                        symbolRect.setLeft(segment.startPos().x());

                    if(segment.endPos().x() > symbolRect.right())
                        symbolRect.setRight(segment.endPos().x());



                    QList<Segment> segments = findAdjacentSymbolSegments(segment,workImage);
                    if(segments.size() > 0)
                        segmentQueue.append(segments);
                }

                m_symbolRegions.push_back(symbolRect);


            }

    qSort(m_symbolRegions.begin(),m_symbolRegions.end(),symbolRectSort);
    aggregateSymbolRegion();

    foreach(QRect rectRegion,m_symbolRegions)
    {
           p.setPen(QColor(qrand() % 255, qrand()%255, 100+qrand()%155));
           p.drawRect(rectRegion);
    }

}

void StaffLineDetect::StaffCleanUp(const Staff &staff)
{
    const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
    const int Black = 1 - White;
    QPoint removeMatrix[4][3];
    for(int i =0; i < 4; i++)
        for(int j =0; j <3;j++)
            removeMatrix[i][j] = QPoint(-1,-1);

    for(int y = staff.boundingRect().topLeft().y(); y <= staff.boundingRect().bottomRight().y(); y++)
    {
        for(int x = staff.boundingRect().topLeft().x(); x <= staff.boundingRect().bottomRight().x(); x++)
        {
            if(m_symbolMap.pixelIndex(x,y) == Black)
            {

                /*
                 * THE MATRIX APPROACH-- A WOW ALGO FOR CLEANUP BUT NEEDS REFINEMENT*/


                removeMatrix[1][1] = QPoint(x,y);
                bool firstRowEmpty = true;
                bool lastRowEmpty = true;

                if(y-1 > 0)
                {
                    if(x-1 > 0 && m_symbolMap.pixelIndex(x-1,y-1) == Black)
                    {
                        removeMatrix[0][0] = QPoint(x-1,y-1);
                        firstRowEmpty = false;
                    }
                    if(m_symbolMap.pixelIndex(x,y-1) == Black)
                    {
                        removeMatrix[0][1] = QPoint(x,y-1);
                        firstRowEmpty = false;
                    }
                    if(x+1 < m_processedImage.width() && m_symbolMap.pixelIndex(x+1,y-1) == Black)
                    {
                        removeMatrix[0][2] = QPoint(x+1,y-1);
                        firstRowEmpty = false;
                    }
                }

                if(y+1 < m_processedImage.height())
                {
                    if(x-1 > 0 && m_symbolMap.pixelIndex(x-1,y+1) == Black)
                        removeMatrix[2][0] = QPoint(x-1,y+1);
                    if(m_symbolMap.pixelIndex(x,y+1) == Black)
                        removeMatrix[2][1] = QPoint(x,y+1);
                    if(x+1 < m_processedImage.height() && m_symbolMap.pixelIndex(x+1,y+1) == Black)
                        removeMatrix[2][2] = QPoint(x+1,y+1);
                }

                if(y+2 < m_processedImage.height())
                {
                    if(x-1 > 0 && m_symbolMap.pixelIndex(x-1,y+2) == Black)
                    {
                        removeMatrix[3][0] = QPoint(x-1,y+2);
                        lastRowEmpty = false;
                    }
                    if(m_symbolMap.pixelIndex(x,y+2) == Black)
                    {
                        removeMatrix[3][1] = QPoint(x,y+2);
                        lastRowEmpty = false;
                    }
                    if(x+1 < m_processedImage.height() && m_symbolMap.pixelIndex(x+1,y+2) == Black)
                    {
                        removeMatrix[3][2] = QPoint(x+1,y+2);
                        lastRowEmpty = false;
                    }
                }

                if(firstRowEmpty && lastRowEmpty)
                {

                    if(removeMatrix[2][1] != QPoint(-1,-1))
                        m_symbolMap.setPixel(removeMatrix[2][1],White);
                    m_symbolMap.setPixel(x,y,White);
                }
                for(int i = 0; i < 4; i++)
                    for(int j =0;j<3;j++)
                        removeMatrix[i][j] = QPoint(-1,-1);

            }
        }
    }



}


QList<Segment> StaffLineDetect::findTopSegments(Segment segment, QImage &workImage)
{
    const int White = workImage.color(0) == 0xffffffff ? 0 : 1;
    const int Black = 1 - White;

    int x = segment.startPos().x()-1;
    int y = segment.startPos().y()-1;


    QList<Segment> topSegments;

    if( y < 0)
        return topSegments;

    if(workImage.pixelIndex(x,y) == Black)
    {
        while(x >= 0 && workImage.pixelIndex(x,y) == Black)
            x--;
        Segment s;
        s.setStartPos(QPoint(x+1,y));
        while(x < workImage.width() && workImage.pixelIndex(x,y) == Black)
            x++;
        s.setEndPos(QPoint(x-1,y));
        topSegments.push_back(s);

    }
    while(x <= segment.endPos().x()+1)
    {
        while(x<= segment.endPos().x()+1 && workImage.pixelIndex(x,y) == White)
            x++;
        Segment s;
        s.setStartPos(QPoint(x,segment.endPos().y()));
        while(x < workImage.width() && workImage.pixelIndex(x,y) == Black)
            x++;
        s.setEndPos(QPoint(x,segment.endPos().y()));
        topSegments.push_back(s);
    }
    return topSegments;
}

QList<Segment> StaffLineDetect::findBottomSegments(Segment segment, QImage &workImage)
{
    const int White = workImage.color(0) == 0xffffffff ? 0 : 1;
    const int Black = 1 - White;

    int x = segment.startPos().x()-1;
    int y = segment.startPos().y()+1;
    QList<Segment> bottomSegments;

    if( y >= workImage.height() || x < 0 )
        return bottomSegments;

    if(workImage.pixelIndex(x,y) == Black)
    {
        while(x >= 0 && workImage.pixelIndex(x,y) == Black)
            x--;
        Segment s;
        s.setStartPos(QPoint(x+1,y));
        x = segment.startPos().x()-1;
        while(x <= segment.endPos().x()+1 && workImage.pixelIndex(x,y) == Black)
            x++;
        s.setEndPos(QPoint(x-1,y));
        bottomSegments.push_back(s);

    }
    while(x < workImage.width() && x <= segment.endPos().x()+1)
    {
        while(x < workImage.width() && x<= segment.endPos().x()+1 && workImage.pixelIndex(x,y) == White)
            x++;
        if(x <= segment.endPos().x()+1)
        {
            Segment s;
            s.setStartPos(QPoint(x,y));
            while(x < workImage.width() && workImage.pixelIndex(x,y) == Black)
                x++;
            s.setEndPos(QPoint(x-1,y));
            bottomSegments.push_back(s);
        }
    }

    return bottomSegments;
}

QList<Segment> StaffLineDetect::findAdjacentSymbolSegments(Segment segment,QImage& workImage)
{
    return findBottomSegments(segment,workImage);
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
            if (m_processedImage.pixelIndex(x,y) == Black)
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
            if (m_processedImage.pixelIndex(x,y) == Black)
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
            if (m_processedImage.pixelIndex(x,y) == Black)
            {
                QPoint start(x,y);
                int count = 0;
                QPoint p = start;
                followLine(p,count);
                if (count >= m_lineWidthLimit * width)
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
    while (!done && x < m_processedImage.width())
    {
        while (x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == Black)
        {
            count++;
            x++;
            p.setX(x);
        }
        int whiterun = 0;
        while (x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White && whiterun < 5)
        {
            whiterun++;
            if (count >= 160 && m_processedImage.pixelIndex(x,y+1) == Black)
            {
                count++;
                countabove++;
            }

            else if (count >= 160 && m_processedImage.pixelIndex(x,y-1) == Black)
            {
                count++;
                countbelow++;

            }
            x++;
            p.setX(x);
        }
        if (whiterun >= 5)
            done = true;
        else
        {
            countabove = 0;
            countbelow = 0;
        }

    }

    if ((countabove > countbelow) && (countabove || countbelow))
    {
        nextjump = 1;
    }
    else if (countabove || countbelow)
    {
        nextjump = -1;
    }

    if (nextjump && x < m_processedImage.width())
    {
        p.setY(y+nextjump);
        followLine(p,count);
    }

}

QList<Staff> StaffLineRemoval::fillDataStructures()
{
    QList<StaffLine> staves;
    QList<Staff> staffList;
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
        if (m_isLine[y])
        {
            int temp = y;
            QPoint start = m_lineLocation[0];
            QPoint end = m_lineLocation[1];

            while (m_isLine[temp])
            {
                temp++;
                thickness++;
                if ( start.x() > m_lineLocation[0].x())
                    start.setX(m_lineLocation[0].x());
                if (end.x() < m_lineLocation[1].x())
                    end.setX(m_lineLocation[1].x());

                m_lineLocation.erase(m_lineLocation.begin());
                m_lineLocation.erase(m_lineLocation.begin());

            }
            y = temp;
            StaffLine st(start,end,thickness);
            thickness = 0;
            staves.append(st);
            countAddedLines++;
            if (countAddedLines == 5)
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

    if (m_processedImage.pixelIndex(x-1,y-1) == Black ||m_processedImage.pixelIndex(x,y-1) == Black || m_processedImage.pixelIndex(x+1,y-1) == Black)
        return false;
    return true;
}

void StaffLineRemoval::removeStaffLines()
{
    QList<Staff> pageStaffList = fillDataStructures();
    for(int i = 0; i < pageStaffList.size(); i++)
    {
        QList<StaffLine> staves = pageStaffList[i].staffLines();
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
        if (m_processedImage.pixelIndex(x,y) == Black)
        {
            int temp = y-1;

            if (m_processedImage.pixelIndex(x,temp) == Black)
            {
                if (m_processedImage.pixelIndex(x-1,temp-1) == White && m_processedImage.pixelIndex(x,temp-1) == White && m_processedImage.pixelIndex(x+1,temp-1) == White)
                    m_processedImage.setPixel(x,temp,White);
            }
            if (m_processedImage.pixelIndex(x-1,temp) == White && m_processedImage.pixelIndex(x,temp) == White && m_processedImage.pixelIndex(x+1,temp) == White)
                m_processedImage.setPixel(x,y,White);
        }
        else if (m_processedImage.pixelIndex(x,y) == White)
        {
            int whiterun = 0,nextjump = 0,countabove = 0,countbelow = 0;
            while (x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White && whiterun < 10)
            {
                whiterun++;
                if (m_processedImage.pixelIndex(x,y+1) == Black)
                {
                    countabove++;
                }
                else if (m_processedImage.pixelIndex(x,y-1) == Black)
                {
                    countbelow++;
                }
                x++;
            }
            if (countabove > countbelow &&(countabove||countbelow))
                nextjump = 1;
            else if (countabove || countbelow)
                nextjump = -1;

            if (whiterun>= 10 && nextjump && x < m_processedImage.width())
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
        if (m_processedImage.pixelIndex(x,y) == Black)
        {
            int temp = y+1;

            if (m_processedImage.pixelIndex(x,temp) == Black)
            {
                if (m_processedImage.pixelIndex(x-1,temp+1) == White && m_processedImage.pixelIndex(x,temp+1) == White && m_processedImage.pixelIndex(x+1,temp+1) == White)
                    m_processedImage.setPixel(x,temp,White);
            }
            if (m_processedImage.pixelIndex(x-1,temp) == White && m_processedImage.pixelIndex(x,temp) == White && m_processedImage.pixelIndex(x+1,temp) == White)
                m_processedImage.setPixel(x,y,White);
        }
        else if (m_processedImage.pixelIndex(x,y) == White)
        {
            int whiterun = 0,nextjump = 0,countabove = 0,countbelow = 0;
            while (x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White && whiterun < 10)
            {
                whiterun++;
                if (m_processedImage.pixelIndex(x,y+1) == Black)
                {
                    countabove++;
                }
                else if (m_processedImage.pixelIndex(x,y-1) == Black)
                {
                    countbelow++;
                }
                x++;
            }
            if (countabove > countbelow &&(countabove||countbelow))
                nextjump = 1;
            else if (countabove || countbelow)
                nextjump = -1;

            if (whiterun>= 10 && nextjump && x < m_processedImage.width())
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
        if (m_processedImage.pixelIndex(x,y) == Black && canBeRemoved(p))
            m_processedImage.setPixel(x,y,White);
        else if (m_processedImage.pixelIndex(x,y) == White)
        {
            int whiterun = 0,nextjump = 0,countabove = 0,countbelow = 0;
            while (x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White && whiterun < 5)
            {
                whiterun++;
                if (m_processedImage.pixelIndex(x,y+1) == Black)
                {
                    countabove++;
                }
                else if (m_processedImage.pixelIndex(x,y-1) == Black)
                {
                    countbelow++;
                }
                x++;
            }
            if (countabove > countbelow &&(countabove||countbelow))
                nextjump = 1;
            else if (countabove || countbelow)
                nextjump = -1;

            if (whiterun>= 5 && nextjump && x < m_processedImage.width())
            {
                y += nextjump;
            }
        }
    }


}

StaffParamExtraction::StaffParamExtraction(const QImage& originalImage,
        bool drawGraph,
        ProcessQueue *queue) :
    ProcessStep(originalImage, queue),
    m_staffSpaceHeight(4,6),
    m_staffLineHeight(1, 3),
    m_drawGraph(drawGraph)
{
    Q_ASSERT(originalImage.format() == QImage::Format_Mono);
    // Ensure non zero dimension;
    Q_ASSERT(originalImage.height() * originalImage.width() > 0);
}

StaffParamExtraction::StaffParamExtraction(const QImage& originalImage,
        ProcessQueue *queue) :
    ProcessStep(originalImage, queue),
    m_staffSpaceHeight(4,6),
    m_staffLineHeight(1, 3),
    m_drawGraph(true)
{
    Q_ASSERT(originalImage.format() == QImage::Format_Mono);
    // Ensure non zero dimension;
    Q_ASSERT(originalImage.height() * originalImage.width() > 0);
}

void StaffParamExtraction::process()
{
    emit started();

    const int Black = m_originalImage.color(0) == 0xffffffff ? 1 : 0;
    const int White = 1 - Black;


    m_runLengths[0].clear();
    m_runLengths[1].clear();

    for (int x = 0; x < m_originalImage.width(); ++x) {
        int runLength = 0;
        int currentColor = m_originalImage.pixelIndex(x, 0);
        for (int y = 0; y < m_originalImage.height(); ++y) {
            if (m_originalImage.pixelIndex(x, y) == currentColor) {
                runLength++;
            } else {
                m_runLengths[currentColor][runLength]++;
                currentColor = m_originalImage.pixelIndex(x, y);
                runLength = 1;
            }
        }
    }

    int maxFreqs[2] = {0, 0};
    int maxRunLengths[2] = {0, 0};
    Range<int> maxRunLengthsRanges[2];

    QList<int> mapKeys[2];

    for (int i = 0; i <= 1; ++i) {
        mapKeys[i] = m_runLengths[i].keys();
        qSort(mapKeys[i]);

        for (int k = 0; k < mapKeys[i].count(); ++k) {
            const int &runLength = mapKeys[i][k];
            const int &freq = m_runLengths[i][runLength];

            if (freq > maxFreqs[i]) {
                maxFreqs[i] = freq;
                maxRunLengths[i] = runLength;
            }
        }

        if (maxRunLengths[i] == 1) {
            maxRunLengthsRanges[i].min = 1;
            maxRunLengthsRanges[i].max = 2;
        } else {
            int interval = int(qRound(.1 * maxRunLengths[i]));
            if (interval == 0) {
                interval = 1;
            }

            maxRunLengthsRanges[i].min = maxRunLengths[i] - interval;
            maxRunLengthsRanges[i].max = maxRunLengths[i] + interval;
        }
    }

    m_staffSpaceHeight = maxRunLengthsRanges[White];
    m_staffLineHeight = maxRunLengthsRanges[Black];

    qDebug() << Q_FUNC_INFO;
    qDebug() << "StaffSpaceHeight:" << m_staffSpaceHeight;
    qDebug() << "StaffLineHeight:" << m_staffLineHeight;

    if (m_drawGraph) {
        bool pruneForClarity = true;

        QDir().mkdir("test_output");
        QString fileNames[2];
        fileNames[Black] = QString("test_output/Black");
        fileNames[White] = QString("test_output/White");

        QString labels[2];
        labels[Black] = QString("StaffLineHeight");
        labels[White] = QString("StaffSpaceHeight");

        for (int i = 0; i <= 1; ++i) {
            QFile file(fileNames[i] + QString(".dat"));
            file.open(QIODevice::WriteOnly | QIODevice::Text);

            QTextStream stream(&file);

            const int xLimit = 50;
            if (pruneForClarity) {
                for (int k = 0; k < mapKeys[i].size(); ++k) {
                    if (mapKeys[i][k] >= xLimit) {
                        int newSize = qMin(mapKeys[i].size(), 2 * k);
                        if (newSize != mapKeys[i].size()) {
                            qDebug() << "Old: " << mapKeys[i].size()
                                << "New: " << newSize;
                            mapKeys[i].erase(mapKeys[i].begin() + (k + 1),
                                    mapKeys[i].end());
                        }
                        break;
                    }
                }
            }
            foreach (int k, mapKeys[i]) {
                stream << k << " " << m_runLengths[i][k] << endl;
            }
            file.close();

            const int y = int(qRound(maxFreqs[i]/3.0));
            QStringList args;
            args << "-e";
            args << QString("set terminal png;"
                    "set output '%1.png';"
                    "set label '  %2 = %3 (%4 freq)' at %5, %6 point;"
                    "set label '  A' at %7, %8 point;"
                    "set label '  B' at %9, %10 point;"
                    "set yrange[0:%11];"
                    "plot '%12.dat' using 1:2 with lines;")
                .arg(fileNames[i]) //1
                .arg(labels[i]) //2
                .arg(maxRunLengths[i]) //3
                .arg(maxFreqs[i]) //4
                .arg(maxRunLengths[i]) //5
                .arg(maxFreqs[i]) //6
                .arg(maxRunLengthsRanges[i].min) //7
                .arg(y) //8
                .arg(maxRunLengthsRanges[i].max) //9
                .arg(y) //10
                .arg(maxFreqs[i] + 250) //11
                .arg(fileNames[i]); // 12

            QProcess::execute(QString("gnuplot"), args);
        }

        QImage plots[2];
        plots[Black].load(fileNames[Black] + QString(".png"));
        plots[White].load(fileNames[White] + QString(".png"));

        QSize sz;
        sz.setWidth(qMax(plots[0].width(), plots[1].width()));
        sz.setHeight(plots[0].height() + 50 + plots[1].height());
        m_processedImage = QImage(sz, QImage::Format_ARGB32_Premultiplied);
        m_processedImage.fill(0xffffffff);

        QPainter p(&m_processedImage);
        p.drawImage(QPoint(0, 0), plots[0]);
        p.drawImage(QPoint(0, plots[0].height() + 50), plots[1]);
        p.end();
    }

    emit ended();
}

void StaffParamExtraction::setDrawGraph(bool b)
{
    m_drawGraph = b;
}

Range<int> StaffParamExtraction::staffSpaceHeight() const
{
    return m_staffSpaceHeight;
}

Range<int> StaffParamExtraction::staffLineHeight() const
{
    return m_staffLineHeight;
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

    m_processedImage = m_processedImage.transformed(transform, Qt::FastTransformation);
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

int ImageCluster::InvalidStaffSpaceHeight = -1;

ImageCluster::ImageCluster(const QImage& originalImage, ProcessQueue *queue) :
    ProcessStep(originalImage, queue)
{
    m_clusterSet.setRadius(ImageCluster::InvalidStaffSpaceHeight);
    m_clusterSet.setMinPoints(ImageCluster::InvalidStaffSpaceHeight);
}

ImageCluster::ImageCluster(const QImage& originalImage, int staffSpaceHeight , ProcessQueue *queue) :
    ProcessStep(originalImage, queue)
{
    QPair<int, int> p = ImageCluster::clusterParams(staffSpaceHeight);
    m_clusterSet.setRadius(p.first);
    m_clusterSet.setMinPoints(p.second);
}

void ImageCluster::process()
{
    emit started();

    if (m_clusterSet.radius() < 0) {
        bool ok;
        int staffSpaceHeight =
            QInputDialog::getInt(0, tr("Staff space height"),
                    tr("Enter staff space height in pixels"),
                    5, 0, 100, 1, &ok);
        if (!ok) {
            staffSpaceHeight = 5;
        }

        QPair<int, int> p = ImageCluster::clusterParams(staffSpaceHeight);
        m_clusterSet.setRadius(p.first);
        m_clusterSet.setMinPoints(p.second);
    }

    m_clusterSet.setImage(m_originalImage);
    m_clusterSet.computeNearestNeighbors();

    mDebug() << Q_FUNC_INFO << "No of core points : " << m_clusterSet.coreSize() << endl;

    m_processedImage = QImage(m_originalImage.size(), QImage::Format_ARGB32_Premultiplied);
    QPainter p(&m_processedImage);
    p.fillRect(QRect(QPoint(0, 0), m_processedImage.size()), Qt::white);
    p.setPen(QColor(Qt::red));
    m_clusterSet.drawCore(p);
    p.end();

    emit ended();
}

QPair<int, int> ImageCluster::clusterParams(int staffSpaceHeight)
{
    if (staffSpaceHeight < 0) {
        return QPair<int, int>(ImageCluster::InvalidStaffSpaceHeight,
                ImageCluster::InvalidStaffSpaceHeight);
    }
    int radius = int(.85 * staffSpaceHeight);
    int area = int(M_PI * radius * radius);
    return qMakePair(staffSpaceHeight, int(qRound(.8 * area)));
}

int SymbolAreaExtraction::InvalidStaffSpaceHeight = -1;

SymbolAreaExtraction::SymbolAreaExtraction(const QImage& originalImage, ProcessQueue *queue) :
    ProcessStep(originalImage, queue)
{

}

SymbolAreaExtraction::SymbolAreaExtraction(const QImage& originalImage,
        int staffSpaceHeight, ProcessQueue *queue) :
    ProcessStep(originalImage, queue)
{
    Q_UNUSED(staffSpaceHeight);
}

void SymbolAreaExtraction::process()
{
    emit started();

    const int Black = m_originalImage.color(0) == 0xffffffff ? 1 : 0;

    DataWarehouse *dw = DataWarehouse::instance();
    QList<Staff> staffList = dw->staffList();

    const int spacing = 50;
    QSize size(0, 0);
    foreach (const Staff& s, staffList) {
        QRect r = s.boundingRect();
        size.rheight() += 2 * r.height() + spacing;
        if (size.width() < r.width()) {
            size.setWidth(r.width());
        }
    }

    m_processedImage = QImage(size, QImage::Format_ARGB32_Premultiplied);
    m_processedImage.fill(0xffffffff);

    int y = 0;
    QPainter p(&m_processedImage);
    foreach (const Staff& s, staffList) {
        QRect bigger = s.boundingRect();

        QRect r = s.staffBoundingRect();
        r.setLeft(bigger.left());
        r.setRight(bigger.right());

        QRect target(QPoint(0, y), bigger.size());
        p.drawImage(target, m_originalImage, bigger);
        p.fillRect(target, QColor(60, 60, 60, 100));

        y += bigger.height() + .5 * spacing;
        QRect projectionRect = bigger.translated(QPoint(0, y) - bigger.topLeft());
        p.setPen(QColor(Qt::blue));
        p.drawRect(projectionRect.adjusted(-2, -2, 2, 2));
        p.setPen(QColor(Qt::red));

        QList<int> counts;
        for (int x = r.left(), X = projectionRect.left(); x <= r.right(); ++x, ++X) {
            int count = 0;
            for (int y = r.top(); y < r.bottom(); ++y) {
                if (m_originalImage.pixelIndex(x, y) == Black) {
                    count++;
                }
            }
            if (count) {
                p.drawLine(X, projectionRect.bottom(), X, projectionRect.bottom() - count);
            }
            counts << count;
        }

        // Just a random push so that there is no crash on empty list.
        if (counts.isEmpty()) {
            counts << 5;
        }

        int staffLineHeight = -1;
        {
            StaffParamExtraction *param =
                new StaffParamExtraction(m_originalImage, false, 0);
            param->process();
            Range<int> range = param->staffLineHeight();
            if (range.size() == 1) {
                staffLineHeight = range.min;
            } else {
                staffLineHeight = int(qRound(.5 * (range.min + range.max)));
            }
            delete param;
        }

        QColor symbolColor(Qt::darkYellow);
        symbolColor.setAlpha(90);

        QColor nonSymbolColor(Qt::darkGray);

        int top = target.top() + (r.top() - bigger.top());
        int bottom = target.bottom() - (bigger.bottom() - r.bottom());
        for (int i = 0; i < counts.size(); ++i) {
            if (counts[i] > 5 * staffLineHeight) {
                p.setPen(symbolColor);
            } else {
                p.setPen(nonSymbolColor);
            }
            p.drawLine(target.left() + i, top, target.left() + i, bottom);
        }
        mDebug() << Q_FUNC_INFO << counts;


        y += bigger.height() + .5 * spacing;
    }

    emit ended();
}
}
