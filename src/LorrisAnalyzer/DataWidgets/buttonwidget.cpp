/**********************************************
**    This file is part of Lorris
**    http://tasssadar.github.com/Lorris/
**
**    See README and COPYING
***********************************************/

#include <QInputDialog>

#include "buttonwidget.h"

ButtonWidget::ButtonWidget(QWidget *parent) : DataWidget(parent)
{
    setTitle(tr("Button"));
    setIcon(":/dataWidgetIcons/button.png");

    m_widgetType = WIDGET_BUTTON;

    m_button = new QPushButton(tr("Button"), this);
    m_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout->setContentsMargins(5, 0, 5, 5);
    layout->addWidget(m_button, 4);

    adjustSize();
}

void ButtonWidget::setUp(Storage *storage)
{
    DataWidget::setUp(storage);

    QAction *btnText = contextMenu->addAction(tr("Set button text..."));

    connect(btnText,  SIGNAL(triggered()), SLOT(setButtonName()));
    connect(m_button, SIGNAL(clicked()),   SLOT(buttonClicked()));
}

void ButtonWidget::buttonClicked()
{
    emit scriptEvent(getTitle() + "_clicked");
}

void ButtonWidget::setButtonName()
{
    QString name = QInputDialog::getText(this, tr("Button text"), tr("Enter new button text"));
    if(name.isEmpty())
        return;
    setButtonName(name);
}

void ButtonWidget::setButtonName(const QString &name)
{
    m_button->setText(name);
}

void ButtonWidget::saveWidgetInfo(DataFileParser *file)
{
    DataWidget::saveWidgetInfo(file);

    file->writeBlockIdentifier("buttonWText");
    {
        QByteArray text = m_button->text().toUtf8();
        quint32 len = text.length();

        file->write((char*)&len, sizeof(quint32));
        file->write(text.data(), len);
    }
}

void ButtonWidget::loadWidgetInfo(DataFileParser *file)
{
    DataWidget::loadWidgetInfo(file);

    if(file->seekToNextBlock("buttonWText", BLOCK_WIDGET))
    {
        quint32 size = 0;
        file->read((char*)&size, sizeof(quint32));

        QString text = QString::fromUtf8(file->read(size), size);
        m_button->setText(text);
    }
}

ButtonWidgetAddBtn::ButtonWidgetAddBtn(QWidget *parent) : DataWidgetAddBtn(parent)
{
    setText(tr("Button"));
    setIconSize(QSize(17, 17));
    setIcon(QIcon(":/dataWidgetIcons/button.png"));

    m_widgetType = WIDGET_BUTTON;
}