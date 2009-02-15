#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QMap>
#include <QWidget>

class QPushButton;
class QVBoxLayout;

class SideBar : public QWidget
{
    Q_OBJECT;
public:
    SideBar(QWidget *parent = 0);
    virtual ~SideBar();

    void addAction(QAction *action);
    void removeAction(QAction *action);

protected:
    void paintEvent(QPaintEvent*);

private:
    QMap<QAction *, QPushButton *> m_actionButtonMap;
    QVBoxLayout *m_layout;
};

#endif
