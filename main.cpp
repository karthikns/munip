#include "pixelviewer.h"
// We met at NS place on 8th october 2008

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  PixelViewer *viewer = new PixelViewer(0);
  viewer->show();

  return app.exec();
}
