
#include <QMouseEvent>
#include <QDrag>

#include "labellayout.h"
#include "sourcedialog.h"
#include "common.h"
#include "packet.h"

LabelLayout::LabelLayout(analyzer_header *header, bool enable_reorder, QWidget *parent) : QHBoxLayout(parent)
{
    setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_spacer = new QSpacerItem(20, 40, QSizePolicy::Expanding, QSizePolicy::Expanding);
    addSpacerItem(m_spacer);
    m_header = header;
    m_enableReorder = enable_reorder;
    if(m_enableReorder)
        ((QWidget*)parent)->setAcceptDrops(true);
}

LabelLayout::~LabelLayout()
{
    ClearLabels();
    removeItem(m_spacer);
    delete m_spacer;
}

void LabelLayout::ClearLabels()
{
    for(quint16 i = 0; i < m_labels.size(); ++i)
    {
        removeWidget(m_labels[i]);
        delete m_labels[i];
    }
    m_labels.clear();
}

void LabelLayout::AddLabel(QString value, qint8 type)
{
    if(type == -1)
        type = GetTypeForPos(m_labels.size());

    DraggableLabel *label = new DraggableLabel(value, m_enableReorder);
    SetLabelType(label, type);
    label->setObjectName(QString::number(m_labels.size()));
    if(m_enableReorder)
        connect(label, SIGNAL(changePos(int,int)), this, SLOT(changePos(int,int)));

    insertWidget(m_labels.size(), label);
    m_labels.push_back(label);
}

void LabelLayout::RemoveLabel(quint16 index)
{
    removeWidget(m_labels[index]);
    delete m_labels[index];

    std::vector<QLabel*>::iterator itr = m_labels.begin();
    for(quint16 i = 0; i < index; ++i)
        ++itr;
    m_labels.erase(itr);

    for(quint16 i = 0; i < m_labels.size(); ++i)
        m_labels[i]->setObjectName(QString::number(i));
}

void LabelLayout::changePos(int this_label, int dragged_label)
{
    QLabel *label = m_labels[this_label];
    m_labels[this_label] = m_labels[dragged_label];
    m_labels[dragged_label] = label;
    m_labels[this_label]->setObjectName(QString::number(this_label));
    m_labels[dragged_label]->setObjectName(QString::number(dragged_label));

    quint8 type = m_header->order[this_label];
    m_header->order[this_label] = m_header->order[dragged_label];
    m_header->order[dragged_label] = type;

    for(quint16 i = 0; i < m_labels.size(); ++i)
        removeWidget(m_labels[i]);

    for(quint16 i = 0; i < m_labels.size(); ++i)
    {
        SetLabelType(m_labels[i], GetTypeForPos(i));
        insertWidget(i, m_labels[i]);
    }
    emit orderChanged();
}

void LabelLayout::SetLabelType(QLabel *label, quint8 type)
{
    QString css;
    if(type == DATA_BODY)
        css = "border: 2px solid black; background-color: #00FFFB";
    else if(type & DATA_HEADER)
    {
         css = "border: 2px solid orange; background-color: ";
         if     (type & DATA_DEVICE_ID)  css += "#00FF4C";
         else if(type & DATA_OPCODE)     css += "#E5FF00";
         else if(type & DATA_LEN)        css += "#FFCC66";
         else if(type & DATA_STATIC)     css += "#FF99FF";
    }
    label->setStyleSheet(css);
}

quint8 LabelLayout::GetTypeForPos(quint32 pos)
{
    if(pos < m_header->length)
    {
        quint8 type = DATA_HEADER;
        quint8 pos_h = 0;
        for(quint8 i = 0; i < 4; ++i)
        {
            switch(m_header->order[i])
            {
                case DATA_STATIC:
                    pos_h += m_header->static_len;
                    break;
                case DATA_LEN:
                    pos_h += (1 << m_header->len_fmt);
                    break;
                case DATA_DEVICE_ID:
                case DATA_OPCODE:
                    ++pos_h;
                    break;
                default:
                    return type;
            }
            if(pos < pos_h)
            {
                type |= m_header->order[i];
                break;
            }
        }

        return type;
    }
    return DATA_BODY;
}

