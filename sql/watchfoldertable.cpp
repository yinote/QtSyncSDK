#include "watchfoldertable.h"
#include "sql/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

WatchFolderTable::WatchFolderTable(ApplicationLogger *l, DatabaseConnection *d)
{
    logger = l;
    db = d;
}

void WatchFolderTable::createTable()
{
    QSqlQuery query(db->getConnection());
    logger->log(HIGH, "Creating table WatchFolder...");
    if (!query.exec("Create table WatchFolders (folder varchar primary key, notebook varchar,"
                    "keep integer, depth integer)"))
        logger->log(HIGH, "Table WatchFolders creation FAILED!!!");
}

// Drop the table
void WatchFolderTable::dropTable()
{
    QSqlQuery query(db->getConnection());
    query.exec("Drop table WatchFolders");
}

// Add an folder
void WatchFolderTable::addWatchFolder(QString folder, QString notebook, bool keep, int depth)
{
    if (exists(folder))
        expungeWatchFolder(folder);

    QSqlQuery query(db->getConnection());
    query.prepare("Insert Into WatchFolders (folder, notebook, keep, depth) "
                  "values (:folder, :notebook, :keep, :depth)");
    query.bindValue(":folder", folder);
    query.bindValue(":notebook", notebook);
    query.bindValue(":keep", keep?1:0);
    query.bindValue(":depth", depth);
    if (!query.exec())
    {
        logger->log(MEDIUM, "Insert into WatchFolder failed.");
    }
}

// Add an folder
bool WatchFolderTable::exists(QString folder)
{
    QSqlQuery query(db->getConnection());
    query.prepare("Select folder from WatchFolders where folder=:folder ");
    query.bindValue(":folder", folder);
    query.exec();
    if (!query.next())
        return false;
    else
        return true;
}

// remove an folder
void WatchFolderTable::expungeWatchFolder(QString folder)
{
    QSqlQuery query(db->getConnection());
    query.prepare("delete from WatchFolders where folder=:folder");
    query.bindValue(":folder", folder);
    if (!query.exec()) {
        logger->log(MEDIUM, "Expunge WatchFolder failed.");
        logger->log(MEDIUM, query.lastError().text());
    }
}

void WatchFolderTable::expungeAll()
{
    QSqlQuery query(db->getConnection());
    if (!query.exec("delete from WatchFolders"))
    {
        logger->log(MEDIUM, "Expunge all WatchFolder failed.");
        logger->log(MEDIUM, query.lastError().text());
    }
}

QList<WatchFolderRecord> WatchFolderTable::getAll()
{
    logger->log(HIGH, "Entering RWatchFolders.getAll");

    QList<WatchFolderRecord> list;
    QSqlQuery query(db->getConnection());

    query.exec("Select folder, (select name from notebook where guid = notebook), keep, depth from WatchFolders");
    while (query.next())
    {
        WatchFolderRecord record;
        record.folder = query.value(0).toString();
        record.notebook = query.value(1).toString();
        record.keep = query.value(2).toBool();
        record.depth = query.value(3).toInt();
        list << record;
    }

    logger->log(HIGH, "Leaving RWatchFolders.getAll");
    return list;

}

QString WatchFolderTable::getNotebook(QString dir)
{
    logger->log(HIGH, "Entering RWatchFolders.getNotebook");
    QSqlQuery query(db->getConnection());
    query.prepare("Select notebook from WatchFolders where folder=:dir");
    query.bindValue(":dir", dir);
    query.exec();
    QString response;
    while (query.next())
    {
        response = query.value(0).toString();
    }
    logger->log(HIGH, "Leaving RWatchFolders.getNotebook");
    return response;
}
