#ifndef PIX_H
#define PIX_H

#include <QtGui>

class PixelViewer : public QWidget
{
    Q_OBJECT
    public:
    PixelViewer(QWidget *parent);


public slots:
    void setImage(const QString& fileName);
    void slotGetImage();

private:
    QImage convertToMonochrome(const QImage& image);

    QImage workImage;
    QLabel *label;
    QTextEdit *pixelEdit;
    QPushButton *button;
};

#endif
