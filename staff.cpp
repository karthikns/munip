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

    QList<StaffLine> Staff::staffLines() const
    {
        return m_staffLines;
    }

    void Staff::addStaffLine(const StaffLine& sline)
    {
        m_staffLines.append(sline);
    }
	
	void Staff :: addStaffLineList(QList<StaffLine> list)
	{
		m_staffLines = list;
	}

    bool Staff::operator<(Staff& other)
    {
        return m_startPos.y() < other.m_startPos.y();
    }
}
