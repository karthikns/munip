#ifndef PROCESSSTEP_H
#define PROCESSSTEP_H

#include <QAction>
#include <QDataStream>
#include <QDebug>
#include <QQueue>
#include <QPoint>
#include <QHash>
#include <QPointer>
#include <QVariant>
#include "staff.h"
#include "segments.h"
#include "cluster.h"

#ifdef MUNIP_DEBUG
    #define mDebug qDebug
    #define mWarning qWarning
#else
    struct MDebug : public QDebug
    {
        MDebug() : QDebug(&myData) {}
        QString myData;
    };

    #define mDebug MDebug
    #define mWarning MDebug
#endif

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
        QRect findStaffBoundingRect(const Staff &s);
        void removeLines();
        void identifySymbolRegions(const Staff &s);
        void popupHistogram();


    private:
        QVector<StaffLine> m_lineList;
        QVector<Segment> m_maxPaths;
        QPixmap m_lineRemovedTracker;
        QPixmap m_rectTracker;
        QImage m_symbolMap;
        QImage m_lineMap;
        QVector<Segment> m_segments[5000];
        QHash<Segment,Segment> m_lookUpTable;
        int  m_connectedComponentID;
        //int m_imageMap[5000][5000];
        QList<QRect> m_symbolRegions;


        void findPaths();
        void drawDetectedLines();

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

        void detectLines();
        bool endOfLine(QPoint& p, int&);
        void removeLine(QPoint& start,QPoint& end);
        void removeStaffLines();
        bool canBeRemoved(QPoint& p);
        void followLine(QPoint& p,int& count);
        QVector<Staff> fillDataStructures();
        void removeFirstLine(QPoint& start,QPoint& end);
        void removeLastLine(QPoint& start,QPoint& end);

    private:
        QList<QPoint> m_lineLocation;
        QList<bool> m_isLine;
        QPixmap m_lineRemovedTracker;
        int m_upperLimit;
        double m_lineWidthLimit;
    };


    class StaffParamExtraction : public ProcessStep
    {
    Q_OBJECT
    public:
        StaffParamExtraction(const QImage& originalImage, ProcessQueue *queue);
        virtual void process();

        int staffSpaceHeight() const;

    private:
        int m_staffSpaceHeight;
        QMap<int, int> m_runLengths[2];
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
        virtual void process();

    private:
        static int InvalidStaffSpaceHeight;
    };

}

#endif

