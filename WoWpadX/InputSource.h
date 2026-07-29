#pragma once

#include "Log.h"
#include "RawInputGamepad.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>

#include <GameInput.h>
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>

namespace InputSource
{
    using GameInputCreateFunction = HRESULT(WINAPI*)(IGameInput**);

    // GameInputEnableBackgroundInput is ABI value 0x40. Some Windows SDK
    // GameInput.h versions expose SetFocusPolicy but do not yet name this
    // newer enum member, so keep the stable wire value local.
    constexpr GameInputFocusPolicy BackgroundInputFocusPolicy =
        static_cast<GameInputFocusPolicy>(0x00000040);

    inline std::mutex stateMutex;
    inline HMODULE gameInputModule = nullptr;
    inline IGameInput* gameInput = nullptr;
    inline bool initializationAttempted = false;
    inline bool nativeReadingAvailable = false;
    inline GameInputGamepadState nativeState{};

    inline bool InitializeNativeGameInput()
    {
        std::lock_guard<std::mutex> lock(stateMutex);

        if (gameInput)
            return true;
        if (initializationAttempted)
            return false;

        initializationAttempted = true;
        gameInputModule = LoadLibraryW(L"gameinput.dll");
        if (!gameInputModule) {
            Log::writeLine(
                "[NativeGameInput] gameinput.dll was not available; using SDL input only. Error=" +
                QString::number(GetLastError()));
            return false;
        }

        const auto createGameInput = reinterpret_cast<GameInputCreateFunction>(
            GetProcAddress(gameInputModule, "GameInputCreate"));
        if (!createGameInput) {
            Log::writeLine(
                "[NativeGameInput] GameInputCreate export was not available; using SDL input only.");
            FreeLibrary(gameInputModule);
            gameInputModule = nullptr;
            return false;
        }

        const HRESULT result = createGameInput(&gameInput);
        if (FAILED(result) || !gameInput) {
            Log::writeLine(
                "[NativeGameInput] GameInputCreate failed with HRESULT 0x" +
                QString::number(static_cast<quint32>(result), 16));
            FreeLibrary(gameInputModule);
            gameInputModule = nullptr;
            gameInput = nullptr;
            return false;
        }

        gameInput->SetFocusPolicy(BackgroundInputFocusPolicy);
        Log::writeLine(
            "[NativeGameInput] Initialized native GameInput and enabled background input focus policy.");
        return true;
    }

    inline void PollNativeGameInput()
    {
        if (!gameInput && !InitializeNativeGameInput())
            return;

        GameInputGamepadState state{};
        bool readingAvailable = false;

        {
            std::lock_guard<std::mutex> lock(stateMutex);

            IGameInputReading* reading = nullptr;
            const HRESULT result = gameInput->GetCurrentReading(
                GameInputKindGamepad,
                nullptr,
                &reading);

            if (SUCCEEDED(result) && reading) {
                readingAvailable = reading->GetGamepadState(&state);
                reading->Release();
            }

            nativeReadingAvailable = readingAvailable;
            if (readingAvailable)
                nativeState = state;
        }
    }

    inline void UpdateGamepads()
    {
        RawInputGamepad::EnsureStarted();
        ::SDL_UpdateGamepads();
        PollNativeGameInput();
    }

    inline bool TryGetNativeButton(
        SDL_GamepadButton button,
        bool& pressed)
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        if (!nativeReadingAvailable)
            return false;

        GameInputGamepadButtons mappedButton = GameInputGamepadNone;
        switch (button) {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            mappedButton = GameInputGamepadA;
            break;
        case SDL_GAMEPAD_BUTTON_EAST:
            mappedButton = GameInputGamepadB;
            break;
        case SDL_GAMEPAD_BUTTON_WEST:
            mappedButton = GameInputGamepadX;
            break;
        case SDL_GAMEPAD_BUTTON_NORTH:
            mappedButton = GameInputGamepadY;
            break;
        case SDL_GAMEPAD_BUTTON_BACK:
            mappedButton = GameInputGamepadView;
            break;
        case SDL_GAMEPAD_BUTTON_START:
            mappedButton = GameInputGamepadMenu;
            break;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            mappedButton = GameInputGamepadLeftThumbstick;
            break;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            mappedButton = GameInputGamepadRightThumbstick;
            break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            mappedButton = GameInputGamepadLeftShoulder;
            break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            mappedButton = GameInputGamepadRightShoulder;
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            mappedButton = GameInputGamepadDPadUp;
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            mappedButton = GameInputGamepadDPadDown;
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            mappedButton = GameInputGamepadDPadLeft;
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            mappedButton = GameInputGamepadDPadRight;
            break;
        default:
            return false;
        }

        pressed = (nativeState.buttons & mappedButton) != 0;
        return true;
    }

    inline bool GetGamepadButton(
        SDL_Gamepad* gamepadHandle,
        SDL_GamepadButton button)
    {
        bool pressed = false;
        if (RawInputGamepad::TryGetButton(button, pressed))
            return pressed;

        if (TryGetNativeButton(button, pressed))
            return pressed;

        return gamepadHandle && ::SDL_GetGamepadButton(gamepadHandle, button);
    }

    inline Sint16 GetGamepadAxis(
        SDL_Gamepad* gamepadHandle,
        SDL_GamepadAxis axis)
    {
        Sint16 rawValue = 0;
        if (RawInputGamepad::TryGetAxis(axis, rawValue))
            return rawValue;

        {
            std::lock_guard<std::mutex> lock(stateMutex);
            if (nativeReadingAvailable) {
                float value = 0.0f;
                bool trigger = false;

                switch (axis) {
                case SDL_GAMEPAD_AXIS_LEFTX:
                    value = nativeState.leftThumbstickX;
                    break;
                case SDL_GAMEPAD_AXIS_LEFTY:
                    // GameInput uses positive-up while SDL uses positive-down.
                    value = -nativeState.leftThumbstickY;
                    break;
                case SDL_GAMEPAD_AXIS_RIGHTX:
                    value = nativeState.rightThumbstickX;
                    break;
                case SDL_GAMEPAD_AXIS_RIGHTY:
                    value = -nativeState.rightThumbstickY;
                    break;
                case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                    value = nativeState.leftTrigger;
                    trigger = true;
                    break;
                case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                    value = nativeState.rightTrigger;
                    trigger = true;
                    break;
                default:
                    break;
                }

                if (trigger) {
                    value = std::clamp(value, 0.0f, 1.0f);
                    return static_cast<Sint16>(std::lround(value * 32767.0f));
                }

                value = std::clamp(value, -1.0f, 1.0f);
                return static_cast<Sint16>(std::lround(value * 32767.0f));
            }
        }

        return gamepadHandle ? ::SDL_GetGamepadAxis(gamepadHandle, axis) : 0;
    }
}
