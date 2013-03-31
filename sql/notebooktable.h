#ifndef NOTEBOOKTABLE_H
#define NOTEBOOKTABLE_H

#include <QSqlQuery>
#include <QList>
#include <QString>
#include <QObject>

#include "evernote/notebook.h"
#include "utilities/applicationlogger.h"

QT_FORWARD_DECLARE_CLASS(DatabaseConnection)

class NotebookTable: public QObject
{
    Q_OBJECT
public:
    NotebookTable(ApplicationLogger* l, DatabaseConnection* d);
    NotebookTable(ApplicationLogger* l, DatabaseConnection* d, QString name);

    void createTable(bool addDefaulte);

    // Drop the table
    void dropTable();

    // Save an individual notebook
    void addNotebook(Notebook* tempNotebook, bool isDirty);

    // Delete the notebook based on a guid
    void expungeNotebook(QString guid);

    // Update a notebook
    void updateNotebook(Notebook* tempNotebook, bool isDirty);

    // Load notebooks from the database
    QList<Notebook*> getAll();

    // Update a notebook sequence number
    void updateNotebookSequence(QString guid, int sequence);

    // Get a list of notes that need to be updated
    Notebook* getNotebook(QString guid);

    // Set the default notebook
    void setDefaultNotebook(QString guid);

    // Get a list of all icons
    QHash<QString, QIcon> getAllIcons();

    // Get the notebooks custom icon
    QIcon getIcon(QString guid);

    // Set the notebooks custom icon
    void setIcon(QString guid, QIcon *icon, QString type) ;

    // Set the notebooks custom icon
    void setReadOnly(QString guid, bool readOnly);

    // does a record exist?
    QString findNotebookByName(QString newname);

    // Get/Set stacks
    void clearStack(QString guid);

    // Get/Set stacks
    void setStack(QString guid, QString stack) ;

    // Get all stack names
    QStringList getAllStackNames();

    // Rename a stack
    void renameStacks(QString oldName, QString newName);

    // Get a notebook's sort order
    int getSortColumn(QString guid);

    // Get a notebook's sort order
    int getSortOrder(QString guid);

    // Get a notebook's sort order
    void setSortOrder(QString guid, int column, int order);

private:
    ApplicationLogger* 		logger;
    DatabaseConnection*     db;
    QString					dbName;
    QSqlQuery*              notebookCountQuery;
};

#endif // NOTEBOOKTABLE_H
