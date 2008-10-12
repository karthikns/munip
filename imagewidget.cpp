#include "imagewidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QMessageBox>

ImageWidget::ImageWidget(const QPixmap& pixmap, QWidget *parent) : QGraphicsView(parent)
{
    init();
    m_pixmapItem->setPixmap(pixmap);
}

ImageWidget::ImageWidget(const QString& fileName, QWidget *parent) : QGraphicsView(parent)
{
    init();
    QFileInfo info(fileName);
    if (info.exists() && info.isReadable()) {
        m_fileName = fileName;
        m_pixmapItem->setPixmap(QPixmap(m_fileName));
    }
    else {
        QMessageBox::warning(0, tr("Couldn't load pixmap"),
                             tr("Either file %1 does not exists or is not readable").arg(fileName));
    }
}

ImageWidget::ImageWidget(QWidget *parent) : QGraphicsView(parent)
{
    init();
}

void ImageWidget::init()
{
    m_scale = 1.0;
    m_showGrid = true;
    m_widgetID = -1;
    m_processorWidget = 0;

    QGraphicsScene *scene = new QGraphicsScene(this);
    m_pixmapItem = new QGraphicsPixmapItem();
    m_pixmapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);

    scene->addItem(m_pixmapItem);
    setScene(scene);

    setDragMode(ScrollHandDrag);
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

QPixmap ImageWidget::pixmap() const
{
    return m_pixmapItem->pixmap();
}

QImage ImageWidget::image() const
{
    return m_pixmapItem->pixmap().toImage();
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
    m_scale *= 2.0;
    QTransform transform;
    transform.scale(m_scale, m_scale);
    m_pixmapItem->setTransform(transform);
}

void ImageWidget::slotZoomOut()
{
    m_scale *= 0.5;
    QTransform transform;
    transform.scale(m_scale, m_scale);
    m_pixmapItem->setTransform(transform);
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
    bool result = m_pixmapItem->pixmap().save(m_fileName);
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
    if (!m_showGrid) {
        QGraphicsView::drawForeground(painter, rect);
        return;
    }

    int gridWidth = int(m_scale);
    if (gridWidth <= 4) {
        return;
    }

    QRect r = rect.toRect();
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
