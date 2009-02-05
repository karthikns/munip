#include "sidebar.h"

#include <QAction>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>

SideBar::SideBar(QWidget *parent) : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
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
    QPushButton *button = new QPushButton(action->icon(),
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
