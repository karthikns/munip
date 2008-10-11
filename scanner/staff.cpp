#include "staff.h"

#include <QVector>
#include <QRgb>

namespace Munip
{
    int monoPixelValue(const QImage& image, int x, int y)
    {
        QRgb rgb = image.pixel(x, y);
        return qGray(rgb) == 0 ? 0:1;
    }

    // value = 0, then its black. Else its white.
    void setMonoPixelValue(QImage &image, int x, int y, int value)
    {
        Q_ASSERT(image.format() == QImage::Format_Mono);
        // But in colortable index 0 might represent white. To take
        // care of that we have following.
        QVector<QRgb> colorTable = image.colorTable();
        //  There are only 2 entries in colortable
        int blackIndex = qGray(colorTable[0]) == 0 ? 0 : 1;
        int whiteIndex = 1 - blackIndex;

        image.setPixel(x, y, value == 0 ? blackIndex : whiteIndex);
    }

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

    StaffLineRemover::StaffLineRemover(const QImage& monoImage)
    {
        m_monoImage = m_processedImage = monoImage;
    }

    StaffLineRemover::~StaffLineRemover()
    {
    }

    void StaffLineRemover::removeLines()
    {
        int startx,starty,endx,endy;
        startx = starty = 0;
        endx = startx+m_processedImage.width();
        endy = starty + m_processedImage.height();

        QVector<bool> parsed(endy - starty, false);

        for(int x = startx; x < endx; x++)
        {
            for(int y = starty; y < endy; y++)
            {
                if (parsed[y] == true) continue;

                int currPixel = monoPixelValue(m_monoImage, x, y);
                if(currPixel == 0)
                {
                    QPoint staffStart(x, y);
                    while(currPixel == 0 && y < endy)
                    {
                        setMonoPixelValue(m_processedImage, x, y, 1);
                        parsed[y] = true;
                        y = y+1;
                        if (y < endy && parsed[y+1]) break;
                        // TODO : CHeck if the new y is parsed or not (should we check ? )
                        currPixel = monoPixelValue(m_monoImage, x, y);
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
