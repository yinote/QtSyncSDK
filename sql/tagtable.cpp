#include "tagtable.h"
#include "sql/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QIcon>
#include <QBuffer>

TagTable::TagTable(ApplicationLogger *l, DatabaseConnection *d)
{
    logger = l;
    db = d;
}

void TagTable::createTable()
{
    QSqlQuery query(db->getConnection());
    logger->log(HIGH, "Creating table Tag...");
    if (!query.exec("Create table Tag (guid varchar primary key, "
                    "parentGuid varchar, sequence integer, name varchar, isDirty integer, isExpunged integer, "
                    "icon BLOB)"))
    {
        logger->log(HIGH, "Table TAG creation FAILED!!!" + query.lastError().text());
    }
}

void TagTable::dropTable()
{
    QSqlQuery query(db->getConnection());
    query.exec("Drop table Tag");
}

QStringList TagTable::getAllTagName()
{
    QStringList names;
    QSqlQuery query(db->getConnection());

    bool check = query.exec("Select name from Tag WHERE isExpunged=0");
    if (!check)
    {
        logger->log(EXTREME, "Tag SQL retrieve has failed." + query.lastError().text());
    }

    while (query.next())
    {
        int fieldNo = query.record().indexOf("name");
        names << query.value(fieldNo).toString();
    }

    return names;
}

// get all tags
QList<Tag*> TagTable::getAll()
{
    QList<Tag*> index;

    QSqlQuery query(db->getConnection());

    bool check = query.exec("Select guid, sequence, name from Tag WHERE isExpunged=0");
    if (!check)
    {
        logger->log(EXTREME, "Tag SQL retrieve has failed." + query.lastError().text());
    }

    while (query.next())
    {
        Tag* tempTag = new Tag();

        int fieldNo = query.record().indexOf("guid");
        tempTag->setGuid(query.value(fieldNo).toString());

        fieldNo = query.record().indexOf("sequence");
        tempTag->setUpdateSequenceNum(query.value(fieldNo).toInt());

        fieldNo = query.record().indexOf("name");
        tempTag->setName(query.value(fieldNo).toString());
        index << tempTag;
    }

    return index;
}

Tag* TagTable::getTagByName(QString name)
{
    QSqlQuery query(db->getConnection());
    if (!query.prepare("Select guid, sequence"
        " from Tag where name=:name AND isExpunged=0"))
    {
        logger->log(EXTREME, "Tag select by guid SQL prepare has failed." + query.lastError().text());
    }

    query.bindValue(":name", name);
    if (!query.exec())
        logger->log(EXTREME, "Tag select by guid SQL exec has failed." + query.lastError().text());

    if (!query.next())
    {
        return NULL;
    }

    Tag* tempTag = new Tag();
    int fieldNo = query.record().indexOf("guid");
    tempTag->setGuid(query.value(fieldNo).toString());

    fieldNo = query.record().indexOf("sequence");
    tempTag->setUpdateSequenceNum(query.value(fieldNo).toInt());

    tempTag->setName(name);
    return tempTag;
}

QString TagTable::getNameByGuid(QString guid)
{
    QSqlQuery query(db->getConnection());
    if (!query.prepare("Select name"
        " from Tag where guid=:guid AND isExpunged=0"))
    {
        logger->log(EXTREME, "Tag select by guid SQL prepare has failed." + query.lastError().text());
    }

    query.bindValue(":guid", guid);
    if (!query.exec())
        logger->log(EXTREME, "Tag select by guid SQL exec has failed." + query.lastError().text());

    if (!query.next())
    {
        return "";
    }

    int fieldNo = query.record().indexOf("name");
    return query.value(fieldNo).toString();
}

