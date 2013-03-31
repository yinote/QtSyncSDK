#include "utilities/listmanager.h"
#include "Global.h"
#include "sql/notetagsrecord.h"
#include "filters/ensearch.h"
#include <QDomDocument>
#include "QDateTime"
#include "nevernote.h"
#include "xml/noteformatter.h"

ListManager* ListManager::m_pListManager = NULL;

QByteArray ListManager::formatNoteContent(Note* note)
{
	if (!note)
	{
		return "";
	}

	NoteFormatter formatter(logger, conn);
	formatter.setNote(note, Global::pdfPreview());
	formatter.setHighlight(getEnSearch());
	QByteArray js;
	if (!noteCache.contains(note->getGuid()))
	{
		// We need to prepend the note with <HEAD></HEAD> or encoded characters are ugly
		js += ("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">");
		js += ("<style type=\"text/css\">.en-crypt-temp { border-collapse:collapse; border-style:solid; border-color:blue; padding:0.0mm 0.0mm 0.0mm 0.0mm; }</style>");
		js += ("<style type=\"text/css\">en-hilight { background-color: rgb(255,255,0) }</style>");
		js += ("<style> img { height:auto; width:auto; max-height:auto; max-width:100%; }</style>");
		if (Global::displayRightToLeft())
			js += ("<style> body { direction:rtl; }</style>");
		js += ("<style type=\"text/css\">en-spell { text-decoration: none; border-bottom: dotted 1px #cc0000; }</style>");
		js += ("</head>");
		formatter.setNote(note, Global::pdfPreview());
		js += (formatter.rebuildNoteHTML());
		js += ("</HTML>");
		js.replace("<!DOCTYPE en-note SYSTEM 'http://xml.evernote.com/pub/enml.dtd'>", "");
		js.replace("<!DOCTYPE en-note SYSTEM 'http://xml.evernote.com/pub/enml2.dtd'>", "");
		js.replace("<?xml version='1.0' encoding='UTF-8'?>", "");
		noteCache.insert(note->getGuid(), js);
		if (formatter.readOnly)
			readOnlyCache.insert(note->getGuid(), true);
	}// end if (!noteCache.contains(note->getGuid()))
	else
	{
		QString cachedContent = formatter.modifyCachedTodoTags(noteCache.value(note->getGuid()));
		js = cachedContent.toLatin1();
	}

	return js;
}

ListManager::ListManager(DatabaseConnection *d, ApplicationLogger *l)
{
    conn = d;
    logger = l;

    conn->getTagTable()->cleanupTags();
/*    status = new StatusSignal();*/
/*    threadSignals = new ThreadSignal();*/

    // setup index locks
    enSearchChanged = false;

    // Setup arrays
    noteModel = new NoteTableModel(this);
    noteThumbnailModel = new NoteThumbnailModel(this);

    reloadIndexes();

//     notebookSignal = new NotebookSignal();
/*    tagSignal = new TagSignal();*/
/*    trashSignal = new TrashSignal();*/
/*    tagSignal = new TagSignal();*/

    logger->log(EXTREME, "Setting save thread");
    //FIXME
    saveRunner = new SaveRunner(logger, conn);
    bRefreshCounters = true;
    refreshCounters();

	 QList<Note*>& notes = getMasterNoteIndex();
	 for (int i=0; i < notes.size(); i++)
	 {
		 formatNoteContent(notes[i]);
	 }

	 m_pRemoteDataBaseConnection = new RemoteDatabaseConnection(this);
	 m_pSyncCenter = new SyncCenter();

	this->SetupSignalSlotRelated(); 
}

void ListManager::reloadTagIndex()
{
    QStringList allTagNames = conn->getTagTable()->getAllTagName();
    foreach(QString tagName, allTagNames)
    {
        if (tagName.isEmpty())
        {
            continue;
        }

        if (getTagByName(tagName))
        {
            continue;
        }

        if (!getNoteCountByTagName(tagName))
        {
            continue;
        }

        Tag* tag = conn->getTagTable()->getTagByName(tagName);
        if (tag)
        {
            tagIndex << tag;
        }
    }// end foreach(QString tagName, allTagNames)
}

void ListManager::reloadIndexes()
{
    // Load notebooks
    setNotebookIndex(conn->getNotebookTable()->getAll());
	this->SetNoteIndexs(conn->getNoteTable()->getAllNotes());

    // Load search helper utility
    enSearch = new EnSearch(conn,  logger, "", getTagIndex(), Global::getRecognitionWeight());
    logger->log(HIGH, "Building note index");

	noteModel->setMasterNoteIndex(m_listNoteIndex);
//     if (getMasterNoteIndex().isEmpty())
//     {
//         noteModel->setMasterNoteIndex(conn->getNoteTable()->getAllNotes());
//     }
    // For performance reasons, we didn't get the tags for every note individually.  We now need to
    // get them
    QList<NoteTagsRecord> noteTags = conn->getNoteTable()->noteTagsTable->getAllNoteTags();
    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        QList<QString> tags;
        QStringList names;
        for (int j=0; j<noteTags.size(); j++)
        {
            if (getMasterNoteIndex().at(i)->getGuid() == noteTags.at(j).noteGuid)
            {
                QString tmpName = getTagNameByGuid(noteTags.at(j).tagGuid);
                if (!tmpName.isEmpty())
                {
                    tags << noteTags.at(j).tagGuid;
                    names << tmpName;
                }
            }
        }// end for (int j=0; j<noteTags.size(); j++)

        getMasterNoteIndex().at(i)->setTagGuids(tags);
        getMasterNoteIndex().at(i)->setTagNames(names);
    }

    reloadTagIndex();
    setNoteIndex(getMasterNoteIndex());
}

QStringList ListManager::getSelectedNotebooks()
{
    return selectedNotebooks;
}

// Set the current selected notebook(s)
void ListManager::setSelectedNotebooks(QStringList s)
{
    selectedNotebooks = s;
}

//***************************************************************
//***************************************************************
//** These functions deal with setting & retrieving the master lists
//***************************************************************
//***************************************************************
// Get the note table model
NoteTableModel* ListManager::getNoteTableModel()
{
    return noteModel;
}

