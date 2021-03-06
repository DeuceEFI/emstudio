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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"
#include <QFile>
#include "datafield.h"
//#include "logloader.h"
#include "freeemscomms.h"

#include <QTimer>
#include <qjson/serializer.h>
#include "headers.h"
#include "datapacketdecoder.h"
#include "comsettings.h"
#include "emsinfoview.h"
#include "tableview.h"
#include "rawdataview.h"
#include "gaugeview.h"
#include "flagview.h"
#include "packetstatusview.h"
#include "aboutview.h"
#include "memorylocation.h"
#include "interrogateprogressview.h"
#include "table2ddata.h"
#include "readonlyramview.h"
//#include "datarawview.h"


class RawDataBlock
{
public:
	unsigned short locationid;
	QByteArray header;
	QByteArray data;
};

class Interrogation
{
public:
	QString firmwareVersion;
	QString interfaceVersion;
	QString compilerVersion;
	QString firmwareBuildDate;
	QString decoderName;
	QString maxPacketSize;
	QString operatingSystem;
	QString emstudioBuilDate;
	QString emstudioCommit;
	QString emstudioHash;
};


class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();
	void setDevice(QString dev);
	void connectToEms();
private:
	Interrogation emsinfo;
	unsigned short m_currentRamLocationId;
	bool m_waitingForRamWriteConfirmation;
	unsigned short m_currentFlashLocationId;
	bool m_waitingForFlashWriteConfirmation;
	//QList<RawDataBlock*> m_ramRawBlockList;
	//QList<RawDataBlock*> m_flashRawBlockList;
	//QList<RawDataBlock*> m_deviceRamRawBlockList;
	//QList<RawDataBlock*> m_deviceFlashRawBlockList;
	QList<MemoryLocation*> m_ramMemoryList;
	QList<MemoryLocation*> m_flashMemoryList;
	QList<MemoryLocation*> m_deviceFlashMemoryList;
	QList<MemoryLocation*> m_deviceRamMemoryList;
	QList<MemoryLocation*> m_tempMemoryList;
	QList<Table3DMetaData> m_table3DMetaData;
	QList<Table2DMetaData> m_table2DMetaData;
	QMap<unsigned short,ReadOnlyRamBlock> m_readOnlyMetaDataMap;
	QList<ConfigData> m_configMetaData;
	QList<ReadOnlyRamData> m_readOnlyMetaData;
	//QMap<unsigned short,QList<ReadOnlyRamData> > m_readOnlyMetaDataMap;
	QMap<unsigned short,QString> m_errorMap;
	//RawDataView *rawData;
	TableView *dataTables;
	GaugeView *dataGauges;
	EmsInfoView *emsInfo;
	FlagView *dataFlags;
	QString m_logFileName;
	PacketStatusView *packetStatus;
	AboutView *aboutView;
	InterrogateProgressView *progressView;
	QList<int> interrogationSequenceList;
	QMap<unsigned short,QWidget*> m_rawDataView;
	QMdiSubWindow *tablesMdiWindow;
	QMdiSubWindow *emsMdiWindow;
	QMdiSubWindow *flagsMdiWindow;
	QMdiSubWindow *gaugesMdiWindow;
	QMdiSubWindow *packetStatusMdiWindow;
	QMdiSubWindow *aboutMdiWindow;
	//QFile *settingsFile;
	void checkMessageCounters(int sequencenumber);
	//QMdiSubWindow *rawMdiWindow;
	//ComSettings *comSettings;
	void populateParentLists();
	DataPacketDecoder *dataPacketDecoder;
	void populateDataFields();
	void updateRamLocation(unsigned short locationid);
	void updateDataWindows(unsigned short locationid);
	Ui::MainWindow ui;
	//LogLoader *logLoader;
	FreeEmsComms *emsComms;
	int pidcount;
	QTimer *timer;
	QTimer *guiUpdateTimer;
	QString m_comPort;
	int m_comBaud;
	int m_comInterByte;
	bool m_saveLogs;
	bool m_clearLogs;
	int m_logsToKeep;
	QString m_logDirectory;
	QString m_firmwareVersion;
	QString m_interfaceVersion;
	QFile *logfile;
	void markRamDirty();
	void markDeviceFlashDirty();
	void markRamClean();
	void markDeviceFlashClean();
	bool m_localRamDirty;
	bool m_deviceFlashDirty;
	bool hasDeviceRamBlock(unsigned short id);
	bool hasLocalRamBlock(unsigned short id);
	bool hasLocalFlashBlock(unsigned short id);
	bool hasDeviceFlashBlock(unsigned short id);
	void setDeviceFlashBlock(unsigned short id,QByteArray data);
	void setLocalRamBlock(unsigned short id,QByteArray data);
	void setDeviceRamBlock(unsigned short id,QByteArray data);
	void setLocalFlashBlock(unsigned short id,QByteArray data);
	QByteArray getLocalRamBlock(unsigned short id);
	QByteArray getLocalFlashBlock(unsigned short id);
	QByteArray getDeviceRamBlock(unsigned short id);
	QByteArray getDeviceFlashBlock(unsigned short id);
	QList<int> m_locIdMsgList;
	QList<int> m_locIdInfoMsgList;
	void checkRamFlashSync();
	bool m_interrogationInProgress;
