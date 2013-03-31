#include "wordstable.h"
#include "sql/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

WordsTable::WordsTable(ApplicationLogger *l, DatabaseConnection *d)
{
    logger = l;
    db = d;
}

void WordsTable::createTable()
{
    QSqlQuery query(db->getIndexConnection());
    logger->log(HIGH, "Creating table WORDS ...");
    if (!query.exec("create table words (word varchar, guid varchar, source varchar, weight int, primary key (word, guid, source));"))
    {
        logger->log(HIGH, "Table WORDS creation FAILED!!!");
        logger->log(HIGH, query.lastError().text());
    }
}

// Drop the table
void WordsTable::dropTable()
{
    QSqlQuery query(db->getIndexConnection());
    query.exec("drop table words");
}

// Count unindexed notes
int WordsTable::getWordCount()
{
    QSqlQuery query(db->getIndexConnection());
    query.exec("select count(*) from words");
    query.next();
    return query.value(0).toInt();
}

// Clear out the word index table
void WordsTable::clearWordIndex()
{
    QSqlQuery query(db->getIndexConnection());
    logger->log(HIGH, "DELETE FROM WORDS");

    bool check = query.exec("DELETE FROM WORDS");
    if (!check)
        logger->log(HIGH, "Table WORDS clear has FAILED!!!");
}

//********************************************************************************
//********************************************************************************
//* Support adding & deleting index words
//********************************************************************************
//********************************************************************************
void WordsTable::expungeFromWordIndex(QString guid, QString type)
{
    QSqlQuery deleteWords(db->getIndexConnection());
    if (!deleteWords.prepare("delete from words where guid=:guid and source=:source"))
    {
        logger->log(EXTREME, "Note SQL select prepare deleteWords has failed.");
        logger->log(MEDIUM, deleteWords.lastError().text());
    }

    deleteWords.bindValue(":guid", guid);
    deleteWords.bindValue(":source", type);
    deleteWords.exec();

}

// Expunge words
void WordsTable::expunge(QString guid)
{
    QSqlQuery deleteWords(db->getIndexConnection());
    if (!deleteWords.prepare("delete from words where guid=:guid")) {
        logger->log(EXTREME, "Word SQL select prepare expunge has failed.");
        logger->log(MEDIUM, deleteWords.lastError().text());
    }

    deleteWords.bindValue(":guid", guid);
    deleteWords.exec();
}

// Reindex a note
void WordsTable::addWordToNoteIndex(QString guid, QString word, QString type, int weight)
{
    QSqlQuery findWords(db->getIndexConnection());
    if (!findWords.prepare("Select weight from words where guid=:guid and source=:type and word=:word"))
    {
        logger->log(MEDIUM, "Prepare failed in addWordToNoteIndex()");
        logger->log(MEDIUM, findWords.lastError().text());
    }

    findWords.bindValue(":guid", guid);
    findWords.bindValue(":type", type);
    findWords.bindValue(":word", word);

    bool addNeeded = true;
    findWords.exec();
    // If we have a match, find out which has the heigher weight & update accordingly
    if (findWords.next())
    {
        int recordWeight = findWords.value(0).toInt();
        addNeeded = true;
        if (recordWeight < weight)
        {
            QSqlQuery updateWord(db->getIndexConnection());
            if (!updateWord.prepare("Update words set weight=:weight where guid=:guid and source=:type and word=:word"))
            {
                logger->log(MEDIUM, "Prepare failed for find words in addWordToNoteIndex()");
                logger->log(MEDIUM, findWords.lastError().text());
            }

            updateWord.bindValue(":weight", weight);
            updateWord.bindValue(":guid", guid);
            updateWord.bindValue(":type", type);
            updateWord.bindValue(":word",word);
            updateWord.exec();
        }
    }

    if (!addNeeded)
        return;

    QSqlQuery insertWords(db->getIndexConnection());
    if (!insertWords.prepare("Insert Into Words (word, guid, weight, source)"
                             " Values(:word, :guid, :weight, :type )"))
    {
        logger->log(EXTREME, "Note SQL select prepare checkWords has failed.");
        logger->log(MEDIUM, insertWords.lastError().text());
    }

    insertWords.bindValue(":word", word);
    insertWords.bindValue(":guid", guid);
    insertWords.bindValue(":weight", weight);
    insertWords.bindValue(":type", type);
    if (!insertWords.exec())
    {
        logger->log(MEDIUM, "Error inserting words into index: " +insertWords.lastError().text());
    }
}

// Get a list of GUIDs in the table
QList<QString> WordsTable::getGuidList()
{
    QSqlQuery query(db->getIndexConnection());
    logger->log(HIGH, "gedGuidList()");

    bool check = query.exec("Select distinct guid from words");
    if (!check)
        logger->log(HIGH, "Table WORDS select distinct guid has FAILED!!!");

    QList<QString> guids;
    while (query.next())
    {
        guids << query.value(0).toString();
    }
    return guids;
}
