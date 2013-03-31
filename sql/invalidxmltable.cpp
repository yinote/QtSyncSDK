#include "invalidxmltable.h"
#include "sql/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

InvalidXMLTable::InvalidXMLTable(ApplicationLogger *l, DatabaseConnection *d)
{
    logger = l;
    db = d;
}

void InvalidXMLTable::createTable()
{
    QSqlQuery query(db->getConnection());
    logger->log(HIGH, "Creating table InvalidXML...");
    if (!query.exec("Create table InvalidXML (type varchar, element varchar, attribute varchar,primary key(type, element,attribute) );"))
        logger->log(HIGH, "Table InvalidXML creation FAILED!!!");
    //        query.clear();

    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'button', '');");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'embed', '');");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'fieldset', '');");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'form', '');");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'input', '');");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'label', '');");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'legend', '');");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'o:p', '')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'option', '')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'script', '')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'select', '')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ELEMENT', 'wbr', '')");

    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'a', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'a', 'done')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'a', 'id')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'a', 'onclick')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'a', 'onmousedown')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'a', 'title')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'div', 'id')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'dl', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'dl', 'id')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'dt', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'h1', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'h2', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'h3', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'h4', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'h5', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'img', 'gptag')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'li', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'ol', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'ol', 'id')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'p', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'p', 'id')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'p', 'span')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'accesskey')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'action')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'alt')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'bgcolor')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'checked')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'flashvars')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'for')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'height')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'id')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'maxlength')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'method')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'name')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'onblur')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'onchange')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'aclick')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'onsubmit')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'quality')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'selected')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'src')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'target')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'type')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'value')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'width')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'span', 'wmode')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'table', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'td', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'tr', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'ul', 'class')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'en-media', 'oncontextmenu')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'en-media', 'title')");
    query.exec("Insert into InvalidXML (type, element, attribute) values ('ATTRIBUTE', 'en-todo', 'style')");
}

// Drop the table
void InvalidXMLTable::dropTable()
{
    QSqlQuery query(db->getConnection());
    query.exec("Drop table InvalidXML");
}

// Add an item to the table
void InvalidXMLTable::addAttribute(QString element, QString attribute)
{
    if (attributeExists(element,attribute))
        return;

    QSqlQuery query(db->getConnection());
    query.prepare("Insert Into InvalidXML (type, element, attribute) Values('ATTRIBUTE', :element, :attribute)");
    query.bindValue(":element", element);
    query.bindValue(":attribute", attribute);
    if (!query.exec())
    {
        logger->log(MEDIUM, "Insert Attribute into invalidXML failed.");
        logger->log(MEDIUM, query.lastError().text());
    }
}

// Add an item to the table
void InvalidXMLTable::addElement(QString element)
{
    if (elementExists(element))
        return;
    QSqlQuery query(db->getConnection());
    query.prepare("Insert Into InvalidXML (type, element) Values('ELEMENT', :element)");
    query.bindValue(":element", element);
    if (!query.exec()) {
        logger->log(MEDIUM, "Insert Element into invalidXML failed.");
        logger->log(MEDIUM, query.lastError().text());
    }
}

// get invalid elements
QStringList InvalidXMLTable::getInvalidElements()
{
    QStringList elements;
    QSqlQuery query(db->getConnection());
    if (!query.exec("Select element from InvalidXML where type = 'ELEMENT'"))
    {
        logger->log(MEDIUM, "getInvalidElement from invalidXML failed.");
        logger->log(MEDIUM, query.lastError().text());
        return elements;
    }

    while (query.next())
    {
        elements << query.value(0).toString();
    }
    return elements;
}

// get invalid elements
QStringList InvalidXMLTable::getInvalidAttributeElements()
{
    QStringList elements;

    QSqlQuery query(db->getConnection());
    if (!query.exec("Select distinct element from InvalidXML where type = 'ATTRIBUTE'")) {
        logger->log(MEDIUM, "getInvalidElement from invalidXML failed.");
        logger->log(MEDIUM, query.lastError().text());
        return elements;
    }

    while (query.next()) {
        elements << query.value(0).toString();
    }
    return elements;
}

// get invalid attributes for a given element
QStringList InvalidXMLTable::getInvalidAttributes(QString element)
{
    QStringList elements;
    QSqlQuery query(db->getConnection());
    query.prepare("Select attribute from InvalidXML where type = 'ATTRIBUTE' and element = :element");
    query.bindValue(":element", element);
    if (!query.exec())
    {
        logger->log(MEDIUM, "getInvalidElement from invalidXML failed.");
        logger->log(MEDIUM, query.lastError().text());
        return elements;
    }

    while (query.next())
    {
        elements << query.value(0).toString();
    }
    return elements;
}

// Determine if an element already is in the table
bool InvalidXMLTable::elementExists(QString element)
{
    QSqlQuery query(db->getConnection());
    query.prepare("Select element from InvalidXML where type='ELEMENT' and element=:element");
    query.bindValue(":element", element);
    if (!query.exec())
    {
        logger->log(MEDIUM, "elementExists in invalidXML failed.");
        logger->log(MEDIUM, query.lastError().text());
    }
    if (query.next())
        return true;
    else
        return false;
}

// Determine if an element already is in the table
bool InvalidXMLTable::attributeExists(QString element, QString attribute)
{
    QSqlQuery query(db->getConnection());
    query.prepare("Select element from InvalidXML where type='ATTRIBUTE' and element=:element and attribute=:attribute");
    query.bindValue(":element", element);
    query.bindValue(":attribute", attribute);
    if (!query.exec()) {
        logger->log(MEDIUM, "attributeExists in invalidXML failed.");
        logger->log(MEDIUM, query.lastError().text());
    }
    if (query.next())
        return true;
    else
        return false;
}
