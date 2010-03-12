#include "staff.h"

#include "datawarehouse.h"
#include "tools.h"

#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QRgb>
#include <QSet>
#include <QList>
#include <cmath>


namespace Munip
{
    StaffLine::StaffLine(const QPoint& start = QPoint(-1, -1),
            const QPoint& end = QPoint(-1, -1))
    {
        m_startPos = start;
        m_endPos = end;
        m_staffLineID = -1;	 // still to be set

        if (start != QPoint(-1, -1)) {
            QPoint minPoint,maxPoint;
            minPoint.setX(qMin(m_startPos.x(),m_endPos.x()));
            minPoint.setY(qMin(m_startPos.y(),m_endPos.y()));
            maxPoint.setX(qMax(m_startPos.x(),m_endPos.x()));
            maxPoint.setY(qMax(m_startPos.y(),m_endPos.y()));
            m_boundingRect.setTopLeft(minPoint);
            m_boundingRect.setBottomRight(maxPoint);
        }
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

    int StaffLine::staffLineID() const
    {
        return m_staffLineID;
    }

    void StaffLine::setStaffLineID(int id)
    {
        m_staffLineID = id;
    }

    QRect StaffLine::boundingBox() const
    {
        return m_boundingRect;
    }

    int StaffLine::length()
    {
        return (m_endPos.x() - m_startPos.x());
    }

    bool StaffLine::contains(const Segment &segment) const
    {
       for(int i = 0; i < m_segmentList.size();i++)
           if( m_segmentList[i] == segment)
               return true;
       return false;
    }

    bool StaffLine::isAdjacent(const StaffLine &line)
    {
        return qAbs(m_boundingRect.bottom() -
                line.boundingBox().top()) == 1;
    }

    bool StaffLine::aggregate(StaffLine &line )
    {

        /*A better way of aggregating i feel
         *Please examine
         *and scrutinise

        int i = 0,j = 0;
        QList<Segment> segmentList = line.segments();
        while( i < m_segmentList.size() && j < segmentList.size())
        {
            if(m_segmentList[i].isConnected(segmentList[j]))
                j++;
            else
                i++;
        }
        if( i == m_segmentList.size() )
        {
            this->addSegmentList(segmentList);
            return true;
        }
        return false;
        */

        if(this->isAdjacent(line))
        {
           QList<Segment> segmentList = line.segments();
           this->addSegmentList(segmentList);
           return true;
        }
       return false;
    }

    void StaffLine::addSegment(const Segment &segment)
    {
        if(segment.isValid())
        {
            m_segmentList.push_back(segment);
            if( segment.startPos().x() < m_startPos.x() )
                m_startPos.setX(segment.startPos().x());
            if( segment.endPos().x() > m_endPos.x() )
                m_endPos.setX(segment.endPos().x());
            m_boundingRect |= QRect(segment.startPos(), segment.endPos());
        }

    }

    void StaffLine::addSegmentList(QList<Segment> &segmentList)
    {
        foreach(Segment s,segmentList)
           if(s.isValid())
           {
                m_segmentList.push_back(s);
                if( s.startPos().x() < m_startPos.x() )
                    m_startPos.setX(s.startPos().x());
                if( s.endPos().x() > m_endPos.x() )
                    m_endPos.setX(s.endPos().x());
                m_boundingRect |= QRect(s.startPos(), s.endPos());
           }
    }

    QList<Segment> StaffLine::segments() const
    {
        return m_segmentList;
    }

    void StaffLine::displaySegments()
    {
        for(int i= 0; i < m_segmentList.size(); i++)
                  qDebug()<<m_segmentList[i].startPos()<<m_segmentList[i].endPos()<<m_segmentList[i].destinationPos();
    }

    void StaffLine::sortSegments()
    {
        qSort(m_segmentList.begin(),m_segmentList.end(),segmentSortByPosition);
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
        constructStaffBoundingRect();
    }

    void Staff::addStaffLineList(const QList<StaffLine>& list)
    {
        m_staffLines = list;
        constructStaffBoundingRect();
    }

    bool Staff::operator<(const Staff& other) const
    {
        return m_startPos.y() < other.m_startPos.y();
    }

    void Staff::clear()
    {
        m_staffLines.clear();
    }

    QRect Staff::boundingRect() const
    {
        return m_boundingRect;

    }

    QRect Staff::staffBoundingRect() const
    {
        return m_staffBoundingRect;
    }

    void Staff::constructStaffBoundingRect()
    {
        if (m_staffLines.isEmpty()) {
            qWarning() << Q_FUNC_INFO <<
                "This staff hasn't been constructed completely";
            return;
        }
        QRect r;

        StaffLine boundaryLines[2] = {
            m_staffLines.first(),
            m_staffLines.last()
        };

        for (int i = 0; i <= 1; ++i) {
            const StaffLine& line = boundaryLines[i];

            const QList<Segment> segments = line.segments();
            foreach (const Segment& segment, segments) {
                QRect segRect(segment.startPos(), segment.endPos());
                if (r.isNull()) {
                    r = segRect;
                } else {
                    r |= segRect;
                }
            }
            if (r.isNull()) {
                r = line.boundingBox();
            } else {
                r |= line.boundingBox();
            }
        }
        m_staffBoundingRect = r;
    }

    void Staff::setBoundingRect(const QRect& boundingRect )
    {
        m_boundingRect = boundingRect;
    }

}
