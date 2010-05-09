#ifndef PROCESSSTEP_H
#define PROCESSSTEP_H

#include "cluster.h"
#include "segments.h"
#include "staff.h"
#include "symbol.h"
#include "tools.h"

#include <QAction>
#include <QDataStream>
#include <QDebug>
#include <QHash>
#include <QPoint>
#include <QPointer>
#include <QQueue>
#include <QVariant>

class QIcon;
class HorizontalRunlengthImage;

inline uint qHash(const QRect &rect)
{
    return(rect.left() * 4000 + rect.right());
}

namespace Munip {
    // Forwared declarations
    class Page;
    class ProcessQueue;

    // Assumes line's start pos is <= 4k and >= 0.
    inline uint qHash( const Munip ::Segment &line )
    {
        return(line.startPos().x() * 4000 + line.endPos().y());
    }

    QColor randColor();

     /**
     * This base class represents a process step like skew
     * detection. It provides access to the original as well as
     * processed image.
     *
     * Also a process queue is maintained, where various ProcessSteps
     * can be added to the queue and the queue can be processed as a
     * whole.
     *
     * Subclassess should reimplement process() mehtod to do their
     * actual work.
     */
    class ProcessStep : public QObject
    {
        Q_OBJECT;
    public:
        ProcessStep(const QImage& originalImage, ProcessQueue *processQueue = 0);
        virtual ~ProcessStep();

        QImage originalImage() const;
        QImage processedImage() const;

        ProcessStep* previousStep() const;
        ProcessStep* nextStep() const;

        ProcessQueue* processQueue() const;

        bool failed() const;
        QString failMessage() const;

        void setFailed(const QString& message);

    Q_SIGNALS:
        void started();
        void ended();

    public Q_SLOTS:
        virtual void process() = 0;

        void slotStarted();
        void slotEnded();

    protected:
        const QImage m_originalImage;
        QImage m_processedImage;

        QPointer<ProcessQueue> m_processQueue;
        bool m_processCompleted;

        bool m_processFailed;
        QString m_failMessage;
    };

    class ProcessQueue : public QObject, public QQueue<ProcessStep*>
    {
        Q_OBJECT;
    };

    class ProcessStepAction : public QAction
    {
        Q_OBJECT;
    public:
        ProcessStepAction(const QByteArray& className,
                          const QIcon& icon = QIcon(),
                          const QString& caption = QString(),
                          QObject *parent = 0);

    public Q_SLOTS:
        void execute();

    private:
        QByteArray m_className;
    };

    struct ProcessStepFactory
    {
        static ProcessStep* create(const QByteArray& className, const QImage& originalImage, ProcessQueue *processQueue = 0);
        static QList<ProcessStepAction*> actions(QObject *parent = 0);
    };

    class GrayScaleConversion : public ProcessStep
    {
        Q_OBJECT;
    public:
        GrayScaleConversion(const QImage& originalImage, ProcessQueue *processQueue = 0);
        virtual void process();
    };

    class MonoChromeConversion : public ProcessStep
    {
        Q_OBJECT;
    public:
        MonoChromeConversion(const QImage& originalImage, ProcessQueue *procecssQueue = 0);
        virtual void process();

        int threshold() const;
        void setThreshold(int threshold);

    private:
        int m_threshold;
    };

    class SkewCorrection : public ProcessStep
    {
        Q_OBJECT;
    public:
        SkewCorrection(const QImage& originalImage, ProcessQueue *processqueue = 0);
        virtual void process();

        double detectSkew();
        void dfs(int x, int y, QList<QPoint> points);
        double findSkew(QList<QPoint> &points);

        QList<double> skewList() const { return m_skewList; }

    Q_SIGNALS:
        void angleCalculated(qreal angleInDegrees);

    private:
        QImage m_workImage;
        const int m_lineSliceSize;
        //const float m_skewPrecision;
        QList<double> m_skewList;
    };

