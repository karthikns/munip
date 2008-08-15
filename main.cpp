#include "pixelviewer.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  PixelViewer *viewer = new PixelViewer(0);
  viewer->show();

  return app.exec();
}
