#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QMap>
#include <QPushButton>

class QVBoxLayout;

class SideBarButton : public QPushButton
{
Q_OBJECT
public:
    SideBarButton(const QIcon &icon, const QString &text, QWidget *parent = 0);
};

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
    QMap<QAction *, SideBarButton *> m_actionButtonMap;
    QVBoxLayout *m_layout;
};

#endif
