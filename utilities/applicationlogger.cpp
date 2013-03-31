#include "utilities/applicationlogger.h"
#include <QDateTime>
#include "utilities/applicationlogger.h"

ApplicationLogger::ApplicationLogger(QString name)
{
    m_TextStream = NULL;
    m_file = new QFile(name);
    if (m_file->open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_TextStream = new QTextStream(m_file);
    }
}

ApplicationLogger::~ApplicationLogger()
{
    if (m_TextStream)
    {
        delete m_TextStream;
    }

    if (m_file)
    {
        m_file->close();
        delete m_file;
    }// end if (m_file)
}

void ApplicationLogger::log(ELogLevel messageLevel, QString s)
{
    //if (messageLevel < MAX)
    //{
    //    if (m_TextStream)
    //    {
    //        QString strDateFormat = "yyyy-MM-dd HH:mm:ss.zzz ";
    //        *m_TextStream << QDateTime::currentDateTime().toString(strDateFormat)+ s <<"\n";
    //        m_TextStream->flush();
    //    }// end if (m_TextStream)
    //}// end if (messageLevel < MAX)
}
