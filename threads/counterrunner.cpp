#include "threads/counterrunner.h"
#include <QThread>
CounterRunner::CounterRunner(QString logname, int t, QString u, QString i, QString r, QString uid, QString pswd, QString cpswd)
{
    type = t;
    logger = new ApplicationLogger(logname);
    conn = new DatabaseConnection(logger);
    conn->dbSetup(u, i, r, uid, pswd, cpswd, "");
    bkeepRunning = true;
/*    notebookSignal = new NotebookSignal();*/
 //   tagSignal = new TagSignal();
/*    trashSignal = new TrashSignal();*/
    ID = 0;
    ready = false;
    abortCount = false;
}

CounterRunner::~CounterRunner()
{
}

void CounterRunner::run()
{
    bool keepRunning = true;
    QThread::currentThread()->setPriority(QThread::LowestPriority);
    while(keepRunning)
    {
        ready = true;
        if (!readyQueue.isEmpty())
        {
            type = readyQueue.dequeue();
            threadLock.lock();
            if (type == EXIT)
                keepRunning = false;
            if (type == NOTEBOOK)
                countNotebookResults();
            if (type == TAG)
                countTagResults();
            if (type == TRASH)
                countTrashResults();
            threadLock.unlock();
        }
    }
    conn->dbShutdown();
}

void CounterRunner::setNoteIndex(QList<Note*> idx)
{
    abortCount = true;
    threadLock.lock();
    abortCount = false;
    records.clear();
    if (!idx.isEmpty())
    {
        for (int i=0; i<idx.size(); i++)
        {
            if  (idx.at(i) == NULL)
            {
                continue;
            }
            if (abortCount)
            {
                threadLock.unlock();
                return;
            }
            NoteRecord record;
            record.notebookGuid = idx.at(i)->getNotebookGuid();
            record.active = idx.at(i)->isActive();
            for (int j=0; j<idx.at(i)->getTagGuidsSize(); j++)
            {
                if (abortCount)
                {
                    threadLock.unlock();
                    return;
                }
                record.tags << idx.at(i)->getTagGuids().at(j);
            }
            records.push_back(record);
        }
    }
    threadLock.unlock();
}

void CounterRunner::release(int type)
{
    threadLock.lock();
    readyQueue.enqueue(type);
    threadLock.unlock();
}

void CounterRunner::setKeepRunning(bool b)
{
    bkeepRunning = b;
}

void CounterRunner::countNotebookResults()
{
    logger->log(EXTREME, "Entering ListManager.countNotebookResults");
    if (abortCount)
        return;

    QList<NotebookCounter> nCounter;
    for (int i=0; i< records.size(); i++)
    {
        if (abortCount)
            return;
        bool found = false;
        for (int j=0; j< nCounter.size(); j++)
        {
            if (abortCount)
                return;
            if (records[i].active && nCounter.at(j).getGuid() == records.at(i).notebookGuid)
            {
                nCounter[j].setCount(nCounter.at(j).getCount()+1);
                found = true;
                break;
            }
        }
        if (!found && records.at(i).active)
        {
            NotebookCounter newCounter;
            newCounter.setGuid(records.at(i).notebookGuid);
            newCounter.setCount(1);
            nCounter << newCounter;
        }
    }
    if (abortCount)
        return;

    emit countsChanged(nCounter);
    logger->log(EXTREME, "Leaving ListManager.countNotebookResults()");
}

void CounterRunner::countTagResults()
{
    logger->log(EXTREME, "Entering ListManager.countTagResults");
    if (abortCount)
        return;

    QList<TagCounter> tCounter;
    for (int i=0; i<records.size(); i++)
    {
        if (abortCount)
            return;

        // Loop through the list of tags so we can count them
        QStringList tags = records.at(i).tags;
        for (int z=0; z<tags.size(); z++)
        {
            bool found = false;
            for (int j=0; j<tCounter.size(); j++)
            {
                if (abortCount)
                    return;

                if (tCounter[j].getGuid() == tags.at(z))
                {
                    tCounter[j].setCount(tCounter[j].getCount()+1);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                TagCounter newCounter;
                newCounter.setGuid(tags.at(z));
                newCounter.setCount(1);
                tCounter << newCounter;
            }
        }
    }
    if (abortCount)
        return;

/*    tagSignal->emitCountsChanged(tCounter);*/
    logger->log(EXTREME, "Leaving ListManager.countTagResults");
}

void CounterRunner::countTrashResults()
{
    logger->log(EXTREME, "Entering CounterRunner.countTrashResults()");
    if (abortCount)
        return;

    int tCounter = 0;
    for (int i=0; i<records.size(); i++)
    {
        if (abortCount)
            return;
        if (!records.at(i).active)
            tCounter++;
    }

    if (abortCount)
        return;

    emit trashCountChanged(tCounter);
    logger->log(EXTREME, "Leaving CounterRunner.countTrashResults()");
}
