#include "notetagstable.h"
#include "sql/databaseconnection.h"
#include <QSqlError>
#include <QSqlRecord>

NoteTagsTable::NoteTagsTable(ApplicationLogger *l, DatabaseConnection *d)
{
    logger = l;
    db = d;
    getNoteTagsQuery = NULL;
}

NoteTagsTable::~NoteTagsTable()
{
    delete getNoteTagsQuery;
}

void NoteTagsTable::createTable()
{
    QSqlQuery query(db->getConnection());
    // Create the NoteTag table
    logger->log(HIGH, "Creating table NoteTags...");
    if (!query.exec("Create table NoteTags (noteGuid varchar, "
                    "tagGuid varchar, primary key(noteGuid, tagGuid))"))
        logger->log(HIGH, "Table NoteTags creation FAILED!!!");

    if (!query.exec("create index NOTETAGS_TAG_INDEX on notetags (tagguid, noteguid);"))
        logger->log(HIGH, "Note NOTETAGS_TAG_INDEX index creation FAILED!!!");

    if (!query.exec("create index TAG_NOTEBOOK_INDEX on notetags (tagguid, noteguid);"))
        logger->log(HIGH, "Note TAG_NOTEBOOK_INDEX index creation FAILED!!!");
}

// Drop the table
void NoteTagsTable::dropTable()
{
    QSqlQuery query(db->getConnection());
    query.exec("drop table NoteTags");
}

// Get a note tags by the note's Guid
QStringList NoteTagsTable::getNoteTags(QString noteGuid)
{
    QStringList tags;
    if (noteGuid.isEmpty())
        return tags;

    if (getNoteTagsQuery == NULL)
        prepareGetNoteTagsQuery();

    getNoteTagsQuery->bindValue(":guid", noteGuid);
    if (!getNoteTagsQuery->exec())
    {
        logger->log(EXTREME, "NoteTags SQL select has failed." + getNoteTagsQuery->lastError().text());
        return tags;
    }
    while (getNoteTagsQuery->next())
    {
        tags << getNoteTagsQuery->value(0).toString();
    }
    return tags;
}

// Get a list of notes by the tag guid
QStringList NoteTagsTable::getTagNotes(QString tagGuid)
{
    QStringList notes;
    if (tagGuid.isEmpty())
        return notes;

    QSqlQuery query(db->getConnection());
    query.prepare("Select NoteGuid from NoteTags where tagGuid = :guid");

    query.bindValue(":guid", tagGuid);
    if (!query.exec())
    {
        logger->log(EXTREME, "getTagNotes SQL select has failed." + query.lastError().text());
        return notes;
    }

    while (query.next())
    {
        notes << query.value(0).toString();
    }
    return notes;
}

void NoteTagsTable::prepareGetNoteTagsQuery()
{
    getNoteTagsQuery = new QSqlQuery(db->getConnection());
    getNoteTagsQuery->prepare("Select TagGuid from NoteTags where noteGuid = :guid");
}

// Get a note tags by the note's Guid
QList<NoteTagsRecord> NoteTagsTable::getAllNoteTags()
{
    QList<NoteTagsRecord> tags;

    QSqlQuery query(db->getConnection());
    if (!query.exec("Select TagGuid, NoteGuid from NoteTags"))
    {
        logger->log(EXTREME, "NoteTags SQL select has failed." + query.lastError().text());
        return tags;
    }

    while (query.next())
    {
        NoteTagsRecord record;
        record.tagGuid = query.value(0).toString();
        record.noteGuid = query.value(1).toString();
        tags << record;
    }
    return tags;
}

// Check if a note has a specific tag already
bool NoteTagsTable::checkNoteNoteTags(QString noteGuid, QString tagGuid)
{
    if (noteGuid.isEmpty() ||
            tagGuid.isEmpty())
        return false;

    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Select "
                          "NoteGuid, TagGuid from NoteTags where noteGuid = :noteGuid and tagGuid = :tagGuid");
    if (!check)
        logger->log(EXTREME, "checkNoteTags SQL prepare has failed.");

    query.bindValue(":noteGuid", noteGuid);
    query.bindValue(":tagGuid", tagGuid);
    check = query.exec();

    if (!check)
    {
        logger->log(EXTREME, "checkNoteTags SQL select has failed." + query.lastError().text());
        return false;
    }

    if (query.next())
    {
        return true;
    }
    return false;
}

// Save Note Tags
void NoteTagsTable::saveNoteTag(QString noteGuid, QString tagGuid)
{
    if (noteGuid.isEmpty() || tagGuid.isEmpty())
    {
        return;
    }

    bool check;
    QSqlQuery query(db->getConnection());

    check = query.prepare("Insert Into NoteTags (noteGuid, tagGuid) "
                          "Values("
                          ":noteGuid, :tagGuid)");
    if (!check)
        logger->log(EXTREME, "Note SQL insert prepare has failed.");

    query.bindValue(":noteGuid", noteGuid);
    query.bindValue(":tagGuid", tagGuid);

    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "NoteTags Table insert failed." + query.lastError().text());
    }

    check = query.prepare("Update Note set isDirty=1 where guid=:guid");
    if (!check)
        logger->log(EXTREME, "RNoteTagsTable.saveNoteTag prepare has failed." + query.lastError().text());
    query.bindValue(":guid", noteGuid);
    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "RNoteTagsTable.saveNoteTag has failed to set note as dirty." + query.lastError().text());
        return;
    }
}


// Delete a note's tags
void NoteTagsTable::deleteNoteTag(QString noteGuid)
{
    bool check;
    QSqlQuery query(db->getConnection());
    check = query.prepare("Delete from NoteTags where noteGuid = :noteGuid");
    if (!check)
        logger->log(EXTREME, "Note SQL delete prepare has failed." + query.lastError().text());

    query.bindValue(":noteGuid", noteGuid);
    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "NoteTags Table delete failed." + query.lastError().text());
        return;
    }
}

// Get a note tag counts
QList<QPair<QString,int> > NoteTagsTable::getTagCounts()
{
    QList<QPair<QString,int> > counts;
    QSqlQuery query(db->getConnection());
    if (!query.exec("select tagguid, count(noteguid) from notetags group by tagguid;"))
    {
        logger->log(EXTREME, "NoteTags SQL getTagCounts has failed." + query.lastError().text());
        return counts;
    }

    while (query.next())
    {
        QPair<QString,int> newCount(query.value(0).toString(), query.value(1).toInt());
        counts << newCount;
    }

    return counts;
}
