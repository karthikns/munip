#include "projection.h"

namespace Munip {
    const qreal ProjectionItem::BarWidth = 3;

    ProjectionItem::ProjectionItem(const ProjectionData& other)
    {
        m_projectionData = other;

        int maxY = 0;
        for(int i = 0; i < m_projectionData.size(); ++i) {
            maxY = qMax(m_projectionData[i], maxY);
        }

        qreal adj = 5;
        m_boundRect.setTopLeft(QPointF(0, -maxY));
        m_boundRect.setBottomRight(QPointF(other.count() * 2 * ProjectionItem::BarWidth, 0));
        m_boundRect.adjust(-adj, -adj, adj, adj);
    }

    ProjectionItem::~ProjectionItem()
    {
    }

    QRectF ProjectionItem::boundingRect() const
    {
        return m_boundRect;
    }

    void ProjectionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem* , QWidget *)
    {
        QRectF r;
        qreal x = 0;
        painter->setBrush(QBrush(Qt::green));
        painter->setPen(Qt::NoPen);
        for(int i = 0; i < m_projectionData.size(); ++i) {
            r.setTopLeft(QPointF(x, -m_projectionData[i]));
            r.setBottomRight(QPointF(x + ProjectionItem::BarWidth, 0));
            painter->drawRect(r);
            x += 2 * ProjectionItem::BarWidth;
        }
    }

    ProjectionWidget::ProjectionWidget(const ProjectionData& data, QWidget *parent) : QGraphicsView(parent)
    {
        QGraphicsScene *scene = new QGraphicsScene(this);
        m_projectionItem = new ProjectionItem(data);
        scene->addItem(m_projectionItem);

        setScene(scene);
        setRenderHint(QPainter::Antialiasing, true);

        fitInView(m_projectionItem);
    }

    ProjectionWidget::~ProjectionWidget()
    {
    }

    void ProjectionWidget::resizeEvent(QResizeEvent *event)
    {
        fitInView(m_projectionItem);
        QGraphicsView::resizeEvent(event);
    }

    ProjectionData horizontalProjection(const MonoImage& image)
    {
        ProjectionData data;
        for(int y = 0; y < image.height(); ++y) {
            int sum = 0;
            for(int x = 0; x < image.width(); ++x) {
                if (image.pixelValue(x, y) == MonoImage::Black) {
                    ++sum;
                }
            }
            data.append(sum);
        }
        return data;
    }

    ProjectionData grayScaleHistogram(const QImage& image)
    {
        ProjectionData data(256, 0);
        for(int x = 0; x < image.width(); ++x) {
            for(int y = 0; y < image.height(); ++y) {
                QRgb rgb = image.pixel(x, y);
                Q_ASSERT(qRed(rgb) == qBlue(rgb) && qBlue(rgb) == qGreen(rgb));
                ++data[qRed(rgb)];
            }
        }
        return data;
    }
}
