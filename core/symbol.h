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

    struct NoteSegment
    {
        static NoteSegment* create() { return new NoteSegment; }

        QRect boundingRect;
        QList<QRect> noteRects;

        QHash<int, int> horizontalProjection;

        /// Its enough to compare bounding rectangles as two note segments
        /// can't have same bounds.
        bool isEqualTo(const NoteSegment* rhs) const {
            return boundingRect == rhs->boundingRect;
        }

    private:
        NoteSegment() {}
    };

    struct StemSegment
    {
        static StemSegment* create() { return new StemSegment; }

        QRect boundingRect;
        NoteSegment *noteSegment;
        int leftFlagCount;
        int rightFlagCount;

        int totalFlagCount() const {
            return qMax(leftFlagCount, rightFlagCount);
        }

        bool isEqualTo(const StemSegment* other) const {
            return noteSegment == other->noteSegment &&
                boundingRect == other->boundingRect;
        }

    private:
        StemSegment() {
            noteSegment = 0;
            leftFlagCount = rightFlagCount = 0;
        }
    };

    struct StaffData
    {
        StaffData(const QImage& img, const Staff& staff);
        ~StaffData();

        void process();

        void findSymbolRegions();

        void findMaxProjections();
        int determinePeakHValueFrom(const QList<int> &horProjValues);

        void extractNoteSegments();
        QHash<int, int> filter(Range width, Range height, const QHash<int, int> &hash);

        void extractStemSegments();

        void eraseStems();
        void extractBeams();
        StemSegment* stemSegmentForPoint(const QPoint& p);
        QList<QPoint> solidifyPath(const QList<QPoint> &pathPoints,
                const StemSegment* left, const StemSegment* right,
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

        QList<NoteSegment*> noteSegments;
        QList<StemSegment*> stemSegments;
        QHash<QPoint, int> beamPoints;

        const QImage& image;
        QImage workImage;
    };

}

#endif
