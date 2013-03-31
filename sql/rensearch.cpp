#include "rensearch.h"
#include "notetable.h"
#include "Global.h"
#include <QRegExp>
#include <QSqlError>

REnSearch::REnSearch(DatabaseConnection *c, ApplicationLogger *l, QString s, QList<Tag *> t, int r)
{
    logger = l;
    conn = c;
    tagIndex = t;
    minimumRecognitionWeight = r;
    any = false;

    if (s == NULL)
        return;

    if (s.trimmed().isEmpty())
        return;

    resolveSearch(s);
}

bool REnSearch::matchTagsAll(QStringList tagNames, QStringList list)
{
    for (int j=0; j<list.size(); j++)
    {
        bool negative = false;
        negative = false;
        if (list.at(j).startsWith("-"))
            negative = true;

        int pos = list.at(j).indexOf(":");
        QString filterName = cleanupWord(list.at(j).mid(pos+1));
        filterName.replace("*", ".*");   // setup for regular expression pattern match

        if (tagNames.size() == 0 && !negative)
            return false;

        bool matchFound = false;
        for (int i=0; i<tagNames.size(); i++)
        {
            QRegExp Pattern(filterName.toLower());
            if (Pattern.indexIn(tagNames.at(i).toLower()) != -1)
                matchFound = true;
        }

        if (negative)
            matchFound = !matchFound;
        if (!matchFound)
            return false;
    }
    return true;
}

// match tag names
bool REnSearch::matchTagsAny(QStringList tagNames, QStringList list)
{
    if (list.size() == 0)
        return true;

    for (int j=0; j<list.size(); j++)
    {
        bool negative = false;
        if (list.at(j).startsWith("-"))
            negative = true;
        int pos = list.at(j).indexOf(":");
        QString filterName = cleanupWord(list.at(j).mid(pos+1));
        filterName = filterName.replace("*", ".*");   // setup for regular expression pattern match

        if (tagNames.size() == 0 && !negative)
            return false;

        for (int i=0; i<tagNames.size(); i++)
        {
            QRegExp Pattern(filterName.toLower());
            if (Pattern.indexIn(tagNames.at(i).toLower()) != -1 && !negative)
                return false;
        }
    }
    return true;
}

// Match notebooks in search terms against notes
bool REnSearch::matchNotebook(QString guid)
{
    if (getNotebooks().size() == 0)
        return true;

    NotebookTable bookTable(logger, conn);
    QList<Notebook*> books = bookTable.getAll();

    QString name;
    for (int i=0; i<books.size(); i++)
    {
        if (guid.compare(books.at(i)->getGuid(), Qt::CaseInsensitive) == 0)
        {
            name = books.at(i)->getName();
            break;
        }
    }// end for (int i=0; i<books.size(); i++)

    if (any)
        return matchListAny(getNotebooks(), name);
    else
        return matchListAll(getNotebooks(), name);
}

// Match notebooks in search terms against notes
bool REnSearch::matchNotebookStack(QString guid)
{
    if (getStack().size() == 0)
        return true;

    NotebookTable bookTable(logger, conn);
    QList<Notebook*> books = bookTable.getAll();

    QString name;
    for (int i=0; i<books.size(); i++)
    {
        if (guid.compare(books.at(i)->getGuid(), Qt::CaseInsensitive) == 0)
        {
            name = books.at(i)->getStack();
            i=books.size();
        }
    }

    if (any)
        return matchListAny(getStack(), name);
    else
        return matchListAll(getStack(), name);
}

// Match notebooks in search terms against notes
bool REnSearch::matchListAny(QStringList list, QString title)
{
    if (list.size() == 0)
        return true;

    bool negative = false;
    bool found = false;

    for (int i=0; i<list.size(); i++)
    {
        int pos = list.at(i).indexOf(":");
        negative = false;
        if (list.at(i).indexOf("-") == 0)
            negative = true;

        QString filterName = cleanupWord(list.at(i).mid(pos+1));
        filterName = filterName.replace("*", ".*");   // setup for regular expression pattern match
        QRegExp Pattern(filterName.toLower());
        if (Pattern.indexIn(title.toLower()) != -1)
            found = true;
    }

    if (negative)
        return !found;
    else
        return found;
}

