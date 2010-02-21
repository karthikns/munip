#include "cluster.h"
#include <QDebug>
#include <QPoint>
#include <QPainter>
#include <cmath>

using namespace Munip;

//! A simple hash function for use with QHash<QPoint>.
// Assumption: p is +ve and p.x() value is <= 4000
static uint qHash(const QPoint& p)
{
    return p.x() * 4000 + p.y();
}

ClusterSet::ClusterSet(int radius, int minPts) :
    m_radius(radius), m_minPoints(minPts)
{
}

ClusterSet::~ClusterSet()
{
}

int ClusterSet::radius() const
{
    return m_radius;
}

void ClusterSet::setRadius(int radius)
{
    m_radius = radius;
    m_neighborMatrix.clear();
}

int ClusterSet::minPoints() const
{
    return m_minPoints;
}

void ClusterSet::setMinPoints(int minPts)
{
    m_minPoints = minPts;
    m_neighborMatrix.clear();
}

void ClusterSet::computeNearestNeighbor(int x, int y)
{
    const int Black = m_image.color(0) == 0xffffffff ? 1 : 0;
    int neighbors = 0;

    const int yInit = qMax(0, y - m_radius);
    const int yLimit = qMin(m_image.height() - 1, y + m_radius);

    const int xInit = qMax(0, x - m_radius);
    const int xLimit = qMin(m_image.width() - 1, x + m_radius);

    for (int yy = yInit; yy < yLimit; ++yy) {
        for (int xx = xInit; xx < xLimit; ++xx) {
            if (xx == x && yy == y) {
                continue;
            }
            if (m_image.pixelIndex(xx, yy) == Black) {
                if (QLineF(xx, yy, x, y).length() <= qreal(m_radius)) {
                    ++neighbors;
                }
            }
        }

    }
    // Only add points having atleast a single neighbor
    if (neighbors > 0) {
        m_neighborMatrix[QPoint(x, y)] = neighbors;
    }
}

void ClusterSet::computeNearestNeighbors()
{
    if (m_image.isNull() || m_image.format() != QImage::Format_Mono) {
        qWarning() << Q_FUNC_INFO << "Not yet initialized with appropriate image";
        return;
    }

    const int Black = m_image.color(0) == 0xffffffff ? 1 : 0;
    for (int y = 0; y < m_image.height(); ++y) {
        for (int x = 0; x < m_image.width(); ++x) {
            if (m_image.pixelIndex(x, y) == Black) {
                computeNearestNeighbor(x, y);
            }
        }
    }
}

int ClusterSet::coreSize() const
{
    int core = 0;
    QHash<QPoint, int>::const_iterator it, end = m_neighborMatrix.end();
    for (it = m_neighborMatrix.begin(); it != end; ++it) {
        if (it.value() > m_minPoints) {
            ++core;
        }
    }

    return core;
}

void ClusterSet::drawCore(QPainter &p)
{
    QHash<QPoint, int>::const_iterator it, end = m_neighborMatrix.end();
    for (it = m_neighborMatrix.begin(); it != end; ++it) {
        if (it.value() > m_minPoints) {
            p.drawPoint(it.key());
        }
    }
}

QImage ClusterSet::image() const
{
    return m_image;
}

void ClusterSet::setImage(const QImage& img)
{
    m_image = img;
    m_neighborMatrix.clear();
    if (m_image.isNull() || m_image.format() != QImage::Format_Mono) {
        qWarning() << Q_FUNC_INFO << "Initialized with invalid image";
    }
}

