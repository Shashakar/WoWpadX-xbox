// ControllerManager.cpp

#include "ControllerManager.h"

#include "AppSettings.h"
#include "Log.h"
#include "OverlayClient.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <unordered_set>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QMap>
#include <QScreen>
#include <QThread>
#include <QTimer>
#include <QWindow>

ControllerManager* ControllerManager::m_instance = nullptr;

ControllerManager* ControllerManager::instance() {
    if (!m_instance)
        m_instance = new ControllerManager();
    return m_instance;
}

void ControllerManager::PumpEvents() {
    // SDL_PollEvent may pump platform events and must run on the main thread.
    if (!SDL_IsMainThread()) {
        Log::writeLine("Skipping SDL event pump outside the main thread.");
        return;
    }

    bool controllerTopologyChanged = false;
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_GAMEPAD_ADDED || e.type == SDL_EVENT_GAMEPAD_REMOVED) {
            controllerTopologyChanged = true;
        }

        if (!AppSettings::instance()->enableTouchpad())
            continue;

        if (e.type == SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION && e.gtouchpad.touchpad == 0) {
            static float lastX = 0.0f;
            static float lastY = 0.0f;

            if (e.gtouchpad.pressure > 0) {
                m_touchDeltaX = (e.gtouchpad.x - lastX) * 1920.0f;
                m_touchDeltaY = (e.gtouchpad.y - lastY) * 1080.0f;
            }

            lastX = e.gtouchpad.x;
            lastY = e.gtouchpad.y;
        }
        else if (e.type == SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN) {
            m_touchButtonDown = true;
            m_touchSideLeft = e.gtouchpad.x < 0.5f;
        }
        else if (e.type == SDL_EVENT_GAMEPAD_TOUCHPAD_UP) {
            m_touchButtonDown = false;
        }
    }

    if (controllerTopologyChanged)
        ReconcileControllers();
}

void ControllerManager::ReconcileControllers() {
    int detectedCount = 0;
    SDL_JoystickID* detectedIds = SDL_GetGamepads(&detectedCount);

    if (!detectedIds) {
        const char* error = SDL_GetError();
        Log::writeLine("SDL_GetGamepads failed: " + QString(error ? error : "Unknown SDL error"));
        return;
    }

    std::unordered_set<SDL_JoystickID> detected;
    detected.reserve(static_cast<size_t>(detectedCount));
    for (int i = 0; i < detectedCount; ++i)
        detected.insert(detectedIds[i]);

    bool listChanged = false;
    bool activeChanged = false;
    std::vector<std::string> disconnectedNames;
    std::string selectedName;

    {
        std::lock_guard<std::mutex> lock(controllerMutex);

        for (auto it = controllers.begin(); it != controllers.end();) {
            SDL_Gamepad* pad = *it;
            const SDL_JoystickID id = pad ? SDL_GetGamepadID(pad) : 0;
            const bool connected = pad && SDL_GamepadConnected(pad) && detected.find(id) != detected.end();

            if (connected) {
                ++it;
                continue;
            }

            const char* rawName = pad ? SDL_GetGamepadName(pad) : nullptr;
            disconnectedNames.emplace_back(rawName ? rawName : "Unknown Controller");

            if (pad == activeController) {
                activeController = nullptr;
                activeInstanceId = -1;
                activeChanged = true;
            }

            if (pad)
                retiredControllers.push_back(pad);

            it = controllers.erase(it);
            listChanged = true;
        }

        for (int i = 0; i < detectedCount; ++i) {
            const SDL_JoystickID id = detectedIds[i];
            const bool alreadyOpen = std::any_of(
                controllers.begin(),
                controllers.end(),
                [id](SDL_Gamepad* pad) {
                    return pad && SDL_GetGamepadID(pad) == id;
                });

            if (alreadyOpen)
                continue;

            SDL_Gamepad* pad = SDL_OpenGamepad(id);
            if (!pad) {
                const char* error = SDL_GetError();
                Log::writeLine(
                    "SDL_OpenGamepad failed for ID " + QString::number(id) + ": " +
                    QString(error ? error : "Unknown SDL error"));
                continue;
            }

            controllers.push_back(pad);
            listChanged = true;

            const char* rawName = SDL_GetGamepadName(pad);
            Log::writeLine(
                "Opened controller ID " + QString::number(id) + ": " +
                QString(rawName ? rawName : "Unknown Controller"));
        }

        if (!activeController && !controllers.empty()) {
            activeController = controllers.front();
            activeInstanceId = SDL_GetGamepadID(activeController);
            activeChanged = true;

            const char* rawName = SDL_GetGamepadName(activeController);
            selectedName = rawName ? rawName : "Unknown Controller";
        }
    }

    SDL_free(detectedIds);

    for (const std::string& name : disconnectedNames) {
        OverlayClient::SendNotification(
            "Controller disconnected",
            name + " was disconnected.");
    }

    if (!selectedName.empty()) {
        Log::writeLine("Selected controller: " + QString::fromStdString(selectedName));
        OverlayClient::SendNotification(
            "Controller selected",
            "Controller " + selectedName + " is now the active controller.");
    }

    if (listChanged)
        emit controllersChanged();
    if (activeChanged)
        emit activeControllerChanged();
}

