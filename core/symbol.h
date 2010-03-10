#ifndef SYMBOL_H
#define SYMBOL_H

#include "staff.h"

#include <QList>
#include <QRect>
#include <QImage>

namespace Munip
{
    class Range;

    struct StaffData
    {
        StaffData(const QImage& img, const Staff& staff);

        void findSymbolRegions();
        void findMaxProjections();
        void findNoteHeads();
        void findStems();
        int determinePeakHValueFrom(const QList<int> &horProjValues);

        QHash<int, int> filter(Range width, Range height, const QHash<int, int> &hash);

        QImage staffImage() const;
        QImage projectionImage(const QHash<int, int> &hash) const;

        int SlidingWindowSize;

        Staff staff;
        QList<QRect> symbolRects;
        QHash<int, int> maxProjections;
        QHash<int, int> noteProjections;
        QHash<int, int> stemsProjections;

        const QImage& image;
    };

}

#endif
