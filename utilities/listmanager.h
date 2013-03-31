#ifndef LISTMANAGER_H
#define LISTMANAGER_H

#include <QString>
#include <QList>
#include <QPair>
#include <QHash>
#include <QSqlQuery>
#include <QStringList>
#include <QThread>
#include "QFile"
#include "QFileInfo"
#include "QImage"
#include "evernote/tag.h"
#include "evernote/notebook.h"
#include "utilities/applicationlogger.h"
#include "sql/databaseconnection.h"
#include "filters/ensearch.h"
//#include "signals/threadsignal.h"
#include "filters/tagcounter.h"
#include "threads/saverunner.h"
#include "threads/counterrunner.h"
#include "gui/notetablemodel.h"
#include "filters/NotebookCounter.h"
#include "gui/notethumbnailmodel.h"
#include <QObject>
#include "sql/remotedatabaseconnection.h"
#include "sql/synccenter.h"

class ListManager: public QObject
{
    Q_OBJECT

signals:
	void listChanged();
	void refreshNotebookTreeCounts(QList<Notebook*>, QList<NotebookCounter>);
	void countsChanged(QList<NotebookCounter>);

	void taglistChanged();
	void tagrefreshTagTreeCounts(QList<TagCounter>);
	void tagcountsChanged(QList<TagCounter>);

	void trashCountChanged(int);

	void SignalUpdateByDownloadedTask(const DownloadedTask&);

public slots:
	void SlotReceiveDownloadedTask(const DownloadedTask& downloadTask);

public:
	void AddNewTagsFromTagsList(const QStringList& tags);
	QStringList GetNewTagsFromGivenTags(const QStringList& tags);
	QStringList GetTagNamesListForNote(Note* n) ;
	void RemoveListTagName(QString guid);
	void RemoveTagItem(QString guid);
	void UpdateNoteTags(QString guid, QStringList tags);
	void HandleNoteBookTask(const DownloadedTask& task);
	void HandleResourceTask(const DownloadedTask& task);
	void HandleNoteTask(const DownloadedTask& task);
	void HandleTagTask(const DownloadedTask& task);
	void HandleNoteTagTask(const DownloadedTask& task);
	
	QStringList GetInvalidAttributeElements();
	QStringList GetInvalidElements();
	RemoteDatabaseConnection* GetRemoteDataBaseConnection();
	DatabaseConnection* GetDataBaseConnection();
	SyncCenter* GetSyncCenter();

    void reloadTagIndex();
    void reloadIndexes();
    QStringList getSelectedNotebooks();

    Note* GetNoteByGuid(QString guid);
    Notebook* GetNoteBookByGuid(QString guid);
    int     getNoteCountByNotebookGuid(QString guid);
    bool  isResourceExist(QString guid);

    QList<NoteTagsRecord> getAllNoteTags();
   
    // Set the current selected notebook(s)
    void setSelectedNotebooks(QStringList s);
    NoteTableModel* getNoteTableModel();
    NoteThumbnailModel* getNoteThumberModel();

	QList<Note*>& GetNoteIndex();
    QList<Tag *> &getTagIndex();
    QList<Notebook *> &getNotebookIndex();
    QString getTagNamesForNote(Note* n);
    QString getTagNameByGuid(QString guid);
    int     getNoteCountByTagName(QString name);
	int     getNoteCountByTagGuid(QString guid);
    Tag*    getTagByName(QString name);
    Tag*    getTagByGuid(QString guid);

    void refreshNoteMetadata();
    void updateNoteMetadata(NoteMetadata meta);
    QList<Note *> &getNoteIndex();
    void setNotebookCounter(QList<NotebookCounter> n);
    QList<NotebookCounter> getNotebookCounter();

    // Save the count of notes for each tag
    void setTagCounter(QList<TagCounter> n);
    QList<TagCounter> getTagCounter();

    // Return a count of items in the trash
    int getTrashCount();
    // get the EnSearch variable
    EnSearch* getEnSearch();
    QList<Note*>& getMasterNoteIndex();
    QHash<QString, NoteMetadata> getNoteMetadata();

    void setEnSearch(QString t);
    // Save search tags
    void setSelectedTags(QStringList selectedTags);