NoteThumbnailModel* ListManager::getNoteThumberModel()
{
    return noteThumbnailModel;
}

// save the tag index
void ListManager::setTagIndex(QList<Tag*> t)
{
    tagIndex = t;
}

// Retrieve the Tag index
QList<Tag*>& ListManager::getTagIndex()
{
    return tagIndex;
}

void ListManager::setNotebookIndex(QList<Notebook*> t)
{
    notebookIndex = t;
}

// Retrieve the Notebook index
QList<Notebook*>& ListManager::getNotebookIndex()
{
    return notebookIndex;
}

void ListManager::SetNoteIndexs(QList<Note*> lt)
{
	m_listNoteIndex = lt;
}

QList<Note*>& ListManager::GetNoteIndex()
{
	return m_listNoteIndex;
}


// Save the current note QList
void ListManager::setNoteIndex(QList<Note*> n)
{
    noteThumbnailModel->setNoteIndex(n);
    noteModel->setNoteIndex(n);
    refreshNoteMetadata();
}

void ListManager::refreshNoteMetadata()
{
    noteModel->setNoteMetadata(conn->getNoteTable()->getNotesMetaInformation());
}

// Update a note's meta data
void ListManager::updateNoteMetadata(NoteMetadata meta)
{
    if (noteModel->metaData.contains(meta.getGuid()))
    {
        noteModel->metaData[meta.getGuid()] = meta;
    }
    else
    {
        noteModel->metaData.insert(meta.getGuid(), meta);
    }
}
// Get the note index
QList<Note*>& ListManager::getNoteIndex()
{
    return noteModel->getNoteIndex();
}

// Save the count of notes per notebook
void ListManager::setNotebookCounter(QList<NotebookCounter> n)
{
    notebookCounter = n;
    emit refreshNotebookTreeCounts(getNotebookIndex(), notebookCounter);
}

QList<NotebookCounter> ListManager::getNotebookCounter()
{
    return notebookCounter;
}

// Save the count of notes for each tag
void ListManager::setTagCounter(QList<TagCounter> n)
{
    tagCounter = n;
    emit tagrefreshTagTreeCounts(tagCounter);
}

QList<TagCounter> ListManager::getTagCounter()
{
    return tagCounter;
}

// Return a count of items in the trash
int ListManager::getTrashCount()
{
    return trashCount;
}

// get the EnSearch variable
EnSearch* ListManager::getEnSearch()
{
    return enSearch;
}

QList<Note*>& ListManager::getMasterNoteIndex()
{
// 	QList<Note*>& listTemp = noteModel->getMasterNoteIndex();
// 	for (int i=0; i < listTemp.size(); i++)
// 	{
// 		Note* pItem = listTemp[i];
// 		QString uidTemp = pItem->getGuid();
// 		m_mapJSContent[uidTemp] = this->formatNoteContent(pItem);
// 	}

    return m_listNoteIndex;/*noteModel->getMasterNoteIndex();*/
}
// Thumbnails
//	HashMap<QString, QImage> getThumbnails() {
//		return thumbnailList;
//	}
QHash<QString, NoteMetadata> ListManager::getNoteMetadata()
{
    return noteModel->metaData;
}

//***************************************************************
//***************************************************************
//** These functions deal with setting & retrieving filters
//***************************************************************
//***************************************************************
void ListManager::setEnSearch(QString t) {
    enSearch = new EnSearch(conn,logger, t, getTagIndex(), Global::getRecognitionWeight());
    enSearchChanged = true;
}
// Save search tags
void ListManager::setSelectedTags(QStringList tags)
{
    selectedTags = tags;
}

// Get search tags
QStringList ListManager::getSelectedTags()
{
    return selectedTags;
}

//***************************************************************
//***************************************************************
//** Note functions
//***************************************************************
//***************************************************************
// Save Note Tags
void ListManager::saveNoteTags(QString noteGuid, QStringList tags)
{
    logger->log(HIGH, "Entering ListManager.saveNoteTags");
    conn->getNoteTable()->noteTagsTable->deleteNoteTag(noteGuid);
    QList<QString> tagGuids;
    for (int i=0; i< tags.size(); i++)
    {
        for (int j=0; j < tagIndex.size(); j++)
        {
            if (tagIndex.at(j)->getName().trimmed().toLower() == tags.at(i).trimmed().toLower())
            {
                conn->getNoteTable()->noteTagsTable->saveNoteTag(noteGuid, tagIndex.at(j)->getGuid());
                tagGuids << tagIndex.at(j)->getGuid();
                break;
            }
        }// end for (int j=0; j < tagIndex.size(); j++)
    }// end for (int i=0; i< tags.size(); i++)

    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        if (getMasterNoteIndex().at(i)->getGuid() == noteGuid)
        {
            getMasterNoteIndex().at(i)->setTagNames(tags);
            getMasterNoteIndex().at(i)->setTagGuids(tagGuids);
            break;
        }
    }// end for (int i=0; i<getMasterNoteIndex().size(); i++)

    logger->log(HIGH, "Leaving ListManager.saveNoteTags");
}

// Delete a note
void ListManager::deleteNote(QString guid)
{
    //FIXME trashCounterRunner->abortCount = true;
    Note* pNote = NULL;
    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        if (getMasterNoteIndex().at(i)->getGuid() == guid)
        {
            pNote = getMasterNoteIndex().at(i);
            pNote->setActive(false);
            pNote->setDeleted(QDateTime::currentMSecsSinceEpoch());
            break;
        }
    }

    for (int i=0; i<getNoteIndex().size(); i++)
    {
        if (getNoteIndex().at(i)->getGuid() == guid)
        {
            pNote = getNoteIndex().at(i);
            pNote->setActive(false);
            pNote->setDeleted(QDateTime::currentMSecsSinceEpoch());
            break;
        }
    }

    conn->getNoteTable()->deleteNote(guid, 1);

    reloadTrashCount();
}

