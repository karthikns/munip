#include "segments.h"
#include<QDebug>

namespace Munip
{

    Segment ::Segment()
    {
    }

    Segment ::Segment(const QPoint& start,const QPoint& end)
    {
        m_startPos = start;
        m_endPos = end;
        m_destinationPos = end;
        m_connectedComponentID = -1;

    }

    Segment ::~Segment()
    {

    }

    void Segment ::setStartPos(const QPoint &pos)
    {
        m_startPos = pos;
    }

    QPoint Segment ::startPos() const
    {
        return m_startPos;
    }

    void Segment ::setEndPos(const QPoint &pos)
    {
        m_endPos = pos;
    }

    QPoint Segment ::endPos() const
    {
        return m_endPos;
    }

    int Segment ::weight() const
    {
        return (m_destinationPos.x()-m_startPos.x()+1);
    }

    void Segment ::setDestinationPos(const QPoint &position)
    {
        m_destinationPos = position;
    }

    QPoint Segment ::destinationPos() const
    {
        return m_destinationPos;
    }

    int Segment ::length() const
    {
        return (m_endPos.x()-m_startPos.x()+1);
    }

    void Segment ::setConnectedComponentID(const int  id)
    {
        m_connectedComponentID = id;
    }

    int Segment ::connectedComponentID() const
    {
        return m_connectedComponentID;
    }

    QList<Segment> Segment ::getConnectedSegments(QList<Segment> list)
    {
        int i = 0;
        QList<Segment> segments;
        while( i < list.size() && !isConnected(list[i]) )
            i++;

        if( i == list.size())
        {
            Segment s = Segment(QPoint(-1,-1),QPoint(-1,-1));
            segments.push_back(s);
            return segments;
        }


        while( i < list.size() && isConnected(list[i]))
        {
            segments.push_back(list[i]);
            i++;
        }
        return segments;

    }

    Segment Segment::getSegment(const QPoint &position,const QList<Segment> &list)
    {
        Segment invalid(QPoint(-1,-1),QPoint(-1,-1));
        int size = list.size();

        if( size == 0 || position.y()!= list[0].startPos().y())
            return invalid;

        int i = 0;
        while(i < size && !(position.x() >= list[i].startPos().x() && position.x() <= list[i].endPos().x() ) )
            i++;

        if( i == size )
            return invalid;

        return list[i];
    }

    Segment Segment ::maxPath(const Segment &segment)
    {


        if( m_startPos == QPoint(-1,-1) )
            return segment;
        if( segment.startPos() == QPoint(-1,-1) )
            return *this;
        if(weight() > segment.weight() )
            return *this;
        return segment;
    }

    bool Segment ::isConnected(const Segment &segment)
    {
        if( segment.endPos().x() < m_startPos.x() )
            return false;
        if( segment.startPos().x() > m_endPos.x() )
            return false;
        return true;
    }

    bool Segment ::isValid() const
    {
        return m_startPos.x() >= 0 && m_startPos.y() >= 0 &&
            m_endPos.x() >=0 && m_endPos.y() >= 0;
    }

}