QString ControllerManager::GetButtonIcon(GamepadBinding button)
{
    QString type;
    QString name;

    {
        std::lock_guard<std::mutex> lock(controllerMutex);
        if (activeController != nullptr) {
            const char* rawName = SDL_GetGamepadName(activeController);
            name = rawName ? QString::fromUtf8(rawName) : QString();
        }
    }

    if (name.contains("DualShock", Qt::CaseInsensitive) || name.contains("DualSense") || name.contains("DS4") || name.contains("DS3") || name.contains("PS4") || name.contains("Playstation") || name.contains("PS3"))
        type = "DS";
    else if (name.contains("Xbox", Qt::CaseInsensitive))
        type = "Xbox";
    else if (name.contains("Switch", Qt::CaseInsensitive) || name.contains("Joy-Con", Qt::CaseInsensitive))
        type = "NS";
    else if (name.contains("Steam", Qt::CaseInsensitive) || name.contains("Deck", Qt::CaseInsensitive))
        type = "Deck";
    else
        type = "Xbox";

    switch (button)
    {
    case GamepadBinding::DPadUp:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_UP.png").arg(type);
    case GamepadBinding::DPadDown:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_DOWN.png").arg(type);
    case GamepadBinding::DPadLeft:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_LEFT.png").arg(type);
    case GamepadBinding::DPadRight:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_RIGHT.png").arg(type);
    case GamepadBinding::North:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_UP.png").arg(type);
    case GamepadBinding::South:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_DOWN.png").arg(type);
    case GamepadBinding::West:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_LEFT.png").arg(type);
    case GamepadBinding::East:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_RIGHT.png").arg(type);
    case GamepadBinding::Back:
        return QString("qrc:/Controllers/%1/Buttons/CP_X_LEFT.png").arg(type);
    case GamepadBinding::Start:
        return QString("qrc:/Controllers/%1/Buttons/CP_X_RIGHT.png").arg(type);
    case GamepadBinding::Guide:
        return QString("qrc:/Controllers/%1/Buttons/CP_X_CENTER.png").arg(type);
    case GamepadBinding::Misc1:
        return QString("qrc:/Controllers/%1/Buttons/CP_X_MISC1.png").arg(type);
    case GamepadBinding::LeftStick:
        return QString("qrc:/Controllers/%1/Buttons/CP_T_L3.png").arg(type);
    case GamepadBinding::RightStick:
        return QString("qrc:/Controllers/%1/Buttons/CP_T_R3.png").arg(type);
    case GamepadBinding::LeftShoulder:
        return QString("qrc:/Controllers/%1/Buttons/CP_TL1.png").arg(type);
    case GamepadBinding::LeftTrigger:
        return QString("qrc:/Controllers/%1/Buttons/CP_TL2.png").arg(type);
    case GamepadBinding::RightShoulder:
        return QString("qrc:/Controllers/%1/Buttons/CP_TR1.png").arg(type);
    case GamepadBinding::RightTrigger:
        return QString("qrc:/Controllers/%1/Buttons/CP_TR2.png").arg(type);
    case GamepadBinding::LeftPaddle1:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_GRIP1.png").arg(type);
    case GamepadBinding::LeftPaddle2:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_GRIP2.png").arg(type);
    case GamepadBinding::RightPaddle1:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_GRIP1.png").arg(type);
    case GamepadBinding::RightPaddle2:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_GRIP2.png").arg(type);
    default:
        return QString();
    }
}

