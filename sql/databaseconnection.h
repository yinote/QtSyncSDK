#ifndef DATABASECONNECTION_H
#define DATABASECONNECTION_H

#include <QString>
#include <QSqlDatabase>
#include "utilities/applicationlogger.h"
#include "tagtable.h"
#include "notetable.h"
#include "notebooktable.h"
#include "invalidxmltable.h"
#include "wordstable.h"
#include "systemicontable.h"

class DatabaseConnection
{
public:
    DatabaseConnection(ApplicationLogger* l);
    ~DatabaseConnection();
    bool dbSetup(QString url, QString indexUrl, QString resourceUrl, QString userid, QString userPassword, QString cypherPassword, QString ConnectionName);
    void dbShutdown();
    void executeSql(QString sql);
    void createTables();
    void createIndexTables();
    void createResourceTables();

    QSqlDatabase getConnection();
    QSqlDatabase getIndexConnection();
    QSqlDatabase getResourceConnection();

    //***************************************************************
    //* Table get methods
    //***************************************************************
    //TODO
    //    DeletedTable* getDeletedTable();
    TagTable* getTagTable();
    NoteTable* getNoteTable();
    NotebookTable* getNotebookTable();
    //    WatchFolderTable* getWatchFolderTable();
    WordsTable* getWordsTable();
    InvalidXMLTable* getInvalidXMLTable();
    //    SyncTable* getSyncTable();
    SystemIconTable* getSystemIconTable();

    //****************************************************************
    //* Begin/End transactions
    //****************************************************************
    void beginTransaction();
    void commitTransaction();

    //****************************************************************
    //* Check if a table exists
    //****************************************************************
    bool dbTableExists(QString name);

    //****************************************************************
    //* Check if a row in a table exists
    //****************************************************************
    bool dbTableColumnExists(QString tableName, QString columnName);

protected:
    void setupTables();

private:
    WordsTable*				wordsTable;
    TagTable*               tagTable;
    NotebookTable*			notebookTable;
    NoteTable*				noteTable;
    //    DeletedTable*				deletedTable;
    //    WatchFolderTable*			watchFolderTable;
    InvalidXMLTable*			invalidXMLTable;
    //    LinkedNotebookTable*		linkedNotebookTable;
    //    SharedNotebookTable*		sharedNotebookTable;
    //    InkImagesTable*				inkImagesTable;
    //    SyncTable*					syncTable;
    SystemIconTable*			systemIconTable;
    ApplicationLogger*          logger;
    QSqlDatabase				conn;
    QSqlDatabase                indexConn;
    QSqlDatabase				resourceConn;
    int                         id;
};

#endif // DATABASECONNECTION_H
