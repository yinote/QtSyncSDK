#include "remotedatabaseconnection.h"
#include <qDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QDateTime>
#include <QSqlRecord>
#include "Global.h"
#include "utilities/listmanager.h"

#define UPDATE_INTERVAL_SEC 5 *60

bool operator == (const NoteTagsRecord& lhs, const NoteTagsRecord& rhs)
{
    return lhs.noteGuid == rhs.noteGuid && lhs.tagGuid == rhs.tagGuid;
}

bool operator != (const NoteTagsRecord& lhs, const NoteTagsRecord& rhs)
{
    return lhs.noteGuid != rhs.noteGuid || lhs.tagGuid != rhs.tagGuid;
}

RemoteDatabaseConnection::RemoteDatabaseConnection(ListManager* l)
{
    listManager = l;

    m_bInitializeSuccess = false;
    uploadTaskTimer = NULL;
    m_pDBMysql = NULL;
    uploadTaskTimer = new QTimer(this);
    uploadTaskTimer->setSingleShot(true);
    uploadTaskTimer->setInterval(200); //200毫秒启动一个任务

    m_bStopThreadAfterEmptyList = false;

    connect(uploadTaskTimer, SIGNAL(timeout()), this, SLOT(OnUploadTimerTimeout()));

    downloadTaskTimer = new QTimer(this);
    downloadTaskTimer->setSingleShot(true);
    indexOfNoneEmitedDownloadTask = 0;
    lastTimeOfStartDownload = 0;
    connect(downloadTaskTimer, SIGNAL(timeout()), this, SLOT(OnDownloadTimerTimeout()));

    lastTimeOfStartUpload = 0;
}

RemoteDatabaseConnection::~RemoteDatabaseConnection()
{
    DeInitializeDB();
}

void RemoteDatabaseConnection::OnThreadStart()
{
    InitializeDB();
    if (m_bInitializeSuccess)
    {
        emit syncThreadStart();
        emit RemoteDataBaseConnectSuccessed();
    }
    else
    {
        emit RemoteDataBaseConnectFailed();
    }
}

bool RemoteDatabaseConnection::InitializeDB()
{
    DeInitializeDB();

    QString ConnectionName("YINOTE_REMOTE_DB_CONNECT_STR");
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL",ConnectionName);
    m_pDBMysql = new QSqlDatabase(db);

    /*m_pDBMysql->setHostName("localhost");
    m_pDBMysql->setDatabaseName("yinotes");
    m_pDBMysql->setUserName("root");
    m_pDBMysql->setPassword("1234");
    m_pDBMysql->setPort(3306);*/

    m_pDBMysql->setHostName("211.147.14.120");
    m_pDBMysql->setDatabaseName("mynotes");
    m_pDBMysql->setUserName("root");
    m_pDBMysql->setPassword("yinotes");
    m_pDBMysql->setPort(13306);

    m_bInitializeSuccess = m_pDBMysql->open();
    if (m_bInitializeSuccess == false)
    {
        qDebug()<<__FUNCTION__<<" line:"<<__LINE__<<" "<< m_pDBMysql->lastError().text();
    }

    return m_bInitializeSuccess;
}

bool RemoteDatabaseConnection::DeInitializeDB()
{
    QString ConnectionName("YINOTE_REMOTE_DB_CONNECT_STR");
    if (m_pDBMysql != NULL)
    {
        if (m_pDBMysql->isOpen())
        {
            m_pDBMysql->close();
        }

        delete m_pDBMysql;
        m_pDBMysql = NULL;

        QSqlDatabase::removeDatabase(ConnectionName);
    }

    return true;
}

void RemoteDatabaseConnection::OnUploadTimerTimeout()
{
    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss(zzz)")<<" DoUploadTask: remian task Count:" <<uploadTaskLst.size();
    if (uploadTaskLst.empty())
    {
        return;
    }

    if (lastTimeOfStartUpload == 0)
    {
        lastTimeOfStartUpload = QDateTime::currentMSecsSinceEpoch();
        emit BeginSynchronizing();
    }

    DoUploadTask(uploadTaskLst.at(0));
    uploadTaskLst.pop_front();

    if (!uploadTaskLst.isEmpty())
    {
        uploadTaskTimer->start();
    }
    else
    {
        if (m_bStopThreadAfterEmptyList)
        {
            QThread::currentThread()->quit();
            return;
        }// end if (m_bStopThreadAfterEmptyList)

        int msecsUsed = (QDateTime::currentMSecsSinceEpoch() - lastTimeOfStartUpload);
        lastTimeOfStartUpload = 0;
        if (msecsUsed < 1600)
        {
            QTimer::singleShot((1600 - msecsUsed), this, SIGNAL(FinishSynchronizing()));
        }
        else
        {
            emit FinishSynchronizing();
        }

        if (downloadedTask.isEmpty() && !downloadTaskTimer->isActive())
        {
            qint64 secs = (QDateTime::currentMSecsSinceEpoch() - lastTimeOfStartDownload)/1000;
            lastTimeOfStartDownload = QDateTime::currentMSecsSinceEpoch();
            if (secs > UPDATE_INTERVAL_SEC)
            {
                   OnDownloadTimerTimeout();
            }
            else
            {
                downloadTaskTimer->start( (UPDATE_INTERVAL_SEC - secs)*1000);
            }
        }
    }
}

