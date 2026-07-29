#include "Log.h"

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

    // Logging is used while AppSettings itself is being constructed. Calling
    // AppSettings::instance() here creates a recursive singleton construction
    // path and eventually raises STATUS_STACK_OVERFLOW (0xC00000FD).
    // Keep the low-level logger independent from application settings.
    if (m_stream) {
        QMutexLocker locker(&m_mutex);
        (*m_stream) << finalText << Qt::endl;
    }
}
