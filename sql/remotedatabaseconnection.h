#ifndef REMOTEDATABASECONNECTION_H
#define REMOTEDATABASECONNECTION_H

#include "sql/RemoteDatabaseConnectionDef.h"
#include <Qlist>
#include <QTimer>
#include <QSqlDatabase>
#include "QHash"
#include "QImage"

QT_FORWARD_DECLARE_CLASS(ListManager);

class RemoteDatabaseConnection: public QObject
{
    Q_OBJECT
public:
    RemoteDatabaseConnection(ListManager* l);
    ~RemoteDatabaseConnection();

signals:
    void syncThreadStart();

    void noteUpdateSequenceNumberChanged(QString, int);
    void notebookUpdateSequenceNumberChanged(QString, int);
    void resourceUpdateSequenceNumberChanged(QString, int);
    void tagUpdateSequenceNumberChanged(QString, int);

    void receiveDownloadedTask(const QList<DownloadedTask>&);

    // 这部分用于通知主窗口，更新同步图标状态
    void RemoteDataBaseConnectFailed();
    void RemoteDataBaseConnectSuccessed();
    void BeginSynchronizing();
    void FinishSynchronizing();

public slots:
    void OnThreadStart();

protected slots:
    void beforeMainThreadStop();

    void OnUploadTimerTimeout();
    void OnDownloadTimerTimeout();

    void OnAddNote(const SyncNoteData&);
    void OnUpdateNote(const SyncNoteData&);
    void OnDeleteNote(QString);

    void OnAddNotebook(const SyncNotebookData&);
    void OnUpdateNotebook(const SyncNotebookData&);
    void OnDeleteNotebook(QString);

    void OnAddResource(const SyncResourceData&);
    void OnUpdateResource(const SyncResourceData&);
    void OnDeleteResource(QString);

    void OnAddTag(const SyncTagData&);
    void OnUpdateTag(const SyncTagData&);
    void OnDeleteTag(QString);

    void onDeleteNoteTagByNoteGuid(QString);
    void addNoteTags(QString noteGuid, QString tagGuid);

protected:
    bool InitializeDB();
    bool DeInitializeDB();

    void DoUploadTask(const UploadTask&);

    void ProcessNoteUploadTask(const SyncNoteData&);
    void ProcessDeleteNoteUploadTask(QString guid);

    void ProcessNotebookUploadTask(const SyncNotebookData&);
    void ProcessDeleteNotebookUploadTask(QString guid);

    void ProcessResourceUploadTask(const SyncResourceData&);
    void ProcessDeleteResourceUploadTask(QString guid);

    void ProcessTagUploadTask(const SyncTagData&);
    void ProcessDeleteTagUploadTask(QString guid);

    void ProcessNoteTagUploadTask(const SyncNoteTagData&);
    void ProcessDeleteNoteTagUploadTask(QString noteGuid);

    void downloadNotebookList();
    void downloadResourceList();
    void downloadNoteList();
    void downloadTagList();
    void downloadNoteTagList();
    void sortDownloadTastList();
    void sendDownloadTast();

    // 判断任务是不是已经存在于下载队列中了。为了防止出现死循环，下载线程通知主线程更新，主线程通知上传线程更新，
    // 下载线程又把上传线程的更新拉回本地。
    bool taskExistInDownloadlist(ESyncOperationType, ESyncObjectType, QString, QString, int);
    bool taskExistInUploadlist(ESyncOperationType, ESyncObjectType, QString, QString);

private:
    QList<UploadTask>       uploadTaskLst;
    QTimer*                 uploadTaskTimer;

    QList<DownloadedTask>   downloadedTask;  //一定是在上传队列为空的时候启动
    QTimer*                 downloadTaskTimer;
    int                     indexOfNoneEmitedDownloadTask;
    qint64                  lastTimeOfStartDownload;
    qint64                  lastTimeOfStartUpload;

    QSqlDatabase*           m_pDBMysql;
    bool                    m_bInitializeSuccess;
    bool                    m_bStopThreadAfterEmptyList;

    ListManager*            listManager;
};

#endif // REMOTEDATABASECONNECTION_H