void RemoteDatabaseConnection::DoUploadTask(const UploadTask& task)
{
    if (task.objectType == E_SYNC_OBJECT_NOTE)
    {
        if (task.operType == E_OPER_ADD ||
            task.operType == E_OPER_UPDATE)
        {
            ProcessNoteUploadTask(task.noteDataBody);
        }
        else
        {
            ProcessDeleteNoteUploadTask(task.noteDataBody.guid);
        }
    }
    else if (task.objectType == E_SYNC_OBJECT_NOTEBOOK)
    {
        if (task.operType == E_OPER_ADD ||
            task.operType == E_OPER_UPDATE)
        {
            ProcessNotebookUploadTask(task.notebookDataBody);
        }
        else
        {
            ProcessDeleteNotebookUploadTask(task.notebookDataBody.guid);
        }
    }
    else if (task.objectType == E_SYNC_OBJECT_RESOURCE)
    {
        if (task.operType == E_OPER_ADD ||
            task.operType == E_OPER_UPDATE)
        {
            ProcessResourceUploadTask(task.resourceDataBody);
        }
        else
        {
            ProcessDeleteResourceUploadTask(task.resourceDataBody.guid);
        }
    }
    else if (task.objectType == E_SYNC_OBJECT_TAG)
    {
        if (task.operType == E_OPER_ADD ||
            task.operType == E_OPER_UPDATE)
        {
            ProcessTagUploadTask(task.tagDataBody);
        }
        else
        {
            ProcessDeleteTagUploadTask(task.tagDataBody.guid);
        }
    }
    else if (task.objectType == E_SYNC_OBJECT_NOTETAG)
    {
        if (task.operType == E_OPER_ADD ||
            task.operType == E_OPER_UPDATE)
        {
            ProcessNoteTagUploadTask(task.noteTagDataBody);
        }
        else
        {
            ProcessDeleteNoteTagUploadTask(task.noteTagDataBody.noteGuid);
        }
    }
}

void RemoteDatabaseConnection::OnAddNote(const SyncNoteData& noteData)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_ADD, E_SYNC_OBJECT_NOTE, noteData.guid, noteData.guid, noteData.updateSequenceNumber))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_NOTE;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_ADD;
    uploadTaskLst[uploadTaskLst.count() - 1].noteDataBody = noteData;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::OnUpdateNote(const SyncNoteData& noteData)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_UPDATE, E_SYNC_OBJECT_NOTE, noteData.guid, noteData.guid, noteData.updateSequenceNumber))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_NOTE;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_UPDATE;
    uploadTaskLst[uploadTaskLst.count() - 1].noteDataBody = noteData;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::OnDeleteNote(QString guid)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_DELETE, E_SYNC_OBJECT_NOTE, guid, guid, 0))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_NOTE;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_DELETE;
    uploadTaskLst[uploadTaskLst.count() - 1].noteDataBody.guid = guid;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::OnAddNotebook(const SyncNotebookData& data)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_ADD, E_SYNC_OBJECT_NOTEBOOK, data.guid, data.guid, data.sequence))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_NOTEBOOK;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_ADD;
    uploadTaskLst[uploadTaskLst.count() - 1].notebookDataBody = data;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::OnUpdateNotebook(const SyncNotebookData& data)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_UPDATE, E_SYNC_OBJECT_NOTEBOOK, data.guid, data.guid, data.sequence))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_NOTEBOOK;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_UPDATE;
    uploadTaskLst[uploadTaskLst.count() - 1].notebookDataBody = data;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::OnDeleteNotebook(QString guid)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_DELETE, E_SYNC_OBJECT_NOTEBOOK, guid, guid, 0))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_NOTEBOOK;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_DELETE;
    uploadTaskLst[uploadTaskLst.count() - 1].notebookDataBody.guid = guid;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::ProcessNoteUploadTask(const SyncNoteData& data)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "CALL InsertOrUpdateNote(:guid, "
        ":title, "
        ":content, "
        ":contentHash, "
        ":contentLength,"
        ":created, "
        ":updated, "
        ":deleted, "
        ":active, "
        ":isExpunged, "
        ":isDirty, "
        ":attributeLatitude, "
        ":attributeLongitude, "
        ":attributeAltitude, "
        ":attributeAuthor, "
        ":attributeSourceUrl, "
        ":notebookGuid, "
        ":updateSequenceNumber);";
    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":guid", data.guid);
    query.bindValue(":title",data.title);
    query.bindValue(":content",data.content);
    query.bindValue(":contentHash","");
    query.bindValue(":contentLength", 0);
    query.bindValue(":created",data.created);
    query.bindValue(":updated",data.updated);
    query.bindValue(":deleted",data.deleted);
    query.bindValue(":active",data.active?1:0);
    query.bindValue(":isExpunged", 0);
    query.bindValue(":isDirty",data.isDirty?1:0);

    query.bindValue(":attributeLatitude",0);
    query.bindValue(":attributeLongitude",0);
    query.bindValue(":attributeAltitude",0);

    query.bindValue(":attributeAuthor", "");
    query.bindValue(":attributeSourceUrl", "");

    query.bindValue(":notebookGuid", data.notebookGuid);
    query.bindValue(":updateSequenceNumber", data.updateSequenceNumber);

    if(!query.exec())
    {
        qDebug()<<__FUNCTION__<<" line:"<<__LINE__<<" "<<query.lastError().text();
    }

    // 所有的usn必须由服务器下发
    sql = "select updateSequenceNumber from note where guid = :guid;";
    QSqlQuery query1(*m_pDBMysql);
    query1.prepare(sql);
    query1.bindValue(":guid", data.guid);
    if(query1.exec() && query1.next())
    {
        int usn = query1.value(0).toInt();
        if (usn > data.updateSequenceNumber)
        {
            // 需要把上传队列中剩余的这个笔记的usn全部更新，不然接下去的就传不上去了
            for (int i = 0; i < uploadTaskLst.size(); i++)
            {
                if (uploadTaskLst.at(i).objectType != E_SYNC_OBJECT_NOTE ||
                    uploadTaskLst.at(i).noteDataBody.guid != data.guid)
                {
                    continue;
                }

                uploadTaskLst[i].noteDataBody.updateSequenceNumber = usn;
            }// end for (int i = 0; i < uploadTaskLst.size(); i++)
            emit noteUpdateSequenceNumberChanged(data.guid, usn);
        }// end if (usn > data.updateSequenceNumber)
    }// end if(query1.exec() && query1.next())
}

