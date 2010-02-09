#ifndef CLUSTER_H
#define CLUSTER_H

#include <QPoint>

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

}

#endif // CLUSTER_H
