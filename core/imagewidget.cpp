#include "imagewidget.h"
#include "mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStyleOptionGraphicsItem>
#include <QWheelEvent>

#include <cmath>

ImageItem::ImageItem(const QImage& image, QGraphicsItem* parent) :
    QGraphicsItem(parent)
{
    setFlag(QGraphicsItem::ItemHasNoContents, true);
    setImage(image);
}

ImageItem::~ImageItem()
{
}

QImage ImageItem::image() const
{
    return m_image;
}

void ImageItem::setImage(const QImage& image)
{
    m_image = image;

    qDeleteAll(m_tiles);
    m_tiles.clear();

    if (m_image.size().isEmpty()) {
        return;
    }

    int tileWidth = image.width() >> 1;
    int tileHeight = image.height() >> 1;

    static const int MaxTileDimension = 50;
    if (tileWidth > MaxTileDimension) {
        tileWidth = MaxTileDimension;
    }

    if (tileHeight > MaxTileDimension) {
        tileHeight = MaxTileDimension;
    }

    for (int y = 0; y < image.height(); y += tileHeight) {
        for (int x = 0; x < image.width(); x += tileWidth) {
            int w = qMin(tileWidth, image.width() - x);
            int h = qMin(tileHeight, image.height() - y);
            QRect srcRect(x, y, w, h);
            QRect destRect(0, 0, w, h);
            QPixmap pix(w, h);

            QPainter p(&pix);
            p.drawImage(destRect, image, srcRect, Qt::ColorOnly);
            p.end();

            QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pix, this);
            item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
            item->setPos(x, y);
            m_tiles << item;
        }
    }
}

QRectF ImageItem::boundingRect() const
{
    return QRectF();
}

QPainterPath ImageItem::shape() const
{
    return QGraphicsItem::shape();
}

void ImageItem::paint(QPainter *, const QStyleOptionGraphicsItem* , QWidget* )
{
}

const qreal RulerItem::Thickness = 10;

RulerItem::RulerItem(Qt::Orientation o, const QRectF& constrainedRect, QGraphicsItem *parent) :
    QGraphicsItem(parent),
    m_orientation(o),
    m_constrainedRect(constrainedRect),
    m_alpha(200)
{
    setFlag(ItemIsMovable);
#if QT_VERSION >= 0x040600
    setFlag(ItemSendsGeometryChanges);
#endif
    setAcceptsHoverEvents(true);
}

QRectF RulerItem::boundingRect() const
{
    QRectF r = m_constrainedRect;
    if (m_orientation == Qt::Horizontal) {
        r.setBottom(r.top() + RulerItem::Thickness);
    } else {
        r.setRight(r.left() + RulerItem::Thickness);
    }
    return r;
}

void RulerItem::paint(QPainter *p, const QStyleOptionGraphicsItem* , QWidget *)
{
    QColor c(Qt::darkGreen);
    c.setAlpha(m_alpha);
    p->fillRect(boundingRect(), c);
}

QVariant RulerItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange) {
        const QRectF constrainedRect = transform().map(m_constrainedRect).boundingRect();
        QPointF p;
        if (m_orientation == Qt::Horizontal) {
            p = QPointF(x(), value.toPointF().y());
            p.ry() = qMax(constrainedRect.top(), p.y());
            p.ry() = qMin(constrainedRect.bottom()-RulerItem::Thickness, p.y());
        } else {
            p = QPointF(value.toPointF().x(), y());
            p.rx() = qMax(constrainedRect.left(), p.x());
            p.rx() = qMin(constrainedRect.right()-RulerItem::Thickness, p.x());
        }
        return p;
    }
    return QGraphicsItem::itemChange(change, value);
}

void RulerItem::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    m_alpha = 100;
    update();
}

void RulerItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    m_alpha = 200;
    update();
}

ImageWidget::ImageWidget(const QImage& image, QWidget *parent) : QGraphicsView(parent)
{
    init(image);
}

ImageWidget::ImageWidget(const QString& fileName, QWidget *parent) : QGraphicsView(parent)
{
    init(QImage(fileName));
}

void ImageWidget::init(const QImage& image)
{
    m_scale = 1.0;
    m_showGrid = true;
    m_widgetID = -1;
    m_processorWidget = 0;

    QImage dummy(10, 10, QImage::Format_Mono);
    dummy.fill(Qt::white);

    QGraphicsScene *scene = new QGraphicsScene(this);

    m_imageItem = new ImageItem(dummy);
    m_imageItem->setImage(image);

    scene->addItem(m_imageItem);

    QRectF itemRect = m_imageItem->childrenBoundingRect();
    itemRect = m_imageItem->mapRectToScene(itemRect);

    RulerItem *ruler = new RulerItem(Qt::Horizontal, itemRect, m_imageItem);
    ruler->setZValue(10);

    ruler = new RulerItem(Qt::Vertical, itemRect, m_imageItem);
    ruler->setZValue(10);

    m_boundaryItem = scene->addRect(itemRect);
    m_boundaryItem->setZValue(5);

    setScene(scene);

    setDragMode(ScrollHandDrag);
    viewport()->setCursor(QCursor(Qt::ArrowCursor));

    MainWindow *instance = MainWindow::instance();
    if (instance) {
        connect(this, SIGNAL(statusMessage(const QString&)),
                instance, SLOT(slotStatusMessage(const QString&)));
    }
}

