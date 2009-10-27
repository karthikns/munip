#ifndef SEGMENTS_H
#define SEGMENTS_H


#include<QPoint>
#include<QVector>


namespace Munip
{
    class Segment
    {

        public:

        Segment();
        Segment(const QPoint& start,const QPoint& end);

        ~Segment();

        void setStartPos(QPoint &pos);
        QPoint startPos() const;

        void setEndPos(QPoint &pos);
        QPoint endPos() const;

        void setWeight( int weight );
        int weight() const;

        void setConnectedComponentID(const int id);
        int connectedComponentID() const;

        void setDestinationPos(const QPoint &position);
        QPoint destinationPos() const;

        int length() const;

        bool isValid() const;

        QVector<Segment> getConnectedSegments(const QVector<Segment> &list);
        Segment getSegment(const QPoint &pos,const QVector<Segment> &list);

        Segment maxPath(const Segment &segment);

        private:

        QPoint m_startPos;
        QPoint m_endPos;
        int m_connectedComponentID;
        QPoint m_destinationPos;
        int m_weight;


        bool isConnected(const Segment &segment);

    };
}

inline bool operator==(const Munip::Segment& line1,const Munip::Segment& line2)
    {
        return (line1.startPos()==line2.startPos() && line1.endPos()==line2.endPos());
    }

#endif // SEGMENTS_H
