#include "processstep.h"

#include "imagewidget.h"
#include "mainwindow.h"
#include "tools.h"

#include <QAction>
#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QRgb>
#include <QSet>
#include <QVector>

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
        return m_processQueue.data();
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

        int h = m_originalImage.height();
        int w = m_originalImage.width();

        m_processedImage = QImage(w, h, QImage::Format_Mono);
        uchar *destData = m_processedImage.bits();
        int destBytes = m_processedImage.numBytes();
        memset(destData, 0, destBytes);

        const int White = 0, Black = 1;
        // Ensure above index values to color table
        m_processedImage.setColor(0, 0xffffffff);
        m_processedImage.setColor(1, 0xff000000);

        for(int x = 0; x < w; ++x) {
            for(int y = 0; y < h; ++y) {
                bool isWhite = (qGray(m_originalImage.pixel(x, y)) > m_threshold);
                int color = isWhite ? White : Black;
                m_processedImage.setPixel(x, y, color);
            }
        }

        emit ended();
    }

    SkewCorrection::SkewCorrection(const QImage& originalImage, ProcessQueue *queue) :
        ProcessStep(originalImage, queue),
        m_workImage(originalImage)
    {
        Q_ASSERT(m_originalImage.format() == QImage::Format_Mono);
    }

    void SkewCorrection::process()
    {
        emit started();
        double theta = std::atan(detectSkew());
        if(theta == 0.0) {
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
        const int Black = 1;
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
        return skew;
    }


    void SkewCorrection::dfs(int x,int y, QList<QPoint> points)
    {
        const int Black = 1;
        const int White = 0;

        m_workImage.setPixel(x, y, White);
        if(points.size() == 20)
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
}
