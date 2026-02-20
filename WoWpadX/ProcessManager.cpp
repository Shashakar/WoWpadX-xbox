#include "ProcessManager.h"
#include "AppSettings.h"
#include "Log.h"
#include "WoWReader.h"
#include "OverlayLoader.h"

#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QDebug>
#include <windows.h>
#include <TlHelp32.h>
#include <functional>

QTimer* ProcessManager::m_timer = nullptr;
HANDLE ProcessManager::m_gameProcess = nullptr;
qint64 ProcessManager::m_gameProcessId = 0;
QString ProcessManager::m_gameProcessName = "";
QStringList ProcessManager::m_knownProcessNames = {};
QMutex ProcessManager::m_mutex;

std::function<void(qint64, const QString&)> ProcessManager::onGameProcessFound = nullptr;
std::function<void()> ProcessManager::onGameProcessExited = nullptr;

static HWND tempWnd = nullptr;

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
    DWORD wndProcId = 0;
    GetWindowThreadProcessId(handle, &wndProcId);

    if (lParam != wndProcId)
        return TRUE;

    tempWnd = handle;
    return FALSE;
}

HWND GetProcessWindow(qint64 pid)
{
    tempWnd = nullptr;
    EnumWindows(EnumWindowsCallback, pid);
    return tempWnd;
}

void ProcessManager::start()
{
    if (!m_timer) {
        m_timer = new QTimer();
        QObject::connect(m_timer, &QTimer::timeout, []() {
            processLoop();
        });
        m_knownProcessNames = AppSettings::instance()->gameProcessNames();
    }

    m_timer->start(500); // 500 ms polling
}

void ProcessManager::stop()
{
    if (m_timer)
        m_timer->stop();

    detachMemoryReader();

    if (m_gameProcess) {
        CloseHandle(m_gameProcess);
        m_gameProcess = nullptr;
    }

    m_gameProcessId = 0;
    m_gameProcessName.clear();
}

HANDLE ProcessManager::getGameProcessHandle()
{
    return m_gameProcess;
}

qint64 ProcessManager::getGameProcessId()
{
    return m_gameProcessId;
}

bool ProcessManager::isGameRunning()
{
    if (!m_gameProcess)
        return false;

    DWORD exitCode;
    if (GetExitCodeProcess(m_gameProcess, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;

}

HWND ProcessManager::getGameProcessWindowHandle()
{
    if (m_gameProcessId == 0)
        return nullptr;

    return GetProcessWindow(m_gameProcessId);
}

QRect ProcessManager::getClientRectangle()
{
    if (!m_gameProcess)
        return QRect();

    HWND hwnd = getGameProcessWindowHandle();
    if (!hwnd)
        return QRect();

    RECT clientRect = { 0 }, windowRect = { 0 };
    GetClientRect(hwnd, &clientRect);
    GetWindowRect(hwnd, &windowRect);

    int borderWidth = (windowRect.right - windowRect.left - clientRect.right) / 2;
    int titleHeight = (windowRect.bottom - windowRect.top) - clientRect.bottom - borderWidth;

    return QRect(windowRect.left + borderWidth, windowRect.top + titleHeight,
        clientRect.right, clientRect.bottom);
}

void ProcessManager::processLoop()
{
    if (!m_gameProcess)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return;

        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(snapshot, &entry))
        {
            do
            {
                QString exeName = QString::fromWCharArray(entry.szExeFile).toLower(); 
                if (exeName.endsWith(".exe"))
                    exeName.chop(4);


                m_knownProcessNames = AppSettings::instance()->gameProcessNames();

                if (m_knownProcessNames.contains(exeName))
                {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);
                    if (hProcess)
                    {
                        m_gameProcess = hProcess;
                        m_gameProcessId = entry.th32ProcessID;
                        m_gameProcessName = exeName;

                        Log::writeLine(QString("Found game process: [%1: %2]")
                            .arg(m_gameProcessId)
                            .arg(m_gameProcessName));

                        if (AppSettings::instance()->enableOverlay())
                        {
                            OverlayLoader::Load(m_gameProcessId, "WoWpadXOverlay.dll");
                        }

                        if (AppSettings::instance()->enableMemoryReading())
                            WoWReader::init(getGameProcessWindowHandle());

                        if (onGameProcessFound)
                            onGameProcessFound(m_gameProcessId, m_gameProcessName);

                        CloseHandle(snapshot);
                        return;
                    }
                }

            } while (Process32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
    }
    else
    {
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(m_gameProcess, &exitCode) || exitCode != STILL_ACTIVE)
        {
            Log::writeLine(QString("Process [%1: %2] has exited")
                .arg(m_gameProcessId)
                .arg(m_gameProcessName));

            detachMemoryReader();
            CloseHandle(m_gameProcess);
            m_gameProcess = nullptr;
            m_gameProcessId = 0;
            m_gameProcessName.clear();

            if (onGameProcessExited)
                onGameProcessExited();
        }
        else
        {
            if (AppSettings::instance()->enableMemoryReading())
            {
                HWND currentHwnd = getGameProcessWindowHandle();

                if (currentHwnd != nullptr && currentHwnd != WoWReader::getWindowHandle()) {
                    Log::writeLine("Game window handle changed. Re-initializing Pixel Bridge.");
                    WoWReader::init(currentHwnd);
                }

                if (!WoWReader::isAttached())
                    WoWReader::init(getGameProcessWindowHandle());
            }
            else
            {
                if (WoWReader::isAttached())
                    detachMemoryReader();
            }
        }
    }
}

void ProcessManager::detachMemoryReader()
{
    if (WoWReader::isAttached())
        WoWReader::close();
}

bool ProcessManager::isValidGameProcess(QProcess* process)
{
    return process && process->state() == QProcess::Running;
}
