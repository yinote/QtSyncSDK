#include "sql/NoteResourceTable.h"
#include "sql/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>

NoteResourceTable::NoteResourceTable(ApplicationLogger *l, DatabaseConnection *d)
{
    logger = l;
    db = d;
}

void NoteResourceTable::createTable()
{
    QSqlQuery query(db->getResourceConnection());

    // Create the NoteResource table
    logger->log(HIGH, "Creating table NoteResource...");
    if (!query.exec("Create table NoteResources (guid varchar primary key, "
                    "noteGuid varchar, updateSequenceNumber integer, dataHash varchar, "
                    "dataSize integer, dataBinary blob, "
                    "mime varchar, width integer, height integer, duration integer, "
                    "active integer, attributeSourceUrl varchar, attributeTimestamp integer, "
                    "attributeLatitude double, attributeLongitude double, "
                    "attributeAltitude double, attributeCameraMake varchar, attributeCameraModel varchar, "
                    "attributeClientWillIndex varchar, attributeFileName varchar,"
                    "attributeAttachment integer, isDirty integer,isExpunged Integer)"))
        logger->log(HIGH, "Table NoteResource creation FAILED!!! " + query.lastError().text());

    if (!query.exec("CREATE INDEX resources_dataheshhex on noteresources (datahash, guid);"))
        logger->log(HIGH, "Noteresources resources_datahash index creation FAILED!!! " + query.lastError().text());

    if (!query.exec("create index RESOURCES_GUID_INDEX on noteresources (noteGuid, guid);"))
        logger->log(HIGH, "Noteresources resources_datahash index creation FAILED!!! " + query.lastError().text());
}

// Drop the table
void NoteResourceTable::dropTable()
{
    QSqlQuery query(db->getResourceConnection());
    query.exec("Drop table NoteResources");
}

void NoteResourceTable::saveNoteResource(Resource* r, bool isDirty)
{
    logger->log(HIGH, "Entering saveNoteResources: isDirty " +isDirty);
    logger->log(HIGH, "Note: " +r->getNoteGuid());
    logger->log(HIGH, "Resource: " +r->getGuid());
    QSqlQuery query(db->getResourceConnection());

    query.prepare("Insert Into NoteResources ("
                  "guid, noteGuid, dataHash, dataSize, dataBinary, updateSequenceNumber, "
                  "mime, width, height, duration, active, attributeSourceUrl, attributeTimestamp, "
                  "attributeLatitude, attributeLongitude, attributeAltitude, attributeCameraMake, "
                  "attributeCameraModel, "
                  "attributeClientWillIndex, attributeFileName, attributeAttachment, isDirty, isExpunged )"
                  "Values("
                  ":guid, :noteGuid, :dataHash,:dataSize, :dataBinary, :updateSequenceNumber, "
                  ":mime, :width, :height, :duration, :active, :attributeSourceUrl, :attributeTimestamp, "
                  ":attributeLatitude, :attributeLongitude, :attributeAltitude, :attributeCameraMake, "
                  ":attributeCameraModel, "
                  ":attributeClientWillIndex, :attributeFileName, :attributeAttachment, "
                  ":isDirty, 0)");

    query.bindValue(":guid", r->getGuid());
    query.bindValue(":noteGuid", r->getNoteGuid());
    if (r->getData() != NULL)
    {
        query.bindValue(":dataHash", QString(r->getData()->hexHash));
        query.bindValue(":dataSize", r->getData()->size);
        query.bindValue(":dataBinary", r->getData()->Data);
    }

    query.bindValue(":updateSequenceNumber", r->getUpdateSequenceNum());
    query.bindValue(":mime", r->getMime());
    query.bindValue(":width", r->getWidth());
    query.bindValue(":height", r->getHeight());
    query.bindValue(":duration", r->getDuration());
    query.bindValue(":active", r->isActive()?1:0);

    if (r->getAttributes() != NULL)
    {
        query.bindValue(":attributeSourceUrl", r->getAttributes()->getSourceURL());
        query.bindValue(":attributeTimestamp", r->getAttributes()->getTimestamp());
        query.bindValue(":attributeLatitude", r->getAttributes()->getLatitude());
        query.bindValue(":attributeLongitude", r->getAttributes()->getLongitude());
        query.bindValue(":attributeAltitude", r->getAttributes()->getAltitude());
        query.bindValue(":attributeCameraMake", r->getAttributes()->getCameraMake());
        query.bindValue(":attributeCameraModel", r->getAttributes()->getCameraModel());
        query.bindValue(":attributeClientWillIndex", r->getAttributes()->isClientWillIndex()?1:0);
        query.bindValue(":attributeFileName", r->getAttributes()->getFileName());
        query.bindValue(":attributeAttachment", r->getAttributes()->isAttachment()?1:0);
    }

    query.bindValue(":isDirty", isDirty?1:0);

    bool check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "*** NoteResource Table insert failed." + query.lastError().text());
        return;
    }

    logger->log(HIGH, "Leaving DBRunner->saveNoteResources");
}

