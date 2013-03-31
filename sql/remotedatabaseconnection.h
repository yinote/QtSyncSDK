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

    // �ⲿ������֪ͨ�����ڣ�����ͬ��ͼ��״̬
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

    // �ж������ǲ����Ѿ����������ض������ˡ�Ϊ�˷�ֹ������ѭ���������߳�֪ͨ���̸߳��£����߳�֪ͨ�ϴ��̸߳��£�
    // �����߳��ְ��ϴ��̵߳ĸ������ر��ء�
    bool taskExistInDownloadlist(ESyncOperationType, ESyncObjectType, QString, QString, int);
    bool taskExistInUploadlist(ESyncOperationType, ESyncObjectType, QString, QString);

private:
    QList<UploadTask>       uploadTaskLst;
    QTimer*                 uploadTaskTimer;

    QList<DownloadedTask>   downloadedTask;  //һ�������ϴ�����Ϊ�յ�ʱ������
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
