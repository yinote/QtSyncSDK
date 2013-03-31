#include "notetable.h"
#include "sql/databaseconnection.h"
#include "Global.h"
#include "evernote/enmlconverter.h"
#include <QDateTime>
#include <QSqlError>
#include <QTextCodec>
#include <QSqlRecord>
NoteTable::NoteTable(ApplicationLogger *l, DatabaseConnection *d)
{
    logger = l;
    db = d;
    noteResourceTable = new NoteResourceTable(logger, db);
    noteTagsTable = new NoteTagsTable(logger, db);
    getQueryWithContent = NULL;
    getQueryWithoutContent = NULL;
    getAllQueryWithContent = NULL;
}

void NoteTable::createTable()
{
    QSqlQuery query(db->getConnection());
    logger->log(HIGH, "Creating table Note...");
    if (!query.exec("Create table Note (guid varchar primary key, "
                    "updateSequenceNumber integer, title varchar, content varchar, contentHash varchar, "
                    "contentLength integer, created integer, updated integer, deleted integer, "
                    "active integer, notebookGuid varchar, "
                    "attributeLatitude double, attributeLongitude double, attributeAltitude double,"
                    "attributeAuthor varchar, attributeSourceUrl varchar, "
                    "isExpunged Integer, "
                    "isDirty Integer)"))
        logger->log(HIGH, "Table Note creation FAILED!!!");

    if (!query.exec("CREATE INDEX unsynchronized_notes on note (isDirty desc, guid);"))
        logger->log(HIGH, "note unsynchronized_notes index creation FAILED!!!");

    if (!query.exec("create index NOTE_NOTEBOOK_INDEX on note (notebookguid, guid);"))
        logger->log(HIGH, "Note NOTE_NOTEBOOK_INDEX index creation FAILED!!!");

    if (!query.exec("create index NOTE_EXPUNGED_INDEX on note (isExpunged, guid);"))
        logger->log(HIGH, "Note NOTE_EXPUNGED_INDEX index creation FAILED!!!");

    noteTagsTable->createTable();
}

// Drop the table
void NoteTable::dropTable()
{
    QSqlQuery query(db->getConnection());
    query.exec("Drop table Note");
    noteTagsTable->dropTable();
    noteResourceTable->dropTable();
}

// Save Note QList from Evernote
void NoteTable::addNote(Note* n, bool isDirty)
{
    logger->log(EXTREME, "Inside addNote");
    if (n == NULL)
        return;

    QSqlQuery query(db->getConnection());
    QString sql = "Insert Into Note ("
        "guid, updateSequenceNumber, title, content, "
        "contentHash, contentLength, created, updated, deleted, active, notebookGuid, "
        "attributeLatitude, attributeLongitude, attributeAltitude, "
        "attributeAuthor, attributeSourceUrl, isExpunged, isDirty"
        ") Values("
        ":guid, :updateSequenceNumber, :title, :content, "
        ":contentHash, :contentLength, :created, :updated, :deleted, :active, :notebookGuid, "
        ":attributeLatitude, :attributeLongitude, :attributeAltitude, :attributeAuthor, :attributeSourceUrl, :isExpunged, :isDirty)";

    query.prepare(sql);
    QString strGuid = n->getGuid();
    query.bindValue(":guid", strGuid);
    query.bindValue(":updateSequenceNumber", n->getUpdateSequenceNum());
    query.bindValue(":title", n->getTitle());

    QString content;
    if (isDirty)
    {
        EnmlConverter enml(logger);
        content = enml.fixEnXMLCrap(enml.fixEnMediaCrap(n->getContent()));
        query.bindValue(":content", content);
    }
    else
    {
        content = n->getContent();
        query.bindValue(":content", n->getContent());
    }

    query.bindValue(":contentHash", QString(n->getContentHash()));
    query.bindValue(":contentLength", n->getContent().size());
    query.bindValue(":created", n->getCreated());
    query.bindValue(":updated", n->getUpdated());
    query.bindValue(":deleted", n->getDeleted());
    query.bindValue(":active", n->isActive()?1:0);
    query.bindValue(":notebookGuid", n->getNotebookGuid());

    if (n->getAttributes() != NULL)
    {
        query.bindValue(":attributeLatitude", n->getAttributes()->getLatitude());
        query.bindValue(":attributeLongitude", n->getAttributes()->getLongitude());
        query.bindValue(":attributeAltitude", n->getAttributes()->getAltitude());
        query.bindValue(":attributeAuthor", n->getAttributes()->getAuthor());
        query.bindValue(":attributeSourceUrl", n->getAttributes()->getSourceURL());
    }
    else
    {
        query.bindValue(":attributeLatitude", 0.0);
        query.bindValue(":attributeLongitude", 0.0);
        query.bindValue(":attributeAltitude", 0.0);
        query.bindValue(":attributeAuthor", "");
        query.bindValue(":attributeSourceUrl", "");
    }

    query.bindValue(":isExpunged", 0);
    query.bindValue(":isDirty", isDirty?1:0);

    if (!query.exec())
        logger->log(MEDIUM, query.lastError().text());

    // Save the note tags
    if (!n->getTagGuids().isEmpty())
    {
        for (int i=0; i<n->getTagGuids().size(); i++)
            noteTagsTable->saveNoteTag(n->getGuid(), n->getTagGuids().at(i));
    }

    logger->log(EXTREME, "Leaving addNote");
}

