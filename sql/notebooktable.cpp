#include "notebooktable.h"
#include "sql/databaseconnection.h"
#include "Global.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>
#include <QBuffer>
#include <QImage>
#include <QPixmap>
#include <QIcon>
NotebookTable::NotebookTable(ApplicationLogger *l, DatabaseConnection *d)
{
    logger = l;
    db = d;
    dbName = "Notebook";
}

NotebookTable::NotebookTable(ApplicationLogger *l, DatabaseConnection *d, QString name)
{
    logger = l;
    db = d;
    dbName = name;
}

void NotebookTable::createTable(bool addDefaulte)
{
    QSqlQuery query(db->getConnection());
    logger->log(HIGH, "Creating table "+dbName+"...");
    if (!query.exec("Create table "+dbName+" (guid varchar primary key, " +
                    "sequence integer, " +
                    "name varchar, "+
                    "defaultNotebook integer, "+
                    "serviceCreated integer, " +
                    "serviceUpdated integer, "+
                    "isDirty integer, stack varchar, icon BLOB,isExpunged Integer, "
                    "NARROW_SORT_ORDER integer, "
                    "WIDE_SORT_ORDER integer, "
                    "WIDE_SORT_COLUMN integer, "
                    "NARROW_SORT_COLUMN integer)"))
	{
        logger->log(HIGH, "Table "+dbName+" creation FAILED!!!" + query.lastError().text());
	}

    // Setup an initial notebook
    query = QSqlQuery(db->getConnection());

    query.prepare("Insert Into "+dbName+" (guid, sequence, name, defaultNotebook, "
                  +"serviceCreated, serviceUpdated,"
                  + "isDirty,isExpunged ) Values("
                  +":guid, :sequence, :name, :defaultNotebook,  "
                  +":serviceCreated, :serviceUpdated, "
                  +":isDirty,0)");

    query.bindValue(":guid", Global::createUuid());
    query.bindValue(":sequence", 0);
    query.bindValue(":name", QString::fromStdWString(L"�ҵıʼǲ�"));
    query.bindValue(":defaultNotebook", addDefaulte? 1: 0);

    query.bindValue(":serviceCreated", QDateTime::currentMSecsSinceEpoch());
    query.bindValue(":serviceUpdated", QDateTime::currentMSecsSinceEpoch());

    query.bindValue(":isDirty", 1);

    bool check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "Initial "+dbName+" Table insert failed." + query.lastError().text());
    }
}

// Drop the table
void NotebookTable::dropTable()
{
    QSqlQuery query(db->getConnection());
    query.exec("Drop table "+dbName);
}

// Save an individual notebook
void NotebookTable::addNotebook(Notebook* tempNotebook, bool isDirty)
{
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Insert Into "+dbName+" (guid, sequence, name, defaultNotebook, "
                               +"serviceCreated, serviceUpdated,"
                               + "isDirty, stack,isExpunged) Values("
                               +":guid, :sequence, :name, :defaultNotebook,  "
                               +":serviceCreated, :serviceUpdated,"
                               +":isDirty, "
                               +":stack,0)");

    query.bindValue(":guid", tempNotebook->getGuid());
    query.bindValue(":sequence", tempNotebook->getUpdateSequenceNum());
    query.bindValue(":name", tempNotebook->getName());
    query.bindValue(":defaultNotebook", tempNotebook->isDefaultNotebook()? 1:0);

    query.bindValue(":serviceCreated", tempNotebook->getServiceCreated());
    query.bindValue(":serviceUpdated", tempNotebook->getServiceUpdated());

    if (isDirty)
        query.bindValue(":isDirty", 1);
    else
        query.bindValue(":isDirty", 0);
    query.bindValue(":stack", tempNotebook->getStack());

    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, ""+dbName+" Table insert failed." + query.lastError().text());
        return;
    }
}

// Delete the notebook based on a guid
void NotebookTable::expungeNotebook(QString guid)
{
    Notebook* n = getNotebook(guid);
    QSqlQuery query(db->getConnection());

    bool check = query.prepare("Update "+dbName+" set isExpunged=1 where guid=:guid");
    if (!check)
    {
        logger->log(EXTREME, dbName+" SQL delete prepare has failed.");
        logger->log(EXTREME, query.lastError().text());
    }
    query.bindValue(":guid", guid);
    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, dbName+" delete failed.");
        return;
    }
}

