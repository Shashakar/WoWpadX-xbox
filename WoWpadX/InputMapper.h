#pragma once

#include <windows.h>
#include "Gamepad/gamepad.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include <cmath>


namespace InputMapper
{
    static inline std::thread inputThread;
    static inline std::atomic<bool> threadRunning{ false };

    static std::unordered_map<GamepadBinding, bool> keyStates;
    static inline bool crosshairShowing = false;
    static inline bool stopWalk = false;
    static inline bool setMouselook = false;
    static inline int cursorX = 0, cursorY = 0;
    static inline std::chrono::steady_clock::time_point mouselookStarted;

    extern SDL_Gamepad* gamepad;

    void Start();
    void Stop();

    void UpdateGamepadButtonStates();      // Called in InputWatcher loop
    void HandleButtonStateChange(GamepadBinding button, bool state);

    void ProcessInput(GamepadBinding button, bool state);
    void ProcessLoginScreen(GamepadBinding button, bool state);
    void ProcessPlayerAoe(GamepadBinding button, bool state);

    void UpdateMovement();
    void UpdateCursorMovement();
    void UpdateTouchpad();

    POINT GetCursorPosSafe();
    void SetCursorPosSafe(int x, int y);

    void SendKey(WORD vk, bool down);
    void SendMouse(bool left, bool down, bool forceDirect = false);
}
