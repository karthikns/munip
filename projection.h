#ifndef PROJECTION_H
#define PROJECTION_H

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QVarLengthArray>

#include "monoimage.h"

namespace Munip {
    typedef QVector<int> ProjectionData;

    class ProjectionItem : public QGraphicsItem
    {
    public:
        ProjectionItem(const ProjectionData &other);
        virtual ~ProjectionItem();

        virtual QRectF boundingRect() const;
        virtual void paint(QPainter *, const QStyleOptionGraphicsItem*, QWidget *);

    private:
        static const qreal BarWidth;
        ProjectionData m_projectionData;
        QRectF m_boundRect;
    };

    class ProjectionWidget : public QGraphicsView
    {
    public:
        ProjectionWidget(const ProjectionData& data, QWidget *parent = 0);
        virtual ~ProjectionWidget();

    protected:
        void resizeEvent(QResizeEvent *event);

    private:
        ProjectionItem *m_projectionItem;
    };

    // Projection calculating methods
    ProjectionData horizontalProjection(const MonoImage& image);
    ProjectionData grayScaleHistogram(const QImage& image);
};

#endif //PROJECTION_H