// Match notebooks in search terms against notes
bool REnSearch::matchContentAny(Note* note)
{
    if (todo.size() == 0 && resource.size() == 0 && searchPhrases.size() == 0)
        return true;

    // pull back the record
    Note* n = conn->getNoteTable()->getNote(note->getGuid(), true, true, false, false);

    // Check for search phrases
    QString text ;// TODO = StringEscapeUtils.unescapeHtml4(n->getContent().replace("\\<.*?\\>", "").toLower());
    bool negative = false;
    for (int i=0; i<searchPhrases.size(); i++)
    {
        QString phrase = searchPhrases.at(i);
        if (phrase.startsWith("-"))
        {
            negative = true;
            phrase = phrase.mid(1);
        }
        else
            negative = false;

        phrase = phrase.mid(1);
        phrase = phrase.mid(0,phrase.length()-1);
        if (text.indexOf(phrase)>=0)
        {
            if (negative)
                return false;
            else
                return true;
        }
        if (text.indexOf(phrase)<0 && negative)
            return true;
    }

    for (int i=0; i<todo.size(); i++)
    {
        QString value = todo.at(i);
        value = value.replace("\"", "");
        bool desiredState;
        if (!value.endsWith(":false") && !value.endsWith(":true") && !value.endsWith(":*") && !value.endsWith("*"))
            return false;

        if (value.endsWith(":false"))
            desiredState = false;
        else
            desiredState = true;

        if (value.startsWith("-"))
            desiredState = !desiredState;

        int pos = n->getContent().indexOf("<en-todo");

        if (pos == -1 && value.startsWith("-") && (value.endsWith("*") || value.endsWith(":")))
            return true;

        if (value.endsWith("*"))
            return true;

        while (pos > -1)
        {
            int endPos = n->getContent().indexOf("/>", pos);
            QString segment = n->getContent().mid(pos, endPos);
            bool currentState;
            if (segment.toLower().indexOf("checked=\"true\"") == -1)
                currentState = false;
            else
                currentState = true;
            if (desiredState == currentState)
                return true;

            pos = n->getContent().indexOf("<en-todo", pos+1);
        }
    }

    // Check resources
    for (int i=0; i<resource.size(); i++)
    {
        QString resourceString = resource.at(i);
        resourceString = resourceString.replace("\"", "");
        if (resourceString.startsWith("-"))
            negative = true;
        resourceString = resourceString.mid(resourceString.indexOf(":")+1);
        for (int j=0; j<n->getResourcesSize(); j++) {
            bool match = stringMatch(n->getResources().at(j)->getMime(), resourceString, negative);
            if (match)
                return true;
        }
    }
    return false;
}

// Take the initial search & split it apart
void REnSearch::resolveSearch(QString search)
{
    QStringList words;
    QString b = search;

    int len = search.length();
    QChar nextChar = ' ';
    bool quote = false;
    for (int i=0, j=0; i<len; i++, j++)
    {
        if (search[i]==nextChar && !quote)
        {
            b[j] = '\0';
            nextChar = ' ';
        }
        else
        {
            if (search[i]=='\"')
            {
                if (!quote)
                {
                    quote=true;
                }
                else
                {
                    quote=false;
                    j++;
                    b.insert(j, "\0");
                }
            }
        }
        if (((i+2)<len) && search[i] == '\\')
        {
            i=i+2;
        }
    }

    search = b;
    int pos = 0;
    for (int i=0; i<search.length(); i++)
    {
        if (search[i] == '\0')
        {
            search = search.mid(1);
            i=0;
        }
        else
        {
            pos = search.indexOf(QChar('\0'));
            if (pos > 0)
            {
                words << search.mid(0,pos).toLower();
                search = search.mid(pos);
                i=0;
            }
        }
    }

    if (search[0]=='\0')
        words << search.mid(1).toLower();
    else
        words << search.toLower();
    parseTerms(words);
}