void RemoteDatabaseConnection::ProcessDeleteNoteUploadTask(QString guid)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "CALL DeleteNote(:guid);";
    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":guid", guid);

    if(!query.exec())
    {
        qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
    }
}

void RemoteDatabaseConnection::ProcessNotebookUploadTask(const SyncNotebookData& data)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "CALL InsertOrUpdateNotebook(:guid, "
        ":name, "
        ":isDirty, "
        ":serviceCreated, "
        ":serviceUpdated, "
        ":stack, "
        ":defaultNotebook, "
        ":sequence);";

    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":guid", data.guid);
    query.bindValue(":name",data.name);
    query.bindValue(":isDirty", 0);
    query.bindValue(":serviceCreated", data.serviceCreated);
    query.bindValue(":serviceUpdated", data.serviceUpdated);
    query.bindValue(":stack",data.stack);
    query.bindValue(":defaultNotebook", data.defaultNotebook?1:0);
    query.bindValue(":sequence",data.sequence);
    if(!query.exec())
    {
        qDebug()<<__FUNCTION__<<" line:"<<__LINE__<<" "<<query.lastError().text();
        return;
    }

    // 所有的usn必须由服务器下发
    sql = "select sequence from notebook where guid = :guid;";
    QSqlQuery query1(*m_pDBMysql);
    query1.prepare(sql);
    query1.bindValue(":guid", data.guid);
    if(query1.exec() && query1.next())
    {
        int usn = query1.value(0).toInt();
        if (usn > data.sequence)
        {
            // 需要把上传队列中剩余的这个笔记的usn全部更新，不然接下去的就传不上去了
            for (int i = 0; i < uploadTaskLst.size(); i++)
            {
                if (uploadTaskLst.at(i).objectType != E_SYNC_OBJECT_NOTEBOOK ||
                    uploadTaskLst.at(i).notebookDataBody.guid != data.guid)
                {
                    continue;
                }

                uploadTaskLst[i].notebookDataBody.sequence = usn;
            }// end for (int i = 0; i < uploadTaskLst.size(); i++)
            emit notebookUpdateSequenceNumberChanged(data.guid, usn);
        }
    }
}

void RemoteDatabaseConnection::ProcessDeleteNotebookUploadTask(QString guid)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "delete FROM notebook WHERE guid = :guid;";
    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":guid", guid);

    if(!query.exec())
    {
        qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
    }
}

void RemoteDatabaseConnection::OnAddResource(const SyncResourceData& data)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_ADD, E_SYNC_OBJECT_RESOURCE, data.guid, data.guid, data.updateSequenceNumber))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_RESOURCE;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_ADD;
    uploadTaskLst[uploadTaskLst.count() - 1].resourceDataBody = data;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::OnUpdateResource(const SyncResourceData& data)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_UPDATE, E_SYNC_OBJECT_RESOURCE, data.guid, data.guid, data.updateSequenceNumber))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_RESOURCE;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_UPDATE;
    uploadTaskLst[uploadTaskLst.count() - 1].resourceDataBody = data;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::OnDeleteResource(QString guid)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_DELETE, E_SYNC_OBJECT_RESOURCE, guid, guid, 0))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_RESOURCE;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_DELETE;
    uploadTaskLst[uploadTaskLst.count() - 1].resourceDataBody.guid = guid;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::ProcessResourceUploadTask(const SyncResourceData& data)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    // 由于目前的实现中不存在资源的变更，上传过的资源就不需要重复上传
    QSqlQuery query1(*m_pDBMysql);
    QString sql1 = "select count(guid) from note_resource where guid = :guid; ";
    query1.prepare(sql1);
    query1.bindValue(":guid", data.guid);
    if (query1.exec() && query1.next())
    {
        int count = query1.value(0).toInt();
        if (count)
        {
            return;
        }
    }// end if (!query.prepare(sql) && query.exec() && query.next())

    QSqlQuery query(*m_pDBMysql);
    QString sql = "CALL InsertOrUpdateResource(:guid, "
        ":dataHash, "
        ":dataSize,"
        ":dataBinary,"
        ":mime,"
        ":width,"
        ":height,"
        ":duration,"
        ":active,"
        ":isDirty,"
        ":attributeSourceUrl,"
        ":attributeTimestamp,"
        ":attributeLatitude,"
        ":attributeLongitude,"
        ":attributeAltitude,"
        ":attributeCameraMake,"
        ":attributeCameraModel,"
        ":attributeClientWillIndex,"
        ":attributeFileName, "
        ":attributeAttachment, "
        ":noteGuid, "
        ":updateSequenceNumber);";

    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":guid", data.guid);
    query.bindValue(":dataHash", data.hash);
    query.bindValue(":dataSize", data.dataBinary.size());
    query.bindValue(":dataBinary", data.dataBinary);
    query.bindValue(":mime", data.mime);
    query.bindValue(":width", 0);
    query.bindValue(":height", 0);
    query.bindValue(":duration", 0);
    query.bindValue(":active", 1);
    query.bindValue(":isDirty", 0);
    query.bindValue(":attributeSourceUrl", "");
    query.bindValue(":attributeTimestamp", 0);
    query.bindValue(":attributeLatitude", 0);
    query.bindValue(":attributeLongitude", 0);
    query.bindValue(":attributeAltitude", 0);
    query.bindValue(":attributeCameraMake", "");
    query.bindValue(":attributeCameraModel", "");
    query.bindValue(":attributeClientWillIndex", "");
    query.bindValue(":attributeFileName", data.fileName);
    query.bindValue(":attributeAttachment", data.isAttachment?1:0);
    query.bindValue(":noteGuid", data.noteGuid);
    query.bindValue(":updateSequenceNumber", data.updateSequenceNumber);

    if(!query.exec())
    {
        qDebug()<<__FUNCTION__<<" line:"<<__LINE__<<" "<<query.lastError().text();
    }

    // 所有的usn必须由服务器下发
    sql = "select updateSequenceNumber from note_resource where guid = :guid;";
    QSqlQuery query2(*m_pDBMysql);
    query2.prepare(sql);
    query2.bindValue(":guid", data.guid);
    if(query2.exec() && query2.next())
    {
        int usn = query2.value(0).toInt();
        if (usn > data.updateSequenceNumber)
        {
            emit resourceUpdateSequenceNumberChanged(data.guid, usn);
        }
    }
}