QString ControllerManager::GetButtonName(GamepadBinding button)
{
    QString type;
    QString name;

    {
        std::lock_guard<std::mutex> lock(controllerMutex);
        if (activeController != nullptr) {
            const char* rawName = SDL_GetGamepadName(activeController);
            name = rawName ? QString::fromUtf8(rawName) : QString();
        }
    }

    if (name.contains("DualShock", Qt::CaseInsensitive) || name.contains("DualSense") || name.contains("DS4") || name.contains("DS3") || name.contains("PS4") || name.contains("Playstation") || name.contains("PS3"))
        type = "DS";
    else if (name.contains("Xbox", Qt::CaseInsensitive))
        type = "Xbox";
    else if (name.contains("Switch", Qt::CaseInsensitive) || name.contains("Joy-Con", Qt::CaseInsensitive))
        type = "NS";
    else if (name.contains("Steam", Qt::CaseInsensitive) || name.contains("Deck", Qt::CaseInsensitive))
        type = "Deck";
    else
        type = "Xbox";

    switch (button)
    {
    case GamepadBinding::DPadUp:
        return "D-pad Up";
    case GamepadBinding::DPadDown:
        return "D-pad Down";
    case GamepadBinding::DPadLeft:
        return "D-pad Left";
    case GamepadBinding::DPadRight:
        return "D-pad Right";
    case GamepadBinding::North:
        return QMap<QString, QString>{{"Xbox", "Y"}, { "Deck", "Y" }, { "NS", "X" }}.value(type, "Triangle");
    case GamepadBinding::South:
        return QMap<QString, QString>{{"Xbox", "A"}, { "Deck", "A" }, { "NS", "B" }}.value(type, "Cross");
    case GamepadBinding::West:
        return QMap<QString, QString>{{"Xbox", "X"}, { "Deck", "X" }, { "NS", "Y" }}.value(type, "Square");
    case GamepadBinding::East:
        return QMap<QString, QString>{{"Xbox", "B"}, { "Deck", "B" }, { "NS", "A" }}.value(type, "Circle");
    case GamepadBinding::Back:
        return QMap<QString, QString>{{"Xbox", "Back"}, { "Deck", "Back" }, { "NS", "Minus" }}.value(type, "Share");
    case GamepadBinding::Start:
        return QMap<QString, QString>{{"Xbox", "Start"}, { "Deck", "Start" }, { "NS", "Plus" }}.value(type, "Options");
    case GamepadBinding::Guide:
        return QMap<QString, QString>{{"Xbox", "Guide"}, { "Deck", "Guide" }, { "NS", "Home" }}.value(type, "PS");
    case GamepadBinding::Misc1:
        return QMap<QString, QString>{{"Xbox", "Share"}, { "Deck", "Capture" }, { "NS", "Capture" }}.value(type, "Microphone");
    case GamepadBinding::LeftStick:
        return "Left Stick";
    case GamepadBinding::RightStick:
        return "Right Stick";
    case GamepadBinding::LeftShoulder:
        return QMap<QString, QString>{{"Xbox", "Left Bumper"}, { "Deck", "L1" }, { "NS", "L" }}.value(type, "L1");
    case GamepadBinding::LeftTrigger:
        return QMap<QString, QString>{{"Xbox", "Left Trigger"}, { "Deck", "L2" }, { "NS", "ZL" }}.value(type, "L2");
    case GamepadBinding::RightShoulder:
        return QMap<QString, QString>{{"Xbox", "Right Bumper"}, { "Deck", "R1" }, { "NS", "R" }}.value(type, "R1");
    case GamepadBinding::RightTrigger:
        return QMap<QString, QString>{{"Xbox", "Right Trigger"}, { "Deck", "R2" }, { "NS", "ZR" }}.value(type, "R2");
    case GamepadBinding::LeftPaddle1:
        return QMap<QString, QString>{{ "Deck", "L4" }}.value(type, "Left Grip 1");
    case GamepadBinding::LeftPaddle2:
        return QMap<QString, QString>{{ "Deck", "L5" }}.value(type, "Left Grip 2");
    case GamepadBinding::RightPaddle1:
        return QMap<QString, QString>{{ "Deck", "R4" }}.value(type, "Right Grip 1");
    case GamepadBinding::RightPaddle2:
        return QMap<QString, QString>{{ "Deck", "R5" }}.value(type, "Right Grip 2");
    default:
        return QString();
    }
}

int ControllerManager::GetBatteryLevel(SDL_Gamepad* pad) {
    if (!pad) return 100;

    int percentage = 0;
    SDL_PowerState state = SDL_GetGamepadPowerInfo(pad, &percentage);

    if (state == SDL_POWERSTATE_UNKNOWN || percentage == -1)
        return 100;

    return percentage;
}

void ControllerManager::SetController(SDL_Gamepad* controller, SDL_JoystickID instanceId) {
    std::string controllerName;
    bool changed = false;

    {
        std::lock_guard<std::mutex> lock(controllerMutex);

        if (activeController == controller && activeInstanceId == instanceId)
            return;

        activeController = controller;
        activeInstanceId = instanceId;
        changed = true;

        if (activeController != nullptr) {
            const char* rawName = SDL_GetGamepadName(activeController);
            controllerName = rawName ? rawName : "Unknown Controller";
        }
    }

    if (!changed)
        return;

    emit activeControllerChanged();

    if (!controllerName.empty()) {
        Log::writeLine("Selected controller: " + QString::fromStdString(controllerName));
        OverlayClient::SendNotification(
            "Controller selected",
            "Controller " + controllerName + " is now the active controller.");
    }
}

