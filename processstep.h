#ifndef PROCESSSTEP_H
#define PROCESSSTEP_H

#include <QAction>
#include <QQueue>
#include <QPoint>
#include <QPointer>
#include "staff.h"

class QIcon;
class HorizontalRunlengthImage;

namespace Munip {

    // Forwared declarations
    class Page;
    class ProcessQueue;

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
                          const QIcon& icon,
                          const QString& caption,
                          QObject *parent = 0);

    public Q_SLOTS:
        void execute();

    private:
        QByteArray m_className;
    };

    struct ProcessStepFactory
    {
        static ProcessStep* create(const QByteArray& className, const QImage& originalImage, ProcessQueue *processQueue = 0);
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

    private:
        QImage m_workImage;
        const int m_lineSliceSize;
        //const float m_skewPrecision;
        QList<double> m_skewList;
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
            void constructStaff();
            void removeLines();
            void drawStaff(Staff& s);

        private:
            QVector<StaffLine> m_lineList;
            QPixmap m_lineRemovedTracker;
        };



    class ConvolutionLineDetect : public ProcessStep
    {
        Q_OBJECT;
    public:
        ConvolutionLineDetect(const QImage& originalImage, ProcessQueue *processQueue = 0);
        virtual void process();

    private:
        int val(int x, int y) const;
        void convolute(int x, int y);

        int m_kernel[3][3];
    };

    class HoughTransformation : public ProcessStep
    {
        Q_OBJECT;
    public:
        HoughTransformation(const QImage& originalImage, ProcessQueue *queue = 0);
        virtual void process();

    private:
    };

    class ImageRotation : public ProcessStep
    {
        Q_OBJECT;
    public:
        ImageRotation(const QImage& originalImage, ProcessQueue *processQueue = 0);
        virtual void process();
    };

    class RunlengthLineDetection : public ProcessStep
    {
        Q_OBJECT;
    public:
        RunlengthLineDetection(const QImage& originalImage, ProcessQueue *processQueue = 0);
        virtual ~RunlengthLineDetection();

        virtual void process();

    private:
        void tryLine(int x, int y);
        bool processed(int y, int i) const;

        QVector<QVector<bool> > m_processedMarker;
        HorizontalRunlengthImage *m_horizontalRunlengthImage;
    };
}

#endif
