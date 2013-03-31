#include "systemicontable.h"
#include "sql/databaseconnection.h"
#include <QSqlQuery>
#include <QIcon>
#include <QSqlError>
#include <QBuffer>

SystemIconTable::SystemIconTable(ApplicationLogger *l, DatabaseConnection *d)
{
    logger = l;
    db = d;
}

// Create the table
void SystemIconTable::createTable()
{
    QSqlQuery query(db->getConnection());
    logger->log(HIGH, "Creating table SystemIcon...");
    if (!query.exec("Create table SystemIcon (name varchar, "
                    "type varchar, icon BLOB, primary key(name, type));"))
    {
        logger->log(HIGH, "Table SystemIcon creation FAILED!!! " + query.lastError().text());
    }
}

// Drop the table
void SystemIconTable::dropTable()
{
    QSqlQuery query(db->getConnection());
    query.exec("Drop table SystemIcon");
}


// Get the notebooks custom icon
QIcon SystemIconTable::getIcon(QString name, QString type)
{
    QSqlQuery query(db->getConnection());

    if (!query.prepare("Select icon from SystemIcon where name=:name and type=:type"))
        logger->log(EXTREME, "Error preparing system icon select.");

    query.bindValue(":name", name);
    query.bindValue(":type", type);

    if (!query.exec())
        logger->log(EXTREME, "Error finding system icon.");

    if (!query.next() || query.value(0).type() == QVariant::Invalid)
        return QIcon();

    QByteArray blob = query.value(0).toByteArray();
    QIcon icon = (QPixmap::fromImage(QImage::fromData(blob)));
    return icon;
}


// Get the notebooks custom icon
bool SystemIconTable::exists(QString name, QString type)
{
    QSqlQuery query(db->getConnection());

    if (!query.prepare("Select icon from SystemIcon where name=:name and type=:type"))
        logger->log(EXTREME, "Error preparing system icon select.");
    query.bindValue(":name", name);
    query.bindValue(":type", type);
    if (!query.exec())
        logger->log(EXTREME, "Error finding system icon.");
    if (!query.next() || query.value(0).type() == QVariant::Invalid)
        return false;
    return true;
}


// Set the notebooks custom icon
void SystemIconTable::setIcon(QString name, QString rectype, QIcon icon, QString filetype)
{
    if (exists(name, rectype))
        updateIcon(name, rectype, icon, filetype);
    else
        addIcon(name, rectype, icon, filetype);
}

// Set the notebooks custom icon
void SystemIconTable::addIcon(QString name, QString rectype, QIcon icon, QString filetype)
{
    QSqlQuery query(db->getConnection());
    if (icon.isNull())
    {
        return;
    }
    else
    {
        if (!query.prepare("Insert into SystemIcon (icon, name, type) values (:icon, :name, :type)"))
            logger->log(EXTREME, "Error preparing notebook icon select.");
        QBuffer buffer;
        if (!buffer.open(QIODevice::ReadWrite))
        {
            logger->log(EXTREME, "Failure to open buffer.  Aborting.");
            return;
        }

        QPixmap p = icon.pixmap(32, 32);
        QImage i = p.toImage();
        i.save(&buffer, filetype.toUpper().toLatin1());
        buffer.close();
        QByteArray b = buffer.buffer();
        if (!b.isNull() && !b.isEmpty())
            query.bindValue(":icon", b);
        else
            return;
    }
    query.bindValue(":name", name);
    query.bindValue(":type", rectype);
    if (!query.exec())
        logger->log(LOW, "Error setting system icon. " +query.lastError().text());
}

// Set the notebooks custom icon
void SystemIconTable::updateIcon(QString name, QString rectype, QIcon icon, QString filetype)
{
    QSqlQuery query(db->getConnection());
    if (icon.isNull())
    {
        if (!query.prepare("delete from SystemIcon where name=:name and type=:type"))
            logger->log(EXTREME, "Error preparing notebook icon select.");
    }
    else
    {
        if (!query.prepare("update SystemIcon set icon=:icon where name=:name and type=:type"))
            logger->log(EXTREME, "Error preparing notebook icon select.");

        QBuffer buffer;
        if (!buffer.open(QIODevice::ReadWrite))
        {
            logger->log(EXTREME, "Failure to open buffer.  Aborting.");
            return;
        }
        QPixmap p = icon.pixmap(32, 32);
        QImage i = p.toImage();
        i.save(&buffer, filetype.toUpper().toLatin1());
        buffer.close();
        QByteArray b = buffer.buffer();
        if (!b.isNull() && !b.isEmpty())
            query.bindValue(":icon", b);
        else
            return;
    }
    query.bindValue(":name", name);
    query.bindValue(":type", rectype);
    if (!query.exec())
        logger->log(LOW, "Error setting system icon. " +query.lastError().text());
}
