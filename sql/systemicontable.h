#ifndef SYSTEMICONTABLE_H
#define SYSTEMICONTABLE_H

#include "utilities/applicationlogger.h"

QT_FORWARD_DECLARE_CLASS(DatabaseConnection)

class SystemIconTable
{
public:
    SystemIconTable(ApplicationLogger* l, DatabaseConnection* d);
    void createTable();
    void dropTable();
    QIcon getIcon(QString name, QString type);
    bool exists(QString name, QString type);

    // Set the notebooks custom icon
    void setIcon(QString name, QString rectype, QIcon icon, QString filetype);
    // Set the notebooks custom icon
    void addIcon(QString name, QString rectype, QIcon icon, QString filetype);

    // Set the notebooks custom icon
    void updateIcon(QString name, QString rectype, QIcon icon, QString filetype);

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*		db;
};

#endif // SYSTEMICONTABLE_H
