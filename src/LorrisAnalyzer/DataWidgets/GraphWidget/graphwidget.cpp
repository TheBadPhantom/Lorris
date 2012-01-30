/****************************************************************************
**
**    This file is part of Lorris.
**    Copyright (C) 2012 Vojtěch Boček
**
**    Contact: <vbocek@gmail.com>
**             https://github.com/Tasssadar
**
**    Lorris is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    Lorris is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with Lorris.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include <QDropEvent>
#include <qwt_plot_curve.h>
#include <QMenu>
#include <QSignalMapper>
#include <QInputDialog>

#include "graphwidget.h"
#include "graph.h"
#include "graphdialogs.h"
#include "graphdata.h"
#include "graphcurve.h"
#include "../../analyzerdatastorage.h"

static const int sampleValues[] = { -1, -2, 10, 50, 100, 200, 500, 1000 };

GraphWidget::GraphWidget(QWidget *parent) : DataWidget(parent)
{
    m_widgetType = WIDGET_GRAPH;

    setTitle(tr("Graph"));
    setIcon(":/dataWidgetIcons/graph.png");

    m_graph = new Graph(this);
    m_add_dialog = NULL;

    layout->addWidget(m_graph);

    resize(400, 200);
}

GraphWidget::~GraphWidget()
{
    delete m_graph;
    delete m_add_dialog;

    for(quint8 i = 0; i < m_curves.size(); ++i)
        delete m_curves[i];
}

void GraphWidget::setUp(AnalyzerDataStorage *storage)
{
    DataWidget::setUp(storage);

    m_storage = storage;

    m_editCurve = contextMenu->addAction(tr("Edit curve properties"));
    m_editCurve->setEnabled(false);
    connect(m_editCurve, SIGNAL(triggered()), SLOT(editCurve()));

    m_deleteCurve = contextMenu->addMenu(tr("Remove Curve"));
    m_deleteCurve->setEnabled(false);
    m_deleteMap = NULL;

    QMenu *sampleSize = contextMenu->addMenu(tr("Sample size"));

    QSignalMapper *sampleMap = new QSignalMapper(this);
    static const QString sampleNames[] =
    {
        tr("Show all data"), tr("Set custom..."), "10", "50", "100", "200", "500", "1000"
    };

    for(quint8 i = 0; i < sizeof(sampleValues)/sizeof(int); ++i)
    {
        if(i == 2)
            sampleSize->addSeparator();

        m_sample_act[i] = sampleSize->addAction(sampleNames[i]);
        m_sample_act[i]->setCheckable(true);
        sampleMap->setMapping(m_sample_act[i], sampleValues[i]);
        connect(m_sample_act[i], SIGNAL(triggered()), sampleMap, SLOT(map()));
    }
    connect(sampleMap, SIGNAL(mapped(int)), SLOT(sampleSizeChanged(int)));

    m_sample_size_idx = 6;
    m_sample_size = 500;
    m_sample_act[6]->setChecked(true);

    m_showLegend = contextMenu->addAction(tr("Show legend"));
    m_showLegend->setCheckable(true);
    m_showLegend->setChecked(true);
    connect(m_showLegend, SIGNAL(triggered(bool)), this, SLOT(showLegend(bool)));
}

void GraphWidget::updateRemoveMapping()
{
    delete m_deleteMap;
    m_deleteMap = new QSignalMapper(this);
    connect(m_deleteMap, SIGNAL(mapped(QString)), SLOT(removeCurve(QString)));

    for(std::map<QString, QAction*>::iterator itr = m_deleteAct.begin(); itr != m_deleteAct.end(); ++itr)
    {
        m_deleteMap->setMapping(itr->second, itr->first);
        connect(itr->second, SIGNAL(triggered()), m_deleteMap, SLOT(map()));
    }
}

void GraphWidget::newData(analyzer_data */*data*/, quint32 index)
{
    for(quint8 i = 0; i < m_curves.size(); ++i)
        m_curves[i]->curve->dataPosChanged(index);
    updateVisibleArea();
}

void GraphWidget::processData(analyzer_data */*data*/)
{

}