void RemoteDatabaseConnection::ProcessDeleteResourceUploadTask(QString guid)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "delete FROM note_resource WHERE guid = :guid;";
    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":guid", guid);

    if(!query.exec())
    {
        qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
    }
}

void RemoteDatabaseConnection::OnAddTag(const SyncTagData& data)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_ADD, E_SYNC_OBJECT_TAG, data.guid, data.guid, data.sequence))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_TAG;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_ADD;
    uploadTaskLst[uploadTaskLst.count() - 1].tagDataBody = data;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }

}

void RemoteDatabaseConnection::OnUpdateTag(const SyncTagData& data)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_UPDATE, E_SYNC_OBJECT_TAG, data.guid, data.guid, data.sequence))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_TAG;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_UPDATE;
    uploadTaskLst[uploadTaskLst.count() - 1].tagDataBody = data;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }

}

void RemoteDatabaseConnection::OnDeleteTag(QString guid)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_DELETE, E_SYNC_OBJECT_TAG, guid, guid, 0))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_TAG;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_DELETE;
    uploadTaskLst[uploadTaskLst.count() - 1].tagDataBody.guid = guid;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::ProcessTagUploadTask(const SyncTagData& data)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "CALL InsertOrUpdateTag(:guid, "
        ":name, "
        ":isDirty, "
        ":sequence);";

    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":guid", data.guid);
    query.bindValue(":name",data.name);
    query.bindValue(":isDirty", 0);
    query.bindValue(":sequence",data.sequence);
    if(!query.exec())
    {
        qDebug()<<__FUNCTION__<<" line:"<<__LINE__<<" "<<query.lastError().text();
    }

    // 所有的usn必须由服务器下发
    sql = "select sequence from tag where guid = :guid;";
    QSqlQuery query1(*m_pDBMysql);
    query1.prepare(sql);
    query1.bindValue(":guid", data.guid);
    if(query1.exec() && query1.next())
    {
        int usn = query1.value(0).toInt();
        if (usn > data.sequence)
        {
            // 需要把上传队列中剩余的这个笔记的usn全部更新，不然接下去的就传不上去了
            for (int i = 0; i < uploadTaskLst.size(); i++)
            {
                if (uploadTaskLst.at(i).objectType != E_SYNC_OBJECT_TAG ||
                    uploadTaskLst.at(i).tagDataBody.guid != data.guid)
                {
                    continue;
                }

                uploadTaskLst[i].tagDataBody.sequence = usn;
            }// end for (int i = 0; i < uploadTaskLst.size(); i++)
            emit tagUpdateSequenceNumberChanged(data.guid, usn);
        }
    }
}

void RemoteDatabaseConnection::ProcessDeleteTagUploadTask(QString guid)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "CALL DeleteTag(:guid);";
    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":guid", guid);

    if(!query.exec())
    {
        qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
    }
}

void RemoteDatabaseConnection::beforeMainThreadStop()
{
    m_bStopThreadAfterEmptyList = true;
    if (uploadTaskLst.isEmpty())
    {
        QThread::currentThread()->quit();
    }
}

void RemoteDatabaseConnection::onDeleteNoteTagByNoteGuid(QString noteGuid)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_DELETE, E_SYNC_OBJECT_NOTETAG, noteGuid, noteGuid, 0))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    SyncNoteTagData data;
    data.noteGuid = noteGuid;

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_NOTETAG;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_DELETE;
    uploadTaskLst[uploadTaskLst.count() - 1].noteTagDataBody = data;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::addNoteTags(QString noteGuid, QString tagGuid)
{
    if (!m_bInitializeSuccess)
    {
        return;
    }

    if (taskExistInDownloadlist(E_OPER_ADD, E_SYNC_OBJECT_NOTETAG, noteGuid, tagGuid, 0))
    {
        return;
    }

    bool bRestartTimer = uploadTaskLst.isEmpty();

    SyncNoteTagData data;
    data.noteGuid = noteGuid;
    data.tagGuid = tagGuid;

    uploadTaskLst.push_back(UploadTask());
    uploadTaskLst[uploadTaskLst.count() - 1].objectType = E_SYNC_OBJECT_NOTETAG;
    uploadTaskLst[uploadTaskLst.count() - 1].operType = E_OPER_ADD;
    uploadTaskLst[uploadTaskLst.count() - 1].noteTagDataBody = data;

    if (bRestartTimer)
    {
        uploadTaskTimer->start();
    }
}

