#pragma once

#include <vector>
#include <Windows.h>
#include "Gamepad/gamepad.h"


// Represents a single keybind between a gamepad button and a virtual key
struct Keybind {
    GamepadBinding bindType;
    int virtualKey; // Using Windows virtual-key codes (VK_*)
} ;

// Container for a full set of keybindings
struct Keybinds {
    std::vector<Keybind> bindings;

    Keybinds() = default;
};

namespace KeybindDefaults {

    inline std::vector<Keybind> defaultBase = {
        { GamepadBinding::North, VK_F9 },     // RFaceUp
        { GamepadBinding::East, VK_F10 },    // RFaceRight
        { GamepadBinding::South, VK_F11 },    // RFaceDown
        { GamepadBinding::West, VK_F12 },    // RFaceLeft

        { GamepadBinding::DPadUp, VK_F1 },  // LFaceUp (custom mapping)
        { GamepadBinding::DPadRight, VK_F2 },// LFaceRight (custom)
        { GamepadBinding::DPadDown, VK_F3 },// LFaceDown (custom)
        { GamepadBinding::DPadLeft, VK_F4 },// LFaceLeft (custom)

        { GamepadBinding::Back, VK_F5 },
        { GamepadBinding::Start, VK_F6 },
        { GamepadBinding::Guide, VK_MULTIPLY },

        { GamepadBinding::LeftStickUp, 'W' }, // LeftStickUp
        { GamepadBinding::LeftStickLeft, 'A' }, // LeftStickLeft
        { GamepadBinding::LeftStickDown, 'S' }, // LeftStickDown
        { GamepadBinding::LeftStickRight, 'D' }, // LeftStickRight
        { GamepadBinding::LeftStickHorz, 'H' }, // LeftStickHorz (used for 16 way movement)
        { GamepadBinding::LeftStickVert, 'V' }, // LeftStickVert (used for 16 way movement)

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
                { GamepadBinding::LeftTrigger , VK_F7 },
                { GamepadBinding::RightTrigger, VK_F8 },
                { GamepadBinding::LeftShoulder, VK_LSHIFT },
                { GamepadBinding::RightShoulder, VK_LCONTROL },
            };
            break;
        }

        binds.insert(binds.end(), defaultBase.begin(), defaultBase.end());
        return binds;
    }

}
