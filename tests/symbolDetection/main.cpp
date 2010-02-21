#include <QtTest/QtTest>
#include <QImage>
#include <QScopedPointer>
#include <QTextStream>

#include <cmath>

#include "processstep.h"

class tst_SymbolDetection : public QObject
{
Q_OBJECT
public:
    tst_SymbolDetection() {}

private Q_SLOTS:
    void clusterDetect_data();
    void clusterDetect();

    void cleanupTestCase();
};

void tst_SymbolDetection::clusterDetect_data()
{
    QTest::addColumn<QString>("filePrefix");
    QTest::addColumn<QImage>("image");

    static const QString prefix = "images/";
    static const QString outPrefix = "test_output/symbolDetection/images";
    QString data[] = {
         "music3.bmp",

// The #if is for easier faster testing (make it 0 to disable temporarily)
#if 0
         "music4.bmp",
         "image.bmp",
         "music2.bmp",
         "scan0022.jpg",
         "scan0022small.jpg",
         "scan0025.jpg",
#endif
        "music1.bmp"
    };

    for (uint i = 0; i < sizeof(data)/sizeof(QString); ++i) {
        const QString fileName = prefix + data[i];
        const QFileInfo fileInfo = QFileInfo(fileName);
        const QImage original = QImage(fileName);

        const qreal aspectRatio = qreal(original.width())/original.height();
        {
            const int startWidth = 500;
            const int stopWidth = 1000;
            const int widthStep = 500;
            for (int width = startWidth; width <= startWidth; width += widthStep) {
                int height = int(qRound(width/aspectRatio));
                QImage scaled = original.scaled(width,
                        height,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation);

                QString newFileName = QString("%1/%2_%3")
                    .arg(outPrefix)
                    .arg(fileInfo.baseName())
                    .arg(width);
                QTest::newRow(qPrintable(QFileInfo(newFileName).baseName())) << newFileName << scaled;
            }
            QString newFileName = QString("%1/%2_original")
                .arg(outPrefix)
                .arg(fileInfo.baseName());
            QTest::newRow(qPrintable(QFileInfo(newFileName).baseName())) << newFileName << original;
        }
    }
}

void tst_SymbolDetection::clusterDetect()
{
    QFETCH(QString, filePrefix);
    QFETCH(QImage, image);

    // Ensure the existence of directories
    {
        QDir dir;
        dir.mkdir("test_output");
        dir.mkdir("test_output/symbolDetection");
        dir.mkdir("test_output/symbolDetection/images");
    }

    image.save(filePrefix + ".png");
    // Generate before skew image
    QScopedPointer<Munip::MonoChromeConversion> mono(new Munip::MonoChromeConversion(image));
    mono->process();
    image = mono->processedImage();

    QScopedPointer<Munip::SkewCorrection> skew(new Munip::SkewCorrection(image));
    skew->process();
    image = skew->processedImage();

    QScopedPointer<Munip::StaffParamExtraction> param(new Munip::StaffParamExtraction(image, 0));
    param->process();

    QScopedPointer<Munip::ImageCluster> cluster(new Munip::ImageCluster(image,
                param->staffSpaceHeight()));
    cluster->process();
    image = cluster->processedImage();

    image.save(filePrefix + "_cluster.png");
}

void tst_SymbolDetection::cleanupTestCase()
{
}

QTEST_MAIN(tst_SymbolDetection)
#include "main.moc"
