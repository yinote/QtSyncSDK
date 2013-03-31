#ifndef TAGTABLE_H
#define TAGTABLE_H
#include <QString>
#include <QList>
#include <QPair>
#include <QHash>
#include <QObject>
#include "evernote/tag.h"
#include "utilities/applicationlogger.h"

QT_FORWARD_DECLARE_CLASS(DatabaseConnection)
QT_FORWARD_DECLARE_CLASS(QIcon)

class TagTable:public QObject
{
    Q_OBJECT
public:
    TagTable(ApplicationLogger* l,DatabaseConnection* d);
    void createTable();
    void dropTable();

    QList<Tag*> getAll();
    QStringList getAllTagName();

    QString getNameByGuid(QString guid);
    Tag* getTag(QString guid);
    Tag* getTagByName(QString name);

    void updateTag(Tag* tempTag, bool isDirty);
    void expungeTag(QString guid);

    void addTag(Tag* tempTag, bool isDirty);

    // Update a Tag*'s parent
    void updateTagParent(QString guid, QString parentGuid);

    //Save tags from Evernote
    void saveTags(QList<Tag*> tags);

    // Update a Tag sequence number
    void updateTagSequence(QString guid, int sequence);

    // Update a Tag sequence number
    void updateTagGuid(QString oldGuid, QString newGuid);

    // Find a guid based upon the name
    QString findTagByName(QString name);

    // Get the custom icon
    QIcon getIcon(QString guid);

    // Set the custom icon
    void setIcon(QString guid, QIcon icon, QString type);

    // Get a QList of all icons
    QHash<QString, QIcon> getAllIcons();

    void cleanupTags();

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*      db;
};

#endif // TAGTABLE_H
