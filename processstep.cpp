#include "processstep.h"
#include "staff.h"

#include "imagewidget.h"
#include "horizontalrunlengthimage.h"
#include "mainwindow.h"
#include "tools.h"
#include "DataWarehouse.h"

#include <QAction>
#include <QDebug>
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
    int pageSkew;

    double normalizedLineTheta(const QLineF& line)
    {
        double angle = line.angle();
        while (angle < 0.0) angle += 360.0;
        while (angle > 360.0) angle -= 360.0;
        if (angle > 180.0) angle -= 360.0;
        return angle < 0.0 ? -angle : angle;
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
        else if (className == QByteArray("StaffLineRemoval"))
            step = new StaffLineRemoval(originalImage, queue);
        else if (className == QByteArray("StaffLineDetect"))
            step = new StaffLineDetect(originalImage, queue);
        else if (className == QByteArray("ConvolutionLineDetect"))
            step = new ConvolutionLineDetect(originalImage, queue);
        else if (className == QByteArray("HoughTransformation"))
            step = new HoughTransformation(originalImage, queue);
        else if (className == QByteArray("ImageRotation"))
            step = new ImageRotation(originalImage, queue);
        else if (className == QByteArray("RunlengthLineDetection"))
            step = new RunlengthLineDetection(originalImage, queue);

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
        m_lineSliceSize((int)originalImage.width()*0.05)
    {
        Q_ASSERT(m_originalImage.format() == QImage::Format_Mono);
                //m_lineSliceSize = (int)originalImage.width()*0.05;
    }

    void SkewCorrection::process()
    {
        emit started();
        double theta = std::atan(detectSkew());
        if(theta <= 0.0) {
            emit ended();
            return;
        }

        QTransform transform, trueTransform;
        double angle = -180.0/M_PI * theta;
        transform.rotate(angle);
        // Find out the true tranformation used (automatically adjusted
        // by QImage::transformed method)
        trueTransform = m_processedImage.trueMatrix(transform, m_processedImage.width(),
                                                    m_processedImage.height());

        // Processed image will have the transformed image rotated by
        // staff skew after following operation. Apart from that it also
        // has black triangular corners produced due to bounding rect
        // extentsion.
        m_processedImage = m_processedImage.transformed(transform, Qt::SmoothTransformation);
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

        foreach(double skew, m_skewList)
            qDebug() << skew;

        int i = 0, n = m_skewList.size();
        int modefrequency = 0;
        int maxstartindex = -1, maxendindex = -1;
        while( i <= n-1)
        {
            int runlength = 1;
            double t = m_skewList[i];
            int runvalue = (int) (t * 100);
            while( i + runlength <= n-1 && (int)(m_skewList[i+runlength]*100) == runvalue)
                runlength++;
            if(runlength > modefrequency)
            {
                modefrequency = runlength;
                maxstartindex = i;
                maxendindex = i + runlength;
            }
            i += runlength;
        }
        double skew = 0;
        for(int i = maxstartindex; i<= maxendindex;i++)
            skew += m_skewList[i];
        skew /= modefrequency;

        qDebug() << Q_FUNC_INFO <<skew;

        if( skew >= 0)
            pageSkew = 1;

        else
            pageSkew = -1;

        return skew;
    }


    void SkewCorrection::dfs(int x,int y, QList<QPoint> points)
    {
        const int Black = m_workImage.color(0) == 0xffffffff ? 1 : 0;
        const int White = 1 - Black;

        m_workImage.setPixel(x, y, White);
        if(points.size() == m_lineSliceSize)
        {
            double skew;
            m_skewList.push_back((skew = findSkew(points)));
            points.clear();
        }
        if(m_workImage.pixelIndex(x+1, y+1) == Black &&
           x+1 < m_workImage.width() &&
           y + 1 < m_workImage.height())
        {
            points.push_back(QPoint(x,y));
            dfs(x+1, y+1, points);
        }
        if(m_workImage.pixelIndex(x+1, y) == Black &&
           x+1 < m_workImage.width())
        {
            points.push_back(QPoint(x,y));
            dfs(x+1, y, points);
        }
        if(m_workImage.pixelIndex(x+1, y-1) == Black && y-1 > 0)
        {
            points.push_back(QPoint(x,y));
            dfs(x+1, y-1, points);
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
		qDebug()<<Q_FUNC_INFO<<p1.x()<<p2.x();
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
						qDebug()<<start<<end<<count<<width;
					}
					done = true;
					
				}
			}
		}
		qDebug() <<"Hi!!!";

    }
	void StaffLineRemoval :: followLine(QPoint& p,int& count)
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
			
        QVector<Staff> StaffLineRemoval :: fillDataStructures()
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

	void StaffLineRemoval :: removeStaffLines()
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
	void StaffLineRemoval :: removeFirstLine(QPoint& start,QPoint& end)
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
	void StaffLineRemoval :: removeLastLine(QPoint& start,QPoint& end)
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
    //TODO Put the constructor and other basic stuff in

     StaffLineDetect::StaffLineDetect(const QImage& originalImage, ProcessQueue *queue) :
        ProcessStep(originalImage, queue)
    {
        Q_ASSERT(originalImage.format() == QImage::Format_Mono);
    }

    void StaffLineDetect ::process()
    {
        emit started();
        m_lineRemovedTracker = QPixmap(m_processedImage.size());
        m_lineRemovedTracker.fill(QColor(Qt::white));
        detectLines();
        constructStaff();

        m_processedImage = m_lineRemovedTracker.toImage();

        emit ended();
    }
     bool StaffLineDetect ::checkDiscontinuity(int countWhite )
    {
        if( m_processedImage.width() <= 500 && countWhite >= 5 )
            return true;
        if( m_processedImage.width() > 500 && countWhite >= (int) 0.005*m_processedImage.width() )
            return true;
        return false;
    }

    bool StaffLineDetect :: isLine( int countBlack )
    {
        if( countBlack >= (int) (0.1*m_processedImage.width()) )
            return true;
        return false;
    }

    bool StaffLineDetect :: isStaff( int countStaffLines )
    {
        if( countStaffLines == 5 ) //TODO Put this in DataWarehouse
            return true;
        return false;
    }


    void StaffLineDetect ::detectLines()
    {
        const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
        const int Black = 1 - White;
        int countBlack = 0,countWhite = 0;
        QPoint start,end;
        QPainter p(&m_lineRemovedTracker);

        for(int y = 0; y < m_processedImage.height(); y++)
        {
            int x = 0;
            while(x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White )
                x++;

            start = QPoint(x,y);
            while( x < m_processedImage.width() )
            {
                while(x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == Black )
                {
                    countBlack++;
                    x++;
                }
                while( x < m_processedImage.width() && m_processedImage.pixelIndex(x,y) == White )
                {
                    countWhite++;
                    x++;
                }
                end = QPoint(x-countWhite,y);
                StaffLine line( start,end,1);
                if( isLine(countBlack) )
                {
                    p.setPen(QColor(qrand() % 255, qrand()%255, 100+qrand()%155));
                    p.drawLine(start,end);
                }


                if( ( m_lineList.size() == 0 && isLine(countBlack) ) || ( isLine(countBlack) &&  !m_lineList[m_lineList.size()-1].aggregate(line) ) )
                     m_lineList.push_back(line);

                countBlack = 0;
                countWhite = 0;
                start = QPoint(x,y);

            }


        }
        for(int i = 0; i < m_lineList.size();i++)
            qDebug() << Q_FUNC_INFO<< m_lineList[i].startPos() << m_lineList[i].endPos() << m_lineList[i].lineWidth() ;
    }

    void StaffLineDetect ::constructStaff()
    {
        Staff s;
        int distance,d;
        int count = 0;
        for(int i = 0; i < m_lineList.size();i++)
        {
            switch(count)
            {
                case 0: s.addStaffLine( m_lineList[i] );
                        count++;
                        break;
                case 1: s.addStaffLine( m_lineList[i] );
                        count++;
                        distance = s.distance(1);
                        break;
                default: s.addStaffLine( m_lineList[i] );
                         d = s.distance(count+1);
                         if( d >= (int) 0.8* distance )
                         {
                             if( d > distance )
                                 distance = d;
                            count++;
                            if( isStaff( count ) )
                            {
                                //DataWarehouse ::instance() ->appendStaff( s );
                                drawStaff(s);
                            }

                         }
                         else
                         {
                             s.clear();
                             count = 1;
                             s.addStaffLine( m_lineList[i]);
                         }
            }
        }
    }

    void StaffLineDetect :: drawStaff( Staff& s )
    {




    }

    ConvolutionLineDetect::ConvolutionLineDetect(const QImage& originalImage, ProcessQueue *processQueue) :
        ProcessStep(originalImage, processQueue)
    {
        const int act_kernel[3][3] = {
            {-1, -1, -1},
            {2, 2, 2},
            {-1, -1, -1}
        };

        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
                m_kernel[i][j] = act_kernel[i][j];

        if (originalImage.format() != QImage::Format_Mono) {
            m_processedImage = QImage(originalImage.width(), originalImage.height(), QImage::Format_Indexed8);
            QVector<QRgb> table(256, 0);
            for(int i = 0; i < 256; ++i)
                table[i] = qRgb(i, i, i);
            m_processedImage.setColorTable(table);
            memset(m_processedImage.bits(), 0, m_processedImage.numBytes());
        }
    }

    int ConvolutionLineDetect::val(int x, int y) const
    {
        return qGray(m_originalImage.pixel(x, y));
    }

    void ConvolutionLineDetect::process()
    {
        emit started();

        for(int x = 0; x < m_originalImage.width() - 3; ++x) {
            for(int y = 0; y < m_originalImage.height() - 3; ++y) {
                convolute(x, y);
            }
        }

        emit ended();
    }

    void ConvolutionLineDetect::convolute(int X, int Y)
    {
        int v = 0;
        for(int x=X; x<X+3; ++x) {
            for(int y=Y; y<Y+3; ++y) {
                v += m_kernel[y-Y][x-X] * val(x, y);
            }
        }
        if (v < 0)
            v = 0;
        if (v > 255)
            v = 255;

        if (m_processedImage.format() == QImage::Format_Mono) {
            bool isWhite = v > 128;
            const int White = m_processedImage.color(0) == 0xffffffff ? 0 : 1;
            const int Black = 1 - White;

            m_processedImage.setPixel(X, Y, isWhite ? White : Black);
        }
        else {
            m_processedImage.setPixel(X, Y, v);
        }
    }

    HoughTransformation::HoughTransformation(const QImage& originalImage, ProcessQueue *queue) :
        ProcessStep(originalImage, queue)
    {
        Q_ASSERT(originalImage.format() == QImage::Format_Mono);
    }

    typedef QPair<QPoint, int> Pair;
    bool pairGreaterThan(const Pair& p1, const Pair& p2)
    {
        return p1.second > p2.second;
    }

    void HoughTransformation::process()
    {
        emit started();

        const int Black = m_originalImage.color(0) == 0xffffffff ? 1 : 0;
        const int White = 1 - Black;
        const int w = m_processedImage.width();
        const int h = m_processedImage.height();
        const double pi_by_180 = M_PI/180;

        QMap<double, double> cosThetaTable;
        QMap<double, double> sinThetaTable;


        for(double i = -95.; i <= 95.0; i += 0.5) {
            cosThetaTable[i] = std::cos(pi_by_180 * i);
            sinThetaTable[i] = std::sin(pi_by_180 * i);
        }


        QList<Pair> accumulator;

        for(int x = 0; x < w; ++x) {
            for(int y = 0; y < h; ++y) {
                if (m_originalImage.pixelIndex(x, y) == White)
                    continue;
                for(double i = -95.0; i <= 95.0; i += 0.5) {
                    double r = x * cosThetaTable[i] + y * sinThetaTable[i];
                    r = qRound(r);
                    QPoint point((int)r, int(i*10));

                    bool found = false;
                    for(int k = 0; k < accumulator.size(); ++k) {
                        if (accumulator[k].first == point) {
                            accumulator[k].second++;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        accumulator.append(qMakePair(point, int(1)));
                }
            }
        }

        //qSort(accumulator.begin(), accumulator.end(), Munip::pairGreaterThan);

        for(int i = 0; i < accumulator.size(); ++i) {
            QPointF p = accumulator.at(i).first;
            p.ry() /= 10;
            qDebug() << p << " = " << accumulator.at(i).second;
        }
        // lines_list_t lines;
        // uchar* binary_image = (uchar*)malloc(sizeof(uchar) * w * h * 2);
        // uchar *start = binary_image;

        // Q_ASSERT(binary_image);

        // qDebug() << Q_FUNC_INFO << "Allocated";

        // for(int x = 0; x < w; ++x) {
        //     for(int y = 0; y < h; ++y) {
        //         uchar val = m_processedImage.pixelIndex(x, y) == Black ? 0 : 1;
        //         binary_image[y*w + x] = val;
        //     }
        // }

        // qDebug() << Q_FUNC_INFO << "Filled the binary_image";
        // kht(lines, binary_image, w, h);
        // qDebug() << Q_FUNC_INFO << "Called kht";

        // for(size_t i = 0; i < lines.size(); ++i) {
        //     qDebug() << "Line(rho, theta) = " << "("
        //              << lines[i].rho << ", " << lines[i].theta << ")";
        // }

        // free(start);
        emit ended();
    }

    ImageRotation::ImageRotation(const QImage& image, ProcessQueue *queue) :
        ProcessStep(image, queue)
    {
    }

    void ImageRotation::process()
    {
        emit started();
        QImage::Format destFormat = m_originalImage.format();
        bool ok;
        int angle = QInputDialog::getInteger(0, tr("Angle"), tr("Enter rotation angle in degrees for image rotation"),
                                         5, -45, 45, 1, &ok);
        if (!ok) {
            angle = 5;
        }

        QTransform transform;
        transform.rotate(angle);
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

    RunlengthLineDetection::RunlengthLineDetection(const QImage& originalImage, ProcessQueue *queue) :
        ProcessStep(originalImage, queue)
    {
        int Black = originalImage.colorTable().at(0) == 0xffffffff ? 1 : 0;
        m_horizontalRunlengthImage = new HorizontalRunlengthImage(originalImage, Black);

        QVector<QVector<LocationRunPair> > &data = m_horizontalRunlengthImage->m_data;
        m_processedMarker.resize(data.size());
        for (int y=0; y<data.size(); ++y) {
            m_processedMarker[y] = QVector<bool>(data[y].size(), false);
        }
    }

    RunlengthLineDetection::~RunlengthLineDetection()
    {
        delete m_horizontalRunlengthImage;
    }

    void RunlengthLineDetection::process()
    {
        emit started();

        m_processedImage = QImage(m_originalImage.size(), QImage::Format_ARGB32);
        m_processedImage.fill(qRgb(0, 0, 0));

#if 0
        QVector<QVector<LocationRunPair> > &data = m_horizontalRunlengthImage->m_data;
        for (int y=0; y<data.size(); ++y) {
            for (int i=0; i<data[y].size(); ++i) {
                if (!processed(y, i)) {
                    tryLine(y, i);
                }
            }
        }
#endif
        for (int x = 0; x < m_processedImage.width(); ++x) {
            for (int y = 0; y < m_processedImage.height(); ++y) {
                int i = m_horizontalRunlengthImage->runForPixel(x, y);
                //qDebug() << Q_FUNC_INFO << y << i;
                if (i != -1 && !processed(y, i)) {
                    tryLine(y, i);
                }
            }
        }
        emit ended();
    }

    void RunlengthLineDetection::tryLine(int y, int i)
    {
        const LocationRunPair startPair = m_horizontalRunlengthImage->run(y, i);
        const QPoint start(startPair.x, y);
        const double ThresholdAngle = 2.0;

        QStack<QLine> lineStack;
        lineStack.push(QLine(startPair.x, y, startPair.x+startPair.run-1, y));

        QPainter p(&m_processedImage);
        p.setPen(QColor(qrand()%255, 100+qrand()%155, qrand()%255));

        QPoint end = start;
        while (!lineStack.isEmpty()) {
            QLine line = lineStack.pop();
            qDebug() << line;
            int col = m_horizontalRunlengthImage->runForPixel(line.x1(), line.y1());
            qDebug() << "\tFound run";
            Q_ASSERT(col != -1);
            m_processedMarker[line.y1()][col] = true;
            //p.drawLine(line);
            end = line.p2();

            int x = line.p2().x() + 1;
            if (x >= m_originalImage.width()) {
                continue;
            }

            int y_minus_1 = line.y1() > 0 ?
                m_horizontalRunlengthImage->runForPixel(x, line.y1()-1) :
                -1;

            int y_plus_1 = line.y1() < m_horizontalRunlengthImage->m_imageSize.height() - 1?
                m_horizontalRunlengthImage->runForPixel(x, line.y1()+1) :
                -1;

            if (y_minus_1 >= 0 && !processed(line.y1()-1, y_minus_1)) {
                LocationRunPair pair = m_horizontalRunlengthImage->run(line.y1()-1, y_minus_1);
                QLine newLine(pair.x, line.y1()-1, pair.x+pair.run-1, line.y1()-1);

                double theta = normalizedLineTheta(QLineF(start, newLine.p2()));
                if (theta <= ThresholdAngle) {
                    lineStack.push(newLine);
                }
            }

            if (y_plus_1 >= 0 && !processed(line.y1()+1, y_plus_1)) {
                LocationRunPair pair = m_horizontalRunlengthImage->run(line.y1()+1, y_plus_1);
                QLine newLine(pair.x, line.y1()+1, pair.x+pair.run-1, line.y1()+1);

                double theta = normalizedLineTheta(QLineF(start, newLine.p2()));
                if (theta <= ThresholdAngle) {
                    lineStack.push(newLine);
                }
            }
        }

        if (qAbs(end.x() - start.x()) > 10) {
            p.drawLine(start, end);
        }
    }

    bool RunlengthLineDetection::processed(int y, int i) const
    {
        return m_processedMarker[y][i];
    }
}