// Delete a note
void ListManager::restoreNote(QString guid)
{
    //trashCounterRunner->abortCount = true;
    Note* pNote = NULL;
    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        if (getMasterNoteIndex().at(i)->getGuid() == guid)
        {
            pNote = getMasterNoteIndex().at(i);
            pNote->setActive(true);
            pNote->setDeleted(0);
            break;
        }
    }

    for (int i=0; i<getNoteIndex().size(); i++)
    {
        if (getNoteIndex().at(i)->getGuid() == guid)
        {
            pNote = getNoteIndex().at(i);
            pNote->setActive(true);
            pNote->setDeleted(0);
            break;
        }
    }

    conn->getNoteTable()->restoreNote(guid);

    reloadTrashCount();
}

// Add a note.
void ListManager::addNote(Note* n, NoteMetadata meta)
{
    noteModel->addNote(n, meta);
    noteThumbnailModel->addNote(n, meta);
    noteThumbnailModel->UpdateView();
    noteModel->metaData.insert(n->getGuid(), meta);
}

// Expunge a note
void ListManager::expungeNote(QString guid)
{
    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        if (getMasterNoteIndex().at(i)->getGuid() == guid)
        {
            getMasterNoteIndex().removeAt(i);
            break;
        }
    }
    for (int i=0; i<getNoteIndex().size(); i++)
    {
        if (getNoteIndex().at(i)->getGuid() == guid)
        {
            getNoteIndex().removeAt(i);
            break;
        }
    }
    conn->getNoteTable()->expungeNote(guid);
    reloadTrashCount();
}
// Expunge a note
void ListManager::emptyTrash()
{
//   trashCounterRunner->abortCount = true;
    for (int i=getMasterNoteIndex().size()-1; i>=0; i--)
    {
        if (!getMasterNoteIndex().at(i)->isActive())
        {
            getMasterNoteIndex().removeAt(i);
        }
    }

    for (int i=getNoteIndex().size()-1; i>=0; i--)
    {
        if (!getNoteIndex().at(i)->isActive())
        {
            getNoteIndex().removeAt(i);
        }
    }

    conn->getNoteTable()->expungeAllDeletedNotes();
    reloadTrashCount();
}

void ListManager::trashSignalReceiver(int i)
{
    trashCount = i;
    emit trashCountChanged(i);
}

// Update note contents
void ListManager::updateNoteContent(QString guid, QString content)
{
    //FIXME saveRunner->addWork(guid, content);
    updateNoteAlteredDate(guid, QDateTime::currentMSecsSinceEpoch());
    saveRunner->updateNoteContent(guid, content);
    if (noteThumbnailModel)
    {
        noteThumbnailModel->updateNoteContent(guid, content);
    }

    logger->log(HIGH, "Leaving ListManager.updateNoteContent");
}

// Update a note creation date
void ListManager::updateNoteCreatedDate(QString guid, qint64 date)
{
    noteModel->updateNoteCreatedDate(guid, date);
    conn->getNoteTable()->updateNoteCreatedDate(guid, date);
}

// Author has changed
void ListManager::updateNoteAuthor(QString guid, QString author) {
    noteModel->updateNoteAuthor(guid, author);
    conn->getNoteTable()->updateNoteAuthor(guid, author);
}

// Author has changed
void ListManager::updateNoteGeoTag(QString guid, double lon, double lat, double alt)
{
    for (int i=0; i<getMasterNoteIndex().size(); i++) {
        if (getMasterNoteIndex().at(i)->getGuid() == guid) {
            getMasterNoteIndex().at(i)->getAttributes()->setLongitude(lon);
            getMasterNoteIndex().at(i)->getAttributes()->setLatitude(lat);
            getMasterNoteIndex().at(i)->getAttributes()->setAltitude(alt);
            i = getMasterNoteIndex().size();
        }
    }
    // Update the QList tables
    for (int i=0; i<getNoteIndex().size(); i++) {
        if (getNoteIndex().at(i)->getGuid() == guid) {
            getNoteIndex().at(i)->getAttributes()->setLongitude(lon);
            getNoteIndex().at(i)->getAttributes()->setLatitude(lat);
            getNoteIndex().at(i)->getAttributes()->setAltitude(alt);
            i = getNoteIndex().size();
        }
    }
    conn->getNoteTable()->updateNoteGeoTags(guid, lon, lat, alt);
}

// Source URL changed
void ListManager::updateNoteSourceUrl(QString guid, QString url)
{
    noteModel->updateNoteSourceUrl(guid, url);
    conn->getNoteTable()->updateNoteSourceUrl(guid, url);
}

// Update a note last changed date
void ListManager::updateNoteAlteredDate(QString guid, qint64 date)
{
    noteModel->updateNoteChangedDate(guid, date);
    conn->getNoteTable()->updateNoteAlteredDate(guid, date);
}

// Update a note title
void ListManager::updateNoteTitle(QString guid, QString title)
{
    conn->getNoteTable()->updateNoteTitle(guid, title);

    logger->log(HIGH, "Entering ListManager.updateNoteTitle");
    noteModel->updateNoteTitle(guid, title);
    noteThumbnailModel->updateNoteTitle(guid, title);
    logger->log(HIGH, "Leaving ListManager.updateNoteTitle");
}

// Update a note's notebook
void ListManager::updateNoteNotebook(QString guid, QString notebookGuid)
{
    logger->log(HIGH, "Entering ListManager.updateNoteNotebook");
    noteModel->updateNoteNotebook(guid, notebookGuid);
    conn->getNoteTable()->updateNoteNotebook(guid, notebookGuid, true);
    logger->log(HIGH, "Leaving ListManager.updateNoteNotebook");
}

// Update a note sequence number
void ListManager::updateNoteSequence(QString guid, int sequence)
{
    logger->log(HIGH, "Entering ListManager.updateNoteSequence");

    conn->getNoteTable()->updateNoteSequence(guid, sequence);

    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        if (getMasterNoteIndex().at(i)->getGuid() == guid)
        {
            getMasterNoteIndex().at(i)->setUpdateSequenceNum(sequence);
        }
    }

    logger->log(HIGH, "Leaving ListManager.updateNoteSequence");
}

