#ifndef INVALIDXMLTABLE_H
#define INVALIDXMLTABLE_H
#include "utilities/applicationlogger.h"
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(DatabaseConnection)

class InvalidXMLTable
{
public:
    InvalidXMLTable(ApplicationLogger* l,DatabaseConnection* d);
    void createTable();
    void dropTable();
    void addAttribute(QString element, QString attribute);
    void addElement(QString element);
    QStringList getInvalidElements();
    QStringList getInvalidAttributeElements();
    QStringList getInvalidAttributes(QString element);
    bool elementExists(QString element) ;
    bool attributeExists(QString element, QString attribute);

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*      db;
};

#endif // INVALIDXMLTABLE_H
