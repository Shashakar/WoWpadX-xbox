#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>

class Log
{
public:
    static void initialize();
    static void writeLine(const QString& text, bool useDateTime = true);

private:
    static QFile* m_file;
    static QTextStream* m_stream;
    static QMutex m_mutex;
};
