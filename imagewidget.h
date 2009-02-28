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
    QPixmap m_pixmap;
};

class RulerItem : public QGraphicsItem
{
public:
    RulerItem(const QRectF& constrainedRect, QGraphicsItem *parent = 0);

    QRectF boundingRect() const;

    void paint(QPainter *, const QStyleOptionGraphicsItem*, QWidget*);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value);
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    static const qreal Thickness;
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
    // QPixmap pixmap() const;
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
    void wheelEvent(QWheelEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    void init();
    void setScale(qreal scale);

    ImageItem *m_imageItem;
    QString m_fileName;
    bool m_showGrid;
    qreal m_scale;

    int m_widgetID;
    ImageWidget *m_processorWidget;

    RulerItem *m_ruler;
    QGraphicsRectItem *m_boundaryItem;
};

#endif
