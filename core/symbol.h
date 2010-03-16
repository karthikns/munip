#ifndef SYMBOL_H
#define SYMBOL_H

#include "staff.h"
#include "tools.h"

#include <QList>
#include <QRect>
#include <QImage>

class QImage;

namespace Munip
{
    class Range;

    struct NoteHeadSegment
    {
        QRect rect;
        QList<QRect> noteRects;

        QHash<int, int> horizontalProjection;

        bool operator<(const NoteHeadSegment& other) const {
            return rect.left() < other.rect.left();
        }

        bool operator==(const NoteHeadSegment& other) const {
            return rect == other.rect && noteRects == other.noteRects;
        }

        bool operator!=(const NoteHeadSegment& other) const {
            return !((*this) == other);
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

        bool operator==(const StemSegment& other) const {
            return boundingRect == other.boundingRect &&
                noteHeadSegment == other.noteHeadSegment &&
                beamAtTop == other.beamAtTop;
        }

        bool operator!=(const StemSegment& other) const {
            return !((*this) == other);
        }
    };

    struct StaffData
    {
        StaffData(const QImage& img, const Staff& staff);

        void process();

        void findSymbolRegions();

        void findMaxProjections();
        int determinePeakHValueFrom(const QList<int> &horProjValues);

        void extractNoteHeadSegments();
        QHash<int, int> filter(Range width, Range height, const QHash<int, int> &hash);

        void extractStemSegments();

        void extractBeams();
        StemSegment stemSegmentForPoint(const QPoint& p, bool &validOutput);
        QList<QPoint> solidifyPath(const QList<QPoint> &pathPoints,
                const StemSegment& left, const StemSegment& right,
                QSet<QPoint> &visited);

        void extractChords();

        void extractFlags();

        QImage staffImage() const;
        QImage projectionImage(const QHash<int, int> &hash) const;
        QImage noteHeadHorizontalProjectionImage() const;

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
