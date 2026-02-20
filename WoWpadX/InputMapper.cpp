#include "InputMapper.h"
#include "AppSettings.h"
#include "WoWReader.h"
#include "ProcessManager.h"
#include "BindManager.h"
#include "OverlayClient.h"
#include "ControllerManager.h"

namespace InputMapper
{
    SDL_Gamepad* gamepad = nullptr;
    bool controllerHotplugged = false;
    static int lastHP = 255;


    void InputWatcher()
    {
        while (threadRunning.load())
        {
            SDL_UpdateGamepads();

            if (gamepad == nullptr)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            if (controllerHotplugged)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                controllerHotplugged = false;
                continue;
            }

            UpdateMovement();
            UpdateCursorMovement();
            UpdateTouchpad();
            UpdateGamepadButtonStates();

            if (WoWReader::isSynced() && WoWReader::readGameState())
            {
                if (AppSettings::instance()->memoryVibrationDamage())
                {
                    auto hpPair = WoWReader::readPlayerHealth(); 

                    int currentHP = hpPair.first;
                    if (currentHP < lastHP)
                    {
                        int damageTaken = lastHP - currentHP;
                        uint16_t intensity = static_cast<uint16_t>(std::min(65535, damageTaken * 2000));

                        if (intensity > 5000) {
                            ControllerManager::instance()->Rumble(intensity, intensity / 2, 200);
                        }
                    }

                    lastHP = currentHP;
                }


                if (WoWReader::isFocused())
                    setMouselook = false;

                if (AppSettings::instance()->memoryAutoCancel() && !setMouselook && WoWReader::readMouselook() && !WoWReader::isFocused())
                {
                    SendMouse(false, true, true);
                    SendMouse(false, false, true);

                    setMouselook = true;
                }

                // Crosshair overlay logic
                if (AppSettings::instance()->enableOverlay() && AppSettings::instance()->enableOverlayCrosshair())
                {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - mouselookStarted).count();

                    if (WoWReader::readMouselook() && elapsed >= 200 /* && !Overlay::CrosshairVisible() */ && !crosshairShowing)
                    {
                        OverlayClient::SetCrosshairState(true, cursorX, cursorY);
                        crosshairShowing = true;
                    }
                    else if (!WoWReader::readMouselook() && crosshairShowing)
                    {
                        OverlayClient::SetCrosshairState(false);
                        crosshairShowing = false;
                    }
                }

                if (!WoWReader::readMouselook())
                {
                    POINT pos = GetCursorPosSafe();
                    cursorX = pos.x;
                    cursorY = pos.y;

                    int memauto = AppSettings::instance()->memoryAutoCenter();
                    int memcount = mouselookStarted.time_since_epoch().count();

                    if (AppSettings::instance()->memoryAutoCenter() && WoWReader::isFocused() && mouselookStarted.time_since_epoch().count() > 0)
                    {
                        auto now = std::chrono::steady_clock::now();
                        auto delay = std::chrono::milliseconds(AppSettings::instance()->memoryAutoCenterDelay());
                        if (now - mouselookStarted >= delay)
                        {
                            POINT pt = WoWReader::getScreenCenter();
                            SetCursorPosSafe(pt.x, pt.y);
                        }
                        mouselookStarted = {};
                    }
                }
                else
                {
                    if (mouselookStarted.time_since_epoch().count() == 0)
                        mouselookStarted = std::chrono::steady_clock::now();
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }


    // Add a small cooldown for the walk toggle
    static auto lastWalkToggle = std::chrono::steady_clock::now();

    void UpdateMovement()
    {
        float x = SDL_GetGamepadAxis(gamepad, !AppSettings::instance()->swapSticks() ? SDL_GAMEPAD_AXIS_LEFTX : SDL_GAMEPAD_AXIS_RIGHTX) / 256.0f;
        float y = SDL_GetGamepadAxis(gamepad, !AppSettings::instance()->swapSticks() ? SDL_GAMEPAD_AXIS_LEFTY : SDL_GAMEPAD_AXIS_RIGHTY) / 256.0f;
        float strength = sqrtf(x * x + y * y);

        float threshold = AppSettings::instance()->movementThreshold();
        float walkThreshold = AppSettings::instance()->walkThreshold();

        bool left = -x > threshold;
        bool right = x > threshold;
        bool up = -y > threshold;
        bool down = y > threshold;

        // 16-Way Logic
        float absX = fabsf(x);
        float absY = fabsf(y);

        bool horz = left || right;
        bool vert = up || down;

        bool sendHorz = horz && vert && (absX > absY * 1.5f);
        bool sendVert = horz && vert && (absY > absX * 1.5f);

        // --- Pixel-Bridge Auto-Walk ---
        if (AppSettings::instance()->memoryAutoWalk() && WoWReader::isSynced()) {
            auto now = std::chrono::steady_clock::now();
            int moveState = WoWReader::readMovementState();
            bool isWalkingInGame = (moveState == 1);

            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastWalkToggle).count() > 200) {
                if (strength >= threshold && strength < walkThreshold && !isWalkingInGame) {
                    SendKey(VK_DIVIDE, true); SendKey(VK_DIVIDE, false);
                    lastWalkToggle = now;
                }
                else if (strength >= walkThreshold && isWalkingInGame) {
                    SendKey(VK_DIVIDE, true); SendKey(VK_DIVIDE, false);
                    lastWalkToggle = now;
                }
            }
        }

