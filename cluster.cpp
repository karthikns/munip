#include "cluster.h"
#include <QDebug>
#include <QPoint>
#include <cmath>

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

void ClusterPoint::setPoint(QPoint pt)
{
    point = pt;
}

void ClusterPoint::incrementNeighbors()
{
    ++neighbors;
}

int ClusterPoint::getNeighbors() const
{
    return neighbors;
}

void ClusterPoint::setClusterNumber(int num)
{
    clusterNumber = num;
}

int  ClusterPoint::pointDistance(ClusterPoint pt)
{
    //Euclidian Distance
    return (int)sqrt( (point.x()-pt.point.x())*(point.x()-pt.point.x()) + (point.y()-pt.point.y())*(point.y()-pt.point.y()) );
}




ClusterSet::ClusterSet() : radius(3), minPts(4)
{
}

void ClusterSet::addPoint(ClusterPoint cp)
{
    points.append(ClusterPoint(cp));
}

void ClusterSet::computeNearestNeighbors()
{
    for(int i=0; i<size()-1; ++i)
    {
        for(int j=i+1; j<size(); ++j)
        {
            qDebug() << points[i].pointDistance(points[j]);
            if( points[i].pointDistance(points[j]) < radius )
            {
                points[i].incrementNeighbors();
                points[j].incrementNeighbors();
            }
        }
        qDebug();
    }
}

int  ClusterSet::size() const
{
    return points.size();
}

int ClusterSet::coreSize() const
{
    int size=0;
    qDebug();
    foreach(ClusterPoint pt, points)
    {
        qDebug() << pt.getNeighbors();
        if(pt.getNeighbors() >= minPts)
            ++size;
    }
    return size;
}