// Setup queries for get to save time later
void NoteTable::prepareQueries()
{
    if (getQueryWithContent == NULL)
    {
        getQueryWithContent = new QSqlQuery(db->getConnection());
        if (!getQueryWithContent->prepare("Select "
                                          "guid, updateSequenceNumber, title, "
                                          "created, updated, deleted, active, notebookGuid, "
                                          "attributeLatitude, attributeLongitude, attributeAltitude, "
                                          "attributeAuthor, attributeSourceUrl, "
                                          "content, contentHash, contentLength"
                                          " from Note where guid=:guid and isExpunged=0"))
        {
            logger->log(EXTREME, "Note SQL select prepare with content has failed.");
            logger->log(MEDIUM, getQueryWithContent->lastError().text());
        }
    }

    if (getQueryWithoutContent == NULL)
    {
        getQueryWithoutContent = new QSqlQuery(db->getConnection());
        if (!getQueryWithoutContent->prepare("Select "
                                             "guid, updateSequenceNumber, title, "
                                             "created, updated, deleted, active, notebookGuid, "
                                             "attributeLatitude, attributeLongitude, attributeAltitude, "
                                             "attributeAuthor, attributeSourceUrl"
                                             " from Note where guid=:guid and isExpunged=0"))
        {
            logger->log(EXTREME, "Note SQL select prepare without content has failed.");
            logger->log(MEDIUM, getQueryWithContent->lastError().text());
        }
    }

    if (getAllQueryWithContent == NULL)
    {
        getAllQueryWithContent = new QSqlQuery(db->getConnection());

        if (!getAllQueryWithContent->prepare("Select "
                                                "guid, updateSequenceNumber, title, "
                                                "created, updated, deleted, active, notebookGuid, "
                                                "attributeLatitude, attributeLongitude, attributeAltitude, "
                                                "attributeAuthor, attributeSourceUrl, "
                                                "content, contentHash, contentLength"
                                                " from Note where isExpunged = 0"))
        {
            logger->log(EXTREME, "Note SQL select prepare without content has failed.");
            logger->log(MEDIUM, getQueryWithoutContent->lastError().text());
        }
    }
}

