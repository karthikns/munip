#ifndef STAFF_H
#define STAFF_H

#include <QPoint>
#include <QRect>
#include <QList>
#include <QHash>

#include "segments.h"

class StaffLine;


namespace Munip {

    class StaffLine
    {
    public:
        StaffLine(const QPoint& start, const QPoint& end);
        ~StaffLine();

        int staffLineID() const;
        void setStaffLineID(int);

        QPoint startPos() const;
        void setStartPos(const QPoint& point);

        QPoint endPos() const;
        void setEndPos(const QPoint& point);

        int length();

        QRect boundingBox() const;

        bool aggregate(StaffLine &line);
        bool isAdjacent(const StaffLine &line);

        QList<Segment> segments() const;
        bool contains(const Segment &segment) const;

        void addSegment(const Segment& segment);
        void addSegmentList(QList<Segment> &segmentList);

        void sortSegments();

        void displaySegments();

    private:
        QPoint m_startPos;
        QPoint m_endPos;
        int m_staffLineID; // The Staff Number To Which The Line Belongs
        QList<Segment> m_segmentList;
        QRect m_boundingRect;
    };

    class Staff
    {
    public:
        Staff(const QPoint& vStart = QPoint(),
                const QPoint& vEnd = QPoint());
        ~Staff();

        QPoint startPos() const;
        void setStartPos(const QPoint& point);

        QPoint endPos() const;
        void setEndPos(const QPoint& point);

        QList<StaffLine> staffLines() const;
        void addStaffLine(const StaffLine& staffLine);
        void addStaffLineList(const QList<StaffLine>& list) ;

        bool operator<(const Staff& other) const;

        void clear();

        QRect boundingRect() const;
        void setBoundingRect(const QRect& boundingRect);

        // This rectangle does not include symbols above or below boudary
        // stafflines.
        QRect staffBoundingRect() const;
        void constructStaffBoundingRect();

    private:
        QList<StaffLine> m_staffLines;
        QPoint m_startPos;
        QPoint m_endPos;
        QRect m_boundingRect;
        QRect m_staffBoundingRect;
    };
}
#endif
