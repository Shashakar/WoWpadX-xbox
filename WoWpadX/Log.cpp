#include "Log.h"

#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QTextStream>

namespace
{
    bool IsTemporaryDiagnostic(const QString& text)
    {
        return text.startsWith("[InputDiag]") ||
            text.startsWith("[Bindings] Effective modifier profile") ||
            text.startsWith("[RawInput] First direct Ally stick state") ||
            text.startsWith("[RawInput] First direct Ally trigger state") ||
            text.startsWith("[RawInput] Ally neutral-stick report") ||
            text.startsWith("[NativeGameInput] Reading available") ||
            text.startsWith("[NativeGameInput] No gamepad reading available") ||
            text.startsWith("[NativeGameInput] First non-neutral background reading");
    }
}

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
    if (IsTemporaryDiagnostic(text))
        return;

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
