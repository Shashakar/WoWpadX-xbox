#include "AppSettings.h"
#include "Log.h"

#include <QCoreApplication>

#include <Windows.h>

#include <array>
#include <atomic>
#include <chrono>
#include <thread>

namespace
{
    constexpr int kRearPaddleModifierStyle = 4;
    constexpr int kPaddle1VirtualKey = VK_F13;
    constexpr int kPaddle2VirtualKey = VK_F14;

    std::atomic<bool> started = false;

    bool IsKeyDown(int virtualKey)
    {
        return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    }

    void SendModifier(WORD virtualKey, bool down)
    {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = virtualKey;
        input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    void ReleaseModifiers(bool& leftShiftDown, bool& leftControlDown)
    {
        if (leftShiftDown) {
            SendModifier(VK_LSHIFT, false);
            leftShiftDown = false;
        }
        if (leftControlDown) {
            SendModifier(VK_LCONTROL, false);
            leftControlDown = false;
        }
    }

    void PaddleThread()
    {
        bool leftShiftDown = false;
        bool leftControlDown = false;
        bool profileWasActive = false;

        Log::writeLine(
            "[PaddleModifiers] Ready. Map M1 to F13 and M2 to F14 in Armoury Crate.");

        while (!QCoreApplication::closingDown()) {
            const bool profileActive =
                AppSettings::instance()->modifierStyle() ==
                kRearPaddleModifierStyle;

            if (!profileActive) {
                if (profileWasActive)
                    ReleaseModifiers(leftShiftDown, leftControlDown);
                profileWasActive = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            profileWasActive = true;
            const bool paddle1Down = IsKeyDown(kPaddle1VirtualKey);
            const bool paddle2Down = IsKeyDown(kPaddle2VirtualKey);

            if (paddle1Down != leftShiftDown) {
                SendModifier(VK_LSHIFT, paddle1Down);
                leftShiftDown = paddle1Down;
            }

            if (paddle2Down != leftControlDown) {
                SendModifier(VK_LCONTROL, paddle2Down);
                leftControlDown = paddle2Down;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        ReleaseModifiers(leftShiftDown, leftControlDown);
    }
}

static void StartPaddleModifierInput()
{
    if (started.exchange(true))
        return;

    std::thread(PaddleThread).detach();
}

Q_COREAPP_STARTUP_FUNCTION(StartPaddleModifierInput)
