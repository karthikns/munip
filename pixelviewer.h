#ifndef PIX_H
#define PIX_H

#include <QtGui>

class PixelViewer : public QWidget
{
Q_OBJECT
public:
  PixelViewer(QWidget *parent);
  void displayImageInfo();

public slots:
  void setImage(const QString& fileName);
  void slotGetImage();

private:
  QImage *workImage;
  QLabel *label;
  QTextEdit *pixelEdit;
  QPushButton *button;
};

#endif
