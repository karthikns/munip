#ifndef TOOLS_H
#define TOOLS_H

#include "segments.h"
#include "staff.h"

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

#endif
