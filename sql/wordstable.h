#ifndef WORDSTABLE_H
#define WORDSTABLE_H
#include "utilities/applicationlogger.h"
#include <QStringList>
#include <QString>
#include <QList>

QT_FORWARD_DECLARE_CLASS(DatabaseConnection)

class WordsTable
{
public:
    WordsTable(ApplicationLogger* l,DatabaseConnection* d);

    void createTable();

    // Drop the table
    void dropTable() ;

    // Count unindexed notes
    int getWordCount();

    // Clear out the word index table
    void clearWordIndex();

    //********************************************************************************
    //********************************************************************************
    //* Support adding & deleting index words
    //********************************************************************************
    //********************************************************************************
    void expungeFromWordIndex(QString guid, QString type);

    // Expunge words
    void expunge(QString guid);

    // Reindex a note
    void addWordToNoteIndex(QString guid, QString word, QString type, int weight);

    // Get a list of GUIDs in the table
    QList<QString> getGuidList();

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*      db;
};

#endif // WORDSTABLE_H
