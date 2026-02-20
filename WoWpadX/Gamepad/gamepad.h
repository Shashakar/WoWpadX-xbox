#pragma once
#include "SDL3/SDL.h"
#include <unordered_map>

enum class GamepadBinding : int {
    Invalid = SDL_GAMEPAD_BUTTON_INVALID,
    DPadUp = SDL_GAMEPAD_BUTTON_DPAD_UP,
    DPadDown = SDL_GAMEPAD_BUTTON_DPAD_DOWN,
    DPadLeft = SDL_GAMEPAD_BUTTON_DPAD_LEFT,
    DPadRight = SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
    North = SDL_GAMEPAD_BUTTON_NORTH,
    South = SDL_GAMEPAD_BUTTON_SOUTH,
    West = SDL_GAMEPAD_BUTTON_WEST,
    East = SDL_GAMEPAD_BUTTON_EAST,
    LeftStick = SDL_GAMEPAD_BUTTON_LEFT_STICK,
    RightStick = SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    LeftShoulder = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
    RightShoulder = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
    Back = SDL_GAMEPAD_BUTTON_BACK,
    Start = SDL_GAMEPAD_BUTTON_START,
    Guide = SDL_GAMEPAD_BUTTON_GUIDE,
    Misc1 = SDL_GAMEPAD_BUTTON_MISC1,
    RightPaddle1 = SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1,
    RightPaddle2 = SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2,
    LeftPaddle1 = SDL_GAMEPAD_BUTTON_LEFT_PADDLE1,
    LeftPaddle2 = SDL_GAMEPAD_BUTTON_LEFT_PADDLE2,
    Misc2 = SDL_GAMEPAD_BUTTON_MISC2,
    Misc3 = SDL_GAMEPAD_BUTTON_MISC3,
    Misc4 = SDL_GAMEPAD_BUTTON_MISC4,
    Misc5 = SDL_GAMEPAD_BUTTON_MISC5,
    Misc6 = SDL_GAMEPAD_BUTTON_MISC6,

    // SDL_GamepadAxis actually, but we use fake values here only to help with keybindings.
    LeftTrigger = -100,
    RightTrigger = -101,
    LeftStickUp = -102,
    LeftStickDown = -103,
    LeftStickLeft = -104,
    LeftStickRight = -105,
    LeftStickHorz = -106,
    LeftStickVert = -107,
};