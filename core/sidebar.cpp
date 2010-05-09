#include "sidebar.h"

#include <QAction>
#include <QDebug>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QVBoxLayout>

SideBarButton::SideBarButton(const QIcon &icon, const QString &text, QWidget *parent) :
    QPushButton(icon, text, parent)
{
}

SideBar::SideBar(QWidget *parent) : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(5);
    m_layout->addStretch(10);
}

SideBar::~SideBar()
{
}

void SideBar::addAction(QAction *action)
{
    if (m_actionButtonMap.contains(action)) {
        qWarning() << "Trying to add already added action";
        return;
    }
    SideBarButton *button = new SideBarButton(action->icon(),
                                          action->text(),
                                          this);

    button->setCheckable(action->isCheckable());
    if (action->isCheckable()) {
        button->setChecked(action->isChecked());
    }

    connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
    connect(button, SIGNAL(toggled(bool)), action, SLOT(setChecked(bool)));

    m_layout->insertWidget(m_layout->count()-1, button);
}

void SideBar::removeAction(QAction *action)
{
    if (!m_actionButtonMap.contains(action)) {
        qWarning() << "Action " << action->text() << " does not exist";
        return;
    }

    delete m_actionButtonMap[action];
    m_actionButtonMap.remove(action);
}

void SideBar::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
