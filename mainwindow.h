#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ImageWidget;
class QMdiArea;
class QMdiSubWindow;

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    MainWindow();
    ~MainWindow();

public slots:
    void slotOpen();
    void slotSave();
    void slotSaveAs();
    void slotClose();
    void slotQuit();

    void slotZoomIn();
    void slotZoomOut();
    void slotToggleShowGrid(bool b);

    void slotConvertToMonochrome();
    void slotRemoveLines();

    void slotAboutMunip();

private slots:
    void slotOnSubWindowActivate(QMdiSubWindow *);

private:
    void setupActions();
    ImageWidget* activeImageWidget() const;

    QAction *m_showGridAction;
    QMdiArea *m_mdiArea;
};

#endif //MAINWINDOW_H
