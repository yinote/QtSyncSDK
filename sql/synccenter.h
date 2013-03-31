#ifndef SYNCCENTER_H
#define SYNCCENTER_H

#include <Qlist>
#include <QTimer>
#include <QObject>
#include "databaseconnection.h"
#include "sql/RemoteDatabaseConnectionDef.h"
#include "../utilities/applicationlogger.h"

QT_FORWARD_DECLARE_CLASS(SyncServiceImpl);

class SyncCenter: public QObject
{
    Q_OBJECT
signals:
    void syncThreadStart();

    void noteUpdateSequenceNumberChanged(QString, int);
    void notebookUpdateSequenceNumberChanged(QString, int);
    void resourceUpdateSequenceNumberChanged(QString, int);
    void tagUpdateSequenceNumberChanged(QString, int);

    void receiveDownloadedTask(const DownloadedTask&);

public:
    SyncCenter();
    ~SyncCenter();

    DatabaseConnection* getDataBaseConnection();
	void handleDownloadedTask(const DownloadedTask&);
	void updateNoteSequenceNumber(QString, int);
	void setLoginInfo(QString accessToken, QString host, int port);

public slots:
    void OnThreadStart();
	void beforeMainThreadStop();
    void OnSyncTaskTimeOut();

protected:
    bool InitializeLocalDBConnection();
    void DeInitializeLocalDBConnection();

private:
    QTimer*                 syncTaskTimer;
    DatabaseConnection*     localDBConnection;
    ApplicationLogger*      dataBaseLogger;
	SyncServiceImpl*		syncService;
	QString					accessToken;
	QString					host;
	int						port;
};

#endif // SYNCCENTER_H