void REnSearch::parseTerms(QStringList words)
{
    for (int i=0; i<words.size(); i++)
    {
        QString word = words.at(i);
        int pos = word.indexOf(":");
        if (word.startsWith("any:"))
        {
            any = true;
            word = word.mid(4).trimmed();
            pos = word.indexOf(":");
        }

        bool searchPhrase = false;
        if (pos < 0 && word.indexOf(" ") > 0)
        {
            searchPhrase=true;
            searchPhrases<<(word.toLower());
        }

        if (!searchPhrase && pos < 0)
        {
            if (word != NULL && word.length() > 0 && !Global::automaticWildcardSearches())
                getWords()<<word;
            if (word != NULL && word.length() > 0 && Global::automaticWildcardSearches())
            {
                QString wildcardWord = word;
                if (!wildcardWord.startsWith("*"))
                    wildcardWord = "*"+wildcardWord;
                if (!wildcardWord.endsWith("*"))
                    wildcardWord = wildcardWord+"*";
                getWords()<<(wildcardWord);
            }

        }
        if (word.startsWith("intitle:"))
            intitle << ("*"+word+"*");
        if (word.startsWith("-intitle:"))
            intitle << ("*"+word+"*");
        if (word.startsWith("notebook:"))
            notebooks<<word;
        if (word.startsWith("-notebook:"))
            notebooks<<word;
        if (word.startsWith("tag:"))
            tags<<word;
        if (word.startsWith("-tag:"))
            tags<<word;
        if (word.startsWith("resource:"))
            resource<<word;
        if (word.startsWith("-resource:"))
            resource<<word;
        if (word.startsWith("author:"))
            author<<word;
        if (word.startsWith("-author:"))
            author<<word;
        if (word.startsWith("source:"))
            source<<word;
        if (word.startsWith("-source:"))
            source<<word;
        if (word.startsWith("sourceapplication:"))
            sourceApplication<<word;
        if (word.startsWith("-sourceapplication:"))
            sourceApplication<<word;
        if (word.startsWith("recotype:"))
            recoType<<word;
        if (word.startsWith("-recotype:"))
            recoType<<word;
        if (word.startsWith("todo:"))
            todo<<word;
        if (word.startsWith("-todo:"))
            todo<<word;
        if (word.startsWith("stack:"))
            stack<<word;
        if (word.startsWith("-stack:"))
            stack<<word;

        if (word.startsWith("latitude:"))
            latitude<<word;
        if (word.startsWith("-latitude:"))
            latitude<<word;
        if (word.startsWith("longitude:"))
            longitude<<word;
        if (word.startsWith("-longitude:"))
            longitude<<word;
        if (word.startsWith("altitude:"))
            altitude<<word;
        if (word.startsWith("-altitude:"))
            altitude<<word;

        if (word.startsWith("created:"))
            created<<word;
        if (word.startsWith("-created:"))
            created<<word;
        if (word.startsWith("updated:"))
            updated<<word;
        if (word.startsWith("-updated:"))
            updated<<word;
    }
}

// Match notebooks in search terms against notes
bool REnSearch::matchListAll(QStringList list, QString title)
{
    if (list.size() == 0)
        return true;
    bool negative = false;
    for (int i=0; i<list.size(); i++)
    {
        int pos = list.at(i).indexOf(":");
        negative = false;
        if (list.at(i).startsWith("-"))
            negative = true;

        QString filterName = cleanupWord(list.at(i).mid(pos+1));
        filterName = filterName.replace("*", ".*");   // setup for regular expression pattern match
        QRegExp Pattern(filterName.toLower());
        if (Pattern.indexIn(title.toLower()) != -1 && negative)
            return false;
        if (Pattern.indexIn(title.toLower()) != -1 && !negative)
            return true;
    }

    if (negative)
        return true;
    else
        return false;
}

