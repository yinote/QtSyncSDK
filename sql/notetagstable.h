#ifndef NOTETAGSTABLE_H
#define NOTETAGSTABLE_H

#include <QString>
#include <QList>
#include <QPair>
#include <QHash>
#include <QSqlQuery>
#include <QStringList>
#include "evernote/tag.h"
#include "utilities/applicationlogger.h"
#include "sql/notetagsrecord.h"

QT_FORWARD_DECLARE_CLASS(DatabaseConnection)

class NoteTagsTable: public QObject
{
    Q_OBJECT

public:
    NoteTagsTable(ApplicationLogger* l,DatabaseConnection* d);
    ~NoteTagsTable();

    // Create the table
    void createTable();

    // Drop the table
    void dropTable();

    // Get a note tags by the note's Guid
    QStringList getNoteTags(QString noteGuid);

    // Get a list of notes by the tag guid
    QStringList getTagNotes(QString tagGuid);

    // Get a note tags by the note's Guid
    QList<NoteTagsRecord> getAllNoteTags();

    // Check if a note has a specific tag already
    bool checkNoteNoteTags(QString noteGuid, QString tagGuid);

    // Save Note Tags
    void saveNoteTag(QString noteGuid, QString tagGuid);

    // Delete a note's tags
    void deleteNoteTag(QString noteGuid);

    // Get a note tag counts
    QList<QPair<QString, int> > getTagCounts();

protected:
    void prepareGetNoteTagsQuery();

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*     db;
    QSqlQuery*              getNoteTagsQuery;
};

#endif // NOTETAGSTABLE_H