// Update a notebook
void NotebookTable::updateNotebook(Notebook* tempNotebook, bool isDirty)
{
    tempNotebook->setServiceUpdated(QDateTime::currentMSecsSinceEpoch());

    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update "+dbName+" set sequence=:sequence, name=:name, defaultNotebook=:defaultNotebook, " +
                               "serviceCreated=:serviceCreated, serviceUpdated=:serviceUpdated, "+
                               "isDirty=:isDirty, "+
                               "stack=:stack " +
                               "where guid=:guid AND isExpunged=0;");

    query.bindValue(":sequence", tempNotebook->getUpdateSequenceNum());
    query.bindValue(":name", tempNotebook->getName());
    query.bindValue(":defaultNotebook", tempNotebook->isDefaultNotebook()?1:0);

    query.bindValue(":serviceCreated", tempNotebook->getServiceCreated());
    query.bindValue(":serviceUpdated", tempNotebook->getServiceUpdated());

    query.bindValue(":isDirty", isDirty?1:0);

    query.bindValue(":guid", tempNotebook->getGuid());
    query.bindValue(":stack", tempNotebook->getStack());

    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, dbName+" Table update failed.");
        logger->log(MEDIUM, query.lastError().text());
        return;
    }
}

// Load notebooks from the database
QList<Notebook*> NotebookTable::getAll()
{
    QList<Notebook*> index;
    QSqlQuery query(db->getConnection());

    bool check = query.exec("Select guid, sequence, name, defaultNotebook, "
                            "serviceCreated, "
                            "serviceUpdated, stack from Notebook WHERE isExpunged=0;");
    if (!check)
        logger->log(EXTREME, dbName+" SQL retrieve has failed.");

    while (query.next())
    {
        Notebook* tempNotebook = new Notebook();
        
        int fieldNo = query.record().indexOf("guid");
        tempNotebook->setGuid(query.value(fieldNo).toString());

        fieldNo = query.record().indexOf("sequence");
        int sequence = query.value(fieldNo).toInt();
        tempNotebook->setUpdateSequenceNum(sequence);

        fieldNo = query.record().indexOf("name");
        tempNotebook->setName(query.value(fieldNo).toString());

        fieldNo = query.record().indexOf("defaultNotebook");
        if (query.value(fieldNo).toString().isEmpty())
        {
            tempNotebook->setDefaultNotebook(false);
        }
        else
        {
            tempNotebook->setDefaultNotebook(query.value(fieldNo).toBool());
        }

        fieldNo = query.record().indexOf("serviceCreated");
        tempNotebook->setServiceCreated(query.value(fieldNo).toLongLong());

        fieldNo = query.record().indexOf("serviceUpdated");
        tempNotebook->setServiceUpdated(query.value(fieldNo).toLongLong());

        fieldNo = query.record().indexOf("stack");
        tempNotebook->setStack(query.value(fieldNo).toString());
        index << tempNotebook;
    }
    return index;
}

// Update a notebook sequence number
void NotebookTable::updateNotebookSequence(QString guid, int sequence)
{
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update "+dbName+" set sequence=:sequence where guid=:guid AND isExpunged=0");
    query.bindValue(":guid", guid);
    query.bindValue(":sequence", sequence);
    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, dbName+" sequence update failed.");
        logger->log(MEDIUM, query.lastError().text());
    }

    QSqlQuery query1(db->getConnection());
    query1.prepare("select sequence from notebook where guid=:guid;");
    query1.bindValue(":guid", guid);
}

// Get a list of notes that need to be updated
Notebook* NotebookTable::getNotebook(QString guid)
{
    Notebook* tempNotebook = NULL;

    QSqlQuery query(db->getConnection());

    query.prepare("Select guid, sequence, name, defaultNotebook, "
                  "serviceCreated, serviceUpdated, stack "
                  "from "+dbName+" where guid=:guid AND isExpunged=0");

    query.bindValue(":guid", guid);

    bool check = query.exec();
    if (!check)
        logger->log(EXTREME, dbName+" SQL retrieve has failed.");

    while (query.next())
    {
        tempNotebook = new Notebook();
        tempNotebook->setGuid(query.value(0).toString());
        int sequence = query.value(1).toInt();
        tempNotebook->setUpdateSequenceNum(sequence);
        tempNotebook->setName(query.value(2).toString());

        if (query.value(3).toString().isEmpty())
        {
            tempNotebook->setDefaultNotebook(false);
        }
        else
        {
            tempNotebook->setDefaultNotebook(query.value(3).toBool());
        }

        tempNotebook->setServiceCreated(query.value(4).toLongLong());
        tempNotebook->setServiceUpdated(query.value(5).toLongLong());

        if (!query.value(6).toString().trimmed().isEmpty())
            tempNotebook->setStack(query.value(6).toString());

        return tempNotebook;
    }
    return NULL;
}