// Match notebooks in search terms against notes
bool REnSearch::matchContentAll(Note* note)
{
    if (todo.size() == 0 && resource.size() == 0 && searchPhrases.size() == 0)
        return true;

    Note* n = conn->getNoteTable()->getNote(note->getGuid(), true, true, false, false);

    // Check for search phrases
    QString text; //TODO = StringEscapeUtils.unescapeHtml4(n->getContent().replace("\\<.*?\\>", "")).toLower();
    bool negative = false;
    for (int i=0; i<searchPhrases.size(); i++)
    {
        QString phrase = searchPhrases.at(i);
        if (phrase.startsWith("-"))
        {
            negative = true;
            phrase = phrase.mid(1);
        }
        else
            negative = false;

        phrase = phrase.mid(1);
        phrase = phrase.mid(0,phrase.length()-1);
        if (text.indexOf(phrase)>=0 && negative)
        {
            return false;
        }
        if (text.indexOf(phrase) < 0 && !negative)
            return false;
    }


    for (int i=0; i<todo.size(); i++)
    {
        QString value = todo.at(i);
        value = value.replace("\"", "");
        bool desiredState;
        if (!value.endsWith(":false") && !value.endsWith(":true") && !value.endsWith(":*") && !value.endsWith("*"))
            return false;
        if (value.endsWith(":false"))
            desiredState = false;
        else
            desiredState = true;
        if (value.startsWith("-"))
            desiredState = !desiredState;
        int pos = n->getContent().indexOf("<en-todo");
        if (pos == -1 && !value.startsWith("-"))
            return false;
        if (pos > -1 && value.startsWith("-") && (value.endsWith("*") || value.endsWith(":")))
            return false;
        if (pos == -1 && !value.startsWith("-"))
            return false;
        bool returnTodo = false;
        while (pos > -1)
        {
            int endPos = n->getContent().indexOf(">", pos);
            QString segment = n->getContent().mid(pos, endPos);
            bool currentState;
            if (segment.toLower().indexOf("checked=\"true\"") == -1)
                currentState = false;
            else
                currentState = true;
            if (desiredState == currentState)
                returnTodo = true;
            if (value.endsWith("*") || value.endsWith(":"))
                returnTodo = true;

            pos = n->getContent().indexOf("<en-todo", pos+1);
        }
        if (!returnTodo)
            return false;
    }

    // Check resources
    for (int i=0; i<resource.size(); i++)
    {
        QString resourceString = resource.at(i);
        resourceString = resourceString.replace("\"", "");
        negative = false;
        if (resourceString.startsWith("-"))
            negative = true;
        resourceString = resourceString.mid(resourceString.indexOf(":")+1);

        if (resourceString.trimmed().isEmpty())
            return false;

        for (int j=0; j<n->getResourcesSize(); j++)
        {
            bool match = stringMatch(n->getResources().at(j)->getMime(), resourceString, negative);
            if (!match && !negative)
                return false;
            if (match && negative)
                return false;
        }
    }

    return true;
}

bool REnSearch::stringMatch(QString content, QString text, bool negative)
{
    QString regex;
    if (content.isEmpty() && !negative)
        return false;
    if (content.isEmpty() && negative)
        return true;

    if (text.endsWith("*"))
    {
        text = text.mid(0,text.length()-1);
        regex = text;
    } else {
        regex = text;
    }

    content = content.toLower();
    regex = regex.toLower();
    bool matches = content.startsWith(regex);
    if (negative)
        return !matches;
    return matches;
}

// Remove odd strings from search terms
QString REnSearch::cleanupWord(QString word)
{
    if (word.startsWith("\""))
        word = word.mid(1);

    if (word.endsWith("\""))
        word = word.mid(0,word.length()-1);

    word = word.replace("\\\"","\"");
    word = word.replace("\\\\","\\");

    return word;
}

