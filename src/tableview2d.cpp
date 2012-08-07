/***************************************************************************
*   Copyright (C) 2012  Michael Carpenter (malcom2073)                     *
*                                                                          *
*   This file is a part of EMStudio                                        *
*                                                                          *
*   EMStudio is free software: you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License version 2 as      *
*   published by the Free Software Foundation.                             *
*                                                                          *
*   EMStudio is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
*   GNU General Public License for more details.                           *
									   *
*   You should have received a copy of the GNU General Public License      *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
****************************************************************************/

#include "tableview2d.h"
#include <QMessageBox>
#include <QDebug>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qjson/serializer.h>
#include <QFileDialog>
TableView2D::TableView2D(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	tableData=0;
	//ui.tableWidget->setColumnCount(1);
	ui.tableWidget->setRowCount(2);
	ui.tableWidget->horizontalHeader()->hide();
	ui.tableWidget->verticalHeader()->hide();
	//ui.tableWidget->setColumnWidth(0,100);
	connect(ui.savePushButton,SIGNAL(clicked()),this,SLOT(saveClicked()));
	connect(ui.loadPushButton,SIGNAL(clicked()),this,SLOT(loadClicked()));
	connect(ui.tableWidget,SIGNAL(cellChanged(int,int)),this,SLOT(tableCellChanged(int,int)));
	connect(ui.tableWidget,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(tableCurrentCellChanged(int,int,int,int)));
	connect(ui.exportPushButton,SIGNAL(clicked()),this,SLOT(exportClicked()));
	QPalette pal = ui.plot->palette();
	pal.setColor(QPalette::Background,QColor::fromRgb(0,0,0));
	ui.plot->setPalette(pal);
	curve = new QwtPlotCurve("Test");
	curve->attach(ui.plot);
	curve->setPen(QPen(QColor::fromRgb(255,0,0),3));
	QwtPlotGrid *grid = new QwtPlotGrid();
	grid->setPen(QPen(QColor::fromRgb(100,100,100)));
	grid->attach(ui.plot);

	//curve->setData()
	//QwtSeriesData<QwtIntervalSample> series;


}
void TableView2D::exportJson(QString filename)
{
	//Create a JSON file similar to MTX's yaml format.
	QVariantMap topmap;
	QVariantMap x;
	QVariantMap y;
	QVariantMap z;
	QVariantList xlist;
	QVariantList ylist;
	QVariantList zlist;

	topmap["2DTable"] = "";
	topmap["title"] = m_metaData.tableTitle;
	topmap["description"] = m_metaData.tableTitle;
	x["unit"] = m_metaData.xAxisTitle;
	x["label"] = m_metaData.xAxisTitle;
	y["unit"] = m_metaData.yAxisTitle;
	y["label"] = m_metaData.yAxisTitle;

	for (int i=1;i<ui.tableWidget->columnCount();i++)
	{
		//Reformat the number to be XXXX.XX to make Fred happy.
		double val = ui.tableWidget->item(0,i)->text().toDouble();
		xlist.append(QString::number(val,'f',2));
		double val2 = ui.tableWidget->item(1,i)->text().toDouble();
		ylist.append(QString::number(val2,'f',2));
	}
	y["values"] = ylist;
	x["values"] = xlist;
	topmap["X"] = x;
	topmap["Y"] = y;

	QJson::Serializer serializer;
	QByteArray serialized = serializer.serialize(topmap);

	//TODO: Open a message box and allow the user to select where they want to save the file.
	QFile file(filename);
	if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
	{
		qDebug() << "Unable to open file to output JSON!";
		return;
	}
	file.write(serialized);
	file.close();
}

void TableView2D::exportClicked()
{
	QString filename = QFileDialog::getSaveFileName(this,"Save Json File",".","Json Files (*.json)");
	if (filename == "")
	{
		return;
	}
	exportJson(filename);
}
void TableView2D::tableCurrentCellChanged(int currentrow,int currentcolumn,int prevrow,int prevcolumn)
{
	Q_UNUSED(prevrow)
	Q_UNUSED(prevcolumn)
	if (currentrow == -1 || currentcolumn == -1)
	{
		return;
	}
	m_currRow = currentrow;
	m_currCol = currentcolumn;
	currentvalue = ui.tableWidget->item(currentrow,currentcolumn)->text().toDouble();
}
void TableView2D::resizeColumnWidths()
{
	unsigned int max = 0;
	for (int i=0;i<ui.tableWidget->columnCount();i++)
	{
		for (int j=0;j<ui.tableWidget->rowCount();j++)
		{
			if (ui.tableWidget->item(j,i))
			{
				//1.3 is required to make text show correctly
				unsigned int test = ui.tableWidget->fontMetrics().width(ui.tableWidget->item(j,i)->text()) * 1.3;
				if (test > max)
				{
					max = test;
				}
			}
		}
	}
	for (int i=0;i<ui.tableWidget->columnCount();i++)
	{
		ui.tableWidget->setColumnWidth(i,max);
	}
}
void TableView2D::loadClicked()
{
	if (QMessageBox::information(0,"Warning","Doing this will reload the table from flash, and wipe out any changes you may have made. Are you sure you want to do this?",QMessageBox::Yes,QMessageBox::No) == QMessageBox::Yes)
	{
		qDebug() << "Ok";
		emit reloadTableData(m_locationid);
	}
	else
	{
		qDebug() << "Not ok";
	}
}

