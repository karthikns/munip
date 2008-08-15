#include "pixelviewer.h"

PixelViewer::PixelViewer(QWidget *par) : QWidget(par)
{
  workImage = 0;
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
  delete workImage;
  workImage = new QImage(fileName);
  displayImageInfo();
}

void PixelViewer::slotGetImage()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Image file name");
  if(!fileName.isEmpty()) {
    setImage(fileName);
  }
}

void PixelViewer::displayImageInfo()
{
  QImage *origImage = workImage;

  // First convert to 8-bit image.
  QImage converted = workImage->convertToFormat(QImage::Format_Indexed8);
  
  // Print original image pixel data.
  QSize sz = origImage->size();
  QString str;
  for(int j=0; j < sz.height(); ++j) {
    for(int i=0; i < sz.width(); ++i) {
      QRgb rgb = origImage->pixel(i, j);
      QString triplet = QString("(%1,%2,%3) ").arg(qRed(rgb)).arg(qGreen(rgb)).arg(qBlue(rgb));
      str.append(triplet);
    }
    str.append('\n');
  }
  pixelEdit->setText(str);

  // Assign converted to workImage now
  *workImage = converted;

  // Modify colortable to our own monochrome
  QVector<QRgb> colorTable = workImage->colorTable();
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
  workImage->setColorTable(colorTable);

  // Finally set the label to show the converted image.
  label->setPixmap(QPixmap::fromImage(*workImage));
}