//************************************************************************************
//************************************************************************************
//**  Tag functions
//************************************************************************************
//************************************************************************************
// Update a tag sequence number
void ListManager::updateTagSequence(QString guid, int sequence)
{
    logger->log(HIGH, "Entering ListManager.updateTagSequence");

    conn->getTagTable()->updateTagSequence(guid, sequence);
    for (int i=0; i<tagIndex.size(); i++)
    {
        if (tagIndex.at(i)->getGuid() == guid)
        {
            getTagIndex().at(i)->setUpdateSequenceNum(sequence);
        }
    }
    logger->log(HIGH, "Leaving ListManager.updateTagSequence");
}

void ListManager::updateResourceSequence(QString guid, int sequence)
{
    logger->log(HIGH, "Entering ListManager.updateResourceSequence");

    conn->getNoteTable()->noteResourceTable->updateResourceSequence(guid, sequence);
    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        for (int j = 0; j < getMasterNoteIndex().at(i)->getResources().size(); j++)
        {
            if (!getMasterNoteIndex().at(i)->getResources().at(j))
            {
                continue;
            }

            if (getMasterNoteIndex().at(i)->getResources().at(j)->getGuid() == guid)
            {
                getMasterNoteIndex().at(i)->getResources().at(j)->setUpdateSequenceNum(sequence);
            }
        }// end for (int j = 0; j < getMasterNoteIndex().at(i)->getResources().size(); j++)
    }// end for (int i=0; i<getMasterNoteIndex().size(); i++)

    logger->log(HIGH, "Leaving ListManager.updateResourceSequence");
}

// Update a tag guid number
void ListManager::updateTagGuid(QString oldGuid, QString newGuid)
{
    logger->log(HIGH, "Entering ListManager.updateTagGuid");

    conn->getTagTable()->updateTagGuid(oldGuid, newGuid);
    for (int i=0; i<tagIndex.size(); i++) {
        if (tagIndex.at(i)->getGuid() == oldGuid)
        {
            tagIndex.at(i)->setGuid(newGuid);
            i=tagIndex.size()+1;
        }
    }
    logger->log(HIGH, "Leaving ListManager.updateTagGuid");

}

//************************************************************************************
//************************************************************************************
//**  Notebook functions
//************************************************************************************
//************************************************************************************
// Delete a notebook
void ListManager::deleteNotebook(QString guid)
{
    for (int i=0; i<getNotebookIndex().size(); i++) {
        if (getNotebookIndex().at(i)->getGuid() == guid)
        {
            getNotebookIndex().removeAt(i);
            break;
        }
    }
}

// Rename a stack
void ListManager::renameStack(QString oldName, QString newName) {
    for (int i=0; i<getNotebookIndex().size(); i++) {
        if (!getNotebookIndex().at(i)->getStack().isEmpty() &&
                getNotebookIndex().at(i)->getStack().toLower() == oldName.toLower())
        {
            getNotebookIndex().at(i)->setStack(newName);
        }
    }
}
// Update a notebook sequence number
void ListManager::updateNotebookSequence(QString guid, int sequence)
{
    logger->log(HIGH, "Entering ListManager.updateNotebookSequence");
    conn->getNotebookTable()->updateNotebookSequence(guid, sequence);

    for (int i=0; i<notebookIndex.size(); i++)
    {
        if (notebookIndex.at(i)->getGuid() == guid)
        {
            notebookIndex.at(i)->setUpdateSequenceNum(sequence);
            break;
        }
    }
    logger->log(HIGH, "Leaving ListManager.updateNotebookSequence");

}

// Update a notebook Guid number
void ListManager::updateNotebookStack(QString oldGuid, QString stack) {
    logger->log(HIGH, "Entering ListManager.updateNotebookGuid");

    conn->getNotebookTable()->setStack(oldGuid, stack);

    for (int i=0; i<notebookIndex.size(); i++) {
        if (notebookIndex.at(i)->getGuid() == oldGuid)
        {
            notebookIndex.at(i)->setStack(stack);
            break;
        }
    }
    logger->log(HIGH, "Leaving ListManager.updateNotebookGuid");

}


//************************************************************************************
//************************************************************************************
//**  Load and filter the note index
//************************************************************************************
//************************************************************************************

void ListManager::noteDownloaded(Note* n)
{
    bool found = false;
    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        if (getMasterNoteIndex().at(i)->getGuid() == n->getGuid())
        {
            getMasterNoteIndex()[i]= n;
            found = true;
            break;
        }
    }

    if (!found)
        getMasterNoteIndex() <<n;

    for (int i=0; i<getNoteIndex().size(); i++)
    {
        if (getNoteIndex().at(i)->getGuid() == n->getGuid())
        {
            if (filterRecord(getNoteIndex().at(i)))
                getNoteIndex() << n;
            getNoteIndex().removeAt(i);
            break;
        }
    }

    if (filterRecord(n))
        getNoteIndex() << n;

}
// Check if a note matches the currently selected notebooks, tags, or attribute searches.
bool ListManager::filterRecord(Note* n)
{
    // Check note status
    if (!n->isActive() && !Global::showDeleted)
    {
        return false;
    }
    else if (n->isActive() && Global::showDeleted)
    {
        return false;
    }

    // Begin filtering results
    if (!filterByNotebook(n->getNotebookGuid()))
    {
        return false;
    }

    if (!filterByTag(n->getTagGuids()))
    {
        return false;
    }

    return true;
}

// Trigger a recount of counters
void ListManager::refreshCounters()
{
    if (!bRefreshCounters)
        return;
    bRefreshCounters = false;
    //tagCounterRunner->abortCount = true;
    //notebookCounterRunner->abortCount = true;
    //trashCounterRunner->abortCount = true;
    countNotebookResults(getNoteIndex());
    reloadTrashCount();

}
// Load the note index based upon what the user wants.
void ListManager::loadNotesIndex()
{
    logger->log(EXTREME, "Entering ListManager.loadNotesIndex()");

    QList<Note*> matches;
    if (enSearchChanged)
        matches = enSearch->matchWords();
    else
        matches = getMasterNoteIndex();

    if (matches.isEmpty())
        matches = getMasterNoteIndex();

    setNoteIndex(QList<Note*>());
    for (int i=0; i<matches.size(); i++)
    {
        if (filterRecord(matches.at(i)))
            getNoteIndex() << matches.at(i);
    }

    noteThumbnailModel->setNoteIndex(getNoteIndex());
    noteThumbnailModel->UpdateView();

    bRefreshCounters = true;
    enSearchChanged = false;
    logger->log(EXTREME, "Leaving ListManager.loadNotesIndex()");
}
void ListManager::countNotebookResults(QList<Note*> index)
{
    logger->log(EXTREME, "Entering ListManager.countNotebookResults()");
    //FIXME
    //notebookCounterRunner->abortCount = true;
    //if (!Global::mimicEvernoteInterface)
    //    notebookCounterRunner->setNoteIndex(index);
    //else
    //    notebookCounterRunner->setNoteIndex(getMasterNoteIndex());
    //notebookCounterRunner->release(CounterRunner::NOTEBOOK);
    logger->log(EXTREME, "Leaving ListManager.countNotebookResults()");
}