void RemoteDatabaseConnection::ProcessNoteTagUploadTask(const SyncNoteTagData& data)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);

    QString sql = "CALL InsertOrUpdateNotetags(:noteGuid, :tagGuid);";
    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":noteGuid", data.noteGuid);
    query.bindValue(":tagGuid",data.tagGuid);

    if(!query.exec())
    {
        qDebug()<<__FUNCTION__<<" line:"<<__LINE__<<" "<<query.lastError().text();
    }

}

void RemoteDatabaseConnection::ProcessDeleteNoteTagUploadTask(QString noteGuid)
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "DELETE FROM notetags WHERE noteGuid = :noteGuid;";
    if (!query.prepare(sql))
    {
        return;
    }

    query.bindValue(":noteGuid", noteGuid);

    if(!query.exec())
    {
        qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
    }
}

void RemoteDatabaseConnection::OnDownloadTimerTimeout()
{
    if (!uploadTaskLst.isEmpty() || !downloadedTask.isEmpty())
    {
        return;
    }

    qint64 beginTime = QDateTime::currentMSecsSinceEpoch();
    emit BeginSynchronizing();
    downloadNotebookList();
    downloadResourceList();
    downloadNoteList();
    downloadTagList();
    downloadNoteTagList();
    sortDownloadTastList();
    sendDownloadTast();

    int msecsUsed = QDateTime::currentMSecsSinceEpoch() - beginTime;
    if (msecsUsed < 5*1000)
    {
        //  如果时间太短的话，就强制让界面显示同步状态维持10秒
        QTimer::singleShot( 5*1000 - msecsUsed, this, SIGNAL(FinishSynchronizing()));
    }
    else
    {
        emit FinishSynchronizing();
    }
}

bool RemoteDatabaseConnection::taskExistInDownloadlist(ESyncOperationType operType, ESyncObjectType objectType, QString guid1, QString guid2, int usn)
{
    if (downloadedTask.isEmpty())
    {
        return false;
    }

    for (int i = 0; i < downloadedTask.size() ; i++)
    {
        if (downloadedTask.at(i).operType != operType ||
            downloadedTask.at(i).objectType != objectType )
        {
            continue;
        }

        if (objectType == E_SYNC_OBJECT_NOTEBOOK &&
            downloadedTask.at(i).notebookDataBody.guid == guid1 &&
            downloadedTask.at(i).notebookDataBody.sequence == usn)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_NOTE &&
            downloadedTask.at(i).noteDataBody.guid == guid1 &&
            downloadedTask.at(i).noteDataBody.updateSequenceNumber == usn)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_TAG &&
            downloadedTask.at(i).tagDataBody.guid == guid1 &&
            downloadedTask.at(i).tagDataBody.sequence == usn)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_TAG &&
            downloadedTask.at(i).tagDataBody.guid == guid1 &&
            downloadedTask.at(i).tagDataBody.sequence == usn)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_RESOURCE &&
            downloadedTask.at(i).resourceDataBody.guid == guid1 &&
            downloadedTask.at(i).resourceDataBody.updateSequenceNumber == usn)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_NOTETAG &&
            downloadedTask.at(i).noteTagDataBody.noteGuid == guid1 &&
            downloadedTask.at(i).noteTagDataBody.tagGuid == guid2)
        {
            return true;
        }
    }// end for (int i = 0; i < downloadedTask.size() ; i++)

    return false;
}

void RemoteDatabaseConnection::downloadNotebookList()
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "select guid, name, serviceCreated, serviceUpdated, defaultNotebook, sequence from notebook";
    if (!query.exec(sql))
    {
         qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
         return;
    }

    QList<QString> serverGuidLst;
    while (query.next())
    {
        int fieldNo = query.record().indexOf("guid");
        QString guid = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("sequence");
        int sequence = query.value(fieldNo).toInt();

        serverGuidLst << guid;

        DownloadedTask task;
        task.objectType = E_SYNC_OBJECT_NOTEBOOK;
        if (!listManager->GetNoteBookByGuid(guid))
        {
            task.operType = E_OPER_ADD;
        }
        else if (listManager->GetNoteBookByGuid(guid)->getUpdateSequenceNum() < sequence)
        {
            task.operType = E_OPER_UPDATE;
        }
        else
        {
            continue;
        }

        task.notebookDataBody.guid = guid;
        task.notebookDataBody.sequence = sequence;

        fieldNo = query.record().indexOf("name");
        task.notebookDataBody.name = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("serviceCreated");
        task.notebookDataBody.serviceCreated = query.value(fieldNo).toLongLong();

        fieldNo = query.record().indexOf("serviceUpdated");
        task.notebookDataBody.serviceUpdated = query.value(fieldNo).toLongLong();

        fieldNo = query.record().indexOf("defaultNotebook");
        task.notebookDataBody.defaultNotebook = query.value(fieldNo).toBool();

        if (!taskExistInUploadlist(task.operType, task.objectType, guid, guid))
        {
            downloadedTask << task;
        }
    }// end while (query.next())

    QList<Notebook*>& books = listManager->getNotebookIndex();
    for (int i = 0; i < books.size(); i++)
    {
        if (!books.at(i))
        {
            continue;
        }

        QString guid = books.at(i)->getGuid();

        if (serverGuidLst.contains(guid))
        {
            continue;
        }

        DownloadedTask task;
        task.objectType = E_SYNC_OBJECT_NOTEBOOK;
        task.operType = E_OPER_DELETE;
        task.notebookDataBody.guid = guid;
        downloadedTask.push_back(task);
    }// end for (int i = 0; i < books.size(); i++)
}

