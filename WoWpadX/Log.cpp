#include "Log.h"
#include "AppSettings.h"

#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QTextStream>

QFile* Log::m_file = nullptr;
QTextStream* Log::m_stream = nullptr;
QMutex Log::m_mutex;

void Log::initialize()
{
    QString defaultPath = "log.txt";
    QString userPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/log.txt";

    if (m_file)
        return;

    QFile* file = new QFile(defaultPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Text)) {
        QDir().mkpath(QFileInfo(userPath).absolutePath());
        file = new QFile(userPath);
        file->open(QIODevice::WriteOnly | QIODevice::Text);
    }

    m_file = file;
    m_stream = new QTextStream(m_file); 
}

void Log::writeLine(const QString& text, bool useDateTime)
{
    QString finalText = useDateTime
        ? QString("[%1] %2").arg(QTime::currentTime().toString("HH:mm:ss"), text)
        : text;

    qDebug().noquote() << finalText;

    if (AppSettings::instance()->enableLogging() && m_stream) {
        QMutexLocker locker(&m_mutex);
        (*m_stream) << finalText << Qt::endl;
    }
}