// Update the count of items in the trash
void ListManager::reloadTrashCount()
{
    logger->log(EXTREME, "Entering ListManager.reloadTrashCount");
    //FIXME
    //trashCounterRunner->abortCount = true;
    //trashCounterRunner->setNoteIndex(getMasterNoteIndex());
    //trashCounterRunner->release(CounterRunner::TRASH);
    logger->log(EXTREME, "Leaving ListManager.reloadTrashCount");
}

bool ListManager::filterByNotebook(QString guid)
{
    bool good = false;
    if (selectedNotebooks.size() == 0)
        good = true;
    if (!good && selectedNotebooks.contains(guid))
        good = true;

    return good;
}
bool ListManager::filterByTag(QList<QString> noteTags)
{
    if (selectedTags.isEmpty())
    {
        return true;
    }

    if (noteTags.isEmpty())
    {
        return false;
    }

    for (int i=0; i<selectedTags.size(); i++)
    {
        QString selectedGuid = selectedTags.at(i);
        bool childMatch = false;
        if (!noteTags.contains(selectedGuid))
            return false;
    }
    return true;
}

void ListManager::setNoteSynchronized(QString guid, bool value)
{
    getNoteTableModel()->updateNoteSyncStatus(guid, value);
}

//********************************************************************************
//********************************************************************************
//* Support signals from the index thread
//********************************************************************************
//********************************************************************************

bool ListManager::threadCheck(int id)
{
    //FIXME
    //if (id == Global::notebookCounterThreadId)
    //    return notebookThread->isRunning();
    //if (id == Global::tagCounterThreadId)
    //    return tagThread->isRunning();
    //if (id == Global::trashCounterThreadId)
    //    return trashThread->isRunning();
    //if (id == Global::saveThreadId)
    //    return saveThread->isRunning();
    //return false;
    return true;
}



//********************************************************************************
//********************************************************************************
//* Utility Functions
//********************************************************************************
//********************************************************************************

// Rebuild the note HTML to something usable
QStringList ListManager::scanNoteForResources(Note* n)
{
    QStringList returnArray;
    logger->log(HIGH, "Entering ListManager.scanNoteForResources");
    logger->log(EXTREME, "Note guid: " +n->getGuid());
    QDomDocument doc;
    bool result = doc.setContent(n->getContent());
    if (!result)
    {
        logger->log(MEDIUM, "Parse error when scanning note for resources.");
        logger->log(MEDIUM, "Note guid: " +n->getGuid());
        return returnArray;
    }

    QDomNodeList anchors = doc.elementsByTagName("en-media");
    for (int i=0; i<anchors.length(); i++)
    {
        QDomElement enmedia = anchors.at(i).toElement();
        if (enmedia.hasAttribute("type"))
        {
            QDomAttr hash = enmedia.attributeNode("hash");
            returnArray << hash.value();
        }
    }
    logger->log(HIGH, "Leaving ListManager.scanNoteForResources");
    return returnArray;
}
// Given a QList of tags, produce a QString QList of tag names
QString ListManager::getTagNamesForNote(Note* n)
{
    QString buffer;
    QStringList v;
    QList<QString> guids = n->getTagGuids();

    if (guids.isEmpty())
        return "";

    for (int i=0; i<guids.size(); i++)
    {
        QString name = getTagNameByGuid(guids.at(i));
        if (!name.isEmpty())
        {
            v << name;
        }
    }// end for (int i=0; i<guids.size(); i++)

    v.sort();
    for (int i = 0; i<v.size(); i++)
    {
        if (i>0)
            buffer += ", ";
        buffer += v.at(i);
    }

    return buffer;
}
// Get a tag name when given a tag guid
QString ListManager::getTagNameByGuid(QString guid)
{
    for (int i=0; i<getTagIndex().size(); i++)
    {
        QString s = getTagIndex().at(i)->getGuid();
        if (s == guid)
        {
            return getTagIndex().at(i)->getName();
        }
    }

    QString name = conn->getTagTable()->getNameByGuid(guid);
    return name;
}

// For a notebook guid, return the name
QString ListManager::getNotebookNameByGuid(QString guid)
{
    if (notebookIndex.isEmpty())
        return "";
    for (int i=0; i<notebookIndex.size(); i++)
    {
        QString s = notebookIndex.at(i)->getGuid();
        if (s == guid)
        {
            return notebookIndex.at(i)->getName();
        }
    }
    return "";
}


// Reload the note's tag names.  This is called when a tag's name changes by
// the user.  It updates all notes with that tag to the new tag name.
void ListManager::reloadNoteTagNames(QString tagGuid, QString newName)
{
    // Set the master index
    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        for (int j=0; j<getMasterNoteIndex().at(i)->getTagGuids().size(); j++)
        {
            if (getMasterNoteIndex().at(i)->getTagGuids().at(j) == tagGuid)
            {
                getMasterNoteIndex().at(i)->getTagNames()[j] = newName;
            }
        }
    }

    // Set the current index
    for (int i=0; i<getNoteIndex().size(); i++)
    {
        for (int j=0; j<getNoteIndex().at(i)->getTagGuids().size(); j++)
        {
            if (getNoteIndex().at(i)->getTagGuids().at(j) == tagGuid)
            {
                getNoteIndex().at(i)->getTagNames()[j] = newName;
            }
        }
    }
}

