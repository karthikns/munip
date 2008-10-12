#ifndef IMAGE_WIDGET_H
#define IMAGE_WIDGET_H

#include <QGraphicsView>
#include <QPixmap>

class ImageWidget : public QGraphicsView
{
Q_OBJECT
public:
    ImageWidget(const QPixmap& pixmap, QWidget *parent = 0);
    ImageWidget(const QString& fileName, QWidget *parent = 0);
    ImageWidget(QWidget *parent = 0);
    ~ImageWidget();

    bool showGrid() const;

    QString fileName() const;
    QPixmap pixmap() const;
    QImage image() const;

    int widgetID() const;
    void setWidgetID(int id);

    ImageWidget* processorWidget() const;
    void setProcessorWidget(ImageWidget *wid);

    void updateWindowTitle();

public slots:
    void slotSetShowGrid(bool b);
    void slotToggleShowGrid();

    void slotZoomIn();
    void slotZoomOut();

    void slotSave();
    void slotSaveAs();

protected:
    void drawForeground(QPainter *painter, const QRectF& rect);

private:
    void init();

    QGraphicsPixmapItem *m_pixmapItem;
    QString m_fileName;
    bool m_showGrid;
    qreal m_scale;

    int m_widgetID;
    ImageWidget *m_processorWidget;
};

#endif
