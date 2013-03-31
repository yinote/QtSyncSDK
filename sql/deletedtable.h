#ifndef DELETEDTABLE_H
#define DELETEDTABLE_H

#include <QString>
#include <QList>
#include <QPair>
#include <QHash>
#include <QSqlQuery>
#include <QStringList>
#include "utilities/applicationlogger.h"

QT_FORWARD_DECLARE_CLASS(DatabaseConnection);

class DeletedTable
{
public:
    DeletedTable(ApplicationLogger* l,DatabaseConnection* d);

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*     db;
};

#endif // DELETEDTABLE_H
