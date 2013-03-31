#ifndef SAVERUNNER_H
#define SAVERUNNER_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QPair>
#include <QString>
#include <QStringList>
//#include "signals/notesignal.h"
#include "utilities/applicationlogger.h"
#include "sql/databaseconnection.h"

class SaveRunner : public QObject
{
    Q_OBJECT
public:
    SaveRunner(ApplicationLogger* l, DatabaseConnection* c);
    void addWork(QString guid, QString content);
    void release(QString guid, QString content);
    int getWorkQueueSize();
    void setKeepRunning(bool b);
    bool keepRunning();
    bool isIdle();
    void updateNoteContent(QString guid, QString content);
    void removeObsoleteResources(QList<Resource*> oldResources, QStringList newResources);

    bool                        bKeepRunning;
    QMutex						threadLock;


private:

    ApplicationLogger*      logger;
    DatabaseConnection* 	conn;
    bool                    idle;
    QQueue<QPair<QString, QString> > workQueue;
};

#endif // SAVERUNNER_H
