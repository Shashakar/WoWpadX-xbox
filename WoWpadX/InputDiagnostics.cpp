#include "InputDiagnostics.h"

#include "ControllerManager.h"
#include "Log.h"
#include "ProcessManager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QString>
#include <QTimer>

#include <Windows.h>
#include <Xinput.h>

#include <array>
#include <cstdint>

namespace
{
    using XInputGetStateFunction = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);

    QTimer* diagnosticsTimer = nullptr;
    HMODULE xinputModule = nullptr;
    XInputGetStateFunction xinputGetState = nullptr;
    QString lastState;
    int heartbeatTicks = 0;

    QString ApplicationStateName()
    {
        switch (QGuiApplication::applicationState()) {
        case Qt::ApplicationActive:
            return "Active";
        case Qt::ApplicationInactive:
            return "Inactive";
        case Qt::ApplicationHidden:
            return "Hidden";
        case Qt::ApplicationSuspended:
            return "Suspended";
        default:
            return "Unknown";
        }
    }

    QString WindowDescription(HWND window)
    {
        if (!window || !IsWindow(window))
            return "none";

        std::array<wchar_t, 512> title{};
        std::array<wchar_t, 256> className{};
        DWORD processId = 0;

        GetWindowTextW(window, title.data(), static_cast<int>(title.size()));
        GetClassNameW(window, className.data(), static_cast<int>(className.size()));
        GetWindowThreadProcessId(window, &processId);

        return QString("0x%1 pid=%2 class=\"%3\" title=\"%4\"")
            .arg(reinterpret_cast<quintptr>(window), 0, 16)
            .arg(processId)
            .arg(QString::fromWCharArray(className.data()))
            .arg(QString::fromWCharArray(title.data()));
    }

    void LoadXInput()
    {
        if (xinputGetState)
            return;

        constexpr std::array<const wchar_t*, 3> libraries = {
            L"xinput1_4.dll",
            L"xinput9_1_0.dll",
            L"xinput1_3.dll"
        };

        for (const wchar_t* library : libraries) {
            HMODULE module = LoadLibraryW(library);
            if (!module)
                continue;

            auto function = reinterpret_cast<XInputGetStateFunction>(
                GetProcAddress(module, "XInputGetState"));

            if (!function) {
                FreeLibrary(module);
                continue;
            }

            xinputModule = module;
            xinputGetState = function;
            Log::writeLine(
                "[InputDiag] Loaded native XInput probe from " +
                QString::fromWCharArray(library));
            return;
        }

        Log::writeLine("[InputDiag] Native XInput probe unavailable.");
    }

    QString ReadSdlState()
    {
        SDL_Gamepad* gamepad = ControllerManager::instance()->getActiveController();
        if (!gamepad)
            return "SDL disconnected";

        const char* rawName = SDL_GetGamepadName(gamepad);
        const QString name = rawName ? QString::fromUtf8(rawName) : "Unknown";
        const bool connected = SDL_GamepadConnected(gamepad);
        const int playerIndex = SDL_GetGamepadPlayerIndex(gamepad);

        quint64 buttons = 0;
        for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT && button < 64; ++button) {
            if (SDL_GetGamepadButton(gamepad, static_cast<SDL_GamepadButton>(button)))
                buttons |= (quint64{1} << button);
        }

        auto quantizedAxis = [gamepad](SDL_GamepadAxis axis) {
            return static_cast<int>(SDL_GetGamepadAxis(gamepad, axis)) / 2048;
        };

        return QString(
            "SDL connected=%1 name=\"%2\" player=%3 buttons=0x%4 "
            "LX=%5 LY=%6 RX=%7 RY=%8 LT=%9 RT=%10")
            .arg(connected ? "yes" : "no")
            .arg(name)
            .arg(playerIndex)
            .arg(buttons, 0, 16)
            .arg(quantizedAxis(SDL_GAMEPAD_AXIS_LEFTX))
            .arg(quantizedAxis(SDL_GAMEPAD_AXIS_LEFTY))
            .arg(quantizedAxis(SDL_GAMEPAD_AXIS_RIGHTX))
            .arg(quantizedAxis(SDL_GAMEPAD_AXIS_RIGHTY))
            .arg(quantizedAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER))
            .arg(quantizedAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
    }

    QString ReadXInputState()
    {
        if (!xinputGetState)
            return "XInput unavailable";

        for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
            XINPUT_STATE state{};
            if (xinputGetState(userIndex, &state) != ERROR_SUCCESS)
                continue;

            const XINPUT_GAMEPAD& pad = state.Gamepad;
            return QString(
                "XInput[%1] connected=yes packet=%2 buttons=0x%3 "
                "LX=%4 LY=%5 RX=%6 RY=%7 LT=%8 RT=%9")
                .arg(userIndex)
                .arg(state.dwPacketNumber)
                .arg(pad.wButtons, 0, 16)
                .arg(static_cast<int>(pad.sThumbLX) / 2048)
                .arg(static_cast<int>(pad.sThumbLY) / 2048)
                .arg(static_cast<int>(pad.sThumbRX) / 2048)
                .arg(static_cast<int>(pad.sThumbRY) / 2048)
                .arg(static_cast<int>(pad.bLeftTrigger) / 16)
                .arg(static_cast<int>(pad.bRightTrigger) / 16);
        }

        return "XInput disconnected";
    }

    void Probe()
    {
        const QString inputState = ReadSdlState() + " | " + ReadXInputState();
        const bool changed = inputState != lastState;
        const bool heartbeat = (++heartbeatTicks >= 100);

        if (!changed && !heartbeat)
            return;

        heartbeatTicks = 0;
        lastState = inputState;

        const HWND foreground = GetForegroundWindow();
        const HWND wowWindow = ProcessManager::getGameProcessWindowHandle();

        const char* backgroundHint = SDL_GetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS);
        const char* gameInputHint = SDL_GetHint(SDL_HINT_JOYSTICK_GAMEINPUT);

        Log::writeLine(
            "[InputDiag] " + inputState +
            " | appState=" + ApplicationStateName() +
            " | allowBackground=" + QString(backgroundHint ? backgroundHint : "unset") +
            " | requestGameInput=" + QString(gameInputHint ? gameInputHint : "unset") +
            " | foreground={" + WindowDescription(foreground) + "}" +
            " | wow={" + WindowDescription(wowWindow) + "}" +
            " | wowIsForeground=" + QString(foreground && foreground == wowWindow ? "yes" : "no"));
    }

    void InstallInputDiagnostics()
    {
        QTimer::singleShot(0, []() {
            InputDiagnostics::Start();
        });
    }
}

namespace InputDiagnostics
{
    void Start()
    {
        if (diagnosticsTimer)
            return;

        LoadXInput();

        diagnosticsTimer = new QTimer(QCoreApplication::instance());
        diagnosticsTimer->setInterval(50);
        QObject::connect(diagnosticsTimer, &QTimer::timeout, &Probe);
        diagnosticsTimer->start();

        Log::writeLine(
            "[InputDiag] Diagnostic probe started. State changes and a five-second heartbeat will be logged.");
    }

    void Stop()
    {
        if (diagnosticsTimer) {
            diagnosticsTimer->stop();
            diagnosticsTimer->deleteLater();
            diagnosticsTimer = nullptr;
        }

        if (xinputModule) {
            FreeLibrary(xinputModule);
            xinputModule = nullptr;
            xinputGetState = nullptr;
        }
    }
}

Q_COREAPP_STARTUP_FUNCTION(InstallInputDiagnostics)
