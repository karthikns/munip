#include "staff.h"

#include "tools.h"

#include <QDebug>
#include <QLabel>
#include <QRgb>
#include <QSet>
#include <QVector>

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

    /**
     * This is method I(gopal) tried to remove lines. The logic is as
     * follows
     *
     * - Look for a black pixel in a row-wise left-to-right fashion.
     *
     * - Call checkForLine method, which returns a list of points
     *   forming a line. The method returns an empty list if there is
     *   no possibility of lines.
     *
     * - Mark all the above points white.
     *
     * @todo Don't remove points that can be part of symbol.
     */
    void StaffLineRemover::removeLines2()
    {
        // Important: USE m_processedImage everywhere as it has an
        //            updated white-marked image which acts as input
        //            to next stage, thereby reducing false positives
        //            considerably.
        QRect imgRect = m_processedImage.rect();
        MonoImage display(m_processedImage);
        for(int i = 0; i < imgRect.right(); ++i)
            for(int j = 0; j < imgRect.height(); ++j)
                display.setPixelValue(i, j, MonoImage::Black);


        for(int y = imgRect.top(); y <= imgRect.bottom(); ++y)
        {
            for(int x = imgRect.left(); x <= imgRect.right(); ++x)
            {
                if (m_processedImage.pixelValue(x, y) == MonoImage::Black) {
                    //// Some work delegated.
                    QList<QPoint> lineSlithers = checkForLine(x, y);

                    if (lineSlithers.isEmpty()) {
                        continue;
                    }

                    int maxX = x, maxY = y;
                    foreach(QPoint point, lineSlithers) {
                        maxX = qMax(point.x(), maxX);
                        maxY = qMax(point.y(), maxY);

                        m_processedImage.setPixelValue(point.x(), point.y(), MonoImage::White);
                        display.setPixelValue(point.x(), point.y(), MonoImage::White);

                    }
                    x = maxX, y = maxY;
                }
            }
        }

        // This shows an image in which only the above detected points
        // are marked white and rest all is black.
        QLabel *label = new QLabel();
        label->setPixmap(QPixmap::fromImage(display));
        label->show();

    }

    /**
     * The logic is as follows.
     *
     * - It first determines a continuous horizontal line (can have
     *   holes in between line, but continuous)
     *
     * - Also nearing points to the above belonging to line (say line
     *   of thickness 3 pixels) are checked and added as part of line.
     *
     * - If the horizontal length is greater than some threshold, then
     *   all the above points are returned.
     */
    QList<QPoint> StaffLineRemover::checkForLine(int x, int y)
    {
        QList<QPoint> slithers;
        slithers.append(QPoint(x, y));
        bool isLine = false;
        QPoint lastSlither(x, y);
        int minY, maxY;
        minY = maxY = y;

        int i = x+1;
        for(; i < m_processedImage.width(); ++i) {
            if (m_processedImage.pixelValue(i, lastSlither.y()) == MonoImage::Black) {
                ;
            }
            else if (lastSlither.y() > 0 && m_processedImage.pixelValue(i, lastSlither.y()-1) == MonoImage::Black) {
                lastSlither.ry()--;
            }
            else if (lastSlither.y() < (m_processedImage.height()-1) &&
                     m_processedImage.pixelValue(i, lastSlither.y() + 1) == MonoImage::Black) {
                lastSlither.ry()++;

            }
            else {
                break;
            }

            lastSlither.rx() = i;
            slithers.append(lastSlither);
            minY = qMin(minY, lastSlither.y());
            maxY = qMax(maxY, lastSlither.y());
        }



        const int ThreasholdLineLength = 80;
        isLine = (i-x) > ThreasholdLineLength;
        if (isLine) {
            // for(int j = x; j < i; ++j) {
            //     for(int k = minY; k <= maxY; ++k) {
            //         if (m_processedImage.pixelValue(j, k) == MonoImage::Black) {
            //             slithers.append(QPoint(j, k));
            //         }
            //     }
            // }
            return slithers;
        }
        else {
            slithers.clear();
            return slithers;
        }
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