// Set the default notebook
void NotebookTable::setDefaultNotebook(QString guid)
{
    QSqlQuery query(db->getConnection());

    query.prepare("Update "+dbName+" set defaultNotebook=0, isDirty=1 where defaultNotebook=1 AND isExpunged=0");
    if (!query.exec())
        logger->log(EXTREME, "Error removing default "+dbName+".");
    query.prepare("Update "+dbName+" set defaultNotebook=1, isDirty=1 where guid=:guid AND isExpunged=0");
    query.bindValue(":guid", guid);
    if (!query.exec())
        logger->log(EXTREME, "Error setting default "+dbName+".");
}

// Get a list of all icons
QHash<QString, QIcon> NotebookTable::getAllIcons()
{
    QHash<QString, QIcon> values;
    QSqlQuery query(db->getConnection());

    if (!query.exec("SELECT guid, icon from "+dbName+" WHERE isExpunged=0"))
        logger->log(EXTREME, "Error executing "+dbName+" getAllIcons select.");

    while (query.next())
    {
        if (!query.value(1).toByteArray().isEmpty())
        {
            QString guid = query.value(0).toString();
            QIcon icon = QPixmap::fromImage(QImage::fromData(query.value(1).toByteArray()));
            values.insert(guid, icon);
        }
    }
    return values;
}

// Get the notebooks custom icon
QIcon NotebookTable::getIcon(QString guid)
{
    QSqlQuery query(db->getConnection());

    if (!query.prepare("Select icon from "+dbName+" where guid=:guid AND isExpunged=0"))
        logger->log(EXTREME, "Error preparing "+dbName+" icon select.");

    query.bindValue(":guid", guid);

    if (!query.exec())
        logger->log(EXTREME, "Error finding "+dbName+" icon.");

    if (!query.next() || query.value(0).toByteArray().isEmpty())
        return QIcon();

    return QIcon(QPixmap::fromImage(QImage::fromData(query.value(0).toByteArray())));
}

// Set the notebooks custom icon
void NotebookTable::setIcon(QString guid, QIcon* icon, QString type)
{
    QSqlQuery query(db->getConnection());
    if (icon == NULL)
    {
        if (!query.prepare("update "+dbName+" set icon=NULL where guid=:guid AND isExpunged=0"))
            logger->log(EXTREME, "Error preparing "+dbName+" icon select.");
    }
    else
    {
        if (!query.prepare("update "+dbName+" set icon=:icon where guid=:guid AND isExpunged=0"))
            logger->log(EXTREME, "Error preparing "+dbName+" icon select.");

        QBuffer buffer;
        if (!buffer.open(QIODevice::ReadWrite))
        {
            logger->log(EXTREME, "Failure to open buffer.  Aborting.");
            return;
        }

        QPixmap p = icon->pixmap(32, 32);
        QImage i = p.toImage();
        i.save(&buffer, type.toUpper().toStdString().c_str());
        buffer.close();

        if (!buffer.buffer().isNull() && !buffer.buffer().isEmpty())
            query.bindValue(":icon", buffer.buffer());
        else
            return;
    }

    query.bindValue(":guid", guid);
    if (!query.exec())
        logger->log(LOW, "Error setting "+dbName+" icon. " +query.lastError().text());
}

// does a record exist?
QString NotebookTable::findNotebookByName(QString newname)
{
    QSqlQuery query(db->getConnection());

    query.prepare("Select guid from "+dbName+" where name=:newname AND isExpunged=0");
    query.bindValue(":newname", newname);
    if (!query.exec())
        logger->log(EXTREME, dbName+" SQL retrieve has failed.");

    QString val;
    if (query.next())
        val = query.value(0).toString();
    return val;
}