// Get a note by Guid
Note* NoteTable::getNote(QString noteGuid, bool loadContent, bool loadResources, bool loadBinary, bool loadTags)
{
    if (noteGuid.isEmpty())
        return NULL;

    prepareQueries();

    QSqlQuery* query = NULL;
    if (loadContent)
    {
        query = getQueryWithContent;
    }
    else
    {
        query = getQueryWithoutContent;
    }

    QString strGuid = noteGuid;
    query->bindValue(":guid", strGuid);
    if (!query->exec())
    {
        logger->log(EXTREME, "Note SQL select exec has failed.");
        logger->log(MEDIUM, query->lastError().text());
        return NULL;
    }

    if (!query->next())
    {
        logger->log(EXTREME, "SQL Retrieve failed for note guid " +noteGuid + " in getNote()");
        logger->log(EXTREME, " -> " +query->lastError().text());
        logger->log(EXTREME, " -> " +query->lastError().text());
        return NULL;
    }

    Note* n = mapNoteFromQuery(query, loadContent, loadResources, loadBinary, loadTags);
    n->setContent(fixCarriageReturn(n->getContent()));
    return n;
}

// Get a note by Guid
Note* NoteTable::mapNoteFromQuery(QSqlQuery* query, bool loadContent, bool loadResources, bool loadBinary, bool loadTags)
{
    Note* n = new Note();
    NoteAttributes* na = new NoteAttributes();
    n->setAttributes(na);

    int fieldNo = query->record().indexOf("guid");
    n->setGuid(query->value(fieldNo).toString());
    
    fieldNo = query->record().indexOf("updateSequenceNumber");
    n->setUpdateSequenceNum(query->value(fieldNo).toInt());

    fieldNo = query->record().indexOf("title");
    n->setTitle(query->value(fieldNo).toString());

    fieldNo = query->record().indexOf("created");
    n->setCreated(query->value(fieldNo).toLongLong());

    fieldNo = query->record().indexOf("updated");
    n->setUpdated(query->value(fieldNo).toLongLong());

    fieldNo = query->record().indexOf("deleted");
    n->setDeleted(query->value(fieldNo).toLongLong());

    fieldNo = query->record().indexOf("active");
    n->setActive(query->value(fieldNo).toBool());

    fieldNo = query->record().indexOf("notebookGuid");
    n->setNotebookGuid(query->value(fieldNo).toString());

    fieldNo = query->record().indexOf("attributeLatitude");
    na->setLatitude(query->value(fieldNo).toFloat());

    fieldNo = query->record().indexOf("attributeLongitude");
    na->setLongitude(query->value(fieldNo).toFloat());

    fieldNo = query->record().indexOf("attributeAltitude");
    na->setAltitude(query->value(fieldNo).toFloat());

    fieldNo = query->record().indexOf("attributeAuthor");
    na->setAuthor(query->value(fieldNo).toString());

    fieldNo = query->record().indexOf("attributeSourceUrl");
    na->setSourceURL(query->value(fieldNo).toString());

    if (loadTags)
    {
        QStringList lstTagGuids = noteTagsTable->getNoteTags(n->getGuid());
        QStringList tagNames;
        QList<QString> tagGuids;
        TagTable* tagTable = db->getTagTable();
        for (int i=0; i< lstTagGuids.size(); i++)
        {
            QString currentGuid = lstTagGuids.at(i);
            tagGuids << currentGuid;
            Tag* tag = tagTable->getTag(currentGuid);
            if (tag != NULL && !tag->getName().isEmpty())
                tagNames << tag->getName();
            else
                tagNames << "";
        }

        n->setTagNames(tagNames);
        n->setTagGuids(tagGuids);
    }

    if (loadContent)
    {
        QTextCodec* codec = QTextCodec::codecForName("UTF-8");
        fieldNo = query->record().indexOf("content");
        QString unicode =  codec->fromUnicode(query->value(fieldNo).toString());

        n->setContent(unicode);

        fieldNo = query->record().indexOf("contentHash");
        QByteArray contentHash = query->value(fieldNo).toByteArray();
        if (!contentHash.isEmpty())
            n->setContentHash(contentHash);
        //n->setContentLength(query->value(19).toInt());
    }

    if (loadResources)
        n->setResources(noteResourceTable->getNoteResources(n->getGuid(), loadBinary));

    n->setContent(fixCarriageReturn(n->getContent()));
    return n;
}

