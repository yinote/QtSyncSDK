#ifndef RENSEARCH_H
#define RENSEARCH_H

#include <QList>
#include <QStringList>
#include <QString>
#include "evernote/note.h"
#include "utilities/applicationlogger.h"
#include "sql/databaseconnection.h"

class REnSearch
{
public:
    REnSearch(DatabaseConnection* c, ApplicationLogger* l, QString s, QList<Tag*> t, int r);

    QStringList getWords() { return searchWords; }
    QStringList getNotebooks() { return notebooks; }
    QStringList getIntitle() {	return intitle; }
    QStringList getTags() {	return tags; }
    QStringList getResource() {	return resource; }
    QStringList getAuthor() { return author; }
    QStringList getSource() { return source; }
    QStringList getSourceApplication() { return sourceApplication; }
    QStringList getRecoType() {	return recoType; }
    QStringList getToDo() { return todo; }
    QStringList getLongitude() { return longitude; }
    QStringList getLatitude() { return latitude; }
    QStringList getAltitude() { return altitude; }
    QStringList getCreated() { return created; }
    QStringList getUpdated() { return updated; }
    QStringList getStack() { return stack; }

    QList<Note*> matchWords();

    // Compare dates
    int dateCheck(QString date, long noteDate);

protected:
    bool matchTagsAll(QStringList tagNames, QStringList QList);

    // match tag names
    bool matchTagsAny(QStringList tagNames, QStringList QList);

    // Match notebooks in search terms against notes
    bool matchNotebook(QString guid);

    // Match notebooks in search terms against notes
    bool matchNotebookStack(QString guid);

    // Match notebooks in search terms against notes
    bool matchListAny(QStringList QList, QString title);

    // Match notebooks in search terms against notes
    bool matchContentAny(Note* n);

    // Take the initial search & split it apart
    void resolveSearch(QString search);

    void parseTerms(QStringList words);

    // Match notebooks in search terms against notes
    bool matchListAll(QStringList QList, QString title);

    // Match notebooks in search terms against notes
    bool matchContentAll(Note* note) ;

    bool stringMatch(QString content, QString text, bool negative) ;

    // Remove odd strings from search terms
    QString cleanupWord(QString word);

    // Match dates
    bool matchDatesAll(QStringList dates, long noteDate);
    bool matchDatesAny(QStringList dates, long noteDate);

    //****************************************
    //****************************************
    // Match search terms against notes
    //****************************************
    //****************************************

    QString getTagNameByGuid(QString guid);
    //TODO
    //GregorianCalendar stringToGregorianCalendar(QString date);

private:
    QStringList     searchWords;
    QStringList     searchPhrases;
    QStringList 	notebooks;
    QStringList 	tags;
    QStringList 	intitle;
    QStringList     created;
    QStringList     updated;
    QStringList 	resource;
    QStringList     longitude;
    QStringList     latitude;
    QStringList     altitude;
    QStringList     author;
    QStringList     source;
    QStringList     sourceApplication;
    QStringList     recoType;
    QStringList     todo;
    QStringList     stack;
    QList<Tag*>		tagIndex;
    ApplicationLogger* logger;
    bool            any;
    int             minimumRecognitionWeight;
    DatabaseConnection* conn;
};

#endif // RENSEARCH_H
