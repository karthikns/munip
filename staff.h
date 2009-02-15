#ifndef STAFF_H
#define STAFF_H

#include "monoimage.h"

#include <QPoint>
#include <QList>

namespace Munip {
    // Forwared declarations
    class Page;

    class StaffLine
    {
    public:
        StaffLine(const QPoint& start, const QPoint& end, int staffID);
        ~StaffLine();

        QPoint startPos() const;
        void setStartPos(const QPoint& point);

        QPoint endPos() const;
        void setEndPos(const QPoint& point);

        int staffID() const;
        void setStaffID(int);

        int lineWidth() const;
        void setLineWidth(int wid);

    private:
        QPoint m_startPos;
        QPoint m_endPos;
        int m_lineWidth; // width between the current line and the next. -1 if last line
        int m_staffID; // The Staff Number To Which The Line Belongs

        // int error; //Contains The Error Code incase of Parallax
        // int paralax;
    };

    class Staff
    {
    public:
        Staff(const QPoint& vStart, const QPoint& vEnd);
        ~Staff();

        QPoint startPos() const;
        void setStartPos(const QPoint& point);

        QPoint endPos() const;
        void setEndPos(const QPoint& point);

        QList<StaffLine> staffLines() const;
        void addStaffLine(const StaffLine& staffLine);

        bool operator<(Staff& other);

    private:
        QList<StaffLine> m_staffLines;
        QPoint m_startPos;
        QPoint m_endPos;

    };
}

#endif