void GraphWidget::saveWidgetInfo(AnalyzerDataFile *file)
{
    DataWidget::saveWidgetInfo(file);

    file->writeBlockIdentifier("graphWSample");
    file->write((char*)&m_sample_size, sizeof(m_sample_size));

    file->writeBlockIdentifier("graphWLegend");
    bool showLegend = m_showLegend->isChecked();
    file->write((char*)&showLegend, 1);

    file->writeBlockIdentifier("graphWCurveCount");
    quint32 size = m_curves.size();
    file->write((char*)&size, sizeof(quint32));

    for(quint8 i = 0; i < m_curves.size(); ++i)
    {
        GraphCurveInfo *info = m_curves[i];

        file->writeBlockIdentifier("graphWCurve");

        // curve name
        file->writeBlockIdentifier("graphWCurveName");
        QByteArray title = info->curve->title().text().toAscii();
        size = title.length();
        file->write((char*)&size, sizeof(quint32));
        file->write(title.data());

        // data info
        file->writeBlockIdentifier("graphWCurveDataInfo");
        char *p = (char*)&info->info;
        file->write(p, sizeof(data_widget_info));

        // data type
        file->writeBlockIdentifier("graphWCurveDataType");
        quint8 type = info->curve->getDataType();
        file->write((char*)&type, sizeof(quint8));

        // color
        file->writeBlockIdentifier("graphWCurveColor");
        QByteArray color = info->curve->pen().color().name().toAscii();
        size = color.length();
        file->write((char*)&size, sizeof(quint32));
        file->write(color.data());
    }
}

void GraphWidget::loadWidgetInfo(AnalyzerDataFile *file)
{
    DataWidget::loadWidgetInfo(file);

    if(file->seekToNextBlock("graphWSample", BLOCK_WIDGET))
    {
        file->read((char*)&m_sample_size, sizeof(m_sample_size));

        m_sample_size_idx = -2;

        for(quint8 i = 0; i < sizeof(sampleValues)/sizeof(int); ++i)
        {
            if(sampleValues[i] == m_sample_size)
                m_sample_size_idx = i;
            m_sample_act[i]->setChecked(sampleValues[i] == m_sample_size);
        }
        if(m_sample_size == -2)
            m_sample_act[1]->setChecked(true);
    }

    if(file->seekToNextBlock("graphWLegend", BLOCK_WIDGET))
    {
        bool show;
        file->read((char*)&show, 1);
        showLegend(show);
    }

    if(!file->seekToNextBlock("graphWCurveCount", BLOCK_WIDGET))
        return;

    quint32 curveCount;
    file->read((char*)&curveCount, sizeof(quint32));

    for(quint32 i = 0; i < curveCount; ++i)
    {
        if(!file->seekToNextBlock("graphWCurve", BLOCK_WIDGET))
            continue;

        QString name;
        quint8 dataType;
        data_widget_info info;
        QString color;

        // title
        if(!file->seekToNextBlock("graphWCurveName", "graphWCurve"))
            continue;
        {
            quint32 size = 0;
            file->read((char*)&size, sizeof(quint32));
            name = file->read(size);
        }

        // data info
        if(!file->seekToNextBlock("graphWCurveDataInfo", "graphWCurve"))
            continue;
        {
            file->read((char*)&info.pos, sizeof(info));
        }

        // data type
        if(!file->seekToNextBlock("graphWCurveDataType", "graphWCurve"))
            continue;
        {
            file->read((char*)&dataType, sizeof(quint8));
        }

        // color
        if(!file->seekToNextBlock("graphWCurveColor", "graphWCurve"))
            continue;
        {
            quint32 size = 0;
            file->read((char*)&size, sizeof(quint32));
            color = file->read(size);
        }

        GraphCurve *curve = new GraphCurve(name, m_storage, info,
                                           m_sample_size, dataType);
        curve->setPen(QPen(QColor(color)));
        curve->attach(m_graph);
        m_graph->showCurve(curve, true);
        m_curves.push_back(new GraphCurveInfo(curve, info));

        QAction *deleteCurve = m_deleteCurve->addAction(name);
        m_deleteCurve->setEnabled(true);
        m_deleteAct[name] = deleteCurve;

        m_editCurve->setEnabled(true);
    }
    updateRemoveMapping();
}

void GraphWidget::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    m_drop_data = event->mimeData()->text();

    if(m_add_dialog)
        delete m_add_dialog;
    m_add_dialog = new GraphCurveAddDialog(this, &m_curves, false);
    connect(m_add_dialog, SIGNAL(accepted()), this, SLOT(addCurve()));
    m_add_dialog->open();
}

