#include "staff.h"

#include "tools.h"

#include <QVector>
#include <QRgb>

namespace Munip
{
    StaffLine::StaffLine(const QPoint& start, const QPoint& end, int staffID)
    {
        m_startPos = start;
        m_endPos = end;
        m_staffID = staffID;
        m_lineWidth = -1; // still to be set
    }

    StaffLine::~StaffLine()
    {

    }

    QPoint StaffLine::startPos() const
    {
        return m_startPos;
    }

    void StaffLine::setStartPos(const QPoint& point)
    {
        m_startPos = point;
    }

    QPoint StaffLine::endPos() const
    {
        return m_endPos;
    }

    void StaffLine::setEndPos(const QPoint& point)
    {
        m_endPos = point;
    }

    int StaffLine::staffID() const
    {
        return m_staffID;
    }

    void StaffLine::setStaffID(int id)
    {
        m_staffID = id;
    }

    int StaffLine::lineWidth() const
    {
        return m_lineWidth;
    }

    void StaffLine::setLineWidth(int wid)
    {
        m_lineWidth = wid;
    }


    Staff::Staff(const QPoint& vStart, const QPoint& vEnd)
    {
        m_startPos = vStart;
        m_endPos = vEnd;
    }

    Staff::~Staff()
    {

    }


    QPoint Staff::startPos() const
    {
        return m_startPos;
    }

    void Staff::setStartPos(const QPoint& point)
    {
        m_startPos = point;
    }


    QPoint Staff::endPos() const
    {
        return m_endPos;
    }

    void Staff::setEndPos(const QPoint& point)
    {
        m_endPos = point;
    }

    QList<StaffLine> Staff::staffLines() const
    {
        return m_staffLines;
    }

    void Staff::addStaffLine(const StaffLine& sline)
    {
        m_staffLines.append(sline);
    }

    bool Staff::operator<(Staff& other)
    {
        return m_startPos.y() < other.m_startPos.y();
    }

    StaffLineRemover::StaffLineRemover(const QImage& monoImage) : m_monoImage(monoImage), m_processedImage(monoImage)
    {
    }

    StaffLineRemover::~StaffLineRemover()
    {
    }

    void StaffLineRemover::removeLines()
    {
        int startx,starty,endx,endy;
        startx = starty = 0;
        endx = startx + m_processedImage.width();
        endy = starty + m_processedImage.height();

        QVector<bool> parsed(endy - starty, false);

        for(int x = startx; x < endx; x++)
        {
            for(int y = starty; y < endy; y++)
            {
                if (parsed[y] == true) continue;

                MonoImage::MonoColor currPixel = m_monoImage.pixelValue(x, y);
                if(currPixel == MonoImage::Black)
                {
                    QPoint staffStart(x, y);
                    while(currPixel == MonoImage::Black && y < endy)
                    {
                        m_processedImage.setPixelValue(x, y, MonoImage::White);
                        parsed[y] = true;
                        y = y+1;
                        if (y < endy && parsed[y+1]) break;
                        // TODO : CHeck if the new y is parsed or not (should we check ? )
                        currPixel = m_monoImage.pixelValue(x, y);
                    }
                    QPoint staffEnd(x, y-1);
                    m_staffList.append(Staff(staffStart, staffEnd));
                }
            }
        }

        //qSort(m_staffList.begin(), m_staffList.end());
    }

    QImage StaffLineRemover::processedImage() const
    {
        return m_processedImage;
    }

    QList<Staff> StaffLineRemover::staffList() const
    {
        return m_staffList;
    }

}
