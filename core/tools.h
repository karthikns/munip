#ifndef TOOLS_H
#define TOOLS_H

#include "segments.h"
#include "staff.h"

#include <QColor>
#include <QDebug>
#include <QImage>

extern bool EnableMDebugOutput;

inline QDebug mDebug()
{
    if (EnableMDebugOutput) {
        return qDebug();
    }
    return QDebug(new QString);
}

inline QDebug mWarning()
{
    if (EnableMDebugOutput) {
        return qWarning();
    }
    return QDebug(new QString);
}

namespace Munip
{
    struct IDGenerator
    {
        static int lastID;

        static int gen()
        {
            return ++lastID;
        }
    };

    struct Range
    {
        Range(int _min = 0, int _max = 0) { min = _min, max = _max; }

        int size() const { return max - min; }
        int dominantValue() const {
            return size() == 1 ? min : int(qRound(.5 * (max + min)));
        }

        int min;
        int max;
    };

    struct Run
    {
        int pos; // x or y coordinate as needed.
        int length; // length of run

        explicit Run(int _pos = -1, int _length=0) :
            pos(_pos),
            length(_length)
        {
        }

        int endPos() const { return pos + length; }

        bool isValid() const {
            return pos >= 0 && length > 0;
        }

        bool operator==(const Run& rhs) const {
            return pos == rhs.pos && length == rhs.length;
        }

        bool operator!=(const Run& rhs) const {
            return !(*this == rhs);
        }
    };

    struct RunCoord
    {
        int pos;
        Run run;

        explicit RunCoord(int p = -1, const Run& r = Run()) : pos(p), run(r)
        {
        }

        bool isValid() const {
            return pos >= 0 && run.isValid();
        }

        bool operator==(const RunCoord& rhs) const {
            return pos == rhs.pos && run == rhs.run;
        }

        bool operator!=(const RunCoord& rhs) const {
            return !(*this == rhs);
        }
    };

    class RunlengthImage
    {
    public:
        explicit RunlengthImage(const QImage& image, Qt::Orientation orientation,
                const QColor& color = QColor(Qt::black));
        virtual ~RunlengthImage();

        Qt::Orientation orientation() const;

        QSize size() const;
        QRect rect() const;

        const QList<Run>& runs(int index) const;
        Run run(int x, int y) const;

        QList<Run> adjacentRunsInNextLine(const RunCoord& runCoord) const;

    private:
        static const QList<Run> InvalidRuns;

        Qt::Orientation m_orientation;
        QList<QList<Run> >  m_data;
        QSize m_size;
    };

    class VerticalRunlengthImage : public RunlengthImage
    {
    public:
        explicit VerticalRunlengthImage(const QImage& image,
                const QColor& color = QColor(Qt::black));
        ~VerticalRunlengthImage();

        const QList<Run>& runsForColumn(int index) const;
        QList<Run> adjacentRunsInNextColumn(const RunCoord& runCoord) const;
    };

    template<typename X>
    static void resizeList(QList<X> &list, int size, const X& defaultValue)
    {
        if (list.size() > size) {
            list.erase(list.begin() + size, list.end());
        } else if (list.size() < size) {
            for (int i = list.size(); i < size; ++i) {
                list << defaultValue;
            }
        }
    }

    QImage convertToMonochrome(const QImage& image, int threshold = 200);
    QPointF meanOfPoints(const QList<QPoint>& pixels);
    QList<double> covariance(const QList<QPoint>& blackPixels, QPointF mean);
    double highestEigenValue(const QList<double> &matrix);


    bool segmentSortByWeight(Segment &s1,Segment &s2);
    bool segmentSortByConnectedComponentID(Segment &s1,Segment &s2);
    bool segmentSortByPosition(Segment &s1,Segment &s2);
    bool staffLineSort(StaffLine &line1,StaffLine &line2);
    bool symbolRectSort(QRect &symbolRect1,QRect &symbolRect2);
}

inline QDebug operator<<(QDebug dbg, const Munip::Range& range)
{
    dbg.nospace() << "Range(" << range.min << "-->" << range.max <<") = " << range.size();
    return dbg.space();
}

inline QDebug operator<<(QDebug dbg, const Munip::Run& run)
{
    dbg.nospace() << "Run [" << run.pos << "] --> " << run.length << " pixels";
    return dbg.space();
}

//! A simple hash function for use with QHash<QPoint>.
// Assumption: p is +ve and p.x() value is <= 4000
inline uint qHash(const QPoint& p)
{
    return p.x() * 4000 + p.y();
}

namespace Munip {
    inline uint qHash(const Munip::Run& run)
    {
        if (!run.isValid()) return 0;
        return run.pos * 4000 + run.length + 1;
    }

    inline uint qHash(const Munip::RunCoord &runCoord)
    {
        if (!runCoord.isValid()) return 0;
        return runCoord.pos * 4000 + qHash(runCoord.run) + 1;
    }
}

#endif