// delete an old resource
void NoteResourceTable::expungeNoteResource(QString guid)
{
    QSqlQuery query(db->getResourceConnection());
    query.prepare("Update NoteResources set isExpunged=1 where guid=:guid AND isExpunged=0");
    query.bindValue(":guid",guid);
    if (!query.exec())
    {
        logger->log(EXTREME, "NoteResources expungeNoteResource failed."+ query.lastError().text());
        return;
    }
}


// Get a note resource from the database by it's hash value
QString NoteResourceTable::getNoteResourceGuidByHashHex(QString noteGuid, QString hash)
{
    logger->log(HIGH, "Entering DBRunner->getNoteResourceGuidByHashHex");
    QSqlQuery query(db->getResourceConnection());

    bool check = query.prepare("Select guid from NoteResources "
                          "where noteGuid=:noteGuid and dataHash=:hash AND isExpunged=0");
    if (check)
        logger->log(EXTREME, "NoteResource SQL select prepare was successful.");
    else
    {
        logger->log(EXTREME, "NoteResource SQL select prepare has failed. "+query.lastError().text());
    }

    query.bindValue(":noteGuid", noteGuid);
    query.bindValue(":hash", hash);

    check = query.exec();
    if (!check)
    {
        logger->log(MEDIUM, "dbRunner->getNoteResourceGuidByHashHex Select failed."
                    "Note Guid:" +noteGuid + "Data Body Hash:" + hash + " " + query.lastError().text());
    }

    if (!query.next())
    {
        logger->log(MEDIUM, "Note Resource not found.");
        return NULL;
    }
    return query.value(0).toString();
}

// Get a note's resourcesby Guid
Resource* NoteResourceTable::getNoteResource(QString guid, bool withBinary)
{
    if (guid.isEmpty())
        return NULL;

    QSqlQuery query(db->getResourceConnection());
    QString queryString = "Select guid, noteGuid, mime, width, height, duration, "
                              "active, updateSequenceNumber, dataHash, dataSize, "
                              "attributeLatitude, attributeLongitude, attributeAltitude, "
                              "attributeCameraMake, attributeCameraModel, attributeClientWillIndex, "
                              "attributeFileName, attributeAttachment, attributeSourceUrl "
                              " from NoteResources where guid=:guid AND isExpunged=0";

    query.prepare(queryString);

    query.bindValue(":guid",guid);
    if (!query.exec())
    {
        logger->log(EXTREME, "NoteResources SQL select has failed."+ query.lastError().text());
        return NULL;
    }

    Resource* r = NULL;
    if (query.next())
    {
        r = new Resource();
        int fieldNo = query.record().indexOf("guid");
        r->setGuid(query.value(fieldNo).toString());     	// Resource Guid

        fieldNo = query.record().indexOf("noteGuid");
        r->setNoteGuid(query.value(fieldNo).toString());   // note Guid

        fieldNo = query.record().indexOf("mime");
        r->setMime(query.value(fieldNo).toString());       // Mime Type

        fieldNo = query.record().indexOf("width");
        r->setWidth(query.value(fieldNo).toInt());  // Width

        fieldNo = query.record().indexOf("height");
        r->setHeight(query.value(fieldNo).toInt());  // Height

        fieldNo = query.record().indexOf("duration");
        r->setDuration(query.value(fieldNo).toInt());  // Duration

        fieldNo = query.record().indexOf("active");
        r->setActive(query.value(fieldNo).toBool());  // active

        fieldNo = query.record().indexOf("updateSequenceNumber");
        r->setUpdateSequenceNum(query.value(fieldNo).toInt());  // update sequence number

        ResourceData data;
        fieldNo = query.record().indexOf("dataHash");
        data.hexHash = query.value(fieldNo).toByteArray();

        fieldNo = query.record().indexOf("dataSize");
        data.size = query.value(fieldNo).toInt();
        r->setData(data);

        ResourceAttributes* a = new ResourceAttributes();
        fieldNo = query.record().indexOf("attributeLatitude");
        if (!query.value(fieldNo).toString().isEmpty())     // Latitude
            a->setLatitude(query.value(fieldNo).toFloat());

        fieldNo = query.record().indexOf("attributeLongitude");
        if (!query.value(fieldNo).toString().isEmpty())              // Longitude
            a->setLongitude(query.value(fieldNo).toFloat());

        fieldNo = query.record().indexOf("attributeAltitude");
        if (!query.value(fieldNo).toString().isEmpty())              // Altitude
            a->setAltitude(query.value(fieldNo).toFloat());

        fieldNo = query.record().indexOf("attributeCameraMake");
        a->setCameraMake(query.value(fieldNo).toString());              // Camera Make

        fieldNo = query.record().indexOf("attributeCameraModel");
        a->setCameraModel(query.value(fieldNo).toString());

        fieldNo = query.record().indexOf("attributeClientWillIndex");
        a->setClientWillIndex(query.value(fieldNo).toBool());  // Camera Model

        fieldNo = query.record().indexOf("attributeFileName");
        a->setFileName(query.value(fieldNo).toString());                  // File Name

        fieldNo = query.record().indexOf("attributeAttachment");
        a->setAttachment(query.value(fieldNo).toBool());

        fieldNo = query.record().indexOf("attributeSourceUrl");
        a->setSourceURL(query.value(fieldNo).toString());
        r->setAttributes(a);

        if (withBinary)
        {
            query.prepare("Select dataBinary from NoteResources where guid=:guid");
            query.bindValue(":guid", r->getGuid());
            query.exec();
            if (query.next())
            {
                if (r->getData())
                    r->getData()->Data = query.value(0).toByteArray();
            }
        }
    }// end if (query.next())
    return r;
}

