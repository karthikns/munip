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
    class StemSegment;

    struct NoteSegment
    {
        static NoteSegment* create() { return new NoteSegment; }

        QRect boundingRect;
        QList<QRect> noteRects;
        StemSegment *stemSegment;

        QHash<int, int> horizontalProjection;

        /// Its enough to compare bounding rectangles as two note segments
        /// can't have same bounds.
        bool isEqualTo(const NoteSegment* rhs) const {
            return boundingRect == rhs->boundingRect;
        }

    private:
        NoteSegment() { stemSegment = 0; }
        //~NoteSegment() { delete stemSegment; }
    };

    struct StemSegment
    {
        static StemSegment* create() { return new StemSegment; }

        QRect boundingRect;
        NoteSegment *noteSegment;
        int leftFlagCount;
        int rightFlagCount;

        QSet<RunCoord> flagRunCoords;
        QSet<RunCoord> partialBeamRunCoords;

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

    struct Region
    {
        int id;
        QList<QPoint> points;
        QRect boundingRect;

        Region() { id = -1; }
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

        StemSegment* stemSegmentForRunCoord(const RunCoord &r);
        void extractBeams();
        void eraseBeams();

        void extractNotes();
        void eraseNotes();

        void extractFlags();
        void eraseFlags();

        void extractPartialBeams();
        void erasePartialBeams();

        void enhanceConnectivity();

        void extractRegions();

        QImage staffImage() const;
        QImage staffImageWithStaffLinesOnly() const;
        QImage projectionImage(const QHash<int, int> &hash) const;
        QImage noteHeadHorizontalProjectionImage() const;

        int SlidingWindowSize;

        Staff staff;
        QList<QRect> symbolRects;
        QHash<int, int> maxProjections;
        QHash<int, int> noteProjections;
        QHash<int, int> stemsProjections;

        QHash<int, int> temp;

        QList<NoteSegment*> noteSegments;
        QList<QList<RunCoord> > beamsRunCoords;
        QList<Region*> regions;

        const QImage& image;
        QImage workImage;
    };

}

#endif
