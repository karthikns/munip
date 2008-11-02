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


    /**
     * The functionality of class is to detect and remove the staff
     * lines and store the data in appropriate data structures.
     *
     * The steps involved are
     * 1) Calculate vertical run-lengths for each column.
     * 2) Connected componented analysis to remove the symbols.
     * 3) Yet to add.
     */
    class StaffLineRemover
    {
    public:
        StaffLineRemover(Page *page);
        ~StaffLineRemover();

        void removeLines();
        void removeLines2();
        QList<QPoint> checkForLine(int seedX, int seedY);
        QImage processedImage() const;

        QList<Staff> staffList() const;

    private:
        Page *m_page;
        QList<Staff> m_staffList;

        /**
         * This is the image with all the staff lines removed.
         */
        MonoImage m_processedImage;
    };

    /**
     * This class represents a single page characterstics. We need to
     * create multiple page objects to process multiple pages (because
     * different pages might have different characterstics).
     */
    class Page
    {
    public:
        Page(const MonoImage& image);
        ~Page();

        const MonoImage& originalImage() const;
        void process();

        MonoImage staffLineRemovedImage() const;
    private:
        MonoImage m_originalImage;
        StaffLineRemover *m_staffLineRemover;
        int m_staffSpaceHeight;
        int m_staffLineHeight;
    };
}

#endif
