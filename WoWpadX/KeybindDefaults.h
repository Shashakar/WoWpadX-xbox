#pragma once

#include <vector>
#include <Windows.h>
#include "Gamepad/gamepad.h"

struct Keybind {
    GamepadBinding bindType;
    int virtualKey;
};

struct Keybinds {
    std::vector<Keybind> bindings;

    Keybinds() = default;
};

namespace KeybindDefaults {

    inline std::vector<Keybind> defaultBase = {
        { GamepadBinding::North, VK_F9 },
        { GamepadBinding::East, VK_F10 },
        { GamepadBinding::South, VK_F11 },
        { GamepadBinding::West, VK_F12 },

        { GamepadBinding::DPadUp, VK_F1 },
        { GamepadBinding::DPadRight, VK_F2 },
        { GamepadBinding::DPadDown, VK_F3 },
        { GamepadBinding::DPadLeft, VK_F4 },

        { GamepadBinding::Back, VK_F5 },
        { GamepadBinding::Start, VK_F6 },
        { GamepadBinding::Guide, VK_MULTIPLY },

        { GamepadBinding::LeftStickUp, 'W' },
        { GamepadBinding::LeftStickLeft, 'A' },
        { GamepadBinding::LeftStickDown, 'S' },
        { GamepadBinding::LeftStickRight, 'D' },
        { GamepadBinding::LeftStickHorz, 'H' },
        { GamepadBinding::LeftStickVert, 'V' },

        { GamepadBinding::Misc1, VK_ADD },
        { GamepadBinding::RightPaddle1, VK_NUMPAD0 },
        { GamepadBinding::RightPaddle2, VK_NUMPAD1 },
        { GamepadBinding::LeftPaddle1, VK_NUMPAD2 },
        { GamepadBinding::LeftPaddle2, VK_NUMPAD3 },
    };

    inline std::vector<Keybind> getDefault(int style) {
        std::vector<Keybind> binds;

        switch (style) {
        case 0:
            binds = {
                { GamepadBinding::LeftShoulder, VK_LSHIFT },
                { GamepadBinding::RightShoulder, VK_F7 },
                { GamepadBinding::LeftTrigger, VK_LCONTROL },
                { GamepadBinding::RightTrigger, VK_F8 },
            };
            break;
        case 1:
            binds = {
                { GamepadBinding::LeftShoulder, VK_F7 },
                { GamepadBinding::RightShoulder, VK_F8 },
                { GamepadBinding::LeftTrigger, VK_LSHIFT },
                { GamepadBinding::RightTrigger, VK_LCONTROL },
            };
            break;
        case 2:
            binds = {
                { GamepadBinding::LeftShoulder, VK_F7 },
                { GamepadBinding::LeftTrigger, VK_F8 },
                { GamepadBinding::RightShoulder, VK_LSHIFT },
                { GamepadBinding::RightTrigger, VK_LCONTROL },
            };
            break;
        case 3:
            binds = {
                { GamepadBinding::LeftTrigger, VK_F7 },
                { GamepadBinding::RightTrigger, VK_F8 },
                { GamepadBinding::LeftShoulder, VK_LSHIFT },
                { GamepadBinding::RightShoulder, VK_LCONTROL },
            };
            break;
        case 4:
            // Armoury Crate emits Left Shift from M1 and Left Ctrl from M2.
            // WoWpadX must not synthesize another modifier pair in this mode.
            // Keep L1/R1 as the normal ConsolePort shoulder actions and leave
            // the combined-axis triggers unbound.
            binds = {
                { GamepadBinding::LeftShoulder, VK_F7 },
                { GamepadBinding::RightShoulder, VK_F8 },
                { GamepadBinding::LeftTrigger, 0 },
                { GamepadBinding::RightTrigger, 0 },
            };
            break;
        default:
            return getDefault(0);
        }

        binds.insert(binds.end(), defaultBase.begin(), defaultBase.end());
        return binds;
    }

}
