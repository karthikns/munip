#ifndef SYMBOL_H
#define SYMBOL_H

#include "staff.h"
#include "tools.h"

#include <QList>
#include <QRect>
#include <QImage>

namespace Munip
{
    class Range;

    struct NoteHeadSegment
    {
        QRect rect;
        QList<QRect> noteRects;

        bool operator<(const NoteHeadSegment& other) const {
            return rect.left() < other.rect.left();
        }
    };

    struct StemSegment
    {
        QRect boundingRect;
        NoteHeadSegment noteHeadSegment;
        bool beamAtTop;

        StemSegment() { beamAtTop = true; }

        bool operator<(const StemSegment& other) const {
            return boundingRect.left() < other.boundingRect.left();
        }
    };

    struct StaffData
    {
        StaffData(const QImage& img, const Staff& staff);

        void findSymbolRegions();
        void findMaxProjections();
        void findNoteHeadSegments();
        void findBeams();
        void extractNoteHeadSegments();
        void extractStemSegments();

        QImage staffImage() const;
        QImage projectionImage(const QHash<int, int> &hash) const;
        int determinePeakHValueFrom(const QList<int> &horProjValues);
        QHash<int, int> filter(Range width, Range height, const QHash<int, int> &hash);

        int SlidingWindowSize;

        Staff staff;
        QList<QRect> symbolRects;
        QHash<int, int> maxProjections;
        QHash<int, int> noteProjections;
        QHash<int, int> stemsProjections;

        QList<NoteHeadSegment> noteHeadSegments;
        QList<StemSegment> stemSegments;
        QHash<QPoint, int> beamPoints;

        const QImage& image;
    };

}

#endif
