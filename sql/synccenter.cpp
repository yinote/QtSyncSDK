#include "synccenter.h"
#include "../Global.h"
#include <QThread>
#include "../sync/service/SyncService.h"
#include "../sync/local/localsyncstatestoreimpl.h"
#include <QStandardPaths>

SyncCenter::SyncCenter()
{
    localDBConnection = NULL;
    dataBaseLogger = NULL;
    syncTaskTimer = new QTimer(this);
    syncTaskTimer->setSingleShot(true);
    syncTaskTimer->setInterval(10*1000); //3分钟
    connect(syncTaskTimer, SIGNAL(timeout()), this, SLOT(OnSyncTaskTimeOut()));

	syncService = NULL;
}

SyncCenter::~SyncCenter()
{
    DeInitializeLocalDBConnection();
	if (syncService)
	{
		delete syncService;
	}
}

DatabaseConnection *SyncCenter::getDataBaseConnection()
{
    return localDBConnection;
}

void SyncCenter::OnThreadStart()
{
	if (syncService)
	{
		delete syncService;
	}

	UserInfo info(accessToken);
	RemoteSetting setting(host, port, info);
	LocalSyncStateStoreImpl* syncStateStore = new LocalSyncStateStoreImpl(this);
	syncService = new SyncServiceImpl(setting, *syncStateStore);
	syncService->enableNotebookSync(new LocalNotebookStore(this));
	syncService->enableTagSync(new LocalTagStore(this));
	syncService->enableNoteSync(new LocalNoteStore(this));
	syncService->enableResourceSync(new LocalResourceStore(this));

    InitializeLocalDBConnection();
	syncTaskTimer->start();
    emit syncThreadStart();
}

void SyncCenter::OnSyncTaskTimeOut()
{
	/*if (syncService)
	{
		XError* error = new XError;
		syncService->doSync(error);
		delete error;
	}*/
	syncTaskTimer->start();
}

bool SyncCenter::InitializeLocalDBConnection()
{
    dataBaseLogger = new ApplicationLogger("logs/sync_database.log");
    localDBConnection = new DatabaseConnection(dataBaseLogger);

    return localDBConnection->dbSetup(Global::getDatabaseUrl(), Global::getIndexDatabaseUrl(), Global::getResourceDatabaseUrl(),
        Global::getDatabaseUserid(), Global::getDatabaseUserPassword(), Global::cipherPassword, "SyncThread");
}

void SyncCenter::DeInitializeLocalDBConnection()
{
    if (localDBConnection)
    {
        localDBConnection->dbShutdown();
        delete localDBConnection;
        localDBConnection = NULL;
    }

    if (dataBaseLogger)
    {
        delete dataBaseLogger;
        dataBaseLogger = NULL;
    }
}

void SyncCenter::handleDownloadedTask(const DownloadedTask& task)
{
	emit receiveDownloadedTask(task);
}

void SyncCenter::updateNoteSequenceNumber(QString guid, int usn)
{
	emit noteUpdateSequenceNumberChanged(guid, usn);
}

void SyncCenter::beforeMainThreadStop()
{
	QThread::currentThread()->quit();
}

void SyncCenter::setLoginInfo(QString token, QString strhost, int iport)
{
	accessToken = token;
	host = strhost;
	port = iport;
}
