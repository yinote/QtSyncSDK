#ifndef REMOTEDATABASECONNECTIONDEF_H
#define REMOTEDATABASECONNECTIONDEF_H

#include <QString>
#include <QObject>
#include <QMetaType>

enum ESyncOperationType
{
	E_OPER_Unknown = -1,
    E_OPER_ADD      = 0,
    E_OPER_UPDATE   = 1,
    E_OPER_DELETE   = 2
};

enum ESyncObjectType
{
	E_SYNC_OBJECT_Unknown = -1,
    E_SYNC_OBJECT_NOTE      = 0,
    E_SYNC_OBJECT_NOTEBOOK  = 1,
    E_SYNC_OBJECT_TAG       = 2,
    E_SYNC_OBJECT_NOTETAG   = 3,
    E_SYNC_OBJECT_RESOURCE  = 4,
};

struct SyncNoteData
{
    QString   guid;
    QString title;
    QString content;
    qint64  created;
    qint64  updated;
    qint64  deleted;
    bool    active;
    bool    isExpunged;
    bool    isDirty;
    QString   notebookGuid;
    int     updateSequenceNumber;
};

struct SyncNotebookData
{
    QString   guid;
    QString name;
    qint64  serviceCreated;
    qint64  serviceUpdated;
    QString stack;
    bool    defaultNotebook;
    int     sequence;
};

struct SyncTagData
{
    QString   guid;
    QString name;
    bool    isDirty;
    int     sequence;
};

struct SyncResourceData
{
    QString       guid;
    QByteArray  dataBinary;
    QString     hash;
    QString     mime;
    QString     fileName;
    QString       noteGuid;
    bool        isAttachment;
    int         updateSequenceNumber;
};

struct SyncNoteTagData
{
    QString noteGuid;
    QString tagGuid;
};

struct UploadTask
{
    ESyncOperationType  operType;
    ESyncObjectType     objectType;

    SyncNoteData        noteDataBody;
    SyncNotebookData    notebookDataBody;
    SyncTagData         tagDataBody;
    SyncResourceData    resourceDataBody;
    SyncNoteTagData     noteTagDataBody;
};

struct DownloadedTask
{
    ESyncOperationType  operType;
    ESyncObjectType     objectType;

    SyncNoteData        noteDataBody;
    SyncNotebookData    notebookDataBody;
    SyncTagData         tagDataBody;
    SyncResourceData    resourceDataBody;
    SyncNoteTagData     noteTagDataBody;
};

#endif // REMOTEDATABASECONNECTIONDEF_H
