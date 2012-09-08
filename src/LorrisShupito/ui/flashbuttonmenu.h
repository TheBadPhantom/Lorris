/**********************************************
**    This file is part of Lorris
**    http://tasssadar.github.com/Lorris/
**
**    See README and COPYING
***********************************************/

#ifndef FLASHBUTTONMENU_H
#define FLASHBUTTONMENU_H

#include <QMenu>
#include <map>
#include <QToolButton>

enum ActionSlots
{
    ACT_FLASH  = 0,
    ACT_EEPROM,
    ACT_ALL,
    ACT_FUSES
};

class FlashButtonMenu : public QMenu
{
    Q_OBJECT
Q_SIGNALS:
    void mainAct();

public:
    explicit FlashButtonMenu(bool read, QToolButton *btn, QObject *target, QWidget *parent = 0);

public slots:
    void setActiveAction(int actInt);

private:
    void createActions(QObject *target);

    std::map<ActionSlots, QAction*> m_actions;
    QToolButton *m_button;
    bool m_read;
    ActionSlots m_active;

    QFont m_font;
    QFont m_boldFont;
    QObject *m_target;
};

#endif // FLASHBUTTONMENU_H