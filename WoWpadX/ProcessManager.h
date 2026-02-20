#pragma once

#include <windows.h>
#include <QString>
#include <QStringList>
#include <QRect>
#include <QTimer>
#include <QProcess>
#include <QMutex>

class ProcessManager
{
public:
    static void start();
    static void stop();

    static bool isGameRunning();
    static QRect getClientRectangle();

    static HANDLE getGameProcessHandle();
    static qint64 getGameProcessId();
    static HWND getGameProcessWindowHandle();

    // You can manually connect to these callbacks elsewhere if needed.
    static std::function<void(qint64, const QString&)> onGameProcessFound;
    static std::function<void()> onGameProcessExited;

private:
    static void processLoop();
    static void detachMemoryReader();
    static bool isValidGameProcess(QProcess* process);

    static QTimer* m_timer;
    static HANDLE m_gameProcess;
    static qint64 m_gameProcessId;
    static QString m_gameProcessName;
    static QStringList m_knownProcessNames;
    static QMutex m_mutex;
};
