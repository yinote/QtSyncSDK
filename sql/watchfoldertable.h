#ifndef WATCHFOLDERTABLE_H
#define WATCHFOLDERTABLE_H

#include "utilities/applicationlogger.h"
#include <QStringList>
#include <QString>
#include <QList>

QT_FORWARD_DECLARE_CLASS(DatabaseConnection)

struct WatchFolderRecord
{
    QString folder;
    QString notebook;
    bool    keep;
    int     depth;
};


class WatchFolderTable
{
public:
    WatchFolderTable(ApplicationLogger* l,DatabaseConnection* d);
    void createTable();

    // Drop the table
    void dropTable();

    // Add an folder
    void addWatchFolder(QString folder, QString notebook, bool keep, int depth);

    // Add an folder
    bool exists(QString folder);

    // remove an folder
    void expungeWatchFolder(QString folder);

    void expungeAll();

    QList<WatchFolderRecord> getAll();

    QString getNotebook(QString dir);

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*      db;
};

#endif // WATCHFOLDERTABLE_H
