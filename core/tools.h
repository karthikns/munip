#ifndef TOOLS_H
#define TOOLS_H

#include "segments.h"
#include "staff.h"

#include <QDebug>
#include <QImage>

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

    QImage convertToMonochrome(const QImage& image, int threshold = 200);
    QPointF meanOfPoints(const QList<QPoint>& pixels);
    QList<double> covariance(const QList<QPoint>& blackPixels, QPointF mean);
    double highestEigenValue(const QList<double> &matrix);


    bool segmentSortByWeight(Segment &s1,Segment &s2);
    bool segmentSortByConnectedComponentID(Segment &s1,Segment &s2);
    bool segmentSortByPosition(Segment &s1,Segment &s2);
    bool staffLineSort(StaffLine &line1,StaffLine &line2);
    bool symbolRectSort(QRect &symbolRect1,QRect &symbolRect2);

    double normalizedLineTheta(const QLineF& line);
}

inline QDebug& operator<<(QDebug& dbg, const Munip::Range& range)
{
    dbg << "Range(" << range.min << "-->" << range.max <<") = " << range.size();
    return dbg.space();
}

#endif