bool ListManager::isResourceExist(QString guid)
{
    for (int i=0; i<getMasterNoteIndex().size(); i++)
    {
        for (int j = 0; j < getMasterNoteIndex().at(i)->getResources().size(); j++)
        {
            if (!getMasterNoteIndex().at(i)->getResources().at(j))
            {
                continue;
            }

            if (getMasterNoteIndex().at(i)->getResources().at(j)->getGuid() == guid)
            {
                return true;
            }
        }// end for (int j = 0; j < getMasterNoteIndex().at(i)->getResources().size(); j++)
    }// end for (int i=0; i<getMasterNoteIndex().size(); i++)

    return false;
}

Note* ListManager::GetNoteByGuid(QString guid)
{
    QList<Note*>& notes = getMasterNoteIndex();
    foreach(Note* note, notes)
    {
        if (note == NULL)
        {
            continue;
        }
        else if (note->getGuid() == guid)
        {
            return note;
        }
    }

    QList<Note*>& notes2 = getNoteIndex();
    foreach(Note* note, notes2)
    {
        if (note == NULL)
        {
            continue;
        }
        else if (note->getGuid() == guid)
        {
            return note;
        }
    }

    return NULL;
}

Notebook* ListManager::GetNoteBookByGuid(QString guid)
{
    QList<Notebook*> books = getNotebookIndex();
    foreach(Notebook* book, books)
    {
        if (book == NULL)
        {
            continue;
        }
        else if (book->getGuid() == guid)
        {
            return book;
        }
    }

    return NULL;
}

void ListManager::StartDrag(QString guid, QString name, int iType)
{
    foreach(DragInfo info, lstDragInfo)
    {
        if (info.guid == guid && info.iType == iType)
        {
            return;
        }
    }// end foreach(DragInfo info, lstDragInfo)

    DragInfo info;
    info.guid = guid;
    info.iType = iType;
    info.name = name;
    lstDragInfo.push_front(info);
}

void ListManager::FinishDrag(QString guid, int iType)
{
    for (int i = 0; i < lstDragInfo.size();)
    {
        if (lstDragInfo.at(i).guid == guid &&
            lstDragInfo.at(i).iType == iType)
        {
            lstDragInfo.removeAt(i);
            i = 0;
        }
        else
        {
            i++;
        }
    }// end for (int i = 0; i < lstDragInfo.size(); i++)
}

bool ListManager::takePendingDragInfo(QString& guid, QString& name, int& iType)
{
    if (lstDragInfo.empty())
    {
        return false;
    }

    DragInfo info = lstDragInfo.front();
    lstDragInfo.pop_front();
    guid = info.guid;
    name = info.name;
    iType = info.iType;
    return true;
}

Tag* ListManager::getTagByGuid(QString guid)
{
    for (int i = 0; i < tagIndex.size(); i++)
    {
        if (!tagIndex.at(i))
        {
            continue;
        }
        if (tagIndex.at(i)->getGuid() == guid)
        {
            return tagIndex.at(i);
        }
    }// end for (int i = 0; i < notes; i++)
    return NULL;
}

Tag* ListManager::getTagByName(QString name)
{
    for (int i = 0; i < tagIndex.size(); i++)
    {
        if (!tagIndex.at(i))
        {
            continue;
        }
        if (tagIndex.at(i)->getName().trimmed().toLower() == name.trimmed().toLower())
        {
            return tagIndex.at(i);
        }
    }// end for (int i = 0; i < notes; i++)
    return NULL;
}

int ListManager::getNoteCountByNotebookGuid(QString guid)
{
    int iCount = 0;
    QList<Note*> notes = ListManager::getMasterNoteIndex();
    for (int i = 0; i < notes.size(); i++)
    {
        if (!notes.at(i) || !notes.at(i)->isActive())
        {
            continue;
        }

        if (notes.at(i)->getNotebookGuid() == guid)
        {
            iCount++;
        }
    }// end for (int i = 0; i < notes; i++)

    return iCount;
}

int ListManager::getNoteCountByTagGuid(QString guid)
{
	int iCount = 0;
	QList<Note*> notes = getMasterNoteIndex();
	for (int i = 0; i < notes.size(); i++)
	{
		if (!notes.at(i))
		{
			continue;
		}
		if (notes.at(i)->getTagGuids().contains(guid))
		{
			iCount++;
		}
	}// end for (int i = 0; i < notes; i++)

	return iCount;
}

int ListManager::getNoteCountByTagName(QString name)
{
    int iCount = 0;
    QList<Note*> notes = ListManager::getMasterNoteIndex();
    for (int i = 0; i < notes.size(); i++)
    {
        if (!notes.at(i))
        {
            continue;
        }
        if (notes.at(i)->getTagNames().contains(name.trimmed()))
        {
            iCount++;
        }
    }// end for (int i = 0; i < notes; i++)

    return iCount;
}

QList<NoteTagsRecord> ListManager::getAllNoteTags()
{
    return conn->getNoteTable()->noteTagsTable->getAllNoteTags();
}


DatabaseConnection* ListManager::GetDataBaseConnection()
{
	return conn;
}

RemoteDatabaseConnection* ListManager::GetRemoteDataBaseConnection()
{
	return m_pRemoteDataBaseConnection;
}

SyncCenter* ListManager::GetSyncCenter()
{
	return m_pSyncCenter;
}

QStringList ListManager::GetInvalidElements()
{
	return conn->getInvalidXMLTable()->getInvalidElements();
}

QStringList ListManager::GetInvalidAttributeElements()
{
	return conn->getInvalidXMLTable()->getInvalidAttributeElements();
}

void ListManager::HandleNoteBookTask(const DownloadedTask& task)
{

}

