#include "staff.h"

#include "tools.h"

#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QRgb>
#include <QSet>
#include <QVector>
#include <cmath>


namespace Munip
{

    StaffLine::StaffLine(const QPoint& start, const QPoint& end, int thickness)
    {
        m_startPos = start;
        m_endPos = end;
        m_lineWidth = thickness;
        m_staffLineID = -1;	 // still to be set
        constructBoundingRect();
    }

    StaffLine :: StaffLine()
    {
    }

    StaffLine::~StaffLine()
    {

    }

    void StaffLine::constructBoundingRect()
    {
        QPoint minPoint,maxPoint;
        minPoint.setX(qMin(m_startPos.x(),m_endPos.x()));
        minPoint.setY(qMin(m_startPos.y(),m_endPos.y()));
        maxPoint.setX(qMax(m_startPos.x(),m_endPos.x()));
        maxPoint.setY(qMax(m_startPos.y(),m_endPos.y()));
        m_boundingBox.setTopLeft(minPoint);
        m_boundingBox.setBottomRight(maxPoint);
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

    int StaffLine::lineWidth() const
    {
        return m_lineWidth;
    }

    void StaffLine::setLineWidth(int wid)
    {
        m_lineWidth = wid;
    }

    void StaffLine::setMaxPosition(const QPoint &maxPos)
    {
        m_boundingBox.setBottomRight(maxPos);
    }

    void StaffLine::setMinPosition(const QPoint &minPos)
    {
        m_boundingBox.setTopLeft(minPos);
    }

    QRect StaffLine::boundingBox() const
    {
        return m_boundingBox;
    }

    int StaffLine :: length()
    {
        return (m_endPos.x() - m_startPos.x());
    }

    bool StaffLine ::contains(const Segment &segment)
    {
       for(int i = 0; i < m_segmentList.size();i++)
           if( m_segmentList[i] == segment)
               return true;
       return false;
    }

    bool StaffLine::isAdjacent(const StaffLine &line)
    {
        return qAbs(m_boundingBox.bottom() -
                line.boundingBox().top()) == 1;
    }

    bool StaffLine :: aggregate(StaffLine &line )
    {

        if( !isAdjacent(line) )
            return false;

        qDebug()<<Q_FUNC_INFO<<m_boundingBox<<line.boundingBox();
        QVector<Segment> lineSegments = line.segments();
        addSegmentList(lineSegments);
        m_boundingBox|=line.boundingBox();
        return true;

        /*
        //TODO how can u aggregate a line at (i,0) to line above it ;)
        if( line.startPos().y() < m_startPos.y() || line.endPos().y() < m_endPos.y() )
            return false;


        if( line.startPos().y() - (m_startPos.y()+m_lineWidth-1) > 1 )
            return false;


        if( line.startPos().x() > m_endPos.x()+1 || line.endPos().x() < m_startPos.x() -1 )
            return false;


        if( line.startPos().y() == m_startPos.y() )
        {
            m_endPos.setX(line.endPos().x());
            return true;
        }

        if( line.startPos().x() >= m_startPos.x() -1 && line.endPos().x() <= m_endPos.x()+1 )
        {
            if( !(m_startPos.y() + m_lineWidth -1 == line.startPos().y() ) )
                m_lineWidth++;
             return true;
        }

        if( line.startPos().x() < m_startPos.x()-1 && line.endPos().x() > m_endPos.x()+1 )
        {
             if( !(m_startPos.y() + m_lineWidth -1 == line.startPos().y()) )
                m_lineWidth++;

             //m_startPos.setX(line.startPos().x());
             //m_endPos.setX(line.endPos().x());
             m_endPos = line.endPos();
             return true;
         }

        if( line.startPos().x() >= m_startPos.x()-1 )
        {
             if( !(startPos().y() + m_lineWidth -1 == line.startPos().y()) )
                m_lineWidth++;
             //m_endPos.setX(line.endPos().x());
             m_endPos = line.endPos();
             return true;
        }

        if( line.endPos().x() <= m_endPos.x()+1 )
        {
             if( !(startPos().y() + m_lineWidth -1 == line.startPos().y()) )
                m_lineWidth++;

             //m_startPos.setX(line.startPos().x());
             return true;
        }
        */

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
            //m_boundingBox |= QRect(segment.startPos(), segment.endPos());
        }

    }

    void StaffLine::addSegmentList(QVector<Segment> &segmentList)
    {
        foreach(Segment s,segmentList)
           if(s.isValid())
           {
                m_segmentList.push_back(s);
                if( s.startPos().x() < m_startPos.x() )
                    m_startPos.setX(s.startPos().x());
                if( s.endPos().x() > m_endPos.x() )
                    m_endPos.setX(s.endPos().x());
                //m_boundingBox |= QRect(s.startPos(), s.endPos());
           }
    }

    QVector<Segment> StaffLine::segments() const
    {
        return m_segmentList;
    }

    bool StaffLine ::isValid() const
    {
        return !( m_startPos == QPoint(-1,-1) || m_endPos == QPoint(-1,-1) || m_lineWidth == -1);
    }

    void StaffLine ::displaySegments()
    {
        for(int i= 0; i < m_segmentList.size(); i++)
                  qDebug()<<m_segmentList[i].startPos()<<m_segmentList[i].endPos()<<m_segmentList[i].destinationPos();
    }

    void StaffLine ::sortSegments()
    {
        qSort(m_segmentList.begin(),m_segmentList.end(),segmentSortByPosition);
    }

    bool StaffLine ::findSegment(Segment& s) const
    {
        for(int i = 0; i < m_segmentList.size();i++)
            if(m_segmentList[i] == s)
                return true;
        return false;

    }

    Staff::Staff(const QPoint& vStart, const QPoint& vEnd)
    {
        m_startPos = vStart;
        m_endPos = vEnd;
    }
	
    Staff :: Staff()
    {
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

    QVector<StaffLine> Staff::staffLines() const
    {
        return m_staffLines;
    }

    void Staff::addStaffLine(const StaffLine& sline)
    {
        m_staffLines.append(sline);
    }
	
    void Staff :: addStaffLineList(QVector<StaffLine> list)
    {
		m_staffLines = list;
    }

    bool Staff::operator<(Staff& other)
    {
        return m_startPos.y() < other.m_startPos.y();
    }

    void Staff :: clear()
    {
        m_staffLines.clear();
    }

    int Staff ::distance( int index )
    {
        qDebug() << Q_FUNC_INFO << " " << index << m_staffLines.size();
        if( index == 0 || index > 5 )  // TODO must be extensible
            return -1;
        int distance = m_staffLines[index].endPos().y() - m_staffLines[index-1].endPos().y();
        distance -= m_staffLines[index-1].lineWidth();
        return distance;
    }

}
