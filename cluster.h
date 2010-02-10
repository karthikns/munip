#ifndef CLUSTER_H
#define CLUSTER_H

#include <QPoint>
#include <QList>

namespace Munip
{
    class ClusterPoint
    {
    public:
        ClusterPoint();
        ClusterPoint(QPoint);
        ClusterPoint(int,int);
        void setPoint(QPoint);
        void incrementNeighbors();
        int getNeighbors() const;
        void setClusterNumber(int);
        int  pointDistance(ClusterPoint);

    private:
        QPoint point;
        int neighbors;
        int clusterNumber;
    };


    class ClusterSet
    {
    public:
        ClusterSet();
        void addPoint(ClusterPoint);
        void computeNearestNeighbors();
        int  size() const;
        int coreSize() const;

    private:

        QList<ClusterPoint> points;
        int radius, minPts;
    };
}

#endif // CLUSTER_H