// Get/Set stacks
void NotebookTable::clearStack(QString guid)
{
    QSqlQuery query(db->getConnection());

    query.prepare("Update "+dbName+" set stack='' where guid=:guid AND isExpunged=0");
    query.bindValue(":guid", guid);
    if (!query.exec())
        logger->log(EXTREME, "Error clearing "+dbName+" stack.");
}

// Get/Set stacks
void NotebookTable::setStack(QString guid, QString stack)
{
    QSqlQuery query(db->getConnection());

    query.prepare("Update "+dbName+" set stack=:stack, isDirty=1 where guid=:guid AND isExpunged=0");
    query.bindValue(":guid", guid);
    query.bindValue(":stack", stack);
    if (!query.exec())
        logger->log(EXTREME, "Error setting notebook stack.");
}

// Get all stack names
QStringList NotebookTable::getAllStackNames()
{
    QStringList stacks;
    QSqlQuery query(db->getConnection());

    if (!query.exec("Select distinct stack from "+dbName))
    {
        logger->log(EXTREME, "Error getting all stack names.");
        return stacks;
    }

    while (query.next())
    {
        if (!query.value(0).toString().trimmed().isEmpty())
            stacks << query.value(0).toString();
    }
    return stacks;
}

// Rename a stack
void NotebookTable::renameStacks(QString oldName, QString newName)
{
    QSqlQuery query(db->getConnection());

    if (!query.prepare("update "+dbName+" set stack=:newName where stack=:oldName"))
    {
        logger->log(EXTREME, "Error preparing in renameStacks.");
        return;
    }

    query.bindValue(":oldName", oldName);
    query.bindValue(":newName", newName);
    if (!query.exec())
    {
        logger->log(EXTREME, "Error updating stack names");
        return;
    }

    if (!query.prepare("update SystemIcon set name=:newName where name=:oldName and type='STACK'"))
    {
        logger->log(EXTREME, "Error preparing icon rename in renameStacks.");
        return;
    }

    query.bindValue(":oldName", oldName);
    query.bindValue(":newName", newName);
    if (!query.exec())
    {
        logger->log(EXTREME, "Error updating stack names for SystemIcon");
        return;
    }
}

// Get a notebook's sort order
int NotebookTable::getSortColumn(QString guid)
{
    bool check;

    QSqlQuery query(db->getConnection());

    if (Global::getSortOrder() != Global::View_List_Wide)
        check = query.prepare("Select wide_sort_column "
                              "from "+dbName+" where guid=:guid");
    else
        check = query.prepare("Select narrow_sort_column "
                              "from "+dbName+" where guid=:guid");
    query.bindValue(":guid", guid);
    check = query.exec();

    if (!check)
    {
        logger->log(EXTREME, "Notebook* SQL retrieve sort order has failed.");
        return -1;
    }

    if (query.next())
    {
        return query.value(0).toInt();
    }
    return -1;
}

// Get a notebook's sort order
int NotebookTable::getSortOrder(QString guid)
{
    bool check;

    QSqlQuery query(db->getConnection());

    if (Global::getSortOrder() != Global::View_List_Wide)
        check = query.prepare("Select wide_sort_order "
                              "from "+dbName+" where guid=:guid");
    else
        check = query.prepare("Select narrow_sort_order "
                              "from "+dbName+" where guid=:guid");
    query.bindValue(":guid", guid);
    check = query.exec();
    if (!check)
    {
        logger->log(EXTREME, "Notebook* SQL retrieve sort order has failed.");
        return -1;
    }
    if (query.next())
    {
        return query.value(0).toInt();
    }
    return -1;
}

// Get a notebook's sort order
void NotebookTable::setSortOrder(QString guid, int column, int order)
{
    bool check;

    QSqlQuery query(db->getConnection());
    if (Global::getSortOrder() != Global::View_List_Wide)
        check = query.prepare("Update "+dbName+" set wide_sort_order=:order, wide_sort_column=:column where guid=:guid");
    else
        check = query.prepare("Update "+dbName+" set narrow_sort_order=:order, narrow_sort_column=:column where guid=:guid");

    query.bindValue(":guid", guid);
    query.bindValue(":order", order);
    query.bindValue(":column", column);
    check = query.exec();
    if (!check)
        logger->log(EXTREME, "Notebook* SQL set sort order has failed.");
}