// Get a note's resourcesby Guid
QList<Resource*> NoteResourceTable::getNoteResources(QString noteGuid, bool withBinary)
{
    QList<Resource*> res;
    if (noteGuid.isEmpty())
        return res;

    QSqlQuery query(db->getResourceConnection());
    query.prepare("Select guid"
                  " from NoteResources where noteGuid = :noteGuid AND isExpunged=0");
    query.bindValue(":noteGuid", noteGuid);

    if (!query.exec()) {
        logger->log(EXTREME, "NoteResources SQL select has failed."+ query.lastError().text());
        return res;
    }
    while (query.next())
    {
        QString guid = (query.value(0).toString());
        Resource* pRes = getNoteResource(guid, withBinary);
        if (pRes)
            res << pRes;
    }
    return res;
}

QByteArray NoteResourceTable::getNoteResourceData(QString guid)
{
    QSqlQuery query(db->getResourceConnection());
    query.prepare("Select dataBinary from NoteResources where guid=:guid AND isExpunged=0;");
    query.bindValue(":guid", guid);
    if (!query.exec())
    {
        return QByteArray();
    }

    if (query.next())
    {
        return query.value(0).toByteArray();
    }

    return QByteArray();
}

QByteArray NoteResourceTable::getNoteResourceData(QString guid, QString dataHash)
{
    QSqlQuery query(db->getResourceConnection());
    query.prepare("Select dataBinary from NoteResources where guid=:guid AND dataHash=:dataHash AND isExpunged=0");
    query.bindValue(":guid", guid);
    query.bindValue(":dataHash", dataHash);
    if (!query.exec())
    {
        return QByteArray();
    }

    if (query.next())
    {
        return query.value(0).toByteArray();
    }

    return QByteArray();
}

// Save Note Resource
void NoteResourceTable::updateNoteResource(Resource* r, bool isDirty)
{
    logger->log(HIGH, "Entering ListManager->updateNoteResource");
    expungeNoteResource(r->getGuid());
    saveNoteResource(r, isDirty);
    logger->log(HIGH, "Leaving RNoteResourceTable.updateNoteResource");
}

// Update note resource GUID
void NoteResourceTable::updateNoteResourceGuid(QString oldGuid, QString newGuid, bool isDirty)
{
    logger->log(HIGH, "Entering RNoteResourceTable.updateNoteResourceGuid");
    QSqlQuery query(db->getResourceConnection());
    query.prepare("update NoteResources set guid=:newGuid, isDirty=:isDirty where guid=:oldGuid AND isExpunged=0");
    query.bindValue(":newGuid", newGuid);
    query.bindValue(":isDirty", isDirty?1:0);
    query.bindValue(":oldGuid", oldGuid);
    if (!query.exec())
    {
        logger->log(HIGH, "NoteResourceTable.updateNoteResourceGuid Failed: " + query.lastError().text());
    }
}

void NoteResourceTable::updateResourceSequence(QString guid, int sequence)
{
    logger->log(HIGH, "Entering RNoteResourceTable.updateResourceSequence");

    QSqlQuery query(db->getResourceConnection());
    query.prepare("update NoteResources set updateSequenceNumber=:updateSequenceNumber where guid=:guid AND isExpunged=0");
    query.bindValue(":guid", guid);
    query.bindValue(":updateSequenceNumber", sequence);
    if (!query.exec())
    {
        logger->log(HIGH, "NoteResourceTable.updateResourceSequence Failed: " + query.lastError().text());
    }
}
