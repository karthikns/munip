#include "cluster.h"
#include <QPoint>

using namespace Munip;

ClusterPoint::ClusterPoint() : neighbors(0), clusterNumber(-1)
{
    point.setX(0);
    point.setY(0);
}

ClusterPoint::ClusterPoint(QPoint pt) : point(pt), neighbors(0), clusterNumber(-1)
{
}

ClusterPoint::ClusterPoint(int x,int y) : neighbors(0), clusterNumber(-1)
{
    point.setX(x);
    point.setY(y);
}

void ClusterPoint::addPoint(QPoint pt)
{
    point = pt;
}

void ClusterPoint::incrementNeighbors()
{
    ++neighbors;
}

void ClusterPoint::setClusterNumber(int num)
{
    clusterNumber = num;
}

void ClusterSet::addPoint(ClusterPoint cp)
{
    points.append(ClusterPoint(cp));
}

void ClusterSet::computeCore()
{

}
