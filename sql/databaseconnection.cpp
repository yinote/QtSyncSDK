#include "sql/databaseconnection.h"
#include "Global.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
#include <QFileInfo>

DatabaseConnection::DatabaseConnection(ApplicationLogger *l)
{
    logger = l;
    id = 0;

    tagTable = NULL;
}

bool DatabaseConnection::dbSetup(QString url, QString indexUrl, QString resourceUrl, QString userid, QString userPassword, QString cypherPassword, QString ConnectionName)
{
    logger->log(HIGH, "Entering DatabaseConnection.dbSetup " +id);

    QStringList drivers = QSqlDatabase::drivers();
    if (!drivers.contains("QSQLITE"))
    {
        logger->log(HIGH, "DataBase Driver Note FOUND ");
        return false;
    }

    setupTables();

    QString dbfileName = url + ".sqlite";
    bool dbExists = QFileInfo(dbfileName).exists();
    QString indexfileName = indexUrl + ".sqlite";
    bool indexDbExists = QFileInfo(indexfileName).exists();
    QString resourcefileName = resourceUrl + ".sqlite";
    bool resourceDbExists = QFileInfo(resourcefileName).exists();

    logger->log(HIGH, "Entering RDatabaseConnection.dbSetup");

    QString passwordString;
    if (cypherPassword.trimmed().isEmpty())
        passwordString = userPassword;
    else
        passwordString = cypherPassword+" "+userPassword;

    conn = QSqlDatabase::addDatabase("QSQLITE", ConnectionName);
    indexConn = QSqlDatabase::addDatabase("QSQLITE", ConnectionName+"index");
    resourceConn = QSqlDatabase::addDatabase("QSQLITE", ConnectionName+"resource");

    conn.setDatabaseName(dbfileName);
    conn.setUserName(userid);
    conn.setPassword(userPassword);

    indexConn.setDatabaseName(indexfileName);
    indexConn.setUserName(userid);
    indexConn.setPassword(userPassword);

    resourceConn.setDatabaseName(resourcefileName);
    resourceConn.setUserName(userid);
    resourceConn.setPassword(userPassword);

    if (conn.open())
    {
        logger->log(HIGH, "DataBase Open Successed: " + dbfileName);
    }
    else
    {
        logger->log(HIGH, "DataBase Open Failed: " + dbfileName);
        return false;
    }

    if (indexConn.open())
    {
        logger->log(HIGH, "Index DataBase Open Successed: " + indexfileName);
    }
    else
    {
        logger->log(HIGH, " Index DataBase Open Failed: " + indexfileName);
        conn.close();
        return false;
    }

    if (resourceConn.open())
    {
        logger->log(HIGH, "Resource DataBase Open Successed: " + indexfileName);
    }
    else
    {
        logger->log(HIGH, "Resource DataBase Open Failed: " + indexfileName);
        conn.close();
        indexConn.close();
        return false;
    }

    // If it doesn't exist and we are the main thread, then we need to create stuff.
    if (!dbExists)
    {
        createTables();
        Global::setAutomaticLogin(false);
    }
    
    if (!resourceDbExists)
    {
        createResourceTables();
    }

    if (!indexDbExists)
    {
        createIndexTables();
        executeSql("Update note set indexneeded=1");
    }

    logger->log(HIGH, "Leaving DatabaseConnection.dbSetup" +id);
    return true;
}

void DatabaseConnection::dbShutdown()
{
    logger->log(HIGH, "Entering RDatabaseConnection.dbShutdown");
    conn.close();
    logger->log(HIGH, "Leaving RDatabaseConnection.dbShutdown");

	indexConn.close();
	resourceConn.close();

}

void DatabaseConnection::setupTables()
{
    //TODO
    tagTable = new TagTable(logger, this);
    notebookTable = new NotebookTable(logger, this);
    noteTable = new NoteTable(logger, this);
    //    deletedTable = new DeletedTable(logger, this);
    //    watchFolderTable = new WatchFolderTable(logger, this);
    invalidXMLTable = new InvalidXMLTable(logger, this);
    wordsTable = new WordsTable(logger, this);
    //    syncTable = new SyncTable(logger, this);
    systemIconTable = new SystemIconTable(logger, this);
}