// Update a note's title
void NoteTable::updateNoteTitle(QString guid, QString title)
{
    logger->log(HIGH, "Entering NoteTable.updateNoteTitle");
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Note set title=:title, isDirty=1 where guid=:guid AND isExpunged = 0");
    if (!check)
    {
        logger->log(EXTREME, "Update note title sql prepare has failed." + query.lastError().text());
    }
    query.bindValue(":title", title);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(EXTREME, "Update note title has failed."+ query.lastError().text());
    }

    logger->log(HIGH, "Leaving NoteTable.updateNoteTitle");
}

// Update a note's creation date
void NoteTable::updateNoteCreatedDate(QString guid, qint64 date)
{
    logger->log(HIGH, "Entering NoteTable.updateNoteCreatedDate");
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Note set created=:created, isDirty=1 where guid=:guid AND isExpunged = 0");
    if (!check)
    {
        logger->log(EXTREME, "Update note creation update sql prepare has failed."+ query.lastError().text());
    }

    query.bindValue(":created", date);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(EXTREME, "Update note creation date has failed."+ query.lastError().text());
    }
    logger->log(HIGH, "Leaving NoteTable.updateNoteCreatedDate");
}

// Update a note's creation date
void NoteTable::updateNoteAlteredDate(QString guid, qint64 date)
{
    logger->log(HIGH, "Entering NoteTable.updateNoteAlteredDate");
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Note set updated=:altered, isDirty=1 where guid=:guid AND isExpunged = 0");
    if (!check)
    {
        logger->log(EXTREME, "Update note altered sql prepare has failed."+ query.lastError().text());
    }

    query.bindValue(":altered", date);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(EXTREME, "Update note altered date has failed."+ query.lastError().text());
    }
    logger->log(HIGH, "Leaving NoteTable.updateNoteAlteredDate");
}

// Update a note's creation date
void NoteTable::updateNoteAuthor(QString guid, QString author)
{
    logger->log(HIGH, "Entering NoteTable.updateNoteAuthor");
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Note set attributeAuthor=:author, isDirty=1 where guid=:guid AND isExpunged = 0");
    if (!check)
    {
        logger->log(EXTREME, "Update note author sql prepare has failed."+ query.lastError().text());
    }

    query.bindValue(":author", author);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(EXTREME, "Update note author has failed."+ query.lastError().text());
    }
    logger->log(HIGH, "Leaving NoteTable.updateNoteAuthor");
}

// Update a note's geo tags
void NoteTable::updateNoteGeoTags(QString guid, double lon, double lat, double alt)
{
    logger->log(HIGH, "Entering NoteTable.updateNoteGeoTags");
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Note set attributeLongitude=:longitude, "
                               "attributeLatitude=:latitude, attributeAltitude=:altitude, isDirty=1 where guid=:guid AND isExpunged = 0");
    if (!check)
    {
        logger->log(EXTREME, "Update note author sql prepare has failed."+ query.lastError().text());
    }

    query.bindValue(":longitude", lon);
    query.bindValue(":latitude", lat);
    query.bindValue(":altitude", alt);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(EXTREME, "Update note geo tag has failed."+ query.lastError().text());
    }
    logger->log(HIGH, "Leaving NoteTable.updateNoteGeoTags");

}

// Update a note's creation date
void NoteTable::updateNoteSourceUrl(QString guid, QString url)
{
    logger->log(HIGH, "Entering NoteTable.updateNoteSourceUrl");
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Note set attributeSourceUrl=:url, isDirty=1 where guid=:guid AND isExpunged = 0");
    if (!check)
    {
        logger->log(EXTREME, "Update note url sql prepare has failed."+ query.lastError().text());
    }

    query.bindValue(":url", url);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(EXTREME, "Update note url has failed."+ query.lastError().text());
    }
    logger->log(HIGH, "Leaving NoteTable.updateNoteSourceUrl");
}