void TableView2D::tableCellChanged(int row,int column)
{
	if (row == -1 || column == -1)
	{
		qDebug() << "Negative array index! Should be unreachable code! FIXME!";
		return;
	}
	if (row >= ui.tableWidget->rowCount() || column >= ui.tableWidget->columnCount())
	{
		qDebug() << "Larger than life, should be unreachable code! FIXME!";
		return;
	}
	bool conversionOk = false;
	double tempValue=ui.tableWidget->item(row,column)->text().toDouble(&conversionOk);
	double oldValue = tempValue;
	if (!conversionOk)
	{
		QMessageBox::information(0,"Error","Value entered is not a number!");
		setSilentValue(row,column,QString::number(currentvalue,'f',2));
		//ui.tableWidget->item(row,column)->setText(QString::number(currentvalue));
		return;
	}
	//qDebug() << "New Value:" << tempValue;
	setSilentValue(row,column,QString::number(tempValue,'f',2));
	tempValue = ui.tableWidget->item(row,column)->text().toDouble(&conversionOk);
	//qDebug() << "Brand New Value:" << tempValue;
	//New value has been accepted. Let's write it.
	if (samples.size() <= column)
	{
		return;
	}
	//unsigned short newval = ui.tableWidget->item(row,column)->text().toInt();
	//currentvalue = newval;
	if (row == 0)
	{
		if (tempValue > tableData->maxXAxis())
		{
			QMessageBox::information(0,"Error",QString("Value entered too large! Value range " + QString::number(tableData->minXAxis()) + "-" + QString::number(tableData->maxXAxis()) + ". Entered value:") + ui.tableWidget->item(row,column)->text());
			setSilentValue(row,column,QString::number(currentvalue,'f',2));
			//ui.tableWidget->item(row,column)->setText(QString::number(currentvalue));
			return;
		}
		else if (tempValue < tableData->minXAxis())
		{
			QMessageBox::information(0,"Error",QString("Value entered too small! Value range " + QString::number(tableData->minXAxis()) + "-" + QString::number(tableData->maxXAxis()) + ". Entered value:") + ui.tableWidget->item(row,column)->text());
			setSilentValue(row,column,QString::number(currentvalue,'f',2));
			//ui.tableWidget->item(row,column)->setText(QString::number(currentvalue));
			return;
		}

		currentvalue = oldValue;
		//tableData->setCell(0,column,currentvalue);


		samples.replace(column,QPointF(ui.tableWidget->item(row,column)->text().toInt(),samples.at(column).y()));
		curve->setSamples(samples);
		ui.plot->replot();
	}
	else if (row == 1)
	{
		if (tempValue > tableData->maxXAxis())
		{
			QMessageBox::information(0,"Error",QString("Value entered too large! Value range " + QString::number(tableData->minXAxis()) + "-" + QString::number(tableData->maxXAxis()) + ". Entered value:") + ui.tableWidget->item(row,column)->text());
			setSilentValue(row,column,QString::number(currentvalue,'f',2));
			//ui.tableWidget->item(row,column)->setText(QString::number(currentvalue));
			return;
		}
		else if (tempValue < tableData->minXAxis())
		{
			QMessageBox::information(0,"Error",QString("Value entered too small! Value range " + QString::number(tableData->minXAxis()) + "-" + QString::number(tableData->maxXAxis()) + ". Entered value:") + ui.tableWidget->item(row,column)->text());
			setSilentValue(row,column,QString::number(currentvalue,'f',2));
			//ui.tableWidget->item(row,column)->setText(QString::number(currentvalue));
			return;
		}
		//tableData->setXAxis(column,currentvalue);
		currentvalue = oldValue;
		samples.replace(column,QPointF(samples.at(column).x(),ui.tableWidget->item(row,column)->text().toInt()));
		curve->setSamples(samples);
		ui.plot->replot();
	}
	//New value has been accepted. Let's write it.
	tableData->setCell(row,column,oldValue); //This will emit saveSingleData
	//ui.tableWidget->resizeColumnsToContents();
	resizeColumnWidths();
}
void TableView2D::resizeEvent(QResizeEvent *evt)
{
	Q_UNUSED(evt)
	/*for (int i=0;i<ui.tableWidget->columnCount();i++)
	{
		ui.tableWidget->setColumnWidth(i,(ui.tableWidget->width() / ui.tableWidget->columnCount())-1);
	}*/
	resizeColumnWidths();
}
void TableView2D::setSilentValue(int row,int column,QString value)
{
	ui.tableWidget->disconnect(SIGNAL(cellChanged(int,int)));
	ui.tableWidget->disconnect(SIGNAL(currentCellChanged(int,int,int,int)));
	ui.tableWidget->item(row,column)->setText(value);
	connect(ui.tableWidget,SIGNAL(cellChanged(int,int)),this,SLOT(tableCellChanged(int,int)));
	connect(ui.tableWidget,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(tableCurrentCellChanged(int,int,int,int)));
}
bool TableView2D::passData(unsigned short locationid,QByteArray data,int physicallocation)
{
	return passData(locationid,data,physicallocation,Table2DMetaData());
}

