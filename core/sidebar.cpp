#include "sidebar.h"

#include <QAction>
#include <QDebug>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QStyleOption>
#include <QVBoxLayout>

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
    QPushButton *button = new QPushButton(action->icon(),
                                          action->text(),
                                          this);
    const char* buttonStyleSheet =
        "QPushButton {"
        "     background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0.036, y2:1, stop:0 rgba(141, 140, 121, 255), stop:0.494845 rgba(43, 42, 8, 255), stop:1 rgba(141, 140, 121, 255));"
        "     border-style: outset;"
        "     border-width: 2px;"
        "     border-radius: 10px;"
        "     border-color: beige;"
        "     font: bold 14px;"
        "     min-width: 10em;"
        "     padding: 6px;"
        "     color: #E3EAA1;"
        " }"
        " QPushButton:pressed {"
        "     background-color:qlineargradient(spread:pad, x1:0, y1:0, x2:0.036, y2:1, stop:0 rgba(43, 42, 8, 255), stop:0.494845 rgba(141, 140, 121, 255), stop:1 rgba(34, 32, 11, 255));"
        "     border-style: inset;"
        " }; ";

    button->setStyleSheet(buttonStyleSheet);
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