void RemoteDatabaseConnection::downloadResourceList()
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query1(*m_pDBMysql);
    QString sql = "select guid from note_resource;";
    if (!query1.exec(sql))
    {
        qDebug()<<__LINE__<<__FUNCTION__<<" "<<query1.lastError().text();
        return;
    }

    QList<QString> serverGuidLst;
    while (query1.next())
    {
        QString guid = query1.value(0).toString();
        if (!listManager->isResourceExist(guid))
        {
            serverGuidLst << guid;
        }
    }// end while (query.next())

    sql = "select dataHash, dataBinary, mime, attributeFileName, attributeAttachment, noteGuid, updateSequenceNumber from note_resource WHERE guid=:guid;";
    foreach(QString guid, serverGuidLst)
    {
        QSqlQuery query(*m_pDBMysql);
        query.prepare(sql);
        query.bindValue(":guid", guid);
        if (!query.exec() || !query.next())
        {
            qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
            continue;
        }

        DownloadedTask task;
        task.objectType = E_SYNC_OBJECT_RESOURCE;
        task.operType = E_OPER_ADD;
        task.resourceDataBody.guid = guid;
        downloadedTask.push_back(task);

        int fieldNo = query.record().indexOf("dataBinary");
        downloadedTask[downloadedTask.count() - 1].resourceDataBody.dataBinary = query.value(fieldNo).toByteArray();

        fieldNo = query.record().indexOf("dataHash");
        downloadedTask[downloadedTask.count() - 1].resourceDataBody.hash = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("mime");
        downloadedTask[downloadedTask.count() - 1].resourceDataBody.mime = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("attributeFileName");
        downloadedTask[downloadedTask.count() - 1].resourceDataBody.fileName = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("attributeAttachment");
        downloadedTask[downloadedTask.count() - 1].resourceDataBody.isAttachment = query.value(fieldNo).toInt() == 1;

        fieldNo = query.record().indexOf("noteGuid");
        downloadedTask[downloadedTask.count() - 1].resourceDataBody.noteGuid = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("updateSequenceNumber");
        downloadedTask[downloadedTask.count() - 1].resourceDataBody.updateSequenceNumber = query.value(fieldNo).toInt();
    }// end foreach(QString guid, serverGuidLst)
}

void RemoteDatabaseConnection::downloadNoteList()
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "select guid, title, content, created, updated, deleted, active, isDirty, notebookGuid, updateSequenceNumber from note WHERE isExpunged = 0;";
    if (!query.exec(sql))
    {
        qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
        return;
    }

    QList<QString> serverGuidLst;
    while (query.next())
    {
        int fieldNo = query.record().indexOf("guid");
        QString guid = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("updateSequenceNumber");
        int updateSequenceNumber = query.value(fieldNo).toInt();

        serverGuidLst << guid;

        DownloadedTask task;
        task.objectType = E_SYNC_OBJECT_NOTE;
        if (!listManager->GetNoteByGuid(guid))
        {
            task.operType = E_OPER_ADD;
        }
        else if (listManager->GetNoteByGuid(guid)->getUpdateSequenceNum() < updateSequenceNumber)
        {
            task.operType = E_OPER_UPDATE;
        }
        else
        {
            continue;
        }

        task.noteDataBody.guid = guid;
        task.noteDataBody.updateSequenceNumber = updateSequenceNumber;

        fieldNo = query.record().indexOf("title");
        task.noteDataBody.title = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("content");
        task.noteDataBody.content = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("created");
        task.noteDataBody.created = query.value(fieldNo).toLongLong();

        fieldNo = query.record().indexOf("updated");
        task.noteDataBody.updated = query.value(fieldNo).toLongLong();

        fieldNo = query.record().indexOf("deleted");
        task.noteDataBody.deleted = query.value(fieldNo).toLongLong();

        fieldNo = query.record().indexOf("active");
        task.noteDataBody.active = query.value(fieldNo).toInt() == 1;

        fieldNo = query.record().indexOf("isDirty");
        task.noteDataBody.isDirty = query.value(fieldNo).toInt() == 1;

        fieldNo = query.record().indexOf("notebookGuid");
        task.noteDataBody.notebookGuid = query.value(fieldNo).toString();

        if (!taskExistInUploadlist(task.operType, task.objectType, guid, guid))
        {
            downloadedTask << task;
        }
    }// end while (query.next())

    QList<Note*>& notes = listManager->getMasterNoteIndex();
    for (int i = 0; i < notes.size(); i++)
    {
        if (!notes.at(i))
        {
            continue;
        }

        QString guid = notes.at(i)->getGuid();

        if (serverGuidLst.contains(guid))
        {
            continue;
        }

        DownloadedTask task;
        task.objectType = E_SYNC_OBJECT_NOTE;
        task.operType = E_OPER_DELETE;
        task.noteDataBody.guid = guid;
        downloadedTask.push_back(task);
    }// end for (int i = 0; i < books.size(); i++)
}

