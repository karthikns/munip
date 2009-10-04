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
		m_staffID = -1;	 // still to be set
    }

    StaffLine :: StaffLine()
    {
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

    bool StaffLine :: aggregate(StaffLine &line )
    {
        //TODO how can u aggregate a line at (i,0) to line above it ;)
        if( line.startPos().y() < m_startPos.y() || line.endPos().y() < m_endPos.y() )
        {

            return false;
        }

        if( line.startPos().y() - (m_startPos.y()+m_lineWidth-1) > 1 )
        {

            return false;
        }

        if( line.startPos().x() > m_endPos.x()+1 || line.endPos().x() < m_startPos.x() -1 )
        {

            return false;
        }

        //qDebug() <<"The line in list"<< m_startPos << m_endPos <<m_lineWidth;
        //qDebug() <<"The external line "<< line.startPos() << line.endPos() <<line.lineWidth();

        if( line.startPos().y() == m_startPos.y() )
        {
            m_endPos.setX(line.endPos().x());
            //qDebug() <<Q_FUNC_INFO <<" condition 1";
            return true;
        }

        if( line.startPos().x() >= m_startPos.x() -1 && line.endPos().x() <= m_endPos.x()+1 )
        {
            if( !(m_startPos.y() + m_lineWidth -1 == line.startPos().y() ) )
                m_lineWidth++;
            //qDebug() <<Q_FUNC_INFO <<" condition 2";
             return true;
        }

        if( line.startPos().x() < m_startPos.x()-1 && line.endPos().x() > m_endPos.x()+1 )
        {
             if( !(m_startPos.y() + m_lineWidth -1 == line.startPos().y()) )
                m_lineWidth++;

             m_startPos.setX(line.startPos().x());
             m_endPos.setX(line.endPos().x());
             //qDebug() <<Q_FUNC_INFO <<" condition 3";
             return true;
         }

        if( line.startPos().x() >= m_startPos.x()-1 )
        {
             if( !(startPos().y() + m_lineWidth -1 == line.startPos().y()) )
                m_lineWidth++;
             m_endPos.setX(line.endPos().x());
             //qDebug() <<Q_FUNC_INFO <<" condition 4";
             return true;
        }

        if( line.endPos().x() <= m_endPos.x()+1 )
        {
             if( !(startPos().y() + m_lineWidth -1 == line.startPos().y()) )
                m_lineWidth++;

             m_startPos.setX(line.startPos().x());
             //qDebug() <<Q_FUNC_INFO <<" condition 5";
             return true;
        }

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
        if( index == 0 || index > 5 )  // TODO must be extensible
            return -1;
        int distance = m_staffLines[index].endPos().y() - m_staffLines[index-1].endPos().y();
        distance -= m_staffLines[index-1].lineWidth();
        return distance;
    }

}
