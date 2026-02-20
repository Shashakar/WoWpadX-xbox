#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <QProcess>
#include <QQuickWindow>
#include <QSGRendererInterface> 
#include <qqmlcontext.h>
#include <qfileinfo.h>
#include <QLocalServer>
#include <QLocalSocket>
#include <iostream>
#include <windows.h>
#include <atomic>

#include "ProcessManager.h"
#include "InputMapper.h"
#include "BindManager.h"
#include "AppSettings.h"
#include "ControllerManager.h"
#include "MainPageController.h"
#include "DevicesController.h"
#include "MemoryReadingController.h"
#include "TrayManager.h" 
#include <shellscalingapi.h> // Add this include
#pragma comment(lib, "Shcore.lib")

const QString APP_VERSION = MainPageController::appVersion();
const QString IPC_SERVER_NAME = "WoWpadX_IPC_Server";
std::atomic<bool> headlessRunning(true);

/**
 * Headless execution path: Launches the game and runs mapping services
 * in a console-attached state without initializing the Qt GUI loop.
 */
int runWithProcessLaunch(const QString& exePath, bool keepAlive)
{
    QFileInfo fileInfo(exePath);
    if (!fileInfo.exists() || !fileInfo.isExecutable()) {
        Log::writeLine("Executable not found: " + exePath);
        return 1;
    }

    // Initialize core non-GUI services
    AppSettings::instance();
    BindManager::loadBindings();
    ControllerManager::instance()->startWatcher();
    InputMapper::Start();
    ProcessManager::start();

    // Start IPC Server for Headless mode so it can be stopped via --stop
    QLocalServer ipcServer;
    QObject::connect(&ipcServer, &QLocalServer::newConnection, [&]() {
        QLocalSocket* client = ipcServer.nextPendingConnection();
        QObject::connect(client, &QLocalSocket::readyRead, [&, client]() {
            if (client->readAll() == "QUIT") {
                Log::writeLine("IPC Shutdown signal received.");
                headlessRunning = false;
            }
        });
    });
    ipcServer.listen(IPC_SERVER_NAME);

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    std::wstring wPath = exePath.toStdWString();

    if (!CreateProcessW(NULL, &wPath[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        Log::writeLine("Failed to launch process: " + exePath);
        return 1;
    }

    Log::writeLine("Game launched (PID: " + QString::number(pi.dwProcessId) + "). Mapping active...");

    // Wait for game or IPC signal
    if (!keepAlive) {
        WaitForSingleObject(pi.hProcess, INFINITE);
    }
    else {
        Log::writeLine("Headless Keep-Alive active. Use 'WoWpadX.exe --stop' to exit.");
        while (headlessRunning) {
            if (WaitForSingleObject(pi.hProcess, 100) == WAIT_OBJECT_0) {
                // Game exited, but we stay alive because keepAlive is true
                Log::writeLine("Game process exited. Staying alive per -k flag.");
                break;
            }
        }
        // If we broke the game wait but keepAlive is true, continue waiting for IPC signal
        while (headlessRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    Log::writeLine("Closing WoWpadX...");
    InputMapper::Stop();
    ControllerManager::instance()->stopWatcher();
    ProcessManager::stop();

    return 0;
}

void showConsoleHelp() {
#ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
    }
#endif
    std::cout << "\nWoWpadX v" << APP_VERSION.toStdString() << " - Gamepad Mapper for WoW 3.3.5a / ConsolePortLK\n";
    std::cout << "---------------------------------------------------------------\n";
    std::cout << "Usage: WoWpadX.exe [options]\n\n";
    std::cout << "Actions:\n";
    std::cout << "  -l <path>      Launch the specified game executable and start mapping.\n";
    std::cout << "  --stop         Sends a shutdown signal to the running instance.\n\n";
    std::cout << "Modifiers (used with -l):\n";
    std::cout << "  -n             No-GUI (Headless) mode. Runs as a background driver.\n";
    std::cout << "  -m             Minimized mode. Launches game and hides WoWpadX in tray.\n";
    std::cout << "  -k             Keep WoWpadX running even after the game process exits.\n\n";
    std::cout << "General:\n";
    std::cout << "  -v             Display current version information.\n";
    std::cout << "  -h, --help     Show this help documentation.\n";
    std::cout << "---------------------------------------------------------------\n" << std::endl;
#ifdef _WIN32
    FreeConsole();
#endif
}

int main(int argc, char* argv[])
{
    QStringList args;
    for (int i = 1; i < argc; ++i) args << argv[i];

    // Single Instance & IPC Command Check
    const wchar_t* mutexName = L"Global\\3F9E2252-6837-4FBA-9A1B-7A87EB150F6C";
    HANDLE hMutex = CreateMutexW(NULL, FALSE, mutexName);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (args.contains("--stop")) {
            QLocalSocket socket;
            socket.connectToServer(IPC_SERVER_NAME);
            if (socket.waitForConnected(1000)) {
                socket.write("QUIT");
                socket.waitForBytesWritten(1000);
            }
            return 0;
        }
        MessageBoxW(nullptr, L"Another instance of WoWpadX is already open", L"Already running", MB_ICONERROR | MB_OK);
        return 0;
    }

    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/Libs");
    Log::initialize();

    if (args.contains("-v")) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            FILE* fp; freopen_s(&fp, "CONOUT$", "w", stdout);
            std::cout << "WoWpadX Version: " << APP_VERSION.toStdString() << std::endl;
            FreeConsole();
        }
        return 0;
    }

    if (args.contains("-h") || args.contains("--help") || args.contains("-help")) {
        showConsoleHelp();
        return 0;
    }

    int lIndex = args.indexOf("-l");
    if (lIndex != -1 && lIndex + 1 < args.size()) {
        if (args.contains("-n")) { 
            SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
            return runWithProcessLaunch(args[lIndex + 1], args.contains("-k"));
        }
    }

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/Resources/wowpadx.png"));

    // GUI IPC Server (so GUI mode can also be stopped via --stop)
    QLocalServer guiIpc;
    QObject::connect(&guiIpc, &QLocalServer::newConnection, [&]() {
        QLocalSocket* client = guiIpc.nextPendingConnection();
        QObject::connect(client, &QLocalSocket::readyRead, [&]() {
            if (client->readAll() == "QUIT") app.quit();
        });
    });
    guiIpc.listen(IPC_SERVER_NAME);

    if (lIndex != -1 && lIndex + 1 < args.size()) {
        QString exePath = args[lIndex + 1];
        QProcess* gameProcess = new QProcess(&app);
        if (!args.contains("-k")) {
            QObject::connect(gameProcess, &QProcess::finished, [&]() {
                Log::writeLine("Game exited. Closing WoWpadX.");
                app.quit();
            });
        }
        gameProcess->startDetached(exePath);
    }

    qmlRegisterSingletonInstance<AppSettings>("WoWpadX", 1, 0, "AppSettings", AppSettings::instance());
    qmlRegisterSingletonType<MainPageController>("WoWpadX", 1, 0, "MainPageController", [](QQmlEngine*, QJSEngine*) -> QObject* { return new MainPageController(); });
    qmlRegisterSingletonType<DevicesController>("WoWpadX", 1, 0, "DevicesController", [](QQmlEngine*, QJSEngine*) -> QObject* { return new DevicesController(); });
    qmlRegisterSingletonType<MemoryReadingController>("WoWpadX", 1, 0, "MemoryReadingController", [](QQmlEngine*, QJSEngine*) -> QObject* { return new MemoryReadingController(); });

    QQmlApplicationEngine engine;

    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");

    BindManager::loadBindings();
    ControllerManager::instance()->startWatcher();
    InputMapper::Start();
    ProcessManager::start();

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/wowpadx/QML/main.qml")));

    TrayManager tray;
    engine.rootContext()->setContextProperty("TrayManager", &tray);
    QWindow* mainWindow = qobject_cast<QWindow*>(engine.rootObjects().first());
    tray.setMainWindow(mainWindow);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        AppSettings::instance()->save();
        InputMapper::Stop();
        ControllerManager::instance()->stopWatcher();
        ProcessManager::stop();
    });

    if (args.contains("-m") && mainWindow) mainWindow->hide();
    else if (mainWindow) mainWindow->show();

    return app.exec();
}