void ListManager::HandleResourceTask(const DownloadedTask& task)
{
	if (task.objectType != E_SYNC_OBJECT_RESOURCE)
	{
		return;
	}
	if (task.operType != E_OPER_ADD)
	{
		return;
	}

	Resource* r = new Resource();
	r->setGuid(task.resourceDataBody.guid);
	r->setNoteGuid(task.resourceDataBody.noteGuid);
	r->setMime(task.resourceDataBody.mime);
	r->setActive(true);
	r->setUpdateSequenceNum(task.resourceDataBody.updateSequenceNumber);
	r->setWidth((short) 0);
	r->setHeight((short) 0);
	r->setDuration((short) 0);

	ResourceData d;
	d.size = task.resourceDataBody.dataBinary.size();
	d.hexHash = task.resourceDataBody.hash.toLatin1();
	d.Data = task.resourceDataBody.dataBinary;
	r->setData(d);

	ResourceAttributes* a = new ResourceAttributes();
	a->setAltitude(0);
	a->setLongitude(0);
	a->setLatitude(0);
	a->setCameraMake("");
	a->setCameraModel("");
	a->setAttachment(task.resourceDataBody.isAttachment);
	a->setClientWillIndex(false);
	a->setSourceURL("");
	a->setTimestamp(0);
	a->setFileName(task.resourceDataBody.fileName);
	r->setAttributes(a);
	conn->getNoteTable()->noteResourceTable->saveNoteResource(r, true);
	delete r;

}

void ListManager::HandleNoteTask(const DownloadedTask& task)
{
	if (task.objectType != E_SYNC_OBJECT_NOTE)
	{
		return;
	}

	if (task.operType == E_OPER_DELETE)
	{
		for (int j=this->getNoteTableModel()->rowCount()-1; j>=0; j--)
		{
			QModelIndex modelIndex =  this->getNoteTableModel()->index(j, Global::noteTableGuidPosition);
			if (modelIndex.isValid())
			{
				QMap<int, QVariant> ix = this->getNoteTableModel()->itemData(modelIndex);
				QString tableGuid =  ix.values().at(0).toString();
				if (tableGuid == task.noteDataBody.guid)
				{
					this->getNoteTableModel()->removeRow(j);
					break;
				}
			}
		}// end for (int j=listManager->getNoteTableModel()->rowCount()-1; j>=0; j--)

		this->expungeNote(task.noteDataBody.guid);
		for (int i=0; i<this->getMasterNoteIndex().size(); i++)
		{
			if (this->getMasterNoteIndex().at(i)->getGuid() == task.noteDataBody.guid)
			{
				this->getMasterNoteIndex().removeAt(i);
				break;
			}
		}
		for (int i=0; i<this->getNoteIndex().size(); i++)
		{
			if (this->getNoteIndex().at(i)->getGuid() == task.noteDataBody.guid)
			{
				this->getNoteIndex().removeAt(i);
				break;
			}
		}
		conn->getNoteTable()->expungeNote(task.noteDataBody.guid);
		return;
	}// end if (task.operType == E_OPER_DELETE)

	conn->getNoteTable()->receiveDownloadTask(task);
	Note* pNote = conn->getNoteTable()->getNote(task.noteDataBody.guid, true, true, true, true);
	if (!this->GetNoteByGuid(task.noteDataBody.guid))
	{
		//listManager->getMasterNoteIndex()<<pNote;
		NoteMetadata metadata;
		metadata.setGuid(pNote->getGuid());
		metadata.setDirty(true);
		this->addNote(pNote, metadata);
	}
	else
	{
		for (int i=0; i<this->getMasterNoteIndex().size(); i++)
		{
			if (this->getMasterNoteIndex().at(i)->getGuid() == task.noteDataBody.guid)
			{
				this->getMasterNoteIndex().removeAt(i);
				this->getMasterNoteIndex()<<pNote;
				break;
			}
		}
		for (int i=0; i<this->getNoteIndex().size(); i++)
		{
			if (this->getNoteIndex().at(i)->getGuid() == task.noteDataBody.guid)
			{
				this->getNoteIndex().removeAt(i);
				this->getNoteIndex()<<pNote;
				break;
			}
		}
	}// end else
}

void ListManager::HandleTagTask(const DownloadedTask& task)
{
	// all task for update data by gui.so this is nothing.
}

void ListManager::HandleNoteTagTask(const DownloadedTask& task)
{

}

void ListManager::UpdateNoteTags(QString guid, QStringList tags)
{



}

void ListManager::RemoveListTagName(QString guid)
{
	logger->log(HIGH, "Entering NeverNote.updateTagName");

	for (int j=0; j<this->getMasterNoteIndex().size(); j++)
	{
		if (this->getMasterNoteIndex().at(j)->getTagGuids().contains(guid))
		{
			for (int i=this->getMasterNoteIndex().at(j)->getTagGuids().size()-1; i>=0; i--)
			{
				if (this->getMasterNoteIndex().at(j)->getTagGuids().at(i) == guid)
				{
					this->getMasterNoteIndex().at(j)->getTagGuids().removeAt(i);
					this->getMasterNoteIndex().at(j)->getTagNames().removeAt(i);
				}
			}

			QString newName = this->getTagNamesForNote(this->getMasterNoteIndex().at(j));
			for (int i=0; i<this->getNoteTableModel()->rowCount(); i++)
			{
				QModelIndex modelIndex =  this->getNoteTableModel()->index(i, Global::noteTableGuidPosition);
				if (modelIndex.isValid())
				{
					QMap<int, QVariant> ix = this->getNoteTableModel()->itemData(modelIndex);
					QString noteGuid = ix.values().at(0).toString();
					if (noteGuid == this->getMasterNoteIndex().at(j)->getGuid())
					{
						QModelIndex tempIndex = this->getNoteTableModel()->index(i, Global::noteTableTagPosition);
						if(tempIndex.isValid())
						{
							this->getNoteTableModel()->setData(tempIndex, newName, Qt::EditRole);
						}
						break;
					}
				}
			}
		}// end if (listManager->getMasterNoteIndex().at(j)->getTagGuids().contains(guid))
	}// end for (int j=0; j<listManager->getMasterNoteIndex().size(); j++)
	logger->log(HIGH, "Leaving NeverNote.updateListNotebook");

}

void ListManager::RemoveTagItem(QString guid)
{
	//Now, remove this tag
	RemoveListTagName(guid);
	conn->getTagTable()->expungeTag(guid);
	for (int a=0; a<this->getTagIndex().size(); a++)
	{
		if (this->getTagIndex().at(a)->getGuid() == guid)
		{
			this->getTagIndex().removeAt(a);
			return;
		}
	}
}

