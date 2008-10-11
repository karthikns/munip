#include "pixelviewer.h"
#include "scanner/staff.h"

PixelViewer::PixelViewer(QWidget *par) : QWidget(par)
{
    label = new QLabel(this);
    pixelEdit = new QTextEdit(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    QSplitter *splitter = new QSplitter();
    layout->addWidget(splitter);
    splitter->setOrientation(Qt::Vertical);

    splitter->addWidget(label);
    splitter->addWidget(pixelEdit);
    button = new QPushButton("File");
    splitter->addWidget(button);

    connect(button, SIGNAL(clicked()), this ,SLOT(slotGetImage()));
}

void PixelViewer::setImage(const QString& fileName)
{
    workImage = convertToMonochrome(QImage(fileName));

    Munip::StaffLineRemover remover(workImage);
    remover.removeLines();


    label->setPixmap(QPixmap::fromImage(remover.processedImage()));
    QString text;
    foreach(Munip::Staff s, remover.staffList()) {
        text.append(QString::number(s.endPos().y() - s.startPos().y()) + "\n");
    }
    pixelEdit->setText(text);
}

void PixelViewer::slotGetImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Image file name");
    if(!fileName.isEmpty()) {
        setImage(fileName);
    }
}

QImage PixelViewer::convertToMonochrome(const QImage& image)
{
    // First convert to 8-bit image.
    QImage converted = image.convertToFormat(QImage::Format_Indexed8);

    // Modify colortable to our own monochrome
    QVector<QRgb> colorTable = converted.colorTable();
    const int threshold = 200;
    for(int i = 0; i < colorTable.size(); ++i) {
        int gray = qGray(colorTable[i]);
        if(gray > threshold) {
            gray = 255;
        }
        else {
            gray = 0;
        }
        colorTable[i] = qRgb(gray, gray, gray);
    }
    converted.setColorTable(colorTable);
    // convert to 1-bit monochrome
    converted = converted.convertToFormat(QImage::Format_Mono);
    return converted;
}
