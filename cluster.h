#ifndef CLUSTER_H
#define CLUSTER_H

#include <QPoint>
#include <QList>
#include <QHash>
#include <QPainter>

namespace Munip
{
    class ClusterSet
    {
    public:
        ClusterSet(int radius = 2, int minPts = 4);
        ~ClusterSet();

        void computeNearestNeighbors();

        int coreSize() const;
        void drawCore(QPainter &p);

        QImage image() const;
        void setImage(const QImage& image);

    private:
        void computeNearestNeighbor(int x, int y);

        QImage m_image;
        QHash<QPoint, int> m_neighborMatrix;

        int m_radius;
        int m_minPoints;
    };
}

#endif // CLUSTER_H