        auto bind = [&](GamepadBinding binding, bool pressed) {
            if (keyStates[binding] != pressed) {
                SendKey(BindManager::getKey(binding), pressed);
                keyStates[binding] = pressed;
            }
        };

        bind(GamepadBinding::LeftStickLeft, left);
        bind(GamepadBinding::LeftStickRight, right);
        bind(GamepadBinding::LeftStickUp, up);
        bind(GamepadBinding::LeftStickDown, down);

        if (!AppSettings::instance()->simpleRadial())
        {
            bind(GamepadBinding::LeftStickHorz, sendHorz);
            bind(GamepadBinding::LeftStickVert, sendVert);
        }
    }

    void UpdateCursorMovement()
    {
        float x = SDL_GetGamepadAxis(gamepad, !AppSettings::instance()->swapSticks() ? SDL_GAMEPAD_AXIS_RIGHTX : SDL_GAMEPAD_AXIS_LEFTX) / 256.0f;
        float y = SDL_GetGamepadAxis(gamepad, !AppSettings::instance()->swapSticks() ? SDL_GAMEPAD_AXIS_RIGHTY : SDL_GAMEPAD_AXIS_LEFTY) / 256.0f;

        float mag = sqrtf(x * x + y * y);

        float deadzone = AppSettings::instance()->cursorDeadzone();
        if (mag < deadzone)
            return;

        x = x / mag * ((mag - deadzone) / (127.0f - deadzone));
        y = y / mag * ((mag - deadzone) / (127.0f - deadzone));

        float speed = AppSettings::instance()->cursorSpeed();
        float curve = AppSettings::instance()->cursorCurve();

        auto applyCurve = [&](float v) {
            float absVal = fabs(v);
            float scaled = powf(absVal * speed * curve * 0.05f, 2.0f) + absVal * speed * curve * 0.05f;
            return (v < 0 ? -scaled : scaled);
        };

        float dx = applyCurve(x);
        float dy = applyCurve(y);

        // Invert X for mouselook turn if needed
        if (AppSettings::instance()->memoryInvertTurn() && WoWReader::readMouselook())
            dx = -dx;

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = static_cast<LONG>(dx);
        input.mi.dy = static_cast<LONG>(dy);
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &input, sizeof(INPUT));
    }

    void UpdateTouchpad() {
        if (!AppSettings::instance()->enableTouchpad()) return;
        
        if(AppSettings::instance()->memoryTouchpadCursorOnly() && WoWReader::isSynced())
            if(WoWReader::readMouselook())
                return;

        auto cm = ControllerManager::instance();

        float dx = cm->m_touchDeltaX;
        float dy = cm->m_touchDeltaY;

        float deadzone = 1.5f;

        if (std::abs(dx) > deadzone || std::abs(dy) > deadzone) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dx = static_cast<LONG>(dx);
            input.mi.dy = static_cast<LONG>(dy);
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            SendInput(1, &input, sizeof(INPUT));
        }

        cm->m_touchDeltaX = 0;
        cm->m_touchDeltaY = 0;

        static bool wasDown = false;
        bool isDown = cm->m_touchButtonDown;

        if (isDown != wasDown) {
            int mode = AppSettings::instance()->touchpadMode(); // 0 = Mouse, 1 = Binds
            if (mode == 0) {
                SendMouse(cm->m_touchSideLeft, isDown); // Left/Right mouse clicks
            }
            else {
                GamepadBinding bind = cm->m_touchSideLeft ? GamepadBinding::Back : GamepadBinding::Start;
                SendKey(BindManager::getKey(bind), isDown);
            }
            wasDown = isDown;
        }
    }


    void ProcessLoginScreen(GamepadBinding button, bool state)
    {
        switch (button)
        {
        case GamepadBinding::DPadUp:
            if (state && !keyStates[button])
                SendKey(VK_UP, true);
            else if (!state && keyStates[button])
                SendKey(VK_UP, false);
            
            keyStates[button] = state;
            return;
        case GamepadBinding::DPadDown:
            if (state && !keyStates[button])
                SendKey(VK_DOWN, true);
            else if (!state && keyStates[button])
                SendKey(VK_DOWN, false);

            keyStates[button] = state;
            return;
        case GamepadBinding::DPadLeft:
            if (state && !keyStates[button])
                SendKey(VK_LEFT, true);
            else if (!state && keyStates[button])
                SendKey(VK_LEFT, false);

            keyStates[button] = state;
            return;
        case GamepadBinding::DPadRight:
            if (state && !keyStates[button])
                SendKey(VK_RIGHT, true);
            else if (!state && keyStates[button])
                SendKey(VK_RIGHT, false);
            
            keyStates[button] = state;
            return;
        case GamepadBinding::South:
            if (state && !keyStates[button])
                SendKey(VK_RETURN, true);
            else if (!state && keyStates[button])
                SendKey(VK_RETURN, false);
            
            keyStates[button] = state;
            return;
        case GamepadBinding::Guide:
            if (state && !keyStates[button])
                SendKey(VK_ESCAPE, true);
            else if (!state && keyStates[button])
                SendKey(VK_ESCAPE, false);
            
            keyStates[button] = state;
            return;
        case GamepadBinding::West:
            if (state && !keyStates[button])
                SendKey(VK_TAB, true);
            else if (!state && keyStates[button])
                SendKey(VK_TAB, false);
            
            keyStates[button] = state;
            return;
        case GamepadBinding::East:
            if (state && keyStates[button])
                SendKey(VK_RETURN, false);
            
            keyStates[button] = state;
            return;
        case GamepadBinding::North:
            if (state && !keyStates[button])
                SendKey(VK_ESCAPE, true);
            else if (!state && keyStates[button])
                SendKey(VK_ESCAPE, false);

            keyStates[button] = state;
            return;

        case GamepadBinding::RightShoulder:
            if (state && !keyStates[button])
                SendKey(VK_BACK, true);
            else if (!state && keyStates[button])
                SendKey(VK_BACK, false);

            keyStates[button] = state;
            return;
        }

        if (button == GamepadBinding::LeftStick)
        {
            SendMouse(true, state);
            keyStates[button] = state;
        }
        else if (button == GamepadBinding::RightStick)
        {
            SendMouse(false, state);
            keyStates[button] = state;
        }


        // All other buttons go through normal flow
        ProcessInput(button, state);
    }

    void ProcessPlayerAoe(GamepadBinding button, bool state)
    {
        if (button == (GamepadBinding)AppSettings::instance()->memoryAoeConfirm())
        {
            if (state && !keyStates[button]) {
                SendMouse(true, true);
                SendMouse(true, false);
            }
            keyStates[button] = state; // consume it
            return;
        }

        if (button == (GamepadBinding)AppSettings::instance()->memoryAoeCancel())
        {
            if (state && !keyStates[button]) {
                SendMouse(false, true);
                SendMouse(false, false);
            }
            keyStates[button] = state; // consume it
            return;
        }

        // All other buttons go through normal flow
        ProcessInput(button, state);
    }


    void ProcessInput(GamepadBinding button, bool state)
    {
        if (button == GamepadBinding::LeftStick)
            SendMouse(true, state);
        else if (button == GamepadBinding::RightStick)
            SendMouse(false, state);

        if (keyStates[button] != state)
        {
            WORD vk = BindManager::getKey(button);
            SendKey(vk, state);
        }

        keyStates[button] = state;
    }

    void UpdateGamepadButtonStates()
    {
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
            auto sdlBtn = static_cast<SDL_GamepadButton>(b);
            auto binding = static_cast<GamepadBinding>(b); // Enum mapping matches SDL button ordering

            bool pressed = SDL_GetGamepadButton(gamepad, sdlBtn);
            bool previous = keyStates[binding];

            if (pressed != previous)
            {
                HandleButtonStateChange(binding, pressed); 
            }
        }

        int lt = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) * 250 / 32767;
        int rt = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) * 250 / 32767;

        bool leftTriggerPressed = lt > AppSettings::instance()->triggerThresholdLeft();
        bool rightTriggerPressed = rt > AppSettings::instance()->triggerThresholdRight();

        if (leftTriggerPressed != keyStates[GamepadBinding::LeftTrigger])
        {
            HandleButtonStateChange(GamepadBinding::LeftTrigger, leftTriggerPressed);
        }

        if (rightTriggerPressed != keyStates[GamepadBinding::RightTrigger])
        {
            HandleButtonStateChange(GamepadBinding::RightTrigger, rightTriggerPressed);
        }
    }


    void HandleButtonStateChange(GamepadBinding button, bool state)
    {
        if (AppSettings::instance()->enableMemoryReading())
        {
            /* Disabled for now, because this is not very reliable with the current state of pixel bridge
            * 
            if (AppSettings::instance()->memoryOverrideLogin() &&
                WoWReader::isAttached() &&
                !WoWReader::readGameState())
            {
                ProcessLoginScreen(button, state);
                return;
            }
            */

            if (WoWReader::readGameState() &&
                AppSettings::instance()->memoryOverrideAoeCast() &&
                WoWReader::readAoeState())
            {
                ProcessPlayerAoe(button, state);
                return;
            }
        }

        ProcessInput(button, state);
    }

    void Start()
    {
        if (threadRunning.load()) return;

        gamepad = ControllerManager::instance()->getActiveController();

        QObject::connect(ControllerManager::instance(), &ControllerManager::activeControllerChanged, []() {
            InputMapper::gamepad = ControllerManager::instance()->getActiveController();
            InputMapper::controllerHotplugged = true;
        });

        threadRunning = true;
        inputThread = std::thread(InputWatcher);
    }

    void Stop()
    {
        threadRunning = false;
        if (inputThread.joinable())
            inputThread.join();
    }

    POINT GetCursorPosSafe()
    {
        POINT p = {};
        GetCursorPos(&p);
        return p;
    }

    void SetCursorPosSafe(int x, int y)
    {
        SetCursorPos(x, y);
    }

    void SendKey(WORD vk, bool down)
    {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    void SendMouse(bool left, bool down, bool forceDirect)
    {
        if (forceDirect)
        {
            auto wnd = ProcessManager::getGameProcessWindowHandle();

            if (IsWindow(wnd))
            {
                RECT rect;
                LPARAM lParam = 0;

                if (GetClientRect(wnd, &rect)) {
                    int centerX = rect.right / 2;
                    int centerY = rect.bottom / 2;

                    lParam = MAKELPARAM(centerX, centerY);
                }
                 
                PostMessageW(wnd, left ? (down ? WM_LBUTTONDOWN : WM_LBUTTONUP) : (down ? WM_RBUTTONDOWN : WM_RBUTTONUP), 0, lParam);
            }

            return;
        }

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = (left ? (down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP)
            : (down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP));
        SendInput(1, &input, sizeof(INPUT));
    }
}