void GraphWidget::addCurve()
{
    if(!m_add_dialog->forceEdit())
    {
        QStringList data = m_drop_data.split(" ");
        qint32 pos = data[0].toInt();
        qint16 device = data[1].toInt();
        qint16 cmd = data[2].toInt();
        setInfo(device, cmd, pos);
    }

    if(!m_add_dialog->edit())
    {
        GraphCurve *curve = new GraphCurve(m_add_dialog->getName(), m_storage, m_info,
                                           m_sample_size, m_add_dialog->getDataType());
        curve->setPen(QPen(QColor(m_add_dialog->getColor())));
        curve->attach(m_graph);
        m_graph->showCurve(curve, true);
        m_curves.push_back(new GraphCurveInfo(curve, m_info));

        QAction *deleteCurve = m_deleteCurve->addAction(m_add_dialog->getName());
        m_deleteCurve->setEnabled(true);
        m_deleteAct[m_add_dialog->getName()] = deleteCurve;

        m_editCurve->setEnabled(true);
    }
    else
    {
        GraphCurveInfo *info = NULL;
        for(quint8 i = 0; !info && i < m_curves.size(); ++i)
        {
            if(m_curves[i]->curve->title().text() == m_add_dialog->getEditName())
                info = m_curves[i];
        }
        if(!info)
            return;

        QString curName = info->curve->title().text();

        m_deleteCurve->removeAction(m_deleteAct[curName]);
        delete m_deleteAct[curName];
        m_deleteAct.erase(curName);

        QAction *deleteCurve = m_deleteCurve->addAction(m_add_dialog->getName());
        m_deleteAct[m_add_dialog->getName()] = deleteCurve;

        info->curve->setTitle(m_add_dialog->getName());
        info->curve->setPen(QPen(QColor(m_add_dialog->getColor())));
        if(m_add_dialog->forceEdit())
            info->curve->setDataInfo(m_info);
        info->curve->setDataType(m_add_dialog->getDataType());
    }

    updateRemoveMapping();
    updateVisibleArea();

    delete m_add_dialog;
    m_add_dialog = NULL;

    m_assigned = true;
    emit updateData();
}

void GraphWidget::updateVisibleArea()
{
    if(m_curves.empty())
        return;

    qint32 max = 0;
    qint32 min = 0;
    quint32 size = 0;

    for(quint8 i = 0; i < m_curves.size(); ++i)
    {
        qint32 c_min = m_curves[i]->curve->getMin();
        qint32 c_max = m_curves[i]->curve->getMax();
        quint32 c_size = m_curves[i]->curve->getSize();

        if(c_min < min)   min  = c_min;
        if(c_max > max)   max  = c_max;
        if(c_size > size) size = c_size;
    }

    if(max == 0) max = 1;
    if(min == 0) min = -1;

    m_graph->setAxisScale(QwtPlot::xBottom, 0, size);
    m_graph->setAxisScale(QwtPlot::yLeft, min, max);

    m_graph->replot();
}

void GraphWidget::sampleSizeChanged(int val)
{
    if(val != -2 && sampleValues[m_sample_size_idx] == val)
        return;

    int sample = val;
    bool ok = true;
    if(val == -2)
    {
        sample = QInputDialog::getInt(this, tr("Set sample size"), tr("Sample size:"), 0, 0, 2147483647, 1, &ok);
        if(ok)
            m_sample_size_idx = -2;
    }

    for(quint8 i = 0; i < sizeof(sampleValues)/sizeof(int); ++i)
    {
        if(sampleValues[i] == val)
            m_sample_size_idx = i;
        m_sample_act[i]->setChecked(sampleValues[i] == val);
    }

    if(!ok || m_sample_size == sample)
        return;

    for(quint8 i = 0; i < m_curves.size(); ++i)
        m_curves[i]->curve->setSampleSize(sample);

    m_sample_size = sample;

    updateVisibleArea();
}

void GraphWidget::editCurve()
{
    if(m_add_dialog)
        delete m_add_dialog;
    m_add_dialog = new GraphCurveAddDialog(this, &m_curves, true);
    connect(m_add_dialog, SIGNAL(accepted()), this, SLOT(addCurve()));
    m_add_dialog->open();
}

void GraphWidget::removeCurve(QString name)
{
    std::vector<GraphCurveInfo*>::iterator itr = m_curves.begin();
    for(; itr != m_curves.end(); ++itr)
    {
        QString cur_name = (*itr)->curve->title().text();
        if(cur_name == name)
            break;
    }

    if(itr == m_curves.end())
        return;

    GraphCurveInfo *info = *itr;
    m_curves.erase(itr);

    delete info->curve;
    delete info;

    m_deleteCurve->removeAction(m_deleteAct[name]);
    delete m_deleteAct[name];
    m_deleteAct.erase(name);

    m_graph->replot();

    m_editCurve->setEnabled(!m_curves.empty());
    m_deleteCurve->setEnabled(!m_curves.empty());
}

void GraphWidget::showLegend(bool show)
{
    m_showLegend->setChecked(show);
    m_graph->showLegend(show);
}

GraphWidgetAddBtn::GraphWidgetAddBtn(QWidget *parent) : DataWidgetAddBtn(parent)
{
    setText(tr("Graph"));
    setIconSize(QSize(17, 17));
    setIcon(QIcon(":/dataWidgetIcons/graph.png"));

    m_widgetType = WIDGET_GRAPH;
}