void DatabaseConnection::executeSql(QString sql)
{
    QSqlQuery query(getConnection());
    if (!query.exec(sql))
    {
        logger->log(EXTREME, "executeSql " +sql + " Failed:" +query.lastError().text());
    }
}

void DatabaseConnection::createTables()
{
    Global::setDatabaseVersion("0.95");
    Global::setAutomaticLogin(false);
    Global::saveCurrentNoteGuid("");

    getTagTable()->createTable();
    notebookTable->createTable(true);
    noteTable->createTable();
    //    deletedTable->createTable();
    //    searchTable->createTable();
    //    watchFolderTable->createTable();
    invalidXMLTable->createTable();
    //    syncTable->createTable();
    systemIconTable->createTable();
}

void DatabaseConnection::createIndexTables()
{
    wordsTable->createTable();
}

void DatabaseConnection::createResourceTables()
{
    noteTable->noteResourceTable->createTable();
}

QSqlDatabase DatabaseConnection::getConnection()
{
    return conn;
}
QSqlDatabase DatabaseConnection::getIndexConnection()
{
    return  indexConn;
}
QSqlDatabase DatabaseConnection::getResourceConnection()
{
    return resourceConn;
}

//***************************************************************
//* Table get methods
//***************************************************************
//DeletedTable* DatabaseConnection::getDeletedTable()
//{
//    return deletedTable;
//}

TagTable *DatabaseConnection::getTagTable()
{
    return tagTable;
}

NoteTable* DatabaseConnection::getNoteTable()
{
    return noteTable;
}

NotebookTable* DatabaseConnection::getNotebookTable()
{
    return notebookTable;
}

//WatchFolderTable *DatabaseConnection::getWatchFolderTable()
//{
//    return watchFolderTable;
//}

WordsTable *DatabaseConnection::getWordsTable()
{
    return wordsTable;
}

InvalidXMLTable *DatabaseConnection::getInvalidXMLTable()
{
    return invalidXMLTable;
}

//SyncTable *DatabaseConnection::getSyncTable()
//{
//    return syncTable;
//}

SystemIconTable *DatabaseConnection::getSystemIconTable()
{
    return systemIconTable;
}

//****************************************************************
//* Begin/End transactions
//****************************************************************
void DatabaseConnection::beginTransaction()
{
    commitTransaction();
    QSqlQuery query(getConnection());
    if (!query.exec("Begin Transaction"))
        logger->log(EXTREME, "Begin transaction has failed: " +query.lastError().text());

}

void DatabaseConnection::commitTransaction()
{
    QSqlQuery query(getConnection());
    if (!query.exec("Commit"))
        logger->log(EXTREME, "Transaction commit has failed: " + query.lastError().text());
}

//****************************************************************
//* Check if a table exists
//****************************************************************
bool DatabaseConnection::dbTableExists(QString name)
{
    QSqlQuery query(getConnection());
    query.prepare("select TABLE_NAME from INFORMATION_SCHEMA.TABLES where TABLE_NAME=:name");
    query.bindValue(":name", name.toUpper());
    query.exec();
    if (query.next())
        return true;
    else
        return false;
}

//****************************************************************
//* Check if a row in a table exists
//****************************************************************
bool DatabaseConnection::dbTableColumnExists(QString tableName, QString columnName)
{
    QSqlQuery query(getConnection());
    query.prepare("select TABLE_NAME from INFORMATION_SCHEMA.COLUMNS where TABLE_NAME=:name and COLUMN_NAME=:column");
    query.bindValue(":name", tableName.toUpper());
    query.bindValue(":column", columnName);
    query.exec();
    if (query.next())
        return true;
    else
        return false;
}

DatabaseConnection::~DatabaseConnection()
{
    resourceConn.close();
    indexConn.close();
    conn.close();
    logger->log(HIGH, "DataBase Closed");
    logger->log(HIGH, "Index DataBase Closed");
    logger->log(HIGH, "Resource DataBase Closed");
}
