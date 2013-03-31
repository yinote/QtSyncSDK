#ifndef NOTERESOURCETABLE_H
#define NOTERESOURCETABLE_H
#include <QString>
#include <QList>
#include <QPair>
#include <QHash>
#include <QStringList>
#include <QObject>
#include "utilities/applicationlogger.h"
#include "evernote/resource.h"

QT_FORWARD_DECLARE_CLASS(DatabaseConnection)

class NoteResourceTable: public QObject
{
    Q_OBJECT
public:
    NoteResourceTable(ApplicationLogger* l,DatabaseConnection* d);
    void createTable();

    void dropTable();

    QByteArray getNoteResourceData(QString guid, QString dataHash);
    QByteArray getNoteResourceData(QString guid);

    void saveNoteResource(Resource* r, bool isDirty);

    // delete an old resource
    void expungeNoteResource(QString guid);

    // Get a note resource from the database by it's hash value
    QString getNoteResourceGuidByHashHex(QString noteGuid, QString hash);

    // Get a note's resourcesby Guid
    Resource* getNoteResource(QString guid, bool withBinary);

    // Get a note's resourcesby Guid
    QList<Resource*> getNoteResources(QString noteGuid, bool withBinary);

    // Save Note Resource
    void updateNoteResource(Resource* r, bool isDirty) ;

    void updateResourceSequence(QString guid, int sequence);

    // Update note Resource GUID
    void updateNoteResourceGuid(QString oldGuid, QString newGuid, bool isDirty);

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*      db;
};

#endif // NOTERESOURCETABLE_H