ImageWidget::~ImageWidget()
{
}

bool ImageWidget::showGrid() const
{
    return m_showGrid;
}

QString ImageWidget::fileName() const
{
    return m_fileName;
}

QImage ImageWidget::image() const
{
    return m_imageItem->image();
}

int ImageWidget::widgetID() const
{
    return m_widgetID;
}

void ImageWidget::setWidgetID(int id)
{
    m_widgetID = id;
    updateWindowTitle();
}

ImageWidget*ImageWidget:: processorWidget() const
{
    return m_processorWidget;
}

void ImageWidget::setProcessorWidget(ImageWidget *wid)
{
    m_processorWidget = wid;
    updateWindowTitle();
}

void ImageWidget::updateWindowTitle()
{
    QString title = QString("wid = %1:  %2 ")
        .arg(m_widgetID)
        .arg(m_fileName.isEmpty() ? tr("Untitled") : QFileInfo(m_fileName).fileName());

    if (m_processorWidget) {
        title.append(tr(" (Processed from wid = %1 )").arg(m_processorWidget->widgetID()));
    }
    setWindowTitle(title);
}

void ImageWidget::slotSetShowGrid(bool b)
{
    if (m_showGrid != b) {
        m_showGrid = b;
        viewport()->update();
    }
}

void ImageWidget::slotToggleShowGrid()
{
    slotSetShowGrid(!m_showGrid);
}

void ImageWidget::slotZoomIn()
{
    setScale(m_scale * 2);
}

void ImageWidget::slotZoomOut()
{
    setScale(m_scale * 0.5);
}

void ImageWidget::slotSave()
{
    if (m_fileName.isEmpty()) {
        m_fileName = QFileDialog::getSaveFileName(this, tr("Save file"), QString(),
                                                  tr("Images (*.png *.xpm *.jpg *.bmp)"));
        if (m_fileName.isEmpty()) {
            return;
        }
    }
    bool result = m_imageItem->image().save(m_fileName);
    if (!result) {
        QMessageBox::warning(this, tr("Failed saving"),
                             tr("Could not save image to file %1").arg(m_fileName));
        m_fileName = QString(); // reset
    }
    else {
        updateWindowTitle();
    }
}

void ImageWidget::slotSaveAs()
{
    QString oldFileName = m_fileName;
    m_fileName = QString();
    slotSave();
    if (m_fileName.isEmpty()) {
        m_fileName = oldFileName;
    }
}

void ImageWidget::drawForeground(QPainter *painter, const QRectF& rect)
{
    if (!m_showGrid || m_scale < 3.0) {
        QGraphicsView::drawForeground(painter, rect);
        return;
    }

    int gridWidth = 1;

    QRect r = rect.toRect().adjusted(-2, -2, +2, +2);
    int startX = r.left() + (gridWidth - (r.left() % gridWidth));
    int startY = r.top() + (gridWidth - (r.top() % gridWidth));
    int endX = r.right() - (r.right() % gridWidth);
    int endY = r.bottom() - (r.bottom() % gridWidth);

    painter->setPen(Qt::lightGray);
    for(int i = startX; i <= endX; i += gridWidth) {
        painter->drawLine(QLineF(i, rect.top(), i, rect.bottom()));
    }

    for(int i = startY; i <= endY; i += gridWidth) {
        painter->drawLine(QLineF(rect.left(), i, rect.right(), i));
    }
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPointF temp = mapToScene(event->pos());
    m_mousePos.setX(int(temp.x()));
    m_mousePos.setY(int(temp.y()));

    updateStatusMessage();
    QGraphicsView::mouseMoveEvent(event);
}

void ImageWidget::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() == Qt::Vertical &&
        event->modifiers() == Qt::ControlModifier) {

        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;

        qreal scale = m_scale;
        if (numSteps > 0) {
            while(numSteps--)
                scale *= 1.1;
        }
        else {
            while(numSteps++)
                scale /= 1.1;
        }
        setScale(scale);
    }
    else {
        QGraphicsView::wheelEvent(event);
    }
}

void ImageWidget::setScale(qreal scale)
{
    if (qFuzzyCompare(scale, 0.0)) {
        return;
    }
    m_scale = scale;
    QTransform tranform;
    tranform.scale(scale, scale);
    setTransform(tranform);
}

void ImageWidget::updateStatusMessage()
{
    QString msg = QString("%1, %2").arg(m_mousePos.x()).arg(m_mousePos.y());
    emit statusMessage(msg);
}