// Update the notebook that a note is assigned to
void NoteTable::updateNoteNotebook(QString guid, QString notebookGuid, bool expungeFromRemote)
{
    logger->log(HIGH, "Entering NoteTable.updateNoteNotebook");
    QString currentNotebookGuid;

    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Note set notebookGuid=:notebook, isDirty=1 where guid=:guid AND isExpunged = 0");
    if (!check)
    {
        logger->log(EXTREME, "Update note notebook sql prepare has failed." + query.lastError().text());
    }
    query.bindValue(":notebook", notebookGuid);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(EXTREME, "Update note notebook has failed."+ query.lastError().text());
        return;
    }

    logger->log(HIGH, "Leaving NoteTable.updateNoteNotebook");
}

// Update a note's title
void NoteTable::updateNoteContent(QString guid, QString content)
{
    logger->log(HIGH, "Entering NoteTable.updateNoteContent");
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Note set content=:content, updated=:updated, isDirty=1 where guid=:guid AND isExpunged = 0");
    if (!check)
    {
        logger->log(EXTREME, "Update note content sql prepare has failed." + query.lastError().text());
    }

    query.bindValue(":content", content);
    query.bindValue(":updated", QDateTime::currentMSecsSinceEpoch());
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(EXTREME, "Update note content has failed."+query.lastError().text());
    }

    logger->log(HIGH, "Leaving NoteTable.updateNoteContent");
}

// Delete a note
void NoteTable::deleteNote(QString guid, qint64 time)
{
    logger->log(HIGH, "Entering NoteTable.deleteNote");
    QSqlQuery query(db->getConnection());
    query.prepare("Update Note set deleted=:Time, active=0, isDirty=1 where guid=:guid AND isExpunged = 0");
    query.bindValue(":guid", guid);
    query.bindValue(":Time", time);
    if (!query.exec())
    {
        logger->log(MEDIUM, "Note delete failed." + query.lastError().text());
    }

    logger->log(HIGH, "Leaving NoteTable.deleteNote");
}

void NoteTable::restoreNote(QString guid)
{
    QSqlQuery query(db->getConnection());
    query.prepare("Update Note set deleted=:reset, active=1, isDirty=1 where guid=:guid AND isExpunged = 0");
    //		query.prepare("Update Note set deleted=0, active=1, isDirty=1 where guid=:guid");
    query.bindValue(":guid", guid);
    query.bindValue(":reset", 0);

    if (!query.exec())
    {
        logger->log(MEDIUM, "Note restore failed."+ query.lastError().text());
        return;
    }
}

// Purge a note (actually delete it instead of just marking it deleted)
void NoteTable::expungeNote(QString guid)
{
    logger->log(HIGH, "Entering NoteTable.expungeNote");
	QSqlQuery note(db->getConnection());
	QSqlQuery resources (db->getResourceConnection());
	QSqlQuery tags(db->getConnection());

	note.prepare("Update Note set isExpunged=1 where guid=:guid");
	resources.prepare("Update NoteResources set isExpunged=1 where noteGuid=:guid");
	tags.prepare("Delete from NoteTags where noteGuid=:guid");

	note.bindValue(":guid", guid);
	resources.bindValue(":guid", guid);
	tags.bindValue(":guid", guid);

	// Start purging notes.
	if (!note.exec())
	{
		logger->log(MEDIUM, "Purge from note failed."+note.lastError().text());
	}

	if (!resources.exec())
	{
		logger->log(MEDIUM, "Purge from resources failed."+resources.lastError().text());
	}

	if (!tags.exec())
	{
		logger->log(MEDIUM, "Note tags delete failed."+tags.lastError().text());
	}
    logger->log(HIGH, "Leaving NoteTable.expungeNote");
}