// Match dates
bool REnSearch::matchDatesAll(QStringList dates, long noteDate)
{
    if (dates.size()== 0)
        return true;

    bool negative = false;
    for (int i=0; i<dates.size(); i++)
    {
        QString requiredDate = dates.at(i);
        if (requiredDate.startsWith("-"))
            negative = true;

        int response = 0;
        requiredDate = requiredDate.mid(requiredDate.indexOf(":")+1);
        response = dateCheck(requiredDate, noteDate);
        if (negative && response < 0)
            return false;
        if (!negative && response > 0)
            return false;
    }
    return true;
}

bool REnSearch::matchDatesAny(QStringList dates, long noteDate)
{
    if (dates.size()== 0)
        return true;

    bool negative = false;
    for (int i=0; i<dates.size(); i++)
    {
        QString requiredDate = dates.at(i);
        if (requiredDate.startsWith("-"))
            negative = true;

        int response = 0;
        requiredDate = requiredDate.mid(requiredDate.indexOf(":")+1);
        response = dateCheck(requiredDate, noteDate);
        if (negative && response > 0)
            return true;
        if (!negative && response < 0)
            return true;
    }
    return false;
}

//****************************************
//****************************************
// Match search terms against notes
//****************************************
//****************************************
QList<Note*> REnSearch::matchWords()
{
    logger->log(EXTREME, "Inside EnSearch.matchWords()");
    bool subSelect = false;

    NoteTable noteTable(logger, conn);
    QStringList validGuids;

    if (searchWords.size() > 0)
        subSelect = true;

    QSqlQuery query(conn->getConnection());

    // Build a temp table for GUID results
    if (!conn->dbTableExists("SEARCH_RESULTS"))
    {
        query.exec("create temporary table SEARCH_RESULTS (guid varchar)");
        query.exec("create temporary table SEARCH_RESULTS_MERGE (guid varchar)");
    }
    else
    {
        query. exec("Delete from SEARCH_RESULTS");
        query. exec("Delete from SEARCH_RESULTS_MERGE");
    }

    QSqlQuery insertQuery(conn->getConnection());
    QSqlQuery indexQuery(conn->getIndexConnection());
    QSqlQuery mergeQuery(conn->getConnection());
    QSqlQuery deleteQuery(conn->getConnection());

    insertQuery.prepare("Insert into SEARCH_RESULTS (guid) values (:guid)");
    mergeQuery.prepare("Insert into SEARCH_RESULTS_MERGE (guid) values (:guid)");

    if (subSelect)
    {
        for (int i=0; i<getWords().size(); i++)
        {
            if (getWords().at(i).indexOf("*") == -1)
            {
                indexQuery.prepare("Select distinct guid from words where weight >= " + QString::number(minimumRecognitionWeight) +
                                   " and word=:word");
                indexQuery.bindValue(":word", getWords().at(i));
            }
            else
            {
                indexQuery.prepare("Select distinct guid from words where weight >= " + QString::number(minimumRecognitionWeight) +
                                   " and word like :word");

                QString word = getWords().at(i);
                indexQuery.bindValue(":word", word.replace(QChar('*'), QChar('%')));
            }

            indexQuery.exec();
            while(indexQuery.next())
            {
                QString guid = indexQuery.value(0).toString();
                if (i==0 || any)
                {
                    insertQuery.bindValue(":guid", guid);
                    insertQuery.exec();
                }
                else
                {
                    mergeQuery.bindValue(":guid", guid);
                    mergeQuery.exec();
                }
            }
            if (i>0 && !any)
            {
                deleteQuery.exec("Delete from SEARCH_RESULTS where guid not in (select guid from SEARCH_RESULTS_MERGE)");
                deleteQuery.exec("Delete from SEARCH_RESULTS_MERGE");
            }
        }

        query.prepare("Select distinct guid from Note where guid in (Select guid from SEARCH_RESULTS)");
        if (!query.exec())
            logger->log(LOW, "Error merging search results:" + query.lastError().text());

        while (query.next())
        {
            validGuids<< query.value(0).toString();
        }
    }

    QList<Note*> noteIndex = noteTable.getAllNotes();
    QList<Note*> guids;

    for (int i=0; i<noteIndex.size(); i++)
    {
        Note* n = noteIndex.at(i);
        bool good = true;

        if (!validGuids.contains(n->getGuid()) && subSelect)
            good = false;

        // Start matching special stuff, like tags & notebooks
        if (any)
        {
            if (good && !matchTagsAny(n->getTagNames(), getTags()))
                good = false;
            if (good && !matchNotebook(n->getNotebookGuid()))
                good = false;
            if (good && !matchNotebookStack(n->getNotebookGuid()))
                good = false;
            if (good && !matchListAny(getIntitle(), n->getTitle()))
                good = false;
            if (good && !matchListAny(getAuthor(), n->getAttributes()->getAuthor()))
                good = false;
            if (good && !matchContentAny(n))
                good = false;
            if (good && !matchDatesAny(getCreated(), n->getCreated()))
                good = false;
            if (good && !matchDatesAny(getUpdated(), n->getUpdated()))
                good = false;
        }
        else
        {
            if (good && !matchTagsAll(n->getTagNames(), getTags()))
                good = false;
            if (good && !matchNotebook(n->getNotebookGuid()))
                good = false;
            if (good && !matchNotebookStack(n->getNotebookGuid()))
                good = false;
            if (good && !matchListAll(getIntitle(), n->getTitle()))
                good = false;
            if (good && !matchListAll(getAuthor(), n->getAttributes()->getAuthor()))
                good = false;
            if (good && !matchContentAll(n))
                good = false;
            if (good && !matchDatesAll(getCreated(), n->getCreated()))
                good = false;
            if (good && !matchDatesAll(getUpdated(), n->getUpdated()))
                good = false;
        }
        if (good)
        {
            guids<<(n);
        }
    }

    // For performance reasons, we didn't get the tags for every note individually.  We now need to
    // get them
    //TODO
    //    QList<NoteTagsRecord> noteTags = noteTable.noteTagsTable->getAllNoteTags();
    //    for (int i=0; i<guids.size(); i++)
    //    {
    //        QStringList tags;
    //        QStringList names;
    //        for (int j=0; j<noteTags.size(); j++)
    //        {
    //            if (guids.at(i).getGuid().equals(noteTags.get(j).noteGuid))
    //            {
    //                tags<<(noteTags.get(j).tagGuid);
    //                names<<(getTagNameByGuid(noteTags.get(j).tagGuid));
    //            }
    //        }

    //        guids.get(i).setTagGuids(tags);
    //        guids.get(i).setTagNames(names);
    //    };
    logger->log(EXTREME, "Leaving EnSearch.matchWords()");
    return guids;
}