void ControllerManager::startWatcher() {
    if (threadRunning.exchange(true))
        return;

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
#ifdef _WIN32
    SDL_SetHint(SDL_HINT_JOYSTICK_GAMEINPUT, "1");
#endif

    if (!SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC)) {
        const char* error = SDL_GetError();
        Log::writeLine("SDL initialization failed: " + QString(error ? error : "Unknown SDL error"));
        threadRunning = false;
        return;
    }

    sdlInitialized = true;
    Log::writeLine("SDL controller subsystem initialized with background input enabled.");
#ifdef _WIN32
    Log::writeLine("SDL GameInput backend requested for Windows.");
#endif

    ReconcileControllers();

    QCoreApplication* app = QCoreApplication::instance();
    if (app && QThread::currentThread() == app->thread()) {
        eventTimer = new QTimer(this);
        eventTimer->setTimerType(Qt::PreciseTimer);
        eventTimer->setInterval(8);
        connect(eventTimer, &QTimer::timeout, this, &ControllerManager::PumpEvents);
        eventTimer->start();
    }
    else {
        Log::writeLine("No Qt main event loop is available; SDL touchpad events will not be pumped in headless mode.");
    }

    watcherThread = std::thread([this]() { ControllerWatcher(); });
}

void ControllerManager::stopWatcher() {
    if (!threadRunning.exchange(false) && !sdlInitialized)
        return;

    if (eventTimer) {
        eventTimer->stop();
        delete eventTimer;
        eventTimer = nullptr;
    }

    if (watcherThread.joinable())
        watcherThread.join();

    bool hadControllers = false;
    bool hadActiveController = false;

    {
        std::lock_guard<std::mutex> lock(controllerMutex);
        hadControllers = !controllers.empty();
        hadActiveController = activeController != nullptr;

        activeController = nullptr;
        activeInstanceId = -1;

        for (SDL_Gamepad* pad : controllers) {
            if (pad)
                SDL_CloseGamepad(pad);
        }
        for (SDL_Gamepad* pad : retiredControllers) {
            if (pad)
                SDL_CloseGamepad(pad);
        }
        controllers.clear();
        retiredControllers.clear();
    }

    if (sdlInitialized) {
        SDL_Quit();
        sdlInitialized = false;
    }

    if (hadActiveController)
        emit activeControllerChanged();
    if (hadControllers)
        emit controllersChanged();
}

void ControllerManager::CheckBatteryWarnings() {
    int battery = 100;

    {
        std::lock_guard<std::mutex> lock(controllerMutex);
        if (!activeController || !SDL_GamepadConnected(activeController))
            return;
        battery = GetBatteryLevel(activeController);
    }

    if (!AppSettings::instance()->enableOverlay() || !AppSettings::instance()->enableOverlayBattery())
        return;

    if (battery > lastBatteryWarn) {
        lastBatteryWarn = 100;
        if (battery > 35) showedBatteryCritical = false;
        if (battery > 45) showedBatteryLow = false;
    }
    else if (battery <= lastBatteryWarn - 10) {
        if (battery <= 30 && !showedBatteryCritical) {
            OverlayClient::SendNotification(
                "Battery critically low",
                "Your controller battery has reached " + std::to_string(battery) + "%. Plug in now to avoid interruption.",
                0);
            showedBatteryCritical = true;
        }
        else if (battery <= 40 && !showedBatteryLow) {
            OverlayClient::SendNotification(
                "Battery low",
                "Your controller battery has reached " + std::to_string(battery) + "%.",
                0);
            showedBatteryLow = true;
        }
        lastBatteryWarn -= 10;
    }
}

void ControllerManager::ControllerWatcher() {
    while (threadRunning.load()) {
        ReconcileControllers();
        CheckBatteryWarnings();

        for (int i = 0; i < 10 && threadRunning.load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

SDL_Gamepad* ControllerManager::getActiveController() {
    std::lock_guard<std::mutex> lock(controllerMutex);
    return activeController;
}

bool ControllerManager::Rumble(uint16_t low_freq, uint16_t high_freq, uint32_t duration_ms) {
    std::lock_guard<std::mutex> lock(controllerMutex);
    if (!activeController || !SDL_GamepadConnected(activeController))
        return false;
    return SDL_RumbleGamepad(activeController, low_freq, high_freq, duration_ms);
}

std::vector<SDL_Gamepad*> ControllerManager::getControllers() {
    std::lock_guard<std::mutex> lock(controllerMutex);
    return controllers;
}