void RemoteDatabaseConnection::downloadTagList()
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "select guid, name, isDirty, sequence from tag;";
    if (!query.exec(sql))
    {
        qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
        return;
    }

    QList<QString> serverGuidLst;
    while (query.next())
    {
        int fieldNo = query.record().indexOf("guid");
        QString guid = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("sequence");
        int sequence = query.value(fieldNo).toInt();

        serverGuidLst << guid;

        DownloadedTask task;
        task.objectType = E_SYNC_OBJECT_TAG;
        if (!listManager->getTagByGuid(guid))
        {
            task.operType = E_OPER_ADD;
        }
        else if (listManager->getTagByGuid(guid)->getUpdateSequenceNum() < sequence)
        {
            task.operType = E_OPER_UPDATE;
        }
        else
        {
            continue;
        }

        task.tagDataBody.guid = guid;
        task.tagDataBody.sequence = sequence;

        fieldNo = query.record().indexOf("name");
        task.tagDataBody.name = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("isDirty");
        task.tagDataBody.isDirty = query.value(fieldNo).toInt() == 1;

        if (!taskExistInUploadlist(task.operType, task.objectType, guid, guid))
        {
            downloadedTask << task;
        }
    }// end while (query.next())

    QList<Tag *>& tags = listManager->getTagIndex();
    for (int i = 0; i < tags.size(); i++)
    {
        if (!tags.at(i))
        {
            continue;
        }

        QString guid = tags.at(i)->getGuid();

        if (serverGuidLst.contains(guid))
        {
            continue;
        }

        DownloadedTask task;
        task.objectType = E_SYNC_OBJECT_TAG;
        task.operType = E_OPER_DELETE;
        task.tagDataBody.guid = guid;
        downloadedTask.push_back(task);
    }// end for (int i = 0; i < books.size(); i++)
}

void RemoteDatabaseConnection::downloadNoteTagList()
{
    if (!m_pDBMysql)
    {
        return;
    }

    if (!m_pDBMysql->isOpen())
    {
        if (!m_pDBMysql->open())
        {
            return;
        }
    }

    QSqlQuery query(*m_pDBMysql);
    QString sql = "select noteGuid, tagGuid from notetags;";
    if (!query.exec(sql))
    {
        qDebug()<<__LINE__<<__FUNCTION__<<" "<<query.lastError().text();
        return;
    }

    QList<NoteTagsRecord> serverNoteTagLst;
    while (query.next())
    {
        int fieldNo = query.record().indexOf("noteGuid");
        QString noteGuid = query.value(fieldNo).toString();

        fieldNo = query.record().indexOf("tagGuid");
        QString tagGuid = query.value(fieldNo).toString();

        NoteTagsRecord record;
        record.noteGuid = noteGuid;
        record.tagGuid = tagGuid;
        serverNoteTagLst.push_back(record);
    }// end while (query.next())

    QList<NoteTagsRecord> localNoteTagLst = listManager->getAllNoteTags();

    foreach(NoteTagsRecord record, serverNoteTagLst)
    {
        if (!localNoteTagLst.contains(record))
        {
            DownloadedTask task;
            task.objectType = E_SYNC_OBJECT_NOTETAG;
            task.operType = E_OPER_ADD;
            task.noteTagDataBody.noteGuid = record.noteGuid;
            task.noteTagDataBody.tagGuid = record.tagGuid;
            downloadedTask.push_back(task);
        }
    }// end foreach(NoteTagsRecord record, serverNoteTagLst)

    foreach(NoteTagsRecord record, localNoteTagLst)
    {
        if (!serverNoteTagLst.contains(record))
        {
            DownloadedTask task;
            task.objectType = E_SYNC_OBJECT_NOTETAG;
            task.operType = E_OPER_DELETE;
            task.noteTagDataBody.noteGuid = record.noteGuid;
            task.noteTagDataBody.tagGuid = record.tagGuid;
            downloadedTask.push_back(task);
        }
    }// end foreach(NoteTagsRecord record, serverNoteTagLst)
}

QString getValidGuidFromDownloadedTask(const DownloadedTask& task)
{
    if (!task.notebookDataBody.guid.isEmpty())
    {
        return task.notebookDataBody.guid;
    }

    if (!task.noteDataBody.guid.isEmpty())
    {
        return task.noteDataBody.guid;
    }

    if (!task.resourceDataBody.guid.isEmpty())
    {
        return task.resourceDataBody.guid;
    }

    if (!task.tagDataBody.guid.isEmpty())
    {
        return task.tagDataBody.guid;
    }

    if (!task.noteTagDataBody.noteGuid.isEmpty())
    {
        return task.noteTagDataBody.noteGuid;
    }

    if (!task.noteTagDataBody.tagGuid.isEmpty())
    {
        return task.noteTagDataBody.tagGuid;
    }

    return QString();
}