void LabelLayout::UpdateTypes()
{
    for(quint16 i = 0; i < m_labels.size(); ++i)
        SetLabelType(m_labels[i], GetTypeForPos(i));
}

void LabelLayout::lenChanged(int len)
{
    if(len < 0 || len == m_labels.size())
        return;

    while(m_labels.size() != len)
    {
        if((uint)len > m_labels.size())
            AddLabel("NULL", -1);
        else
            RemoveLabel();
    }
}

ScrollDataLayout::ScrollDataLayout(analyzer_header *header, bool enable_reorder, QWidget *parent) :
    LabelLayout(header, enable_reorder, parent)
{
    m_format = FORMAT_HEX;
}

ScrollDataLayout::~ScrollDataLayout()
{

}

void ScrollDataLayout::fmtChanged(int len)
{
    m_format = len;
}

void ScrollDataLayout::SetData(QByteArray data)
{
    QString value;
    if(m_header->data_mask & DATA_LEN)
    {
        quint8 pos_h = 0;
        for(quint8 i = 0; i < 4; ++i)
        {
            switch(m_header->order[i])
            {
                case DATA_STATIC:
                    pos_h += m_header->static_len;
                    break;
                case DATA_LEN:
                    pos_h += (1 << m_header->len_fmt);
                    break;
                case DATA_DEVICE_ID:
                case DATA_OPCODE:
                    ++pos_h;
                    break;
            }
            if(m_header->order[i] == DATA_LEN)
                break;
        }
        if(data.length() >= pos_h)
        {
            switch(m_header->len_fmt)
            {
                case 0:

                    lenChanged(m_header->length + (int)data[pos_h-1]);
                    break;
                //TODO: implement
                case 1:
                case 2:
                    break;
            }
        }
    }
    for(quint32 i = 0; i < data.length() && i < m_labels.size(); ++i)
    {
        switch(m_format)
        {
            case FORMAT_HEX:    value = Utils::hexToString((quint8)data[i], true); break;
            case FORMAT_BYTE:   value = QString::number((int)data[i]);             break;
            case FORMAT_STRING: value = Utils::parseChar(data[i]);                 break;
        }
        m_labels[i]->setText(value);
    }
}

DraggableLabel::DraggableLabel(const QString &text, bool drag, QWidget *parent, Qt::WindowFlags f) :
    QLabel(text, parent, f)
{
    m_drag = drag;

    setFixedWidth(50);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setAlignment(Qt::AlignCenter);

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
    setAcceptDrops(true);
}

DraggableLabel::~DraggableLabel()
{

}

void DraggableLabel::mousePressEvent(QMouseEvent *event)
{
    if(!m_drag)
    {
        QLabel::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(objectName());
        drag->setMimeData(mimeData);

        QPixmap pixmap(size());
        this->render(&pixmap);

        drag->setPixmap(pixmap);

        drag->exec();
        event->accept();
    }
}

void DraggableLabel::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->text() == objectName())
        return;
    event->acceptProposedAction();
    QString css = styleSheet();
    css.replace(QRegExp("orange"), "red");
    setStyleSheet(css);
}

void DraggableLabel::dragLeaveEvent(QDragLeaveEvent *event)
{
    QString css = styleSheet();
    css.replace(QRegExp("red"), "orange");
    setStyleSheet(css);
}

void DraggableLabel::dropEvent(QDropEvent *event)
{
    emit changePos(objectName().toInt(), event->mimeData()->text().toInt());
    QString css = styleSheet();
    css.replace(QRegExp("red"), "orange");
    setStyleSheet(css);

    event->acceptProposedAction();
}
