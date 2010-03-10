#ifndef IMAGE_WIDGET_H
#define IMAGE_WIDGET_H

#include <QGraphicsItem>
#include <QGraphicsView>

class ImageItem : public QGraphicsItem
{
public:
    ImageItem(const QImage& image, QGraphicsItem* parent = 0);
    ~ImageItem();

    QImage image() const;
    void setImage(const QImage& image);

    QRectF boundingRect() const;
    QPainterPath shape() const;

    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

private:
    QImage m_image;
    QList<QGraphicsPixmapItem*> m_tiles;
};

class RulerItem : public QGraphicsItem
{
public:
    RulerItem(Qt::Orientation o, const QRectF& constrainedRect, QGraphicsItem *parent = 0);

    QRectF boundingRect() const;

    void paint(QPainter *, const QStyleOptionGraphicsItem*, QWidget*);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value);
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    static const qreal Thickness;
    Qt::Orientation m_orientation;
    QRectF m_constrainedRect;
    int m_alpha;
};

class ImageWidget : public QGraphicsView
{
Q_OBJECT
public:
    ImageWidget(const QImage& image, QWidget *parent = 0);
    ImageWidget(const QString& fileName, QWidget *parent = 0);
    ~ImageWidget();

    bool showGrid() const;

    QString fileName() const;
    QImage image() const;

    int widgetID() const;
    void setWidgetID(int id);

    ImageWidget* processorWidget() const;
    void setProcessorWidget(ImageWidget *wid);

    void updateWindowTitle();

public Q_SLOTS:
    void slotSetShowGrid(bool b);
    void slotToggleShowGrid();

    void slotZoomIn();
    void slotZoomOut();

    void slotSave();
    void slotSaveAs();

Q_SIGNALS:
    void statusMessage(const QString& string);

protected:
    void drawForeground(QPainter *painter, const QRectF& rect);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    void init(const QImage& image);
    void setScale(qreal scale);
    void updateStatusMessage();

    ImageItem *m_imageItem;
    QString m_fileName;
    bool m_showGrid;
    qreal m_scale;

    int m_widgetID;
    ImageWidget *m_processorWidget;

    QGraphicsRectItem *m_boundaryItem;
    QPoint m_mousePos;
};

#endif