    class StaffLineDetect : public ProcessStep
    {
        Q_OBJECT;
    public:
        StaffLineDetect(const QImage& originalImage, ProcessQueue *processQueue = 0);

        virtual void process();

        bool checkDiscontinuity(int countWhite );
        bool isLine(int countBlack );
        bool isStaff( int countStaffLines );
        void detectLines();
        Segment findMaxPath(Segment segment);
        void constructStaff();
        void estimateStaffParametersFromYellowAreas();
        QRect findStaffBoundingRect(const Staff &s);
        void removeLines();
        void identifySymbolRegions(const Staff &s);
        void popupHistogram();


    private:
        QList<StaffLine> m_lineList;
        QList<Segment> m_maxPaths;
        QPixmap m_lineRemovedTracker;
        QPixmap m_rectTracker;
        QImage m_symbolMap;
        QImage m_lineMap;
        QList<Segment> m_segments[5000];
        QHash<Segment,Segment> m_lookUpTable;
        int  m_connectedComponentID;
        //int m_imageMap[5000][5000];
        QList<QRect> m_symbolRegions;


        void findPaths();
        void drawDetectedLines();
        void segmentCleanUp(const Segment& segment);
        int findTopHeight(QPoint pos,QImage& workImage);
        int findBottomHeight(QPoint pos,QImage& workImage);

        QList<Segment> findTopSegments(Segment segment,QImage& workImage);
        QList<Segment> findBottomSegments(Segment segment,QImage& workImage);
        QList<Segment> findAdjacentSymbolSegments(Segment segment,QImage& workImage);

        void  aggregateSymbolRegion();
        QRect aggregateSymbolRects(QRect rect1,QRect rect2);
        QRect aggregateAdjacentRegions(QRect symbolRect1,QRect symbolRect2);


        void StaffCleanUp(const Staff &staff);

   };

    class StaffLineRemoval : public ProcessStep
    {
        Q_OBJECT;
    public:
        StaffLineRemoval(const QImage& originalImage, ProcessQueue *processQueue = 0);

        virtual void process();

    private:
        void addDebugInfoToProcessedImage();
        void crudeRemove();
        void yellowToBlack();
        void cleanupNoise();
        void staffCleanUp();
    };


    class StaffParamExtraction : public ProcessStep
    {
    Q_OBJECT
    public:
        StaffParamExtraction(const QImage& originalImage, bool drawGraph,
                ProcessQueue *queue);
        StaffParamExtraction(const QImage& originalImage, ProcessQueue *queue);

        virtual void process();

        void setDrawGraph(bool status);

    private:
        bool m_drawGraph;
    };

    class ImageRotation : public ProcessStep
    {
        Q_OBJECT;
    public:
        ImageRotation(const QImage& originalImage, ProcessQueue *processQueue = 0);
        ImageRotation(const QImage& originalImage, qreal _angle, ProcessQueue *processQueue = 0);
        virtual void process();

    private:
        static const qreal InvalidAngle;
        qreal m_angle;
    };

    class ImageCluster : public ProcessStep
    {
        Q_OBJECT
    public:
        ImageCluster(const QImage& originalImage, ProcessQueue *queue = 0);
        ImageCluster(const QImage& originalImage,
                int staffSpaceHeight = ImageCluster::InvalidStaffSpaceHeight,
                ProcessQueue *queue = 0);
        virtual void process();

        static QPair<int, int> clusterParams(int staffSpaceHeight);

    private:
        static int InvalidStaffSpaceHeight;
        ClusterSet m_clusterSet;
    };

    class SymbolAreaExtraction : public ProcessStep
    {
        Q_OBJECT
    public:
        SymbolAreaExtraction(const QImage& originalImage, ProcessQueue *queue = 0);
        SymbolAreaExtraction(const QImage& originalImage,
                int staffSpaceHeight = SymbolAreaExtraction::InvalidStaffSpaceHeight,
                ProcessQueue *queue = 0);
        void extraStuff();
        virtual void process();

    private:
        static int InvalidStaffSpaceHeight;
    };
}

#endif

