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
        void addPoint(QPoint);
        void incrementNeighbors();
        void setClusterNumber(int);

    private:
        QPoint point;
        int neighbors;
        int clusterNumber;
    };

    class ClusterSet
    {
    public:
        void addPoint(ClusterPoint);
        void computeCore();
    private:
        QList<ClusterPoint> points;
    };
}

#endif // CLUSTER_H
