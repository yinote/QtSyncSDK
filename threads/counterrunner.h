#ifndef COUNTERRUNNER_H
#define COUNTERRUNNER_H

#include <QObject>
#include "utilities/applicationlogger.h"
#include "sql/databaseconnection.h"
#include <vector>
#include <QMutex>
#include <QQueue>
#include "evernote/note.h"
#include "filters/notebookcounter.h"
#include "filters/tagcounter.h"

class CounterRunner : public QObject
{
    Q_OBJECT

signals:
	void listChanged();
	void refreshNotebookTreeCounts(QList<Notebook*>, QList<NotebookCounter>);
	void countsChanged(QList<NotebookCounter>);

	 void trashCountChanged(int);

// 	void listChanged();
// 	void refreshTagTreeCounts(QList<TagCounter>);
// 	void countsChanged(QList<TagCounter>);

public:
    CounterRunner(QString logname, int t, QString u, QString i, QString r, QString uid, QString pswd, QString cpswd);
    ~CounterRunner();
    void setNoteIndex(QList<Note*> idx);
    void release(int type);
    void setKeepRunning(bool b);
    bool keepRunning() {return bkeepRunning;}

    int							ID;
    int							type;
    QMutex						threadLock;
    enum RunnerType
    {
        EXIT = 0,
        NOTEBOOK = 1,
        TAG = 2,
        TRASH = 3
    };
    bool        				ready;
    bool        				abortCount;

public slots:
    void run();

protected:
    void countNotebookResults();
    void countTagResults();
    void countTrashResults();

private:
    struct NoteRecord
    {
        QString notebookGuid;
        QStringList tags;
        bool active;
    };
    ApplicationLogger*                  logger;
    DatabaseConnection*                 conn;
    bool                                bkeepRunning;
    std::vector<NoteRecord>             records;
    QQueue<int>                         readyQueue;
};

#endif // COUNTERRUNNER_H
