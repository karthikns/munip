#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ImageWidget;
class QMdiArea;
class QMdiSubWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT;
public:
    MainWindow();
    ~MainWindow();

    ImageWidget* activeImageWidget() const;
    void addSubWindow(QWidget *widget);

    static MainWindow* instance();

public slots:
    void slotOpen();
    void slotSave();
    void slotSaveAs();
    void slotClose();
    void slotQuit();

    void slotZoomIn();
    void slotZoomOut();
    void slotToggleShowGrid(bool b);

    void slotProjection();

    void slotAboutMunip();

private slots:
    void slotOnSubWindowActivate(QMdiSubWindow *);

private:
    void setupActions();

    QAction *m_showGridAction;
    QMdiArea *m_mdiArea;

    static MainWindow* m_instance;
};

#endif //MAINWINDOW_H
