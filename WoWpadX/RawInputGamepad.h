#pragma once

#include <SDL3/SDL_gamepad.h>

namespace RawInputGamepad
{
    // Starts the background Raw Input message window once. Safe to call repeatedly.
    void EnsureStarted();

    // Return true when Raw Input has a current value for the requested control.
    // Callers should fall back to GameInput/SDL when false is returned.
    bool TryGetButton(SDL_GamepadButton button, bool& pressed);
    bool TryGetAxis(SDL_GamepadAxis axis, Sint16& value);

    bool HasUsableState();
}
