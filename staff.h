#ifndef STAFF_H
#define STAFF_H

#include <QPoint>
#include <QVector>
#include <QHash>

#include "segments.h"

class StaffLine;


namespace Munip {
    // Forwared declarations

    class StaffLine;

    class StaffLine
    {
    public:
        StaffLine(const QPoint& start, const QPoint& end, int thickness);
        StaffLine();
        ~StaffLine();

        QPoint startPos() const;
        void setStartPos(const QPoint& point);

        QPoint endPos() const;
        void setEndPos(const QPoint& point);

        int staffLineID() const;
        void setStaffLineID(int);

        int lineWidth() const;
        void setLineWidth(int wid);

        bool aggregate( StaffLine &line );

        bool contains( const Segment &segment);


        int length();

        void addSegment(const Segment& segment);
        QVector<Segment> segments() const;

        bool isValid() const;

        void displaySegments();

        void sortSegments();

        //bool operator<(StaffLine line);
        //bool operator==(StaffLine line);


    private:
        QPoint m_startPos;
        QPoint m_endPos;
        int m_lineWidth; // width between the current line and the next. -1 if last line
        int m_staffLineID; // The Staff Number To Which The Line Belongs
        QVector<Segment> m_segmentList;
        // int error; //Contains The Error Code incase of Parallax
        // int paralax;
    };

    class Staff
    {
    public:
        Staff(const QPoint& vStart, const QPoint& vEnd);
        ~Staff();
        Staff();

        QPoint startPos() const;
        void setStartPos(const QPoint& point);

        QPoint endPos() const;
        void setEndPos(const QPoint& point);

        QVector<StaffLine> staffLines() const;
        void addStaffLine(const StaffLine& staffLine);
		
        void addStaffLineList(QVector<StaffLine> list) ;

        bool operator<(Staff& other);

        void clear();

        int distance( int index);
    private:
        QVector<StaffLine> m_staffLines;
        QPoint m_startPos;
        QPoint m_endPos;

    };
}
#endif