QStringList ListManager::GetNewTagsFromGivenTags(const QStringList& tags)
{
	QStringList newTags;
	for (int i = 0; i < tags.size(); i++)
	{
		bool bFound = false;
		for (int j = 0;  j < this->getTagIndex().size(); j++)
		{
			if (!this->getTagIndex().at(j))
			{
				continue;
			}

			if (this->getTagIndex().at(j)->getName().trimmed().toLower() == tags.at(i).trimmed().toLower())
			{
				bFound = true;
			}
		}// end for (int j = 0;  j < listManager->getTagIndex().size(); j++)

		if (!bFound)
		{
			newTags << tags.at(i);
		}
	}// end for (int i = 0; i < tags.size(); i++)

	return newTags;
}

void ListManager::AddNewTagsFromTagsList(const QStringList& tags)
{
	for (int i = 0; i < tags.size(); i++)
	{
		Tag* nTag = new Tag();
		nTag->setName(tags.at(i).trimmed());
		nTag->setUpdateSequenceNum(0);
		nTag->setGuid(Global::createUuid());
		conn->getTagTable()->addTag(nTag, true);
		this->getTagIndex() << nTag;
	}
}

QStringList ListManager::GetTagNamesListForNote(Note* n) 
{
	QStringList v;

	logger->log(HIGH, "Entering NeverNote.getTagNamesForNote");
	if (n == NULL || n->getGuid().isEmpty())
		return v;

	QList<QString> guids = n->getTagGuids();

	if (guids.isEmpty())
		return v;

	for (int i=0; i<guids.size(); i++)
	{
		QString tmpName = this->getTagNameByGuid(guids.at(i));
		if (!tmpName.isEmpty())
		{
			v << tmpName;
		}
	}// end for (int i=0; i<guids.size(); i++)

	v.sort();
	logger->log(HIGH, "Leaving NeverNote.getTagNamesForNote");
	return v;
}


void ListManager::SlotReceiveDownloadedTask(const DownloadedTask& downloadTask)
{
	if (downloadTask.objectType == E_SYNC_OBJECT_NOTEBOOK)
	{
		HandleNoteBookTask(downloadTask);
	}// end if (downloadTaskLst.at(0).objectType == E_SYNC_OBJECT_NOTEBOOK)
	else if (downloadTask.objectType == E_SYNC_OBJECT_RESOURCE)
	{
		HandleResourceTask(downloadTask);
	}
	else if (downloadTask.objectType == E_SYNC_OBJECT_NOTE)
	{
		HandleNoteTask(downloadTask);
	}
	else if (downloadTask.objectType == E_SYNC_OBJECT_TAG)
	{
		//		HandleTagTask(downloadTask);
	}
	else if (downloadTask.objectType == E_SYNC_OBJECT_NOTETAG)
	{
		//		HandleNoteTagTask(downloadTask);
	}

	emit SignalUpdateByDownloadedTask(downloadTask);
}

void ListManager::SetupSignalSlotRelated()
{
	//QObject::connect(m_pRemoteDataBaseConnection, SIGNAL(noteUpdateSequenceNumberChanged(QString, int)), this, SLOT(updateNoteSequence(QString, int)));
	//QObject::connect(m_pRemoteDataBaseConnection, SIGNAL(tagUpdateSequenceNumberChanged(QString, int)), this, SLOT(updateTagSequence(QString, int)));
	//QObject::connect(m_pRemoteDataBaseConnection, SIGNAL(notebookUpdateSequenceNumberChanged(QString, int)), this, SLOT(updateNotebookSequence(QString, int)));
	//QObject::connect(m_pRemoteDataBaseConnection, SIGNAL(resourceUpdateSequenceNumberChanged(QString, int)), this, SLOT(updateResourceSequence(QString, int)));
	//QObject::connect(m_pRemoteDataBaseConnection, SIGNAL(receiveDownloadedTask(const QList<DownloadedTask>&)), this, SLOT(SlotReceiveDownloadedTask(const QList<DownloadedTask>&)),Qt::QueuedConnection);

	QObject::connect(m_pSyncCenter, SIGNAL(noteUpdateSequenceNumberChanged(QString, int)), this, SLOT(updateNoteSequence(QString, int)));
	QObject::connect(m_pSyncCenter, SIGNAL(tagUpdateSequenceNumberChanged(QString, int)), this, SLOT(updateTagSequence(QString, int)));
	QObject::connect(m_pSyncCenter, SIGNAL(notebookUpdateSequenceNumberChanged(QString, int)), this, SLOT(updateNotebookSequence(QString, int)));
	QObject::connect(m_pSyncCenter, SIGNAL(resourceUpdateSequenceNumberChanged(QString, int)), this, SLOT(updateResourceSequence(QString, int)));
	QObject::connect(m_pSyncCenter, SIGNAL(receiveDownloadedTask(const DownloadedTask&)), this, SLOT(SlotReceiveDownloadedTask(const DownloadedTask&)),Qt::QueuedConnection);

}

ListManager* ListManager::GetListManagerInstance()
{
	if (NULL == m_pListManager)
	{
		ApplicationLogger* dataBaseLogger = new ApplicationLogger("logs/simpleNote-database.log");
		QString f = Global::getFileManager()->getDbDirPath(Global::databaseName + ".sqlite");
		QString fr = Global::getFileManager()->getDbDirPath(Global::resourceDatabaseName + ".sqlite");
		QString fi = Global::getFileManager()->getDbDirPath(Global::resourceDatabaseName + ".sqlite");
		if (!QFileInfo(f).exists())
			Global::setDatabaseUrl("");
		if (!QFileInfo(fr).exists())
			Global::setResourceDatabaseUrl("");
		if (!QFileInfo(fi).exists())
			Global::setIndexDatabaseUrl("");
		DatabaseConnection* conn = new DatabaseConnection(dataBaseLogger);
		conn->dbSetup(Global::getDatabaseUrl(), Global::getIndexDatabaseUrl(), Global::getResourceDatabaseUrl(),
                      Global::getDatabaseUserid(), Global::getDatabaseUserPassword(), Global::cipherPassword, "MainThread");
		ApplicationLogger* logger = new ApplicationLogger("logs/ListManagerLog.log");
		m_pListManager = new ListManager(conn, logger);

	}
	return m_pListManager;
}