QString REnSearch::getTagNameByGuid(QString guid)
{
    for (int i=0; i<tagIndex.size(); i++)
    {
        if (tagIndex.at(i)->getGuid() == guid)
            return tagIndex.at(i)->getName();
    }
    return "";
}

// Compare dates
int REnSearch::dateCheck(QString date, long noteDate)
{
    return 0;
    //    int offset = 0;
    //    bool found = false;
    //    GregorianCalendar calendar = new GregorianCalendar();

    //    if (date.contains("-")) {
    //        QString modifier = date.mid(date.indexOf("-")+1);
    //        offset = new Integer(modifier);
    //        offset = 0-offset;
    //        date = date.mid(0,date.indexOf("-"));
    //    }

    //    if (date.contains("+")) {
    //        QString modifier = date.mid(date.indexOf("+")+1);
    //        offset = new Integer(modifier);
    //        date = date.mid(0,date.indexOf("+"));
    //    }

    //    if (date.equalsIgnoreCase("today")) {
    //        calendar<<(Calendar.DATE, offset);
    //        calendar.set(Calendar.HOUR, 0);
    //        calendar.set(Calendar.MINUTE, 0);
    //        calendar.set(Calendar.SECOND, 1);
    //        found = true;
    //    }

    //    if (date.equalsIgnoreCase("month")) {
    //        calendar<<(Calendar.MONTH, offset);
    //        calendar.set(Calendar.DAY_OF_MONTH, 1);
    //        calendar.set(Calendar.HOUR, 0);
    //        calendar.set(Calendar.MINUTE, 0);
    //        calendar.set(Calendar.SECOND, 1);
    //        found = true;
    //    }

    //    if (date.equalsIgnoreCase("year")) {
    //        calendar<<(Calendar.YEAR, offset);
    //        calendar.set(Calendar.MONTH, Calendar.JANUARY);
    //        calendar.set(Calendar.DAY_OF_MONTH, 1);
    //        calendar.set(Calendar.HOUR, 0);
    //        calendar.set(Calendar.MINUTE, 0);
    //        calendar.set(Calendar.SECOND, 1);
    //        found = true;
    //    }

    //    if (date.equalsIgnoreCase("week")) {
    //        calendar<<(Calendar.DATE, 0-calendar.get(Calendar.DAY_OF_WEEK)+1);
    //        calendar<<(Calendar.DATE,(offset*7));
    //        calendar.set(Calendar.HOUR, 0);
    //        calendar.set(Calendar.MINUTE, 0);
    //        calendar.set(Calendar.SECOND, 1);

    //        found = true;
    //    }

    //    // If nothing was found, then we have a date number
    //    if (!found) {
    //        calendar = stringToGregorianCalendar(date);
    //    }


    //    QString dateTimeFormat = new QString("yyyyMMdd-HHmmss");
    //    SimpleDateFormat simple = new SimpleDateFormat(dateTimeFormat);
    //    StringBuilder creationDate = new StringBuilder(simple.format(noteDate));
    //    GregorianCalendar nCalendar = stringToGregorianCalendar(creationDate.toString().replace("-", "T"));
    //    if (calendar == NULL || nCalendar == NULL)  // If we have something invalid, it automatically fails
    //        return 1;
    //    return calendar.compareTo(nCalendar);
}
//TODO
//GregorianCalendar REnSearch::stringToGregorianCalendar(QString date)
//{
//    QString datePart = date;
//    GregorianCalendar calendar = new GregorianCalendar();
//    bool GMT = false;
//    QString timePart = "";
//    if (date.contains("T")) {
//        datePart = date.mid(0,date.indexOf("T"));
//        timePart = date.mid(date.indexOf("T")+1);
//    } else {
//        timePart = "000001";
//    }
//    if (datePart.length() != 8)
//        return NULL;
//    calendar.set(Calendar.YEAR, new Integer(datePart.mid(0,4)));
//    calendar.set(Calendar.MONTH, new Integer(datePart.mid(4,6))-1);
//    calendar.set(Calendar.DAY_OF_MONTH, new Integer(datePart.mid(6)));
//    if (timePart.endsWith("Z")) {
//        GMT = true;
//        timePart = timePart.mid(0,timePart.length()-1);
//    }
//    timePart = timePart.concat("000000");
//    timePart = timePart.mid(0,6);
//    calendar.set(Calendar.HOUR, new Integer(timePart.mid(0,2)));
//    calendar.set(Calendar.MINUTE, new Integer(timePart.mid(2,4)));
//    calendar.set(Calendar.SECOND, new Integer(timePart.mid(4)));
//    if (GMT)
//        calendar.set(Calendar.ZONE_OFFSET, -1*(calendar.get(Calendar.ZONE_OFFSET)/(1000*60*60)));
//    return calendar;
//}
