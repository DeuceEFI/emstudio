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

#include "mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include "datafield.h"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSettings>
#include <tableview2d.h>
#include <tableview3d.h>
#include <qjson/parser.h>
#define define2string_p(x) #x
#define define2string(x) define2string_p(x)
#define TABLE_3D_PAYLOAD_SIZE 1024
#define TABLE_2D_PAYLOAD_SIZE 64
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	progressView=0;
	m_interrogationInProgress = false;
	qDebug() << "Loading config file freeems.config.json";
	QFile file("freeems.config.json");
	file.open(QIODevice::ReadOnly);
	QByteArray filebytes = file.readAll();
	file.close();

	QJson::Parser parser;
	QVariant top = parser.parse(filebytes);
	if (!top.isValid())
	{
		QString errormsg = QString("Error parsing JSON from config file on line number: ") + QString::number(parser.errorLine()) + " error text: " + parser.errorString();
		QMessageBox::information(0,"Error",errormsg);
		qDebug() << "Error parsing JSON";
		qDebug() << "Line number:" << parser.errorLine() << "error text:" << parser.errorString();
		return;
	}
	QVariantMap topmap = top.toMap();
	QVariantMap errormap = topmap["errormap"].toMap();
	QVariantMap::iterator i = errormap.begin();
	while (i != errormap.end())
	{
		bool ok = false;
		m_errorMap[i.value().toString().mid(2).toInt(&ok,16)] = i.key();
		i++;
	}

	QVariantMap ramvars = topmap["ramvars"].toMap();
	i = ramvars.begin();
	while (i != ramvars.end())
	{
		bool ok = false;
		unsigned short locid = i.key().mid(2).toInt(&ok,16);
		m_readOnlyMetaDataMap[locid] = ReadOnlyRamBlock();
		QVariantMap locidlist = i.value().toMap();
		QString title = locidlist["title"].toString();
		m_readOnlyMetaDataMap[locid].title = title;
		QVariantList locidmap = locidlist["vars"].toList();
		int offset = 0;
		for (int j=0;j<locidmap.size();j++)
		{
			QVariantMap newlocidmap = locidmap[j].toMap();
			ReadOnlyRamData rdata;
			rdata.dataTitle = newlocidmap["name"].toString();
			rdata.dataDescription = newlocidmap["title"].toString();
			rdata.locationId = locid;
			rdata.offset = offset;
			rdata.size = newlocidmap["size"].toInt();
			offset += rdata.size;
			m_readOnlyMetaDataMap[locid].m_ramData.append(rdata);
			m_readOnlyMetaData.append(rdata);
			//m_readOnlyMetaDataMap[locid].append(rdata);

		}
		/*QVariantMap::iterator j = locidmap.begin();
		while (j != locidmap.end())
		{
			if (j.key() == "title")
			{
				QString title = j.value().toString();
				qDebug() << "Location title:" << title;
			}
			else
			{
				qDebug() << j.key();
				QVariantMap valuemap = j.value().toMap();
				if (valuemap.contains("type"))
				{
					ConfigData cdata;
					cdata.configDescription = valuemap["title"].toString();
					cdata.configTitle = j.key();
					cdata.elementSize = valuemap["size"].toInt();
					cdata.locationId = locid;
					cdata.offset = valuemap["offset"].toInt();
					cdata.type = valuemap["type"].toString();
					QVariantMap calcmap = valuemap["calc"].toMap();
					QList<QPair<QString,double> > calclist;
					QVariantMap::iterator k = calcmap.begin();
					while (k != calcmap.end())
					{
						calclist.append(QPair<QString,double>(k.key(),k.value().toDouble()));
						k++;
					}
					cdata.elementCalc = calclist;
					if (valuemap["type"] == "value")
					{

					}
					else if (valuemap["type"] == "array")
					{
						cdata.arraySize = valuemap["arraysize"].toInt();
					}
					m_configMetaData.append(cdata);
				}

			}
			j++;
		}*/
		i++;
	}
	qDebug() << m_readOnlyMetaData.size() << "Ram entries found";
	QVariantMap tables = topmap["tables"].toMap();
	i = tables.begin();
	while (i != tables.end())
	{
		//qDebug() << "Table:" << i.key();
		QVariantMap tabledata = i.value().toMap();
		if (tabledata["type"] == "3D")
		{
			Table3DMetaData meta;
			QString id = tabledata["locationid"].toString();
			QString xtitle = tabledata["xtitle"].toString();
			QVariantList xcalc = tabledata["xcalc"].toList();
			QString xdp = tabledata["xdp"].toString();
			unsigned int size = tabledata["size"].toInt();

			QString ytitle = tabledata["ytitle"].toString();
			QVariantList ycalc = tabledata["ycalc"].toList();
			QString ydp = tabledata["ydp"].toString();

			QString ztitle = tabledata["ztitle"].toString();
			QVariantList zcalc = tabledata["zcalc"].toList();
			QString zdp = tabledata["zdp"].toString();

			//QVariantMap::iterator calci = xcalc.begin();
			QList<QPair<QString,double> > xcalclist;
			QList<QPair<QString,double> > ycalclist;
			QList<QPair<QString,double> > zcalclist;
			for (int j=0;j<xcalc.size();j++)
			{
				qDebug() << "XCalc:" << xcalc[j].toMap()["type"].toString() << xcalc[j].toMap()["value"].toDouble();
				xcalclist.append(QPair<QString,double>(xcalc[j].toMap()["type"].toString(),xcalc[j].toMap()["value"].toDouble()));
			}
			for (int j=0;j<ycalc.size();j++)
			{
				ycalclist.append(QPair<QString,double>(ycalc[j].toMap()["type"].toString(),ycalc[j].toMap()["value"].toDouble()));
			}
			for (int j=0;j<zcalc.size();j++)
			{
				zcalclist.append(QPair<QString,double>(zcalc[j].toMap()["type"].toString(),zcalc[j].toMap()["value"].toDouble()));
			}

			bool ok = false;
			meta.locationId = id.mid(2).toInt(&ok,16);
			meta.tableTitle = i.key();
			meta.xAxisCalc = xcalclist;
			meta.xAxisTitle = xtitle;
			meta.xDp = xdp.toInt();
			meta.yAxisCalc = ycalclist;
			meta.yAxisTitle = ytitle;
			meta.yDp = ydp.toInt();
			meta.zAxisCalc = zcalclist;
			meta.zAxisTitle = ztitle;
			meta.zDp = zdp.toInt();
			meta.size = size;
			m_table3DMetaData.append(meta);
		}
		else if (tabledata["type"] == "2D")
		{
			Table2DMetaData meta;
			QString id = tabledata["locationid"].toString();
			QString xtitle = tabledata["xtitle"].toString();
			QVariantList xcalc = tabledata["xcalc"].toList();
			QString xdp = tabledata["xdp"].toString();
			QString ytitle = tabledata["ytitle"].toString();
			QVariantList ycalc = tabledata["ycalc"].toList();
			QString ydp = tabledata["ydp"].toString();
			unsigned int size = tabledata["size"].toInt();

			QList<QPair<QString,double> > xcalclist;
			QList<QPair<QString,double> > ycalclist;

			for (int j=0;j<xcalc.size();j++)
			{
				qDebug() << "XCalc:" << xcalc[j].toMap()["type"].toString() << xcalc[j].toMap()["value"].toDouble();
				xcalclist.append(QPair<QString,double>(xcalc[j].toMap()["type"].toString(),xcalc[j].toMap()["value"].toDouble()));
			}
			for (int j=0;j<ycalc.size();j++)
			{
				ycalclist.append(QPair<QString,double>(ycalc[j].toMap()["type"].toString(),ycalc[j].toMap()["value"].toDouble()));
			}
			bool ok = false;
			meta.locationId = id.mid(2).toInt(&ok,16);
			meta.tableTitle = i.key();
			meta.xAxisCalc = xcalclist;
			meta.xAxisTitle = xtitle;
			meta.xDp = xdp.toInt();
			meta.yAxisCalc = ycalclist;
			meta.yAxisTitle = ytitle;
			meta.yDp = ydp.toInt();
			meta.size = size;
			m_table2DMetaData.append(meta);
		}
		i++;
	}
	qDebug() << m_errorMap.keys().size() << "Error Keys Loaded";
	qDebug() << m_table3DMetaData.size() << "3D Tables Loaded";
	qDebug() << m_table2DMetaData.size() << "2D Tables Loaded";
	//return;
	m_currentRamLocationId=0;
	//populateDataFields();
	m_localRamDirty = false;
	m_deviceFlashDirty = false;
	m_waitingForRamWriteConfirmation = false;
	m_waitingForFlashWriteConfirmation = false;
	ui.setupUi(this);
	this->setWindowTitle(QString("EMStudio ") + QString(define2string(GIT_COMMIT)));
	emsinfo.emstudioCommit = define2string(GIT_COMMIT);
	emsinfo.emstudioHash = define2string(GIT_HASH);
	ui.actionDisconnect->setEnabled(false);
	connect(ui.actionSettings,SIGNAL(triggered()),this,SLOT(menu_settingsClicked()));
	connect(ui.actionConnect,SIGNAL(triggered()),this,SLOT(menu_connectClicked()));
	connect(ui.actionDisconnect,SIGNAL(triggered()),this,SLOT(menu_disconnectClicked()));
	connect(ui.actionEMS_Info,SIGNAL(triggered()),this,SLOT(menu_windows_EmsInfoClicked()));
	connect(ui.actionGauges,SIGNAL(triggered()),this,SLOT(menu_windows_GaugesClicked()));
	connect(ui.actionTables,SIGNAL(triggered()),this,SLOT(menu_windows_TablesClicked()));
	connect(ui.actionFlags,SIGNAL(triggered()),this,SLOT(menu_windows_FlagsClicked()));
	connect(ui.actionExit_3,SIGNAL(triggered()),this,SLOT(close()));
	connect(ui.actionPacket_Status,SIGNAL(triggered()),this,SLOT(menu_windows_PacketStatusClicked()));
	connect(ui.actionAbout,SIGNAL(triggered()),this,SLOT(menu_aboutClicked()));
	//connect(ui.action_Raw_Data,SIGNAL(triggered()),this,SLOT(menu_window_rawDataClicked()));

	//connect(ui.saveDataPushButton,SIGNAL(clicked()),this,SLOT(ui_saveDataButtonClicked()));

	emsInfo=0;
	dataTables=0;
	dataFlags=0;
	dataGauges=0;


	//connect(ui.connectPushButton,SIGNAL(clicked()),this,SLOT(connectButtonClicked()));
	//connect(ui.loadLogPushButton,SIGNAL(clicked()),this,SLOT(loadLogButtonClicked()));
	//connect(ui.playLogPushButton,SIGNAL(clicked()),this,SLOT(playLogButtonClicked()));
	//connect(ui.pauseLogPushButton,SIGNAL(clicked()),this,SLOT(pauseLogButtonClicked()));
	//connect(ui.stopLogPushButton,SIGNAL(clicked()),this,SLOT(stopLogButtonClicked()));


	//connect(ui.interByteDelaySpinBox,SIGNAL(valueChanged(int)),this,SLOT(interByteDelayChanged(int)));
	dataPacketDecoder = new DataPacketDecoder(this);
	connect(dataPacketDecoder,SIGNAL(payloadDecoded(QVariantMap)),this,SLOT(dataLogDecoded(QVariantMap)));
	//

	/*logLoader = new LogLoader(this);
	connect(logLoader,SIGNAL(endOfLog()),this,SLOT(logFinished()));
	connect(logLoader,SIGNAL(payloadReceived(QByteArray,QByteArray)),this,SLOT(logPayloadReceived(QByteArray,QByteArray)));
	connect(logLoader,SIGNAL(logProgress(qlonglong,qlonglong)),this,SLOT(logProgress(qlonglong,qlonglong)));
	*/
	emsComms = new FreeEmsComms(this);
	m_logFileName = QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss");
	emsComms->setLogFileName(m_logFileName);
	connect(emsComms,SIGNAL(commandTimedOut(int)),this,SLOT(commandTimedOut(int)));
	connect(emsComms,SIGNAL(connected()),this,SLOT(emsCommsConnected()));
	connect(emsComms,SIGNAL(disconnected()),this,SLOT(emsCommsDisconnected()));
	connect(emsComms,SIGNAL(dataLogPayloadReceived(QByteArray,QByteArray)),this,SLOT(logPayloadReceived(QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(firmwareVersion(QString)),this,SLOT(firmwareVersion(QString)));
	connect(emsComms,SIGNAL(compilerVersion(QString)),this,SLOT(emsCompilerVersion(QString)));
	connect(emsComms,SIGNAL(interfaceVersion(QString)),this,SLOT(interfaceVersion(QString)));
	connect(emsComms,SIGNAL(locationIdList(QList<unsigned short>)),this,SLOT(locationIdList(QList<unsigned short>)));
	connect(emsComms,SIGNAL(unknownPacket(QByteArray,QByteArray)),this,SLOT(unknownPacket(QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(commandSuccessful(int)),this,SLOT(commandSuccessful(int)));
	connect(emsComms,SIGNAL(commandFailed(int,unsigned short)),this,SLOT(commandFailed(int,unsigned short)));
	connect(emsComms,SIGNAL(locationIdInfo(unsigned short,unsigned short,QList<FreeEmsComms::LocationIdFlags>,unsigned short,unsigned char,unsigned char,unsigned short,unsigned short,unsigned short)),this,SLOT(locationIdInfo(unsigned short,unsigned short,QList<FreeEmsComms::LocationIdFlags>,unsigned short,unsigned char,unsigned char,unsigned short,unsigned short,unsigned short)));
	connect(emsComms,SIGNAL(ramBlockRetrieved(unsigned short,QByteArray,QByteArray)),this,SLOT(ramBlockRetrieved(unsigned short,QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(flashBlockRetrieved(unsigned short,QByteArray,QByteArray)),this,SLOT(flashBlockRetrieved(unsigned short,QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(decoderName(QString)),this,SLOT(emsDecoderName(QString)));
	connect(emsComms,SIGNAL(operatingSystem(QString)),this,SLOT(emsOperatingSystem(QString)));
	connect(emsComms,SIGNAL(firmwareBuild(QString)),this,SLOT(emsFirmwareBuildDate(QString)));

	//connect(emsComms,SIGNAL(destroyed()),this,SLOT(dataTablesDestroyed()));

	emsInfo = new EmsInfoView();
	//connect(emsComms,SIGNAL(locationIdInfo(unsigned short,QString,unsigned short,QList<FreeEmsComms::LocationIdFlags>,unsigned short,unsigned char,unsigned char,unsigned short,unsigned short,unsigned short)),emsInfo,SLOT(locationIdInfo(unsigned short,QString,unsigned short,QList<FreeEmsComms::LocationIdFlags>,unsigned short,unsigned char,unsigned char,unsigned short,unsigned short,unsigned short)));

	emsInfo->setFirmwareVersion(m_firmwareVersion);
	emsInfo->setInterfaceVersion(m_interfaceVersion);
	connect(emsInfo,SIGNAL(displayLocationId(int,bool,int)),this,SLOT(emsInfoDisplayLocationId(int,bool,int)));


	emsMdiWindow = ui.mdiArea->addSubWindow(emsInfo);
	emsMdiWindow->setGeometry(emsInfo->geometry());
	emsMdiWindow->hide();
	emsMdiWindow->setWindowTitle("EMS Info");


	aboutView = new AboutView();
	aboutView->setHash(define2string(GIT_HASH));
	aboutView->setCommit(define2string(GIT_COMMIT));
	aboutMdiWindow = ui.mdiArea->addSubWindow(aboutView);
	aboutMdiWindow->setGeometry(aboutView->geometry());
	aboutMdiWindow->hide();
	aboutMdiWindow->setWindowTitle("About");

	dataGauges = new GaugeView();
	//connect(dataGauges,SIGNAL(destroyed()),this,SLOT(dataGaugesDestroyed()));
	gaugesMdiWindow = ui.mdiArea->addSubWindow(dataGauges);
	gaugesMdiWindow->setGeometry(dataGauges->geometry());
	gaugesMdiWindow->hide();
	gaugesMdiWindow->setWindowTitle("Gauges");

	dataTables = new TableView();
	//connect(dataTables,SIGNAL(destroyed()),this,SLOT(dataTablesDestroyed()));
	dataTables->passDecoder(dataPacketDecoder);
	tablesMdiWindow = ui.mdiArea->addSubWindow(dataTables);
	tablesMdiWindow->setGeometry(dataTables->geometry());
	tablesMdiWindow->hide();
	tablesMdiWindow->setWindowTitle("Data Tables");

	dataFlags = new FlagView();
	//connect(dataFlags,SIGNAL(destroyed()),this,SLOT(dataFlagsDestroyed()));
	dataFlags->passDecoder(dataPacketDecoder);
	flagsMdiWindow = ui.mdiArea->addSubWindow(dataFlags);
	flagsMdiWindow->setGeometry(dataFlags->geometry());
	flagsMdiWindow->hide();
	flagsMdiWindow->setWindowTitle("Flags");

	packetStatus = new PacketStatusView();
	connect(emsComms,SIGNAL(packetSent(unsigned short,QByteArray,QByteArray)),packetStatus,SLOT(passPacketSent(unsigned short,QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(packetAcked(unsigned short,QByteArray,QByteArray)),packetStatus,SLOT(passPacketAck(unsigned short,QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(packetNaked(unsigned short,QByteArray,QByteArray,unsigned short)),packetStatus,SLOT(passPacketNak(unsigned short,QByteArray,QByteArray,unsigned short)));
	connect(emsComms,SIGNAL(decoderFailure(QByteArray)),packetStatus,SLOT(passDecoderFailure(QByteArray)));
	packetStatusMdiWindow = ui.mdiArea->addSubWindow(packetStatus);
	packetStatusMdiWindow->setGeometry(packetStatus->geometry());
	packetStatusMdiWindow->hide();
	packetStatusMdiWindow->setWindowTitle("Packet Status");



	//Load settings
	QSettings settings("settings.ini",QSettings::IniFormat);
	settings.beginGroup("comms");
	m_comPort = settings.value("port","/dev/ttyUSB0").toString();
	m_comBaud = settings.value("baud",115200).toInt();
	m_comInterByte = settings.value("interbytedelay",0).toInt();
	m_saveLogs = settings.value("savelogs",true).toBool();
	m_clearLogs = settings.value("clearlogs",false).toBool();
	m_logsToKeep = settings.value("logstokeep",0).toInt();
	m_logDirectory = settings.value("logdir",".").toString();
	settings.endGroup();

	emsComms->setBaud(m_comBaud);
	emsComms->setPort(m_comPort);
	emsComms->setLogDirectory(m_logDirectory);
	emsComms->setLogsEnabled(m_saveLogs);


	pidcount = 0;

	timer = new QTimer(this);
	connect(timer,SIGNAL(timeout()),this,SLOT(timerTick()));
	timer->start(1000);

	guiUpdateTimer = new QTimer(this);
	connect(guiUpdateTimer,SIGNAL(timeout()),this,SLOT(guiUpdateTimerTick()));
	guiUpdateTimer->start(250);

	statusBar()->addWidget(ui.ppsLabel);
	statusBar()->addWidget(ui.statusLabel);


	logfile = new QFile("myoutput.log");
	logfile->open(QIODevice::ReadWrite | QIODevice::Truncate);



	/*QFile file("log.inandout.log");
	file.open(QIODevice::ReadOnly);
	QByteArray filebytes = file.readAll();
	file.close();
	unsigned char data2[] = {
	0xAA,0x00,0x01,0x04,
	0x00,0x01,0x00,0x00,0x00,0x00,0x06,0xCC
	};

	for (int i=0;i<filebytes.size();i++)
	{
		for (int j=0;j<12;j++)
		{
			if ((unsigned char)filebytes[i+j] == data2[j])
			{
				if (j == 11)
				{
					//Good data.
					//1032 bytes
					filebytes.mid(i+j+1,1032);
					qDebug() << "Found:";
				}
			}
			else
			{
				break;
			}
		}
	}
	//38686
	//39709
	filebytes = filebytes.mid(38686,(39709-38686)+1);
	qDebug() <<"Start:" << QString::number((unsigned char)filebytes[0],16) << QString::number((unsigned char)filebytes[filebytes.length()-1],16);
	//filebytes = filebytes.mid(6,filebytes.length()-7);
	qDebug() <<"Start:" << QString::number((unsigned char)filebytes[0],16) << QString::number((unsigned char)filebytes[filebytes.length()-1],16);

	//TEST 3D TABLE
	QByteArray data;
	unsigned short xlength = 24;
	unsigned short ylength = 19;
	QByteArray xdata;
	QByteArray ydata;
	data.append((char)(((xlength) >> 8) & 0xFF));
	data.append((xlength) & 0xFF);
	data.append((char)(((ylength) >> 8) & 0xFF));
	data.append((ylength) & 0xFF);
	for (int i=0;i<xlength;i++)
	{
		unsigned short r = i * (65535/xlength);
		data.append((char)(((r) >> 8) & 0xFF));
		data.append((r) & 0xFF);
	}
	for (int i=data.size();i<58;i++)
	{
		data.append((char)0x00);
	}
	for (int i=0;i<ylength;i++)
	{
		unsigned short r = i * (65535/ylength);
		data.append((char)(((r) >> 8) & 0xFF));
		data.append((r) & 0xFF);
	}
	for (int i=data.size();i<100;i++)
	{
		data.append((char)0x00);
	}
	for (int i=0;i<xlength;i++)
	{
		for (int j=0;j<ylength;j++)
		{
			unsigned short r = j*(65535/ylength);
			data.append((char)(((r) >> 8) & 0xFF));
			data.append((r) & 0xFF);
		}
	}
	TableView3D *view  = new TableView3D();
	view->passData(0xABCD,data,0);
	connect(view,SIGNAL(destroyed(QObject*)),this,SLOT(rawDataViewDestroyed(QObject*)));
	//connect(view,SIGNAL(saveData(unsigned short,QByteArray,int)),this,SLOT(rawViewSaveData(unsigned short,QByteArray,int)));
	connect(view,SIGNAL(saveSingleData(unsigned short,QByteArray,unsigned short,unsigned short)),this,SLOT(saveSingleData(unsigned short,QByteArray,unsigned short,unsigned short)));
	QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
	win->setWindowTitle("Ram Location 0x" + QString::number(0xABCD,16).toUpper());
	win->setGeometry(view->geometry());
	m_rawDataView[0xABCD] = view;
	win->show();
	win->raise();*/
	//
	/*//TEST 2d TABLE!!!
	QByteArray data;
	for (int i=0;i<16;i++)
	{

		data.append((char)(((i * 1000) >> 8) & 0xFF));
		data.append((i * 1000) & 0xFF);
	}

	for (int i=0;i<16;i++)
	{
		unsigned short random = rand();
		//unsigned short random = ((i) * (65535/15.0));
		data.append((char)((random >> 8) & 0xFF));
		data.append((char)(random & 0xFF));
	}

	TableView2D *view = new TableView2D();
	view->passData(0xABCD,data,0);
	connect(view,SIGNAL(destroyed(QObject*)),this,SLOT(rawDataViewDestroyed(QObject*)));
	connect(view,SIGNAL(saveData(unsigned short,QByteArray,int)),this,SLOT(rawViewSaveData(unsigned short,QByteArray,int)));
	QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
	win->setWindowTitle("Ram Location 0x" + QString::number(0xABCD,16).toUpper());
	win->setGeometry(view->geometry());
	m_rawDataView[0xABCD] = view;
	win->show();
	win->raise();*/
}


void MainWindow::emsCommsDisconnected()
{
	ui.actionConnect->setEnabled(true);
	ui.actionDisconnect->setEnabled(false);
	if (progressView)
	{
		progressView->hide();
		progressView->deleteLater();
		progressView=0;
	}
}

void MainWindow::setDevice(QString dev)
{
	m_comPort = dev;
	emsComms->setPort(dev);
}

void MainWindow::connectToEms()
{
	emsComms->start();
	menu_connectClicked();
}
void MainWindow::tableview3d_reloadTableData(unsigned short locationid)
{
	if (hasLocalFlashBlock(locationid))
	{
		TableView3D *table = qobject_cast<TableView3D*>(sender());
		if (table)
		{
			for (int j=0;j<m_table3DMetaData.size();j++)
			{
				if (m_table3DMetaData[j].locationId == locationid)
				{
					table->passData(locationid,getLocalFlashBlock(locationid),0,m_table3DMetaData[j]);
					emsComms->updateBlockInRam(locationid,0,getLocalFlashBlock(locationid).size(),getLocalFlashBlock(locationid));
					setLocalRamBlock(locationid,getLocalFlashBlock(locationid));
					return;
				}
			}
			table->passData(locationid,getLocalFlashBlock(locationid),0);
			setLocalRamBlock(locationid,getLocalFlashBlock(locationid));
			emsComms->updateBlockInRam(locationid,0,getLocalFlashBlock(locationid).size(),getLocalFlashBlock(locationid));
		}
	}
}

void MainWindow::tableview2d_reloadTableData(unsigned short locationid)
{
	if (hasLocalFlashBlock(locationid))
	{
		TableView2D *table = qobject_cast<TableView2D*>(sender());
		if (table)
		{
			for (int j=0;j<m_table2DMetaData.size();j++)
			{
				if (m_table2DMetaData[j].locationId == locationid)
				{
					table->passData(locationid,getLocalFlashBlock(locationid),0,m_table2DMetaData[j]);
					emsComms->updateBlockInRam(locationid,0,getLocalFlashBlock(locationid).size(),getLocalFlashBlock(locationid));
					setLocalRamBlock(locationid,getLocalFlashBlock(locationid));
					return;
				}
			}
			table->passData(locationid,getLocalFlashBlock(locationid),0);
			emsComms->updateBlockInRam(locationid,0,getLocalFlashBlock(locationid).size(),getLocalFlashBlock(locationid));
			setLocalRamBlock(locationid,getLocalFlashBlock(locationid));
		}
	}
}

void MainWindow::dataViewSaveLocation(unsigned short locationid,QByteArray data,int physicallocation)
{
	if (physicallocation == 0)
	{
		//RAM
		emsComms->updateBlockInRam(locationid,0,data.size(),data);
		for (int i=0;i<m_ramMemoryList.size();i++)
		{
			if (m_ramMemoryList[i]->locationid == locationid)
			{
				m_ramMemoryList[i]->setData(data);
			}
		}
	}
	else if (physicallocation == 1)
	{
		//FLASH
		emsComms->updateBlockInFlash(locationid,0,data.size(),data);
		for (int i=0;i<m_flashMemoryList.size();i++)
		{
			if (m_flashMemoryList[i]->locationid == locationid)
			{
				m_flashMemoryList[i]->setData(data);
			}
		}
	}
}
void MainWindow::menu_aboutClicked()
{
	if (aboutMdiWindow->isVisible())
	{
		aboutMdiWindow->hide();
	}
	else
	{
		aboutMdiWindow->show();
	}
}
void MainWindow::menu_windows_PacketStatusClicked()
{
	if (packetStatusMdiWindow->isVisible())
	{
		packetStatusMdiWindow->hide();
	}
	else
	{
		packetStatusMdiWindow->show();
	}
}
void MainWindow::updateView(unsigned short locid,QWidget *view,QByteArray data,int type)
{
	if (type == 1)
	{
		bool found = false;
		for (int j=0;j<m_table2DMetaData.size();j++)
		{
			if (m_table2DMetaData[j].locationId == locid)
			{
				found = true;
				qobject_cast<TableView2D*>(view)->passData(locid,data,0,m_table2DMetaData[j]);
			}
		}
		if (!found)
		{
			qobject_cast<TableView2D*>(view)->passData(locid,data,0);
		}
	}
	else if (type == 3)
	{
		bool found = false;
		for (int j=0;j<m_table3DMetaData.size();j++)
		{
			if (m_table3DMetaData[j].locationId == locid)
			{
				found = true;
				qobject_cast<TableView3D*>(view)->passData(locid,data,0,m_table3DMetaData[j]);
			}
		}
		if (!found)
		{
			qobject_cast<TableView3D*>(view)->passData(locid,data,0);
		}

	}
	else
	{
		qobject_cast<RawDataView*>(view)->setData(locid,data,true);
	}
	m_rawDataView[locid]->show();
	//m_rawDataView[locid]->activateWindow();
	//m_rawDataView[locid]->mdiArea()->setActiveSubWindow(m_rawDataView[locid]);
	m_rawDataView[locid]->raise();
	QApplication::postEvent(m_rawDataView[locid], new QEvent(QEvent::Show));
	QApplication::postEvent(m_rawDataView[locid], new QEvent(QEvent::WindowActivate));

}
void MainWindow::createView(unsigned short locid,QByteArray data,int type)
{
	if (type == 1)
	{
		qDebug() << "Creating new table view for location: 0x" << QString::number(locid,16).toUpper();
		TableView2D *view = new TableView2D();
		QString title;
		bool found = false;
		for (int j=0;j<m_table2DMetaData.size();j++)
		{
			if (m_table2DMetaData[j].locationId == locid)
			{
				found = true;
				if (!view->passData(locid,data,0,m_table2DMetaData[j]))
				{
					view->deleteLater();
					QMessageBox::information(0,"Error","Table view contains invalid data! Please check your firmware");
					return;
				}
				title = m_table2DMetaData[j].tableTitle;
			}
		}
		if (!found)
		{
			if (!view->passData(locid,data,0))
			{
				QMessageBox::information(0,"Error","Table view contains invalid data! Please check your firmware");
				view->deleteLater();
				return;
			}
		}
		connect(view,SIGNAL(destroyed(QObject*)),this,SLOT(rawDataViewDestroyed(QObject*)));
		//connect(view,SIGNAL(saveData(unsigned short,QByteArray,int)),this,SLOT(rawViewSaveData(unsigned short,QByteArray,int)));
		connect(view,SIGNAL(saveSingleData(unsigned short,QByteArray,unsigned short,unsigned short)),this,SLOT(saveSingleData(unsigned short,QByteArray,unsigned short,unsigned short)));
		connect(view,SIGNAL(saveToFlash(unsigned short)),this,SLOT(saveFlashLocationId(unsigned short)));
		connect(view,SIGNAL(reloadTableData(unsigned short)),this,SLOT(tableview2d_reloadTableData(unsigned short)));
		QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
		win->setWindowTitle("Ram Location 0x" + QString::number(locid,16).toUpper() + " " + title);
		win->setGeometry(view->geometry());
		m_rawDataView[locid] = view;
		win->show();
		QApplication::postEvent(win, new QEvent(QEvent::Show));
		QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
	}
	else if (type == 3)
	{
		TableView3D *view = new TableView3D();
		QString title;
		bool found = false;
		for (int j=0;j<m_table3DMetaData.size();j++)
		{
			if (m_table3DMetaData[j].locationId == locid)
			{
				found = true;
				if (!view->passData(locid,data,0,m_table3DMetaData[j]))
				{
					QMessageBox::information(0,"Error","Table view contains invalid data! Please check your firmware");
					view->deleteLater();
					return;
				}
				title = m_table3DMetaData[j].tableTitle;
			}
		}
		if (!found)
		{
			if (!view->passData(locid,data,0))
			{
				QMessageBox::information(0,"Error","Table view contains invalid data! Please check your firmware");
				view->deleteLater();
				return;
			}
		}
		connect(view,SIGNAL(destroyed(QObject*)),this,SLOT(rawDataViewDestroyed(QObject*)));
		//connect(view,SIGNAL(saveData(unsigned short,QByteArray,int)),this,SLOT(rawViewSaveData(unsigned short,QByteArray,int)));
		connect(view,SIGNAL(saveSingleData(unsigned short,QByteArray,unsigned short,unsigned short)),this,SLOT(saveSingleData(unsigned short,QByteArray,unsigned short,unsigned short)));
		connect(view,SIGNAL(saveToFlash(unsigned short)),this,SLOT(saveFlashLocationId(unsigned short)));
		connect(view,SIGNAL(reloadTableData(unsigned short)),this,SLOT(tableview3d_reloadTableData(unsigned short)));
		QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
		win->setWindowTitle("Ram Location 0x" + QString::number(locid,16).toUpper() + " " + title);
		win->setGeometry(view->geometry());
		m_rawDataView[locid] = view;
		win->show();
		QApplication::postEvent(win, new QEvent(QEvent::Show));
		QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
	}
	else
	{
		if (m_readOnlyMetaDataMap.contains(locid))
		{
			int length=0;
			for (int j=0;j<m_readOnlyMetaDataMap[locid].m_ramData.size();j++)
			{
				length += m_readOnlyMetaDataMap[locid].m_ramData[j].size;
			}
			if (data.size() != length)
			{
				//Wrong size!
				qDebug() << "Invalid meta data size for location id:" << "0x" + QString::number(locid,16).toUpper();
				qDebug() << "Expected:" << length << "Got:" << data.size();
				QMessageBox::information(this,"Error",QString("Meta data indicates this location ID should be ") + QString::number(length) + " however it is " + QString::number(data.size()) + ". Unable to load memory location. Please fix your config.json file");
				return;
			}
			//m_readOnlyMetaDataMap[locid]
			ReadOnlyRamView *view = new ReadOnlyRamView();
			view->passData(locid,data,m_readOnlyMetaDataMap[locid].m_ramData);
			connect(view,SIGNAL(readRamLocation(unsigned short)),this,SLOT(reloadLocationId(unsigned short)));
			connect(view,SIGNAL(destroyed(QObject*)),this,SLOT(rawDataViewDestroyed(QObject*)));
			QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
			win->setWindowTitle("Ram Location: 0x" + QString::number(locid,16).toUpper());
			win->setGeometry(view->geometry());
			m_rawDataView[locid] = view;
			win->show();
			QApplication::postEvent(win, new QEvent(QEvent::Show));
			QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
		}
		else
		{
			RawDataView *view = new RawDataView();
			view->setData(locid,data,true);
			connect(view,SIGNAL(saveData(unsigned short,QByteArray,int)),this,SLOT(rawViewSaveData(unsigned short,QByteArray,int)));
			connect(view,SIGNAL(destroyed(QObject*)),this,SLOT(rawDataViewDestroyed(QObject*)));
			QMdiSubWindow *win = ui.mdiArea->addSubWindow(view);
			win->setWindowTitle("Ram Location: 0x" + QString::number(locid,16).toUpper());
			win->setGeometry(view->geometry());
			m_rawDataView[locid] = view;
			win->show();
			QApplication::postEvent(win, new QEvent(QEvent::Show));
			QApplication::postEvent(win, new QEvent(QEvent::WindowActivate));
		}
	}
}

void MainWindow::emsInfoDisplayLocationId(int locid,bool isram,int type)
{
	Q_UNUSED(type)
	Q_UNUSED(isram)
	if (hasLocalRamBlock(locid))
	{
		if (m_rawDataView.contains(locid))
		{
			updateView(locid,m_rawDataView[locid],getLocalRamBlock(locid),type);
		}
		else
		{
			createView(locid,getLocalRamBlock(locid),type);
		}
	}
	else if (hasLocalFlashBlock(locid))
	{
		if (m_rawDataView.contains(locid))
		{
			updateView(locid,m_rawDataView[locid],getLocalFlashBlock(locid),type);
		}
		else
		{
			createView(locid,getLocalFlashBlock(locid),type);
		}
	}
}
void MainWindow::rawViewSaveData(unsigned short locationid,QByteArray data,int physicallocation)
{
	Q_UNUSED(physicallocation)
	markRamDirty();
	bool found = false;
	if (physicallocation==0)
	{
		for (int i=0;i<m_ramMemoryList.size();i++)
		{
			if (m_ramMemoryList[i]->locationid == locationid)
			{
				if (m_ramMemoryList[i]->data() == data)
				{
					qDebug() << "Data in application memory unchanged, no reason to send write";
					return;
				}
				m_ramMemoryList[i]->setData(data);
				found = true;
			}
		}
		if (!found)
		{
			qDebug() << "Attempted to save data for location id:" << "0x" + QString::number(locationid,16) << "but no valid location found in Ram list. Ram list size:" << m_ramMemoryList.size();
		}
		qDebug() << "Requesting to update ram location:" << "0x" + QString::number(locationid,16).toUpper() << "data size:" << data.size();
		m_currentRamLocationId = locationid;
		m_waitingForRamWriteConfirmation=true;
		emsComms->updateBlockInRam(locationid,0,data.size(),data);
	}
	else if (physicallocation == 1)
	{
		for (int i=0;i<m_flashMemoryList.size();i++)
		{
			if (m_flashMemoryList[i]->locationid == locationid)
			{
				if (m_flashMemoryList[i]->data() == data)
				{
					qDebug() << "Data in application memory unchanged, no reason to send write";
					return;
				}
				m_flashMemoryList[i]->setData(data);
				found = true;
			}
		}
		if (!found)
		{
			qDebug() << "Attempted to save data for location id:" << "0x" + QString::number(locationid,16) << "but no valid location found in Flash list. Flash list size:" << m_flashMemoryList.size();
		}
		qDebug() << "Requesting to update flash location:" << "0x" + QString::number(locationid,16).toUpper() << "data size:" << data.size();
		m_currentFlashLocationId = locationid;
		m_waitingForFlashWriteConfirmation=true;
		emsComms->updateBlockInFlash(locationid,0,data.size(),data);
	}


}

void MainWindow::rawDataViewDestroyed(QObject *object)
{
	QMap<unsigned short,QWidget*>::const_iterator i = m_rawDataView.constBegin();
	while( i != m_rawDataView.constEnd())
	{
		if (i.value() == object)
		{
			//This is the one that needs to be removed.
			m_rawDataView.remove(i.key());
			QMdiSubWindow *win = qobject_cast<QMdiSubWindow*>(object->parent());
			if (!win)
			{
				//qDebug() << "Raw Data View without a QMdiSubWindow parent!!";
				return;
			}
			win->hide();
			ui.mdiArea->removeSubWindow(win);
			return;
		}
		i++;
	}
}
void MainWindow::markRamDirty()
{
	m_localRamDirty = true;
	emsInfo->setLocalRam(true);
}
void MainWindow::markDeviceFlashDirty()
{
	m_deviceFlashDirty = true;
	emsInfo->setDeviceFlash(true);
}
void MainWindow::markRamClean()
{
	m_localRamDirty = false;
	emsInfo->setLocalRam(false);
}
void MainWindow::markDeviceFlashClean()
{
	m_deviceFlashDirty = false;
	emsInfo->setDeviceFlash(false);
}
QByteArray MainWindow::getLocalRamBlock(unsigned short id)
{
	for (int i=0;i<m_ramMemoryList.size();i++)
	{
		if (m_ramMemoryList[i]->locationid == id)
		{
			return m_ramMemoryList[i]->data();
		}
	}
	return QByteArray();
}

QByteArray MainWindow::getLocalFlashBlock(unsigned short id)
{
	for (int i=0;i<m_flashMemoryList.size();i++)
	{
		if (m_flashMemoryList[i]->locationid == id)
		{
			return m_flashMemoryList[i]->data();
		}
	}
	return QByteArray();
}
QByteArray MainWindow::getDeviceRamBlock(unsigned short id)
{
	for (int i=0;i<m_deviceRamMemoryList.size();i++)
	{
		if (m_deviceRamMemoryList[i]->locationid == id)
		{
			return m_deviceRamMemoryList[i]->data();
		}
	}
	return QByteArray();
}

QByteArray MainWindow::getDeviceFlashBlock(unsigned short id)
{
	for (int i=0;i<m_deviceFlashMemoryList.size();i++)
	{
		if (m_deviceFlashMemoryList[i]->locationid == id)
		{
			return m_deviceFlashMemoryList[i]->data();
		}
	}
	return QByteArray();
}

bool MainWindow::hasDeviceRamBlock(unsigned short id)
{
	for (int i=0;i<m_deviceRamMemoryList.size();i++)
	{
		if (m_deviceRamMemoryList[i]->locationid == id)
		{
			return true;
		}
	}
	return false;
}
bool MainWindow::hasLocalRamBlock(unsigned short id)
{
	for (int i=0;i<m_ramMemoryList.size();i++)
	{
		if (m_ramMemoryList[i]->locationid == id)
		{
			return true;
		}
	}
	return false;
}
void MainWindow::setLocalRamBlock(unsigned short id,QByteArray data)
{
	for (int i=0;i<m_ramMemoryList.size();i++)
	{
		if (m_ramMemoryList[i]->locationid == id)
		{
			m_ramMemoryList[i]->setData(data);
			return;
		}
	}
}

void MainWindow::setDeviceRamBlock(unsigned short id,QByteArray data)
{
	for (int i=0;i<m_deviceRamMemoryList.size();i++)
	{
		if (m_deviceRamMemoryList[i]->locationid == id)
		{
			m_deviceRamMemoryList[i]->setData(data);
			return;
		}
	}
}

void MainWindow::setLocalFlashBlock(unsigned short id,QByteArray data)
{
	for (int i=0;i<m_flashMemoryList.size();i++)
	{
		if (m_flashMemoryList[i]->locationid == id)
		{
			m_flashMemoryList[i]->setData(data);
			return;
		}
	}
}

bool MainWindow::hasLocalFlashBlock(unsigned short id)
{
	for (int i=0;i<m_flashMemoryList.size();i++)
	{
		if (m_flashMemoryList[i]->locationid == id)
		{
			return true;
		}
	}
	return false;
}
bool MainWindow::hasDeviceFlashBlock(unsigned short id)
{
	for (int i=0;i<m_deviceFlashMemoryList.size();i++)
	{
		if (m_deviceFlashMemoryList[i]->locationid == id)
		{
			return true;
		}
	}
	return false;
}

void MainWindow::setDeviceFlashBlock(unsigned short id,QByteArray data)
{
	for (int i=0;i<m_deviceFlashMemoryList.size();i++)
	{
		if (m_deviceFlashMemoryList[i]->locationid == id)
		{
			m_deviceFlashMemoryList[i]->setData(data);
			return;
		}
	}
}
void MainWindow::ramBlockRetrieved(unsigned short locationid,QByteArray header,QByteArray payload)
{
	Q_UNUSED(header)
	if (!hasDeviceRamBlock(locationid))
	{
		//This should not happen
		/*RawDataBlock *block = new RawDataBlock();
		block->locationid = locationid;
		block->header = header;
		block->data = payload;
		//m_flashRawBlockList.append(block);
		m_deviceRamRawBlockList.append(block);*/
	}
	else
	{
		//Check to see if it's supposed to be a table, and if so, check size
		for (int i=0;i<m_table3DMetaData.size();i++)
		{
			if (m_table3DMetaData[i].locationId == locationid)
			{
				//It's a 3D table. Should be 1024
				if (payload.size() != TABLE_3D_PAYLOAD_SIZE)
				{
					//Stop interrogation and kill everything.
					interrogateProgressViewCancelClicked();
					QMessageBox::information(this,"Error","RAM Location ID 0x" + QString::number(locationid,16).toUpper() + " should be 1024 sized, but it is " + QString::number(payload.size()) + ". This should never happen");
					return;
				}
			}
		}
		for (int i=0;i<m_table2DMetaData.size();i++)
		{
			if (m_table2DMetaData[i].locationId == locationid)
			{
				//It's a 3D table. Should be 1024
				if (payload.size() != TABLE_2D_PAYLOAD_SIZE)
				{
					//Stop interrogation and kill everything.
					interrogateProgressViewCancelClicked();
					QMessageBox::information(this,"Error","RAM Location ID 0x" + QString::number(locationid,16).toUpper() + " should be 1024 sized, but it is " + QString::number(payload.size()) + ". This should never happen");
					return;
				}
			}
		}
		for (int i=0;i<m_deviceRamMemoryList.size();i++)
		{
			if (m_deviceRamMemoryList[i]->locationid == locationid)
			{
				if (m_deviceRamMemoryList[i]->isEmpty)
				{
					//Initial retrieval.
					m_deviceRamMemoryList[i]->setData(payload);
				}
				else
				{
					if (getDeviceRamBlock(locationid) != payload)
					{
						qDebug() << "Ram block on device does not match ram block on tuner! This should ONLY happen during a manual update!";
						qDebug() << "Tuner ram size:" << m_deviceRamMemoryList[i]->data().size();
						m_deviceRamMemoryList[i]->setData(payload);
						for (int k=0;k<m_ramMemoryList.size();k++)
						{
							if (m_ramMemoryList[k]->locationid == locationid)
							{
								m_ramMemoryList[k]->setData(payload);
							}
						}
					}
				}

			}
		}
		updateDataWindows(locationid);
	}
	return;
}


void MainWindow::flashBlockRetrieved(unsigned short locationid,QByteArray header,QByteArray payload)
{
	Q_UNUSED(header)
	for (int i=0;i<m_table3DMetaData.size();i++)
	{
		if (m_table3DMetaData[i].locationId == locationid)
		{
			//It's a 3D table. Should be 1024
			if (payload.size() != TABLE_3D_PAYLOAD_SIZE)
			{
				//Stop interrogation and kill everything.
				interrogateProgressViewCancelClicked();
				QMessageBox::information(this,"Error","RAM Location ID 0x" + QString::number(locationid,16).toUpper() + " should be 1024 sized, but it is " + QString::number(payload.size()) + ". This should never happen");
				return;
			}
		}
	}
	for (int i=0;i<m_table2DMetaData.size();i++)
	{
		if (m_table2DMetaData[i].locationId == locationid)
		{
			//It's a 3D table. Should be 1024
			if (payload.size() != TABLE_2D_PAYLOAD_SIZE)
			{
				//Stop interrogation and kill everything.
				interrogateProgressViewCancelClicked();
				QMessageBox::information(this,"Error","RAM Location ID 0x" + QString::number(locationid,16).toUpper() + " should be 1024 sized, but it is " + QString::number(payload.size()) + ". This should never happen");
				return;
			}
		}
	}
	for (int i=0;i<m_deviceFlashMemoryList.size();i++)
	{
		if (m_deviceFlashMemoryList[i]->locationid == locationid)
		{
			if (m_deviceFlashMemoryList[i]->isEmpty)
			{
				m_deviceFlashMemoryList[i]->setData(payload);
			}
			else
			{
				if (getDeviceFlashBlock(locationid) != payload)
				{
					qDebug() << "Flash block in memory does not match flash block on tuner! This should not happen!";
					qDebug() << "Flash size:" << m_deviceFlashMemoryList[i]->data().size();
					m_deviceFlashMemoryList[i]->setData(payload);
				}
			}
		}
	}
	return;
}

void MainWindow::ui_saveDataButtonClicked()
{

}

void MainWindow::menu_settingsClicked()
{
	ComSettings *settings = new ComSettings();
	settings->setComPort(m_comPort);
	settings->setBaud(m_comBaud);
	settings->setSaveDataLogs(m_saveLogs);
	settings->setClearDataLogs(m_clearLogs);
	settings->setNumLogsToSave(m_logsToKeep);
	settings->setDataLogDir(m_logDirectory);
	//m_saveLogs = settings.value("savelogs",true).toBool();
	//m_clearLogs = settings.value("clearlogs",false).toBool();
	//m_logsToKeep = settings.value("logstokeep",0).toInt();
	//m_logDirectory = settings.value("logdir",".").toString();
	connect(settings,SIGNAL(saveClicked()),this,SLOT(settingsSaveClicked()));
	connect(settings,SIGNAL(cancelClicked()),this,SLOT(settingsCancelClicked()));
	QMdiSubWindow *win = ui.mdiArea->addSubWindow(settings);
	win->setGeometry(settings->geometry());
	win->show();
	settings->show();
}

void MainWindow::menu_connectClicked()
{
	ui.actionConnect->setEnabled(false);
	m_interrogationInProgress = true;
	emsComms->connectSerial(m_comPort,m_comBaud);
}

void MainWindow::menu_disconnectClicked()
{
	emsComms->disconnectSerial();
	ui.actionConnect->setEnabled(true);
	ui.actionDisconnect->setEnabled(false);
}

void MainWindow::timerTick()
{
	ui.ppsLabel->setText("PPS: " + QString::number(pidcount));
	pidcount = 0;
}
void MainWindow::settingsSaveClicked()
{
	ComSettings *comSettingsWidget = qobject_cast<ComSettings*>(sender());
	m_comBaud = comSettingsWidget->getBaud();
	m_comPort = comSettingsWidget->getComPort();
	m_comInterByte = comSettingsWidget->getInterByteDelay();
	m_saveLogs = comSettingsWidget->getSaveDataLogs();
	m_clearLogs = comSettingsWidget->getClearDataLogs();
	m_logsToKeep = comSettingsWidget->getNumLogsToSave();
	m_logDirectory = comSettingsWidget->getDataLogDir();
	/*if (!subwin)
	{
		subwin->deleteLater();
	}*/
	comSettingsWidget->hide();
	QSettings settings("settings.ini",QSettings::IniFormat);
	settings.beginGroup("comms");
	settings.setValue("port",m_comPort);
	settings.setValue("baud",m_comBaud);
	settings.setValue("interbytedelay",m_comInterByte);
	settings.setValue("savelogs",m_saveLogs);
	settings.setValue("clearlogs",m_clearLogs);
	settings.setValue("logstokeep",m_logsToKeep);
	settings.setValue("logdir",m_logDirectory);
	/*m_saveLogs = settings.value("savelogs",true).toBool();
	m_clearLogs = settings.value("clearlogs",false).toBool();
	m_logsToKeep = settings.value("logstokeep",0).toInt();
	m_logDirectory = settings.value("logdir",".").toString();*/
	settings.endGroup();
	QMdiSubWindow *subwin = qobject_cast<QMdiSubWindow*>(comSettingsWidget->parent());
	ui.mdiArea->removeSubWindow(subwin);
	comSettingsWidget->deleteLater();

}
void MainWindow::locationIdInfo(unsigned short locationid,unsigned short rawFlags,QList<FreeEmsComms::LocationIdFlags> flags,unsigned short parent, unsigned char rampage,unsigned char flashpage,unsigned short ramaddress,unsigned short flashaddress,unsigned short size)
{
	Q_UNUSED(size)
	Q_UNUSED(rawFlags)
	Q_UNUSED(parent)
	Q_UNUSED(rampage)
	Q_UNUSED(flashpage)
	Q_UNUSED(ramaddress)
	Q_UNUSED(flashaddress)
	QString title;
	for (int i=0;i<m_table3DMetaData.size();i++)
	{
		if (locationid == m_table3DMetaData[i].locationId)
		{
			title = m_table3DMetaData[i].tableTitle;
			if (m_table3DMetaData[i].size != size)
			{
				//Error here, since size is not equal to table meta data.
				interrogateProgressViewCancelClicked();
				QMessageBox::information(0,"Interrogate Error","Error: Meta data for table location 0x" + QString::number(locationid,16).toUpper() + " is not valid for actual table. Size: " + QString::number(size) + " expected: " + QString::number(m_table3DMetaData[i].size));
			}
		}
	}
	for (int i=0;i<m_table2DMetaData.size();i++)
	{
		if (locationid == m_table2DMetaData[i].locationId)
		{
			title = m_table2DMetaData[i].tableTitle;
			if (m_table2DMetaData[i].size != size)
			{
				//Error here, since size is not equal to table meta data.
				interrogateProgressViewCancelClicked();
				QMessageBox::information(0,"Interrogate Error","Error: Meta data for table location 0x" + QString::number(locationid,16).toUpper() + " is not valid for actual table. Size: " + QString::number(size) + " expected: " + QString::number(m_table2DMetaData[i].size));
			}
		}
	}
	if (m_readOnlyMetaDataMap.contains(locationid))
	{
		title = m_readOnlyMetaDataMap[locationid].title;
		//m_readOnlyMetaDataMap[locationid]
	}
	qDebug() << "Found location ID info";
	emsInfo->locationIdInfo(locationid,title,rawFlags,flags,parent,rampage,flashpage,ramaddress,flashaddress,size);
	if (flags.contains(FreeEmsComms::BLOCK_IS_RAM) && flags.contains((FreeEmsComms::BLOCK_IS_FLASH)))
	{
		MemoryLocation *loc = new MemoryLocation();
		loc->locationid = locationid;
		loc->size = size;
		if (flags.contains(FreeEmsComms::BLOCK_HAS_PARENT))
		{
			loc->parent = parent;
			loc->hasParent = true;
		}
		loc->isRam = true;
		loc->isFlash = true;
		loc->ramAddress = ramaddress;
		loc->ramPage = rampage;
		loc->flashAddress = flashaddress;
		loc->flashPage = flashpage;
		m_deviceRamMemoryList.append(loc);
		//m_flashMemoryList.append(new MemoryLocation(*loc));
		m_deviceFlashMemoryList.append(new MemoryLocation(*loc));

	}
	else if (flags.contains(FreeEmsComms::BLOCK_IS_FLASH))
	{
		MemoryLocation *loc = new MemoryLocation();
		loc->locationid = locationid;
		loc->size = size;
		if (flags.contains(FreeEmsComms::BLOCK_HAS_PARENT))
		{
			loc->parent = parent;
			loc->hasParent = true;
		}
		loc->isFlash = true;
		loc->isRam = false;
		loc->flashAddress = flashaddress;
		loc->flashPage = flashpage;
		m_deviceFlashMemoryList.append(loc);
	}
	else if (flags.contains(FreeEmsComms::BLOCK_IS_RAM))
	{
		MemoryLocation *loc = new MemoryLocation();
		loc->locationid = locationid;
		loc->size = size;
		if (flags.contains(FreeEmsComms::BLOCK_HAS_PARENT))
		{
			loc->parent = parent;
			loc->hasParent = true;
		}
		loc->isRam = true;
		loc->isFlash = false;
		loc->ramAddress = ramaddress;
		loc->ramPage = rampage;
		m_deviceRamMemoryList.append(loc);
	}
}

void MainWindow::settingsCancelClicked()
{
	//comSettings->hide();
	ComSettings *comSettingsWidget = qobject_cast<ComSettings*>(sender());
	comSettingsWidget->hide();
	QMdiSubWindow *subwin = qobject_cast<QMdiSubWindow*>(comSettingsWidget->parent());
	ui.mdiArea->removeSubWindow(subwin);
	comSettingsWidget->deleteLater();
}
void MainWindow::menu_windows_GaugesClicked()
{
	if (gaugesMdiWindow->isVisible())
	{
		gaugesMdiWindow->hide();
	}
	else
	{
		gaugesMdiWindow->show();
	}
}

void MainWindow::menu_windows_EmsInfoClicked()
{
	if (emsMdiWindow->isVisible())
	{
		emsMdiWindow->hide();
	}
	else
	{
		emsMdiWindow->show();
	}
}

void MainWindow::menu_windows_TablesClicked()
{
	if (tablesMdiWindow->isVisible())
	{
		tablesMdiWindow->hide();
	}
	else
	{
		tablesMdiWindow->show();
	}
}
void MainWindow::menu_windows_FlagsClicked()
{
	if (flagsMdiWindow->isVisible())
	{
		flagsMdiWindow->hide();
	}
	else
	{
		flagsMdiWindow->show();
	}
}

void MainWindow::unknownPacket(QByteArray header,QByteArray payload)
{
	QString result = "";
	for (int i=0;i<header.size();i++)
	{
		result += (((unsigned char)header[i] < (char)0xF) ? "0" : "") + QString::number((unsigned char)header[i],16).toUpper();
	}
	for (int i=0;i<payload.size();i++)
	{
		result += (((unsigned char)payload[i] < (char)0xF) ? "0" : "") + QString::number((unsigned char)payload[i],16).toUpper();
	}
}

void MainWindow::loadLogButtonClicked()
{
	QFileDialog file;
	if (file.exec())
	{
		if (file.selectedFiles().size() > 0)
		{
			QString filename = file.selectedFiles()[0];
			ui.statusLabel->setText("Status: File loaded and not playing");
			//logLoader->loadFile(filename);
			emsComms->loadLog(filename);

		}
	}
}
void MainWindow::interByteDelayChanged(int num)
{
	emsComms->setInterByteSendDelay(num);
}

void MainWindow::logFinished()
{
	ui.statusLabel->setText("Status: File loaded and log finished");
}

void MainWindow::playLogButtonClicked()
{
	//logLoader->start();
	emsComms->playLog();
	ui.statusLabel->setText("Status: File loaded and playing");
}
void MainWindow::locationIdList(QList<unsigned short> idlist)
{
	for (int i=0;i<idlist.size();i++)
	{
		//ui/listWidget->addItem(QString::number(idlist[i]));
		MemoryLocation *loc = new MemoryLocation();
		loc->locationid = idlist[i];
		m_tempMemoryList.append(loc);
		int seq = emsComms->getLocationIdInfo(idlist[i]);
		progressView->setMaximum(progressView->maximum()+1);
		m_locIdMsgList.append(seq);
		interrogationSequenceList.append(seq);
	}
}
void MainWindow::blockRetrieved(int sequencenumber,QByteArray header,QByteArray payload)
{
	Q_UNUSED(sequencenumber)
	Q_UNUSED(header)
	Q_UNUSED(payload)
}
void MainWindow::dataLogPayloadReceived(QByteArray header,QByteArray payload)
{
	Q_UNUSED(header)
	Q_UNUSED(payload)
}
void MainWindow::interfaceVersion(QString version)
{
	//ui.interfaceVersionLineEdit->setText(version);
	m_interfaceVersion = version;
	if (emsInfo)
	{
		emsInfo->setInterfaceVersion(version);

	}
	emsinfo.interfaceVersion = version;
}
void MainWindow::firmwareVersion(QString version)
{
	//ui.firmwareVersionLineEdit->setText(version);
	m_firmwareVersion = version;
	this->setWindowTitle(QString("EMStudio ") + QString(define2string(GIT_COMMIT)) + " Firmware: " + version);
	if (emsInfo)
	{
		emsInfo->setFirmwareVersion(version);
	}
	emsinfo.firmwareVersion = version;
}
void MainWindow::error(QString msg)
{
	Q_UNUSED(msg)
}
void MainWindow::interrogateProgressViewCancelClicked()
{
	emsComms->terminate();
	emsComms->wait(5000);
	emsComms->deleteLater();
	emsComms = 0;

	//Need to reset everything here.
	m_ramMemoryList.clear();
	m_flashMemoryList.clear();
	m_deviceFlashMemoryList.clear();
	m_deviceRamMemoryList.clear();
	m_tempMemoryList.clear();
	interrogationSequenceList.clear();
	m_locIdMsgList.clear();
	m_locIdInfoMsgList.clear();

	emsComms = new FreeEmsComms(this);
	//m_logFileName = QDateTime::currentDateTime().toString("yyyy.MM.dd.hh.mm.ss");
	emsComms->setLogFileName(m_logFileName);
	connect(emsComms,SIGNAL(connected()),this,SLOT(emsCommsConnected()));
	connect(emsComms,SIGNAL(disconnected()),this,SLOT(emsCommsDisconnected()));
	connect(emsComms,SIGNAL(dataLogPayloadReceived(QByteArray,QByteArray)),this,SLOT(logPayloadReceived(QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(firmwareVersion(QString)),this,SLOT(firmwareVersion(QString)));
	connect(emsComms,SIGNAL(decoderName(QString)),this,SLOT(emsDecoderName(QString)));
	connect(emsComms,SIGNAL(compilerVersion(QString)),this,SLOT(emsCompilerVersion(QString)));
	connect(emsComms,SIGNAL(interfaceVersion(QString)),this,SLOT(interfaceVersion(QString)));
	connect(emsComms,SIGNAL(operatingSystem(QString)),this,SLOT(emsOperatingSystem(QString)));
	connect(emsComms,SIGNAL(locationIdList(QList<unsigned short>)),this,SLOT(locationIdList(QList<unsigned short>)));
	connect(emsComms,SIGNAL(firmwareBuild(QString)),this,SLOT(emsFirmwareBuildDate(QString)));
	connect(emsComms,SIGNAL(unknownPacket(QByteArray,QByteArray)),this,SLOT(unknownPacket(QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(commandSuccessful(int)),this,SLOT(commandSuccessful(int)));
	connect(emsComms,SIGNAL(commandTimedOut(int)),this,SLOT(commandTimedOut(int)));
	connect(emsComms,SIGNAL(commandFailed(int,unsigned short)),this,SLOT(commandFailed(int,unsigned short)));
	connect(emsComms,SIGNAL(locationIdInfo(unsigned short,unsigned short,QList<FreeEmsComms::LocationIdFlags>,unsigned short,unsigned char,unsigned char,unsigned short,unsigned short,unsigned short)),this,SLOT(locationIdInfo(unsigned short,unsigned short,QList<FreeEmsComms::LocationIdFlags>,unsigned short,unsigned char,unsigned char,unsigned short,unsigned short,unsigned short)));
	connect(emsComms,SIGNAL(ramBlockRetrieved(unsigned short,QByteArray,QByteArray)),this,SLOT(ramBlockRetrieved(unsigned short,QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(flashBlockRetrieved(unsigned short,QByteArray,QByteArray)),this,SLOT(flashBlockRetrieved(unsigned short,QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(packetSent(unsigned short,QByteArray,QByteArray)),packetStatus,SLOT(passPacketSent(unsigned short,QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(packetAcked(unsigned short,QByteArray,QByteArray)),packetStatus,SLOT(passPacketAck(unsigned short,QByteArray,QByteArray)));
	connect(emsComms,SIGNAL(packetNaked(unsigned short,QByteArray,QByteArray,unsigned short)),packetStatus,SLOT(passPacketNak(unsigned short,QByteArray,QByteArray,unsigned short)));
	connect(emsComms,SIGNAL(decoderFailure(QByteArray)),packetStatus,SLOT(passDecoderFailure(QByteArray)));
	//connect(emsComms,SIGNAL(locationIdInfo(unsigned short,QString,unsigned short,QList<FreeEmsComms::LocationIdFlags>,unsigned short,unsigned char,unsigned char,unsigned short,unsigned short,unsigned short)),emsInfo,SLOT(locationIdInfo(unsigned short,QString,unsigned short,QList<FreeEmsComms::LocationIdFlags>,unsigned short,unsigned char,unsigned char,unsigned short,unsigned short,unsigned short)));
	emsComms->setBaud(m_comBaud);
	emsComms->setPort(m_comPort);
	emsComms->setLogDirectory(m_logDirectory);
	emsComms->setLogsEnabled(m_saveLogs);
	emsComms->start();
	progressView->hide();
	progressView->deleteLater();
	progressView=0;
	emsInfo->clear();
	this->setEnabled(true);
	ui.actionConnect->setEnabled(true);
}
void MainWindow::emsCompilerVersion(QString version)
{
	emsinfo.compilerVersion = version;
}

void MainWindow::emsFirmwareBuildDate(QString date)
{
	emsinfo.firmwareBuildDate = date;
}

void MainWindow::emsDecoderName(QString name)
{
	emsinfo.decoderName = name;
}

void MainWindow::emsOperatingSystem(QString os)
{
	emsinfo.operatingSystem = os;
}

void MainWindow::emsCommsConnected()
{
	//New log and settings file here.

	progressView = new InterrogateProgressView();
	connect(progressView,SIGNAL(cancelClicked()),this,SLOT(interrogateProgressViewCancelClicked()));
	progressView->setMaximum(0);
	progressView->show();
	this->setEnabled(false);
	interrogationSequenceList.append(emsComms->getFirmwareVersion());
	interrogationSequenceList.append(emsComms->getInterfaceVersion());
	interrogationSequenceList.append(emsComms->getLocationIdList(0x00,0x00));
	interrogationSequenceList.append(emsComms->getCompilerVersion());
	interrogationSequenceList.append(emsComms->getDecoderName());
	interrogationSequenceList.append(emsComms->getFirmwareBuildDate());
	interrogationSequenceList.append(emsComms->getMaxPacketSize());
	interrogationSequenceList.append(emsComms->getOperatingSystem());

	progressView->setMaximum(8);
	//progressView->setMax(progressView->max()+1);
}
void MainWindow::checkSyncRequest()
{
	emsComms->getLocationIdList(0,0);
}
void MainWindow::updateRamLocation(unsigned short locationid)
{
	bool hasparent = false;
	unsigned short tempRamParentId=0;
	bool isparent = false;
	QList<unsigned short> childlist;
	for (int j=0;j<m_ramMemoryList.size();j++)
	{
		if (m_ramMemoryList[j]->parent == locationid)
		{
			//qDebug() << "Child of:" << "0x" + QString::number(m_currentRamLocationId,16).toUpper() << "is" << "0x" + QString::number(m_ramMemoryList[j]->locationid,16).toUpper();
			childlist.append(m_ramMemoryList[j]->locationid);
			isparent = true;
		}
		if (m_ramMemoryList[j]->locationid == locationid)
		{
			if (m_ramMemoryList[j]->hasParent)
			{
				hasparent = true;
				tempRamParentId = m_ramMemoryList[j]->parent;
			}
			for (int i=0;i<m_deviceRamMemoryList.size();i++)
			{
				if (m_deviceRamMemoryList[i]->locationid == locationid)
				{
					m_deviceRamMemoryList[i]->setData(m_ramMemoryList[j]->data());
				}
			}
		}
	}
	//Find all windows that use that location id
	if (hasparent && isparent)
	{
		//This should never happen.
		qDebug() << "Found a memory location that is parent AND child!!! This should not happen.";
		qDebug() << "Parent:" << "0x" + QString::number(tempRamParentId);
		qDebug() << "Current:" << "0x" + QString::number(locationid);
		QString children;
		for (int i=0;i<childlist.size();i++)
		{
			children += "0x" + QString::number(childlist[i],16).toUpper() + " ";
		}
		qDebug() << "Children" << children;
	}
	else if (hasparent)
	{
		//qDebug() << "No children, is a child for:" << "0x" + QString::number(m_currentRamLocationId,16).toUpper();
		updateDataWindows(tempRamParentId);
	}
	else if (isparent)
	{
		for (int i=0;i<childlist.size();i++)
		{
			updateDataWindows(childlist[i]);
		}

	}
	//qDebug() << "No children for:" << "0x" + QString::number(m_currentRamLocationId,16).toUpper();
	updateDataWindows(locationid);
}
void MainWindow::commandTimedOut(int sequencenumber)
{
	qDebug() << "Command timed out:" << QString::number(sequencenumber);
	if (m_waitingForRamWriteConfirmation)
	{
		m_waitingForRamWriteConfirmation = false;
		m_currentRamLocationId=0;
		return;
	}
	if (m_waitingForFlashWriteConfirmation)
	{
		m_waitingForFlashWriteConfirmation = false;
		m_currentFlashLocationId=0;
		return;
	}
	if (m_interrogationInProgress)
	{
		//If interrogation is in progress, we need to stop, since something has gone
		//horribly wrong.
		interrogateProgressViewCancelClicked();
		QMessageBox::information(0,"Error","Something has gone serious wrong, one of the commands timed out during interrogation. This should be properly investigated before continuing");
	}

}
void MainWindow::commandSuccessful(int sequencenumber)
{
	qDebug() << "Command succesful:" << QString::number(sequencenumber);
	if (m_waitingForRamWriteConfirmation)
	{
		m_waitingForRamWriteConfirmation = false;
		updateRamLocation(m_currentRamLocationId);
		checkRamFlashSync();
		m_currentRamLocationId=0;
		return;
	}
	if (m_waitingForFlashWriteConfirmation)
	{
		m_waitingForFlashWriteConfirmation = false;
		m_currentFlashLocationId=0;
		return;
	}
	checkMessageCounters(sequencenumber);

}
void MainWindow::checkMessageCounters(int sequencenumber)
{
	if (m_locIdInfoMsgList.contains(sequencenumber))
	{
		m_locIdInfoMsgList.removeOne(sequencenumber);
		if (m_locIdInfoMsgList.size() == 0)
		{
			qDebug() << "All Ram and Flash locations updated";
			//End of the location ID information messages.
			checkRamFlashSync();
		}
	}
	if (m_locIdMsgList.contains(sequencenumber))
	{
		m_locIdMsgList.removeOne(sequencenumber);
		if (m_locIdMsgList.size() == 0)
		{
			qDebug() << "All ID information recieved. Requesting Ram and Flash updates";
			populateParentLists();
			for (int i=0;i<m_deviceFlashMemoryList.size();i++)
			{
				if (!m_deviceFlashMemoryList[i]->hasParent)
				{
					int seq = emsComms->retrieveBlockFromFlash(m_deviceFlashMemoryList[i]->locationid,0,0);
					m_locIdInfoMsgList.append(seq);
					progressView->setMaximum(progressView->maximum()+1);
					interrogationSequenceList.append(seq);
				}
			}
			for (int i=0;i<m_deviceRamMemoryList.size();i++)
			{
				if (!m_deviceRamMemoryList[i]->hasParent)
				{
					int seq = emsComms->retrieveBlockFromRam(m_deviceRamMemoryList[i]->locationid,0,0);
					m_locIdInfoMsgList.append(seq);
					progressView->setMaximum(progressView->maximum()+1);
					interrogationSequenceList.append(seq);
				}
			}
		}
	}
	if (interrogationSequenceList.contains(sequencenumber))
	{
		progressView->setProgress(progressView->progress()+1);
		interrogationSequenceList.removeOne(sequencenumber);
		if (interrogationSequenceList.size() == 0)
		{
			m_interrogationInProgress = false;
			progressView->hide();
			progressView->deleteLater();
			progressView=0;
			this->setEnabled(true);
			qDebug() << "Interrogation complete";
			emsInfo->show();
			//Write everything to the settings.
			QString json = "";
			json += "{";
			QJson::Serializer jsonSerializer;
			QVariantMap top;
			top["firmwareversion"] = emsinfo.firmwareVersion;
			top["interfaceversion"] = emsinfo.interfaceVersion;
			top["compilerversion"] = emsinfo.compilerVersion;
			top["firmwarebuilddate"] = emsinfo.firmwareBuildDate;
			top["decodername"] = emsinfo.decoderName;
			top["operatingsystem"] = emsinfo.operatingSystem;
			top["emstudiohash"] = emsinfo.emstudioHash;
			top["emstudiocommit"] = emsinfo.emstudioCommit;
			QVariantMap memorylocations;
			for (int i=0;i<m_deviceRamMemoryList.size();i++)
			{
				QVariantMap tmp;
				tmp["flashaddress"] =  m_deviceRamMemoryList[i]->flashAddress;
				tmp["flashpage"] = m_deviceRamMemoryList[i]->flashPage;
				tmp["rampage"] = m_deviceRamMemoryList[i]->ramPage;
				tmp["ramaddress"] = m_deviceRamMemoryList[i]->ramAddress;
				tmp["hasparent"] = (m_deviceRamMemoryList[i]->hasParent) ? "true" : "false";
				tmp["size"] = m_deviceRamMemoryList[i]->size;
				QString memory = "";
				for (int j=0;j<m_deviceRamMemoryList[i]->data().size();j++)
				{
					memory += QString("0x") + (((unsigned char)m_deviceRamMemoryList[i]->data()[j] <= 0xF) ? "0" : "") + QString::number((unsigned char)m_deviceRamMemoryList[i]->data()[j],16).toUpper() + ",";
				}
				memory = memory.mid(0,memory.length()-1);
				tmp["data"] = memory;
				//memorylocations["0x" + QString::number(m_deviceRamMemoryList[i]->locationid,16).toUpper()] = tmp;

			}
			for (int i=0;i<m_deviceFlashMemoryList.size();i++)
			{
				QVariantMap tmp;
				tmp["flashaddress"] =  m_deviceFlashMemoryList[i]->flashAddress;
				tmp["flashpage"] = m_deviceFlashMemoryList[i]->flashPage;
				tmp["rampage"] = m_deviceFlashMemoryList[i]->ramPage;
				tmp["ramaddress"] = m_deviceFlashMemoryList[i]->ramAddress;
				tmp["hasparent"] = (m_deviceFlashMemoryList[i]->hasParent) ? "true" : "false";
				tmp["size"] = m_deviceFlashMemoryList[i]->size;
				QString memory = "";
				for (int j=0;j<m_deviceFlashMemoryList[i]->data().size();j++)
				{
					memory += QString("0x") + (((unsigned char)m_deviceFlashMemoryList[i]->data()[j] <= 0xF) ? "0" : "") + QString::number((unsigned char)m_deviceFlashMemoryList[i]->data()[j],16).toUpper() + ",";
				}
				memory = memory.mid(0,memory.length()-1);
				tmp["data"] = memory;
				//memorylocations["0x" + QString::number(m_deviceFlashMemoryList[i]->locationid,16).toUpper()] = tmp;
			}
			//top["memory"] = memorylocations;
			if (m_saveLogs)
			{
				QFile *settingsFile = new QFile(m_logDirectory + "/" + m_logFileName + ".meta.json");
				settingsFile->open(QIODevice::ReadWrite);
				settingsFile->write(jsonSerializer.serialize(top));
				settingsFile->close();
			}
		}
		else
		{
			qDebug() << interrogationSequenceList.size() << "messages left to go. First one:" << interrogationSequenceList[0];
		}
	}
}

void MainWindow::retrieveFlashLocationId(unsigned short locationid)
{
	emsComms->retrieveBlockFromFlash(locationid,0,0);
}

void MainWindow::retrieveRamLocationId(unsigned short locationid)
{
	emsComms->retrieveBlockFromRam(locationid,0,0);
}

void MainWindow::updateDataWindows(unsigned short locationid)
{
	if (m_rawDataView.contains(locationid))
	{
		RawDataView *rawview = qobject_cast<RawDataView*>(m_rawDataView[locationid]);
		if (rawview)
		{
			rawview->setData(locationid,getLocalRamBlock(locationid),true);
		}
		else
		{
			TableView2D *tableview = qobject_cast<TableView2D*>(m_rawDataView[locationid]);
			if (tableview)
			{
				for (int j=0;j<m_table2DMetaData.size();j++)
				{
					if (m_table2DMetaData[j].locationId == locationid)
					{
						tableview->passData(locationid,getLocalRamBlock(locationid),0,m_table2DMetaData[j]);
					}
				}
			}
			else
			{
				TableView3D *tableview3d = qobject_cast<TableView3D*>(m_rawDataView[locationid]);
				if (tableview3d)
				{
					for (int j=0;j<m_table3DMetaData.size();j++)
					{
						if (m_table3DMetaData[j].locationId == locationid)
						{
							tableview3d->passData(locationid,getLocalRamBlock(locationid),0,m_table3DMetaData[j]);
						}
					}

				}
				else
				{
					ReadOnlyRamView *readonlyview = qobject_cast<ReadOnlyRamView*>(m_rawDataView[locationid]);
					if (readonlyview)
					{
						readonlyview->passData(locationid,getLocalRamBlock(locationid),m_readOnlyMetaDataMap[locationid].m_ramData);
					}
					else
					{
						qDebug() << "GUI Window open with memory location, but no valid window type found!";
					}
				}
			}
		}
	}
	else
	{
		//qDebug() << "Attempted to update a window that does not exist!" << "0x" + QString::number(locationid,16).toUpper();
	}
}

void MainWindow::checkRamFlashSync()
{
	if (m_ramMemoryList.size() == 0)
	{
		//Internal ram list is empty. Let's fill it.
		for (int i=0;i<m_deviceRamMemoryList.size();i++)
		{
			m_ramMemoryList.append(new MemoryLocation(*m_deviceRamMemoryList[i]));
		}
		//We have to do something special here, since m_ramMemoryList's parents point to m_deviceRamMemoryList.
		for (int i=0;i<m_ramMemoryList.size();i++)
		{
			if (m_ramMemoryList[i]->hasParent)
			{
				for (int j=0;j<m_ramMemoryList.size();j++)
				{
					if (m_ramMemoryList[i]->parent== m_ramMemoryList[j]->locationid)
					{
						m_ramMemoryList[i]->setParent(m_ramMemoryList[j]);
					}
				}
			}
		}
	}
	if (m_flashMemoryList.size() == 0)
	{
		for (int i=0;i<m_deviceFlashMemoryList.size();i++)
		{
			m_flashMemoryList.append(new MemoryLocation(*m_deviceFlashMemoryList[i]));
		}
		//We have to do something special here, since m_ramMemoryList's parents point to m_deviceRamMemoryList.
		for (int i=0;i<m_flashMemoryList.size();i++)
		{
			if (m_flashMemoryList[i]->hasParent)
			{
				for (int j=0;j<m_flashMemoryList.size();j++)
				{
					if (m_flashMemoryList[i]->parent== m_flashMemoryList[j]->locationid)
					{
						m_flashMemoryList[i]->setParent(m_flashMemoryList[j]);
					}
				}
			}
		}
	}

}

void MainWindow::commandFailed(int sequencenumber,unsigned short errornum)
{
	qDebug() << "Command failed:" << QString::number(sequencenumber) << "0x" + QString::number(errornum,16);
	if (!m_interrogationInProgress)
	{
		QMessageBox::information(0,"Command Failed","Command failed with error: " + m_errorMap[errornum]);
	}
	bool found = false;
	if (m_waitingForRamWriteConfirmation)
	{
		m_waitingForRamWriteConfirmation = false;
		for (int i=0;i<m_ramMemoryList.size();i++)
		{
			if (m_ramMemoryList[i]->locationid == m_currentRamLocationId)
			{
				found=true;
				for (int j=0;j<m_deviceRamMemoryList.size();j++)
				{
					if (m_deviceRamMemoryList[j]->locationid == m_currentRamLocationId)
					{
						found = true;
						qDebug() << "Data reverting for location id 0x" + QString::number(m_ramMemoryList[i]->locationid,16);
						if (m_ramMemoryList[i]->data() == m_deviceRamMemoryList[j]->data())
						{
							if (m_ramMemoryList[i] == m_deviceRamMemoryList[j])
							{
								qDebug() << "Ram memory list and Device memory list are using the same pointer! This should NOT happen!!";
							}
							qDebug() << "Data valid. No need for a revert.";
						}
						else
						{
							qDebug() << "Invalid data, reverting...";
							m_ramMemoryList[i]->setData(m_deviceRamMemoryList[j]->data());
							if (m_ramMemoryList[i]->data() != m_deviceRamMemoryList[j]->data())
							{
								qDebug() << "Failed to revert!!!";
							}
							updateRamLocation(m_currentRamLocationId);
						}
						break;
					}
				}
				break;
			}
		}
		if (!found)
		{
			qDebug() << "Unable to find memory location " << QString::number(m_currentRamLocationId,16) << "in local or device memory!";
		}
		//Find all windows that use that location id
		m_currentRamLocationId = 0;
		//checkRamFlashSync();
	}
	else
	{
		//qDebug() << "Error reverting! " << QString::number(m_currentRamLocationId,16) << "Location not found!";
	}
	if (m_waitingForFlashWriteConfirmation)
	{
		m_waitingForFlashWriteConfirmation = false;
		for (int i=0;i<m_flashMemoryList.size();i++)
		{
			if (m_flashMemoryList[i]->locationid == m_currentFlashLocationId)
			{
				found=true;
				for (int j=0;j<m_deviceFlashMemoryList.size();j++)
				{
					if (m_deviceFlashMemoryList[j]->locationid == m_currentFlashLocationId)
					{
						found = true;
						qDebug() << "Data reverting for location id 0x" + QString::number(m_flashMemoryList[i]->locationid,16);
						if (m_flashMemoryList[i]->data() == m_deviceFlashMemoryList[j]->data())
						{
							if (m_flashMemoryList[i] == m_deviceFlashMemoryList[j])
							{
								qDebug() << "Ram memory list and Device memory list are using the same pointer! This should NOT happen!!";
							}
							qDebug() << "Data valid. No need for a revert.";
						}
						else
						{
							qDebug() << "Invalid data, reverting...";
							m_flashMemoryList[i]->setData(m_deviceFlashMemoryList[j]->data());
							if (m_flashMemoryList[i]->data() != m_deviceFlashMemoryList[j]->data())
							{
								qDebug() << "Failed to revert!!!";
							}
							updateRamLocation(m_currentFlashLocationId);
						}
						break;
					}
				}
				break;
			}
		}
		if (!found)
		{
			qDebug() << "Unable to find memory location " << QString::number(m_currentFlashLocationId,16) << "in local or device memory!";
		}
		//Find all windows that use that location id
		m_currentFlashLocationId = 0;
		//checkRamFlashSync();
		return;
	}
	else
	{
		//qDebug() << "Error reverting! " << QString::number(m_currentFlashLocationId,16) << "Location not found!";
	}
	checkMessageCounters(sequencenumber);

}
void MainWindow::populateParentLists()
{
	//Need to get a list of all IDs here now.
	qDebug() << "Populating internal memory parent list.";
	qDebug() << m_deviceFlashMemoryList.size() << "Device flash locations";
	qDebug() << m_flashMemoryList.size() << "Application flash locations";
	qDebug() << m_deviceRamMemoryList.size() << "Device Ram locations";
	qDebug() << m_ramMemoryList.size() << "Application Ram locations";
	for (int i=0;i<m_deviceFlashMemoryList.size();i++)
	{
		if (m_deviceFlashMemoryList[i]->hasParent && m_deviceFlashMemoryList[i]->getParent() == 0)
		{
			for (int j=0;j<m_deviceFlashMemoryList.size();j++)
			{
				if (m_deviceFlashMemoryList[i]->parent== m_deviceFlashMemoryList[j]->locationid)
				{
					m_deviceFlashMemoryList[i]->setParent(m_deviceFlashMemoryList[j]);
				}
			}
		}
	}
	for (int i=0;i<m_deviceRamMemoryList.size();i++)
	{
		if (m_deviceRamMemoryList[i]->hasParent && m_deviceRamMemoryList[i]->getParent() == 0)
		{
			for (int j=0;j<m_deviceRamMemoryList.size();j++)
			{
				if (m_deviceRamMemoryList[i]->parent== m_deviceRamMemoryList[j]->locationid)
				{
					m_deviceRamMemoryList[i]->setParent(m_deviceRamMemoryList[j]);
				}
			}
		}
	}
}

void MainWindow::pauseLogButtonClicked()
{

}
void MainWindow::saveFlashLocationIdBlock(unsigned short locationid,QByteArray data)
{
	qDebug() << "Burning flash block:" << "0x" + QString::number(locationid,16).toUpper();
	if (hasLocalFlashBlock(locationid))
	{
		setLocalFlashBlock(locationid,data);
	}
	emsComms->updateBlockInFlash(locationid,0,data.size(),data);
}

void MainWindow::saveFlashLocationId(unsigned short locationid)
{
	qDebug() << "Burning block from ram to flash for locationid:" << "0x"+QString::number(locationid,16).toUpper();
	emsComms->burnBlockFromRamToFlash(locationid,0,0);
	if (hasLocalFlashBlock(locationid))
	{
		if(hasDeviceRamBlock(locationid))
		{
			setLocalFlashBlock(locationid,getDeviceRamBlock(locationid));
			//getLocalFlashBlock(locationid);
			//getDeviceRamBlock(locationid);
		}
	}
}

void MainWindow::saveSingleData(unsigned short locationid,QByteArray data, unsigned short offset, unsigned short size)
{
	bool found = false;
	for (int i=0;i<m_ramMemoryList.size();i++)
	{
		if (m_ramMemoryList[i]->locationid == locationid)
		{
			if (m_ramMemoryList[i]->data().mid(offset,size) == data)
			{
				qDebug() << "Data in application memory unchanged, no reason to send write for single value";
				return;
			}

			m_ramMemoryList[i]->setData(m_ramMemoryList[i]->data().replace(offset,size,data));
			found = true;
		}
	}
	if (!found)
	{
		qDebug() << "Attempted to save data for single value at location id:" << "0x" + QString::number(locationid,16) << "but no valid location found in Ram list. Ram list size:" << m_ramMemoryList.size();
	}
	qDebug() << "Requesting to update single value at ram location:" << "0x" + QString::number(locationid,16).toUpper() << "data size:" << data.size();
	qDebug() << "Offset:" << offset << "Size:" << size  <<  "Data:" << data;
	m_currentRamLocationId = locationid;
	m_waitingForRamWriteConfirmation = true;
	emsComms->updateBlockInRam(locationid,offset,size,data);
}

void MainWindow::stopLogButtonClicked()
{

}
void MainWindow::connectButtonClicked()
{
	emsComms->connectSerial(m_comPort,m_comBaud);
}

void MainWindow::logProgress(qlonglong current,qlonglong total)
{
	Q_UNUSED(current)
	Q_UNUSED(total)
	//setWindowTitle(QString::number(current) + "/" + QString::number(total) + " - " + QString::number((float)current/(float)total));
}
void MainWindow::guiUpdateTimerTick()
{
}
void MainWindow::dataLogDecoded(QVariantMap data)
{
	//m_valueMap = data;
	if (dataTables)
	{
		dataTables->passData(data);
	}
	if (dataGauges)
	{
		dataGauges->passData(data);
	}
	if (dataFlags)
	{
		dataFlags->passData(data);
	}
}

void MainWindow::logPayloadReceived(QByteArray header,QByteArray payload)
{
	Q_UNUSED(header)
	pidcount++;
	if (payload.length() != 96)
	{
		//Wrong sized payload!
		//We should do something here or something...
		//return;
	}
	dataPacketDecoder->decodePayload(payload);
	//guiUpdateTimerTick();

}

MainWindow::~MainWindow()
{
	emsComms->terminate();
	emsComms->wait(1000);
	delete emsComms;
}