bool TableView2D::passData(unsigned short locationid,QByteArray rawdata,int physicallocation,Table2DMetaData metadata)
{
	Q_UNUSED(physicallocation)
	m_metaData = metadata;
	if (tableData)
	{
		tableData->deleteLater();
	}
	tableData = new Table2DData(locationid,rawdata,m_metaData);
	connect(tableData,SIGNAL(saveSingleData(unsigned short,QByteArray,unsigned short,unsigned short)),this,SIGNAL(saveSingleData(unsigned short,QByteArray,unsigned short,unsigned short)));
	qDebug() << "TableView2D::passData" << "0x" + QString::number(locationid,16).toUpper();
	samples.clear();
	m_locationid = locationid;
	ui.tableWidget->disconnect(SIGNAL(cellChanged(int,int)));
	ui.tableWidget->clear();
	ui.tableWidget->setColumnCount(0);
	ui.tableWidget->setRowCount(2);
	double first = tableData->axis()[0];
	int order = 0;
	for (int i=0;i<tableData->columns();i++)
	{
		if (i == 1)
		{
			if (tableData->axis()[i] < first)
			{
				order = 1;
			}
			else
			{
				order = 2;
			}
		}
		if (order == 1)
		{
			if (tableData->axis()[i] > first)
			{
				//Out of order table axis.
				return false;
			}
		}
		else if (order == 2)
		{
			if (tableData->axis()[i] < first)
			{
				//Out of order table axis.
				return false;
			}
		}
		first = tableData->axis()[i];

		ui.tableWidget->setColumnCount(ui.tableWidget->columnCount()+1);
		ui.tableWidget->setItem(0,ui.tableWidget->columnCount()-1,new QTableWidgetItem(QString::number(tableData->axis()[i],'f',m_metaData.xDp)));
		ui.tableWidget->setItem(1,ui.tableWidget->columnCount()-1,new QTableWidgetItem(QString::number(tableData->values()[i],'f',m_metaData.yDp)));

		if (tableData->values()[i] < tableData->maxXAxis()/4)
		{
			ui.tableWidget->item(1,ui.tableWidget->columnCount()-1)->setBackgroundColor(QColor::fromRgb(0,(255*((tableData->values()[i])/(tableData->maxXAxis()/4.0))),255));
		}
		else if (tableData->values()[i] < ((tableData->maxXAxis()/4)*2))
		{
			ui.tableWidget->item(1,ui.tableWidget->columnCount()-1)->setBackgroundColor(QColor::fromRgb(0,255,255-(255*((tableData->values()[i]-((tableData->maxXAxis()/4.0)))/(tableData->maxXAxis()/4.0)))));
		}
		else if (tableData->values()[i] < ((tableData->maxXAxis()/4)*3))
		{
			ui.tableWidget->item(1,ui.tableWidget->columnCount()-1)->setBackgroundColor(QColor::fromRgb((255*((tableData->values()[i]-((tableData->maxXAxis()/4.0)*2))/(tableData->maxXAxis()/4.0))),255,0));
		}
		else
		{
			ui.tableWidget->item(1,ui.tableWidget->columnCount()-1)->setBackgroundColor(QColor::fromRgb(255,255-(255*((tableData->values()[i]-((tableData->maxXAxis()/4.0)*3))/(tableData->maxXAxis()/4.0))),0));
		}
		samples.append(QPointF(tableData->axis()[i],tableData->values()[i]));
	}
	connect(ui.tableWidget,SIGNAL(cellChanged(int,int)),this,SLOT(tableCellChanged(int,int)));
	curve->setSamples(samples);
	ui.plot->replot();
	ui.tableWidget->setCurrentCell(m_currRow,m_currCol);
	//ui.tableWidget->resizeColumnsToContents();
	resizeColumnWidths();
	return true;
}

TableView2D::~TableView2D()
{
}
void TableView2D::saveClicked()
{
	emit saveToFlash(m_locationid);
}