    // Get search tags
    QStringList getSelectedTags();

    void saveNoteTags(QString noteGuid, QStringList tags);
    void deleteNote(QString guid);
    void restoreNote(QString guid);
    void addNote(Note* n, NoteMetadata meta);
    void expungeNote(QString guid);
    void emptyTrash();
    void updateNoteContent(QString guid, QString content);

    // Update a note's notebook
    void updateNoteNotebook(QString guid, QString notebookGuid);
    void updateTagGuid(QString oldGuid, QString newGuid);
    void deleteNotebook(QString guid);
    void renameStack(QString oldName, QString newName);

    void updateNotebookStack(QString oldGuid, QString stack);
    void noteDownloaded(Note* n);
    bool filterRecord(Note* n) ;
    void refreshCounters();
    void loadNotesIndex();
    void countNotebookResults(QList<Note*> index) ;
    void reloadTrashCount();
    bool filterByNotebook(QString guid);
    bool filterByTag(QList<QString> noteTags);
    void setNoteSynchronized(QString guid, bool value);

    bool threadCheck(int id);
    QStringList scanNoteForResources(Note* n);
    QString getNotebookNameByGuid(QString guid);
    void reloadNoteTagNames(QString tagGuid, QString newName);

    // ÍÏ×§µÄÖÐ×ªÕ¾
    void StartDrag(QString guid, QString name, int iType);
    void FinishDrag(QString guid, int iType);
    bool takePendingDragInfo(QString& guid, QString& name, int& iType);
    SaveRunner*				saveRunner;					// Thread used to save content.  Used because the xml conversion is slowwwww
    bool 					bRefreshCounters;			// Used to control when to recount QLists
    
    QHash<QString, QByteArray>	noteCache;			// Cash of note content
    QHash<QString, bool>	readOnlyCache;		// List of cashe notes that are read-only
    QHash<QString, QImage>    imageCache;

public slots:
    void trashSignalReceiver(int i);
    // Update a note title
    void updateNoteTitle(QString guid, QString title);
    void updateNoteCreatedDate(QString guid, qint64 date);
    void updateNoteAlteredDate(QString guid, qint64 date);
    void updateNoteAuthor(QString guid, QString author);
    void updateNoteGeoTag(QString guid, double lon, double lat, double alt);
    void updateNoteSourceUrl(QString guid, QString url);

    void updateNotebookSequence(QString guid, int sequence);
    void updateNoteSequence(QString guid, int sequence);
    void updateTagSequence(QString guid, int sequence);
    void updateResourceSequence(QString guid, int sequence);

protected:

	void SetupSignalSlotRelated();
	 ListManager(DatabaseConnection* d, ApplicationLogger* l);
	 ListManager(){};
	QByteArray ListManager::formatNoteContent(Note* note);

	void SetNoteIndexs(QList<Note*> lt);

    void setTagIndex(QList<Tag*> t);
    void setNotebookIndex(QList<Notebook*> t);
    void setArchiveNotebookIndex(QList<Notebook*> t) ;
    void setNoteIndex(QList<Note*> n);

    QHash<QString, QString>	wordMap;

public:
	static ListManager* GetListManagerInstance();

private:

	RemoteDatabaseConnection* m_pRemoteDataBaseConnection;
	SyncCenter*					m_pSyncCenter;
	static ListManager*			m_pListManager;

    ApplicationLogger* 		logger;
    DatabaseConnection*     conn;

    QList<Tag*>				tagIndex;
    QList<Notebook*>		notebookIndex;

	QList<Note*>   m_listNoteIndex;

    QStringList             selectedNotebooks;

    NoteTableModel*         noteModel;
    NoteThumbnailModel*     noteThumbnailModel;

    QStringList             selectedTags;

    QList<NotebookCounter>	notebookCounter;				// count of displayed notes in each notebook
    QList<TagCounter>		tagCounter;						// count of displayed notes for each tag

    EnSearch*				enSearch;
    bool                    enSearchChanged;
    int						trashCount;
    struct DragInfo
    {
        QString guid;
        QString name;
        int  iType;
    };
    QList<DragInfo> lstDragInfo;
};

#endif // LISTMANAGER_H
