#ifndef NOTETABLE_H
#define NOTETABLE_H

#include <QString>
#include <QList>
#include <QPair>
#include <QHash>
#include <QSqlQuery>
#include <QStringList>
#include "utilities/applicationlogger.h"
#include "sql/notetagstable.h"
#include "sql/NoteResourceTable.h"
#include "evernote/notemetadata.h"
#include "evernote/note.h"
#include "sql/RemoteDatabaseConnectionDef.h"

class NoteTable: public QObject
{
    Q_OBJECT
public:
    NoteTable(ApplicationLogger* l,DatabaseConnection* d);

    NoteTagsTable*          noteTagsTable;
    NoteResourceTable*		noteResourceTable;

    // Create the table
    void createTable();

    // Drop the table
    void dropTable();

    void receiveDownloadTask(const DownloadedTask& task);

    // Save Note QList from Evernote
    void addNote(Note* n, bool isDirty);

    // Get a note by Guid
    Note* getNote(QString noteGuid, bool loadContent, bool loadResources, bool loadBinary, bool loadTags);

    // Get all notes
    QList<Note*> getAllNotes();

    // Update a note's title
    void updateNoteTitle(QString guid, QString title);

    // Update a note's creation date
    void updateNoteCreatedDate(QString guid, qint64 date);

    // Update a note's creation date
    void updateNoteAlteredDate(QString guid, qint64 date);

    // Update a note's creation date
    void updateNoteAuthor(QString guid, QString author);

    // Update a note's geo tags
    void updateNoteGeoTags(QString guid, double lon, double lat, double alt);

    // Update a note's creation date
    void updateNoteSourceUrl(QString guid, QString url);

    // Update the notebook that a note is assigned to
    void updateNoteNotebook(QString guid, QString notebookGuid, bool expungeFromRemote);

    // Update a note's title
    void updateNoteContent(QString guid, QString content);

    // Delete a note
    void deleteNote(QString guid, qint64 time);

    void restoreNote(QString guid);

    // Purge a note (actually delete it instead of just marking it deleted)
    void expungeNote(QString guid);

    // Purge all deleted notes;
    void expungeAllDeletedNotes();

    // Update the note sequence number
    void updateNoteSequence(QString guid, int sequence);

    // Fix CRLF problem that is on some notes
    QString fixCarriageReturn(QString Note);

    // Get all note meta information
    QHash<QString, NoteMetadata> getNotesMetaInformation();

    // Get note meta information
    NoteMetadata getNoteMetaInformation(QString guid);

    // Update a note content's hash.  This happens if a resource is edited outside of NN
    void updateResourceContentHash(QString guid, QString oldHash, QString newHash);

protected:
    // Setup queries for get to save time later
    void prepareQueries();

    // Get a note by Guid
    Note* mapNoteFromQuery(QSqlQuery* query, bool loadContent, bool loadResources, bool loadBinary, bool loadTags);

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*     db;
    int                     id;

    // Prepared Queries to improve speed
    QSqlQuery*              getQueryWithContent;
    QSqlQuery*              getQueryWithoutContent;
    QSqlQuery*              getAllQueryWithContent;
};

#endif // NOTETABLE_H