// Purge all deleted notes;
void NoteTable::expungeAllDeletedNotes()
{
    logger->log(HIGH, "Entering NoteTable.expungeAllDeletedNotes");
    QSqlQuery query(db->getConnection());
    query.exec("select guid, updateSequenceNumber from note where active = 0");
    QList<QString> guids;
    while (query.next())
    {
        guids << query.value(0).toString();
    }

    for (int i=0; i<guids.size(); i++)
    {
		expungeNote(guids.at(i));
    }
    logger->log(HIGH, "Leaving NoteTable.expungeAllDeletedNotes");
}

// Update the note sequence number
void NoteTable::updateNoteSequence(QString guid, int sequence)
{
    logger->log(HIGH, "Entering NoteTable.updateNoteSequence");
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Note set updateSequenceNumber=:sequence where guid=:guid;");

    query.bindValue(":sequence", sequence);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "Note sequence update failed."+query.lastError().text());
    }
    logger->log(HIGH, "Leaving NoteTable.updateNoteSequence");
}

// Get all notes
QList<Note*> NoteTable::getAllNotes()
{
    QList<Note*> notes;
    prepareQueries();

    if (getAllQueryWithContent == NULL)
        prepareQueries();
    QSqlQuery* query = getAllQueryWithContent;
    bool check = query->exec();
    if (!check)
        logger->log(EXTREME, "Notebook SQL retrieve has failed: "+ query->lastError().text());

    // Get a list of the notes
    while (query->next())
    {
        notes << mapNoteFromQuery(query, true, true, true, true);
    }
    return notes;
}

// Fix CRLF problem that is on some notes
QString NoteTable::fixCarriageReturn(QString note)
{
    if (note.isEmpty() || !Global::enableCarriageReturnFix)
        return note;

    QByteArray a0Hex = "a0";
    QString a0 = QByteArray::fromHex(a0Hex);
    note.replace("<div>"+a0+"</div>", "<div>&nbsp;</div>");
    return note.replace("<div/>", "<div>&nbsp;</div>");
}

// Get all note meta information
QHash<QString, NoteMetadata> NoteTable::getNotesMetaInformation()
{
    QHash<QString, NoteMetadata> returnValue;
    QSqlQuery query(db->getConnection());

    if (!query.exec("Select guid, isDirty from Note"))
        logger->log(EXTREME, "Note SQL retrieve has failed on getNoteMetaInformation" + query.lastError().text());

    // Get a list of the notes
    while (query.next())
    {
        NoteMetadata note;
        note.setGuid(query.value(0).toString());
        note.setDirty(query.value(1).toBool());
        returnValue.insert(note.getGuid(), note);
    }

    return returnValue;
}

// Get note meta information
NoteMetadata NoteTable::getNoteMetaInformation(QString guid)
{
    QSqlQuery query(db->getConnection());

    if (!query.prepare("Select guid, isDirty, from Note where guid=:guid"))
    {
        logger->log(EXTREME, "Note SQL retrieve has failed on getNoteMetaInformation. " + query.lastError().text());
        return NoteMetadata();
    }
    query.bindValue(":guid", guid);
    query.exec();

    // Get a list of the notes
    while (query.next())
    {
        NoteMetadata note;
        note.setGuid(query.value(0).toString());
        note.setDirty(query.value(1).toBool());
        return note;
    }

    return NoteMetadata();
}