private slots:
	void emsOperatingSystem(QString os);
	void emsDecoderName(QString name);
	void emsFirmwareBuildDate(QString date);
	void emsCommsDisconnected();
	void emsCompilerVersion(QString version);
	void checkSyncRequest();
	void rawViewSaveData(unsigned short locationid,QByteArray data,int physicallocation);
	void rawDataViewDestroyed(QObject *object);
	void emsInfoDisplayLocationId(int locid,bool isram,int type);
	void locationIdInfo(unsigned short locationid,unsigned short rawFlags,QList<FreeEmsComms::LocationIdFlags> flags,unsigned short parent, unsigned char rampage,unsigned char flashpage,unsigned short ramaddress,unsigned short flashaddress,unsigned short size);
	void dataViewSaveLocation(unsigned short locationid,QByteArray data,int phyiscallocation);
	void menu_windows_GaugesClicked();
	void menu_windows_EmsInfoClicked();
	void menu_windows_TablesClicked();
	void menu_windows_FlagsClicked();
	void menu_windows_PacketStatusClicked();
	void menu_settingsClicked();
	void menu_connectClicked();
	void menu_aboutClicked();
	void interrogateProgressViewCancelClicked();
	void ui_saveDataButtonClicked();
	void menu_disconnectClicked();
	void settingsSaveClicked();
	void settingsCancelClicked();
	void guiUpdateTimerTick();
	void timerTick();
	void connectButtonClicked();
	void dataLogDecoded(QVariantMap data);
	void logPayloadReceived(QByteArray header,QByteArray payload);
	void logProgress(qlonglong current,qlonglong total);
	void logFinished();
	void tableview3d_reloadTableData(unsigned short locationid);
	void tableview2d_reloadTableData(unsigned short locationid);
	void loadLogButtonClicked();
	void playLogButtonClicked();
	void pauseLogButtonClicked();
	void saveSingleData(unsigned short locationid,QByteArray data, unsigned short offset, unsigned short size);
	void saveFlashLocationId(unsigned short locationid);
	void stopLogButtonClicked();
	void emsCommsConnected();
	void unknownPacket(QByteArray header,QByteArray payload);
	void locationIdList(QList<unsigned short> idlist);
	//void locationIdInfo(unsigned short locationid,unsigned short rawFlags,QList<FreeEmsComms::LocationIdFlags> flags,unsigned short parent, unsigned char rampage,unsigned char flashpage,unsigned short ramaddress,unsigned short flashaddress,unsigned short size);
	void blockRetrieved(int sequencenumber,QByteArray header,QByteArray payload);
	void ramBlockRetrieved(unsigned short locationid,QByteArray header,QByteArray payload);
	void flashBlockRetrieved(unsigned short locationid,QByteArray header,QByteArray payload);
	void dataLogPayloadReceived(QByteArray header,QByteArray payload);
	void interfaceVersion(QString version);
	void firmwareVersion(QString version);
	void error(QString msg);
	void commandSuccessful(int sequencenumber);
	void commandFailed(int sequencenumber,unsigned short errornum);
	void commandTimedOut(int sequencenumber);
	void interByteDelayChanged(int num);
	void saveFlashLocationIdBlock(unsigned short locationid,QByteArray data);
	void retrieveRamLocationId(unsigned short locationid);
	void retrieveFlashLocationId(unsigned short locationid);

};

#endif // MAINWINDOW_H