Tag* TagTable::getTag(QString guid)
{
    QSqlQuery query(db->getConnection());
    if (!query.prepare("Select guid, sequence, name"
                       " from Tag where guid=:guid AND isExpunged=0"))
    {
        logger->log(EXTREME, "Tag select by guid SQL prepare has failed." + query.lastError().text());
    }

    query.bindValue(":guid", guid);
    if (!query.exec())
        logger->log(EXTREME, "Tag select by guid SQL exec has failed." + query.lastError().text());

    if (!query.next())
    {
        return NULL;
    }

    Tag* tempTag = new Tag();
    int fieldNo = query.record().indexOf("guid");
    tempTag->setGuid(query.value(fieldNo).toString());

    fieldNo = query.record().indexOf("sequence");
    tempTag->setUpdateSequenceNum(query.value(fieldNo).toInt());

    fieldNo = query.record().indexOf("name");
    tempTag->setName(query.value(fieldNo).toString());

    return tempTag;
}

// Update a tag
void TagTable::updateTag(Tag* tempTag, bool isDirty)
{
    bool check;

    QSqlQuery query(db->getConnection());
    check = query.prepare("Update Tag set sequence=:sequence, "
                          "name=:name, isDirty=:isDirty "
                          "where guid=:guid AND isExpunged=0");

    if (!check)
    {
        logger->log(EXTREME, "Tag SQL update prepare has failed." + query.lastError().text());
    }

    query.bindValue(":sequence", tempTag->getUpdateSequenceNum());
    query.bindValue(":name", tempTag->getName());
    query.bindValue(":isDirty", isDirty?1:0);
    query.bindValue(":guid", tempTag->getGuid());

    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "Tag Table update failed."  + query.lastError().text());
    }
}

// Delete a tag
void TagTable::expungeTag(QString guid)
{
    Tag* t = getTag(guid);
    QSqlQuery query(db->getConnection());

    bool check = query.prepare("Update Tag set isExpunged=1 where guid=:guid");
    if (!check)
    {
        logger->log(EXTREME, "Tag SQL delete prepare has failed." + query.lastError().text());
        return;
    }

    query.bindValue(":guid", guid);
    check = query.exec();
    if (!check)
        logger->log(MEDIUM, "Tag delete failed." + query.lastError().text());

    check = query.prepare("delete from NoteTags where tagGuid=:guid");
    if (!check)
    {
        logger->log(EXTREME, "NoteTags SQL delete prepare has failed." + query.lastError().text());
    }

    query.bindValue(":guid", guid);
    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "NoteTags delete failed." + query.lastError().text());
    }

}

// Save a tag
void TagTable::addTag(Tag* tempTag, bool isDirty)
{
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Insert Into Tag (guid, sequence, name, isDirty,isExpunged)"
                               " Values(:guid, :sequence, :name, :isDirty,0)");
    if (!check)
    {
        logger->log(EXTREME, "Tag SQL insert prepare has failed." + query.lastError().text());
    }

    query.bindValue(":guid", tempTag->getGuid());
    query.bindValue(":sequence", tempTag->getUpdateSequenceNum());
    query.bindValue(":name", tempTag->getName());
    query.bindValue(":isDirty", isDirty?1:0);

    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "Tag Table insert failed." + query.lastError().text());
    }
}

// Update a tag's parent
void TagTable::updateTagParent(QString guid, QString parentGuid)
{
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Tag set parentGuid=:parentGuid where guid=:guid AND isExpunged=0");
    if (!check)
    {
        logger->log(EXTREME, "Tag SQL tag parent update prepare has failed." + query.lastError().text());

    }

    query.bindValue(":parentGuid", parentGuid);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "Tag parent update failed." + query.lastError().text());
    }
}

//Save tags from Evernote
void TagTable::saveTags(QList<Tag*> tags)
{
    foreach(Tag* tag, tags)
        addTag(tag, false);
}

// Update a tag sequence number
void TagTable::updateTagSequence(QString guid, int sequence)
{
    QSqlQuery query(db->getConnection());
    bool check = query.prepare("Update Tag set sequence=:sequence where guid=:guid AND isExpunged=0");
    query.bindValue(":sequence", sequence);
    query.bindValue(":guid", guid);

    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "Tag sequence update failed." + query.lastError().text());
    }
}