// Update a note content's hash.  This happens if a resource is edited outside of NN
void NoteTable::updateResourceContentHash(QString guid, QString oldHash, QString newHash)
{
    Note* n = getNote(guid, true, false, false,false);
    int position = n->getContent().indexOf("<en-media");
    int endPos;
    for (;position>-1;)
    {
        endPos = n->getContent().indexOf(">", position+1);
        QString oldSegment = n->getContent().mid(position,endPos);
        int hashPos = oldSegment.indexOf("hash=\"");
        int hashEnd = oldSegment.indexOf("\"", hashPos+7);
        QString hash = oldSegment.mid(hashPos+6, hashEnd);
        if (hash.compare(oldHash, Qt::CaseInsensitive) == 0)
        {
            QString newSegment = oldSegment.replace(oldHash, newHash);
            QString content = n->getContent().mid(0,position) + newSegment +
                    n->getContent().mid(endPos);

            QSqlQuery query(db->getConnection());
            query.prepare("update note set isdirty=1, content=:content where guid=:guid");
            query.bindValue(":content", content);
            query.bindValue(":guid", n->getGuid());
            query.exec();
        }
        position = n->getContent().indexOf("<en-media", position+1);
    }
}

void NoteTable::receiveDownloadTask(const DownloadedTask& task)
{
    if (task.objectType != E_SYNC_OBJECT_NOTE)
    {
        return;
    }

    if (task.operType == E_OPER_DELETE)
    {
        return;
    }

    QSqlQuery querySelect(db->getConnection());
    QString sql = "select count(guid) from Note WHERE guid = :guid AND isExpunged = 0;";
    querySelect.prepare(sql);
    querySelect.bindValue("guid", task.noteDataBody.guid);
    if (!querySelect.exec() || !querySelect.next())
    {
        return;
    }

    const bool bExist = querySelect.value(0).toInt() != 0;
    if (!bExist)
    {
        sql = "Insert Into Note ("
            "guid, updateSequenceNumber, title, content, "
            "contentHash, contentLength, created, updated, deleted, active, notebookGuid, "
            "attributeLatitude, attributeLongitude, attributeAltitude, "
            "attributeAuthor, attributeSourceUrl, isExpunged, isDirty"
            ") Values("
            ":guid, :updateSequenceNumber, :title, :content, "
            ":contentHash, :contentLength, :created, :updated, :deleted, :active, :notebookGuid, "
            ":attributeLatitude, :attributeLongitude, :attributeAltitude, :attributeAuthor, :attributeSourceUrl, :isExpunged, :isDirty)";
    }
    else
    {
        sql = "Update Note set "
            "updateSequenceNumber=:updateSequenceNumber, title=:title, content=:content, "
            "contentHash=:contentHash, contentLength=:contentLength, created=:created, updated=:updated,"
            "deleted=:deleted, active=:active, notebookGuid=:notebookGuid, "
            "attributeLatitude=:attributeLatitude, attributeLongitude=:attributeLongitude, attributeAltitude=:attributeAltitude, "
            "attributeAuthor=:attributeAuthor, attributeSourceUrl=:attributeSourceUrl, isExpunged=:isExpunged, isDirty=:isDirty WHERE guid=:guid;";
    }

    QSqlQuery query(db->getConnection());
    query.prepare(sql);
    query.bindValue(":guid", task.noteDataBody.guid);
    query.bindValue(":updateSequenceNumber", task.noteDataBody.updateSequenceNumber);
    query.bindValue(":title", task.noteDataBody.title);
    query.bindValue(":content", task.noteDataBody.content);
    query.bindValue(":contentHash", "");
    query.bindValue(":contentLength", 0);
    query.bindValue(":created", task.noteDataBody.created);
    query.bindValue(":updated", task.noteDataBody.updated);
    query.bindValue(":deleted", task.noteDataBody.deleted);
    query.bindValue(":active", task.noteDataBody.active?1:0);
    query.bindValue(":notebookGuid", task.noteDataBody.notebookGuid);
    query.bindValue(":attributeLatitude", 0);
    query.bindValue(":attributeLongitude", 0);
    query.bindValue(":attributeAltitude", 0);
    query.bindValue(":attributeAuthor", "");
    query.bindValue(":attributeSourceUrl", "");
    query.bindValue(":isExpunged", 0);
    query.bindValue(":isDirty", task.noteDataBody.isDirty?1:0);
    if (!query.exec())
    {
        logger->log(MEDIUM, query.lastError().text());
    }
}
