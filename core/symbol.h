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
        QRect boundingRect;
        QList<QRect> noteRects;

        QHash<int, int> horizontalProjection;

        bool operator<(const NoteHeadSegment& other) const {
            return boundingRect.left() < other.boundingRect.left();
        }

        bool operator==(const NoteHeadSegment& other) const {
            return boundingRect == other.boundingRect
                && noteRects == other.noteRects;
        }

        bool operator!=(const NoteHeadSegment& other) const {
            return !((*this) == other);
        }
    };

    struct StemSegment
    {
        QRect boundingRect;
        NoteHeadSegment noteHeadSegment;
        int flagCount;
        bool beamAtTop;

        StemSegment() { beamAtTop = true; flagCount = 0;}

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

        void eraseStems();
        void extractBeams();
        StemSegment stemSegmentForPoint(const QPoint& p, bool &validOutput);
        QList<QPoint> solidifyPath(const QList<QPoint> &pathPoints,
                const StemSegment& left, const StemSegment& right,
                QSet<QPoint> &visited);

        void eraseBeams();
        void extractChords();

        void eraseChords();
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
        QImage workImage;
    };

}

#endif