// Update a tag sequence number
void TagTable::updateTagGuid(QString oldGuid, QString newGuid)
{
    QSqlQuery query(db->getConnection());
    query.prepare("Update Tag set guid=:newGuid where guid=:oldGuid AND isExpunged=0");
    query.bindValue(":newGuid", newGuid);
    query.bindValue(":oldGuid", oldGuid);
    bool check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "Tag guid update failed." + query.lastError().text());
    }

    query.prepare("Update Tag set parentGuid=:newGuid where parentGuid=:oldGuid");
    query.bindValue(":newGuid", newGuid);
    query.bindValue(":oldGuid", oldGuid);
    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "Tag guid update failed."+ query.lastError().text());
    }

    query.prepare("Update NoteTags set tagGuid=:newGuid where tagGuid=:oldGuid");
    query.bindValue(":newGuid", newGuid);
    query.bindValue(":oldGuid", oldGuid);
    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "Tag guid update failed for NoteTags."+ query.lastError().text());
    }
}

// Find a guid based upon the name
QString TagTable::findTagByName(QString name)
{
    QSqlQuery query(db->getConnection());
    query.prepare("Select guid from tag where name=:name AND isExpunged=0");
    query.bindValue(":name", name);
    if (!query.exec())
        logger->log(EXTREME, "Tag SQL retrieve has failed." + query.lastError().text());
    QString val;
    if (query.next())
        val = query.value(0).toString();
    return val;
}

// Get the custom icon
QIcon TagTable::getIcon(QString guid)
{
    QSqlQuery query(db->getConnection());

    if (!query.prepare("Select icon from tag where guid=:guid AND isExpunged=0"))
        logger->log(EXTREME, "Error preparing tag icon select."+ query.lastError().text());

    query.bindValue(":guid", guid);

    if (!query.exec())
        logger->log(EXTREME, "Error finding tag icon."+ query.lastError().text());

    if (!query.next())
        return QIcon();

    if (!query.value(0).toByteArray().isEmpty())
    {
        QIcon icon = QPixmap::fromImage(QImage::fromData(query.value(0).toByteArray()));
        return icon;
    }

    return QIcon();
}

// Set the custom icon
void TagTable::setIcon(QString guid, QIcon icon, QString type)
{
    QSqlQuery query(db->getConnection());
    if (icon.isNull())
    {
        if (!query.prepare("update tag set icon=NULL where guid=:guid AND isExpunged=0"))
            logger->log(EXTREME, "Error preparing tag icon update."+ query.lastError().text());
    }
    else
    {
        if (!query.prepare("update tag set icon=:icon where guid=:guid AND isExpunged=0"))
            logger->log(EXTREME, "Error preparing tag icon update."+ query.lastError().text());

        QBuffer buffer;
        if (!buffer.open(QIODevice::ReadWrite))
        {
            logger->log(EXTREME, "Failure to open buffer.  Aborting.");
            return;
        }

        QPixmap p = icon.pixmap(32, 32);
        QImage i = p.toImage();
        i.save(&buffer, type.toUpper().toStdString().c_str());
        buffer.close();

        if (!buffer.buffer().isNull() && !buffer.buffer().isEmpty())
            query.bindValue(":icon", buffer.buffer());
    }

    query.bindValue(":guid", guid);
    if (!query.exec())
        logger->log(LOW, "Error setting tag icon. "+ query.lastError().text());
}

// Get a QList of all icons
QHash<QString, QIcon> TagTable::getAllIcons()
{
    QHash<QString, QIcon> values;
    QSqlQuery query(db->getConnection());

    if (!query.exec("SELECT guid, icon from tag WHERE isExpunged=0"))
        logger->log(EXTREME, "Error executing getAllIcons select."+ query.lastError().text());

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

void TagTable::cleanupTags()
{
    QSqlQuery query(db->getConnection());
    if (!query.exec("Update tag set parentguid=NULL where parentguid not in (select distinct guid from tag);"))
    {
        logger->log(LOW, "Error cleanupTags "+ query.lastError().text());
    }
}
