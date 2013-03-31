#include "threads/saverunner.h"
#include "Global.h"
#include "evernote/enmlconverter.h"

SaveRunner::SaveRunner(ApplicationLogger *l, DatabaseConnection *c)
{
    logger = l;
    conn = c;
    bKeepRunning = true;
 //   noteSignals = new NoteSignal();
}

void SaveRunner::addWork(QString guid, QString content)
{
    //TODO while(workQueue.size() > 0) {}
    QPair<QString, QString> pair(guid, content);
    workQueue.enqueue(pair);
}

void SaveRunner::release(QString guid, QString content)
{
    QPair<QString, QString> pair(guid, content);
    workQueue.enqueue(pair);
}

int SaveRunner::getWorkQueueSize()
{
    return workQueue.size();
}

void SaveRunner::setKeepRunning(bool b)
{
    bKeepRunning = b;
}

bool SaveRunner::keepRunning()
{
    return bKeepRunning;
}

bool SaveRunner::isIdle()
{
    return idle;
}

void SaveRunner::updateNoteContent(QString guid, QString content)
{
    logger->log(HIGH, "Entering ListManager.updateNoteContent");

    // Actually save the content
    EnmlConverter enml(logger);
    QString newContent = enml.convert(guid, content);
    QString fixedContent = enml.fixEnXMLCrap(newContent);
    if (!fixedContent.isEmpty())
    {
        conn->getNoteTable()->updateNoteContent(guid, fixedContent);
        logger->log(EXTREME, "Saving new note resources");
        QList<Resource*> oldResources = conn->getNoteTable()->noteResourceTable->getNoteResources(guid, false);
        QStringList newResources = enml.getResources();
        removeObsoleteResources(oldResources, newResources);
    }
    else
    {
 //       noteSignals->EmitNoteSaveRunnerError(guid, "");
    }
    logger->log(HIGH, "Leaving ListManager.updateNoteContent");
}

void SaveRunner::removeObsoleteResources(QList<Resource *> oldResources, QStringList newResources)
{
    if (oldResources.isEmpty())
        return;

    if (newResources.isEmpty())
    {
        for (int i=0; i < oldResources.size(); i++)
        {
            conn->getNoteTable()->noteResourceTable->expungeNoteResource(oldResources.at(i)->getGuid());
        }
    }

    for (int i=0; i<oldResources.size(); i++)
    {
        bool matchFound = false;
        for (int j=0; j<newResources.size(); j++)
        {
            if (newResources.at(j).toLower() == oldResources.at(i)->getGuid().toLower())
                matchFound = true;
            if (Global::resourceMap.contains(newResources.at(j)))
            {
                if (Global::resourceMap.value(newResources.at(j)).toLower() == oldResources.at(i)->getGuid().toLower())
                    matchFound = true;
            }
            if (matchFound)
                break;
        }

        if (!matchFound)
            conn->getNoteTable()->noteResourceTable->expungeNoteResource(oldResources.at(i)->getGuid());
    }
}