bool downloadedTaskLessThan(const DownloadedTask &s1, const DownloadedTask &s2)
{
    if (s1.operType != E_OPER_DELETE && s2.operType == E_OPER_DELETE)
    {
        return true;
    }

    if (s1.operType == E_OPER_DELETE && s2.operType != E_OPER_DELETE)
    {
        return false;
    }

    if (s1.operType == E_OPER_ADD ||
        s1.operType == E_OPER_UPDATE)
    {
        int iS1ObjectPriority = 0;
        int iS2ObjectPriority = 0;

        switch(s1.objectType)
        {
        case E_SYNC_OBJECT_NOTEBOOK: iS1ObjectPriority = 5;break;
        case E_SYNC_OBJECT_RESOURCE: iS1ObjectPriority = 4;break;
        case E_SYNC_OBJECT_NOTE: iS1ObjectPriority = 3;break;
        case E_SYNC_OBJECT_TAG: iS1ObjectPriority = 2;break;
        case E_SYNC_OBJECT_NOTETAG: iS1ObjectPriority = 1;break;
        }

        switch(s2.objectType)
        {
        case E_SYNC_OBJECT_NOTEBOOK: iS2ObjectPriority = 5;break;
        case E_SYNC_OBJECT_RESOURCE: iS2ObjectPriority = 4;break;
        case E_SYNC_OBJECT_NOTE: iS2ObjectPriority = 3;break;
        case E_SYNC_OBJECT_TAG: iS2ObjectPriority = 2;break;
        case E_SYNC_OBJECT_NOTETAG: iS2ObjectPriority = 1;break;
        }

        if (iS1ObjectPriority != iS2ObjectPriority)
        {
            return iS1ObjectPriority > iS2ObjectPriority;
        }
    }

    if (s1.operType == E_OPER_DELETE)
    {
        int iS1ObjectPriority = 0;
        int iS2ObjectPriority = 0;

        switch(s1.objectType)
        {
        case E_SYNC_OBJECT_NOTEBOOK: iS1ObjectPriority = 1;break;
        case E_SYNC_OBJECT_RESOURCE: iS1ObjectPriority = 2;break;
        case E_SYNC_OBJECT_NOTE: iS1ObjectPriority = 3;break;
        case E_SYNC_OBJECT_TAG: iS1ObjectPriority = 4;break;
        case E_SYNC_OBJECT_NOTETAG: iS1ObjectPriority = 5;break;
        }

        switch(s2.objectType)
        {
        case E_SYNC_OBJECT_NOTEBOOK: iS2ObjectPriority = 1;break;
        case E_SYNC_OBJECT_RESOURCE: iS2ObjectPriority = 2;break;
        case E_SYNC_OBJECT_NOTE: iS2ObjectPriority = 3;break;
        case E_SYNC_OBJECT_TAG: iS2ObjectPriority = 4;break;
        case E_SYNC_OBJECT_NOTETAG: iS2ObjectPriority = 5;break;
        }

        if (iS1ObjectPriority != iS2ObjectPriority)
        {
            return iS1ObjectPriority > iS2ObjectPriority;
        }
    }// end if (s1.operType == E_OPER_DELETE)

    //同一种元素、类型的操作，如果是修改或者增加的话，修改在前。这个没有必然意义，就是定格顺序而已
    if (s1.operType != s2.operType)
    {
        return s1.operType > s2.operType;
    }

    //这个时候顺序意义就不大了。为了有统一的顺序，就取guid小的在前吧
    QString guid1 = getValidGuidFromDownloadedTask(s1);
    QString guid2 = getValidGuidFromDownloadedTask(s2);

    return guid1 > guid2;
}

void RemoteDatabaseConnection::sortDownloadTastList()
{
    indexOfNoneEmitedDownloadTask = 0;
    if (downloadedTask.isEmpty())
    {
        return;
    }
    qSort(downloadedTask.begin(), downloadedTask.end(), downloadedTaskLessThan);
}

void RemoteDatabaseConnection::sendDownloadTast()
{
    while(indexOfNoneEmitedDownloadTask < downloadedTask.size())
    {
        QList<DownloadedTask> lstToBeSend;
        lstToBeSend.push_back(downloadedTask.at(indexOfNoneEmitedDownloadTask));
        indexOfNoneEmitedDownloadTask++;
        for ( ;indexOfNoneEmitedDownloadTask < downloadedTask.size(); indexOfNoneEmitedDownloadTask++)
        {
            if ((lstToBeSend.back().operType == E_OPER_DELETE && downloadedTask.at(indexOfNoneEmitedDownloadTask).operType != E_OPER_DELETE) ||
                lstToBeSend.back().operType != E_OPER_DELETE && downloadedTask.at(indexOfNoneEmitedDownloadTask).operType == E_OPER_DELETE)
            {
                break;
            }

            if (lstToBeSend.back().objectType != downloadedTask.at(indexOfNoneEmitedDownloadTask).objectType)
            {
                break;
            }

            lstToBeSend.push_back(downloadedTask.at(indexOfNoneEmitedDownloadTask));
        }// end for ( ;indexOfNoneEmitedDownloadTask < downloadedTask.size(); indexOfNoneEmitedDownloadTask++)

        emit receiveDownloadedTask(lstToBeSend);
		Global::sleepByMSecs(500);
    }// end while(indexOfNoneEmitedDownloadTask < downloadedTask.size())

    downloadedTask.clear();
    indexOfNoneEmitedDownloadTask = 0;
    lastTimeOfStartDownload = QDateTime::currentMSecsSinceEpoch();
    downloadTaskTimer->start(UPDATE_INTERVAL_SEC*1000); //5分钟
}

bool RemoteDatabaseConnection::taskExistInUploadlist(ESyncOperationType operType, ESyncObjectType objectType, QString guid1, QString guid2)
{
    if (uploadTaskLst.isEmpty())
    {
        return false;
    }

    for (int i = 0; i < uploadTaskLst.size() ; i++)
    {
        if (uploadTaskLst.at(i).operType != operType ||
            uploadTaskLst.at(i).objectType != objectType )
        {
            continue;
        }

        if (objectType == E_SYNC_OBJECT_NOTEBOOK &&
            uploadTaskLst.at(i).notebookDataBody.guid == guid1)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_NOTE &&
            uploadTaskLst.at(i).noteDataBody.guid == guid1)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_TAG &&
            uploadTaskLst.at(i).tagDataBody.guid == guid1)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_TAG &&
            uploadTaskLst.at(i).tagDataBody.guid == guid1)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_RESOURCE &&
            uploadTaskLst.at(i).resourceDataBody.guid == guid1)
        {
            return true;
        }

        if (objectType == E_SYNC_OBJECT_NOTETAG &&
            uploadTaskLst.at(i).noteTagDataBody.noteGuid == guid1 &&
            uploadTaskLst.at(i).noteTagDataBody.tagGuid == guid2)
        {
            return true;
        }
    }// end for (int i = 0; i < uploadTaskLst.size() ; i++)

    return false;
}
