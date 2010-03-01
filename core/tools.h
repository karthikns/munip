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

    template <typename X>
    struct Range
    {
        Range(const X& _min = 0, const X& _max = 0) { min = _min, max = _max; }

        int size() const { return max - min; }

        X min;
        X max;
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

template<typename X>
inline QDebug& operator<<(QDebug& dbg, const Munip::Range<X>& range)
{
    dbg << "Range(" << range.min << "-->" << range.max <<") = " << range.size();
    return dbg.space();
}

#endif
