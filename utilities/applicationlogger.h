#ifndef APPLICATIONLOGGER_H
#define APPLICATIONLOGGER_H

#include <QFile>
#include <QTextStream>

enum ELogLevel
{
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    EXTREME = 4,
    MAX = 5
};

class ApplicationLogger
{
public:
    ApplicationLogger(QString name);
    ~ApplicationLogger();
    void log(ELogLevel messageLevel, QString s);

private:
    QFile*          m_file;
    QTextStream*    m_TextStream;
};

#endif // APPLICATIONLOGGER_H
