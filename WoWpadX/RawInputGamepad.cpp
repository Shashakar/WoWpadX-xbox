#include "RawInputGamepad.h"

#include "Log.h"

#include <QCoreApplication>
#include <QString>

#include <Windows.h>
#include <hidpi.h>
#include <hidusage.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr wchar_t kWindowClass[] = L"WoWpadXAllyRawInput";
    constexpr USHORT kAsusVendorId = 0x0B05;
    constexpr USHORT kAllyControllerProductId = 0x1B4C;

    struct ControllerState
    {
        std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> buttons{};
        std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> buttonKnown{};
        std::array<Sint16, SDL_GAMEPAD_AXIS_COUNT> axes{};
        std::array<bool, SDL_GAMEPAD_AXIS_COUNT> axisKnown{};
        bool available = false;
    };

    struct DeviceContext
    {
        HANDLE handle = nullptr;
        QString path;
        std::vector<BYTE> preparsedData;
        HIDP_CAPS caps{};
        std::vector<HIDP_BUTTON_CAPS> buttonCaps;
        std::vector<HIDP_VALUE_CAPS> valueCaps;
        bool firstStickLogged = false;
        bool firstTriggerLogged = false;
        std::array<BYTE, 16> lastDiagnosticReport{};
        bool hasLastDiagnosticReport = false;
    };

    std::atomic<bool> started = false;
    std::mutex stateMutex;
    ControllerState currentState;

    // Owned exclusively by the Raw Input message thread.
    std::unordered_map<std::uintptr_t, DeviceContext> devices;

    QString DevicePath(HANDLE device)
    {
        UINT characterCount = 0;
        if (GetRawInputDeviceInfoW(
                device,
                RIDI_DEVICENAME,
                nullptr,
                &characterCount) == static_cast<UINT>(-1) ||
            characterCount == 0) {
            return {};
        }

        std::vector<wchar_t> buffer(characterCount + 1, L'\0');
        if (GetRawInputDeviceInfoW(
                device,
                RIDI_DEVICENAME,
                buffer.data(),
                &characterCount) == static_cast<UINT>(-1)) {
            return {};
        }

        return QString::fromWCharArray(buffer.data());
    }

    std::optional<SDL_GamepadButton> MapButtonUsage(USAGE usage)
    {
        switch (usage) {
        case 1:  return SDL_GAMEPAD_BUTTON_SOUTH;
        case 2:  return SDL_GAMEPAD_BUTTON_EAST;
        case 3:  return SDL_GAMEPAD_BUTTON_WEST;
        case 4:  return SDL_GAMEPAD_BUTTON_NORTH;
        case 5:  return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
        case 6:  return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
        case 7:  return SDL_GAMEPAD_BUTTON_BACK;
        case 8:  return SDL_GAMEPAD_BUTTON_START;
        case 9:  return SDL_GAMEPAD_BUTTON_LEFT_STICK;
        case 10: return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
        case 11: return SDL_GAMEPAD_BUTTON_GUIDE;
        default: return std::nullopt;
        }
    }

    std::vector<USAGE> ButtonUsages(const HIDP_BUTTON_CAPS& cap)
    {
        std::vector<USAGE> usages;
        if (cap.IsRange) {
            for (USAGE usage = cap.Range.UsageMin;
                 usage <= cap.Range.UsageMax;
                 ++usage) {
                usages.push_back(usage);
                if (usage == std::numeric_limits<USAGE>::max())
                    break;
            }
        }
        else {
            usages.push_back(cap.NotRange.Usage);
        }
        return usages;
    }

    Sint16 NormalizeStick(BYTE value)
    {
        return static_cast<Sint16>((static_cast<int>(value) - 128) * 256);
    }

    Sint16 NormalizeTriggerMagnitude(int magnitude, int maximum)
    {
        if (magnitude <= 0 || maximum <= 0)
            return 0;

        return static_cast<Sint16>(
            std::min(32767, magnitude * 32767 / maximum));
    }

    void ParseButtons(
        DeviceContext& device,
        const BYTE* report,
        ULONG reportLength,
        ControllerState& state)
    {
        auto preparsed = reinterpret_cast<PHIDP_PREPARSED_DATA>(
            device.preparsedData.data());

        for (const HIDP_BUTTON_CAPS& cap : device.buttonCaps) {
            if (cap.ReportID != 0 &&
                (reportLength == 0 || report[0] != cap.ReportID)) {
                continue;
            }

            if (cap.UsagePage != HID_USAGE_PAGE_BUTTON)
                continue;

            const std::vector<USAGE> supportedUsages = ButtonUsages(cap);
            for (USAGE usage : supportedUsages) {
                if (const auto mapped = MapButtonUsage(usage))
                    state.buttonKnown[*mapped] = true;
            }

            ULONG activeCount = static_cast<ULONG>(supportedUsages.size());
            if (activeCount == 0)
                continue;

            std::vector<USAGE> activeUsages(activeCount);
            const NTSTATUS result = HidP_GetUsages(
                HidP_Input,
                cap.UsagePage,
                cap.LinkCollection,
                activeUsages.data(),
                &activeCount,
                preparsed,
                reinterpret_cast<PCHAR>(const_cast<BYTE*>(report)),
                reportLength);

            if (result != HIDP_STATUS_SUCCESS)
                continue;

            for (ULONG index = 0; index < activeCount; ++index) {
                if (const auto mapped = MapButtonUsage(activeUsages[index]))
                    state.buttons[*mapped] = true;
            }
        }
    }

    void ParseDPad(
        DeviceContext& device,
        const BYTE* report,
        ULONG reportLength,
        ControllerState& state)
    {
        auto preparsed = reinterpret_cast<PHIDP_PREPARSED_DATA>(
            device.preparsedData.data());

        for (const HIDP_VALUE_CAPS& cap : device.valueCaps) {
            if (cap.ReportID != 0 &&
                (reportLength == 0 || report[0] != cap.ReportID)) {
                continue;
            }

            if (cap.UsagePage != HID_USAGE_PAGE_GENERIC)
                continue;

            USAGE usage = 0;
            if (cap.IsRange) {
                if (HID_USAGE_GENERIC_HATSWITCH < cap.Range.UsageMin ||
                    HID_USAGE_GENERIC_HATSWITCH > cap.Range.UsageMax) {
                    continue;
                }
                usage = HID_USAGE_GENERIC_HATSWITCH;
            }
            else {
                usage = cap.NotRange.Usage;
                if (usage != HID_USAGE_GENERIC_HATSWITCH)
                    continue;
            }

            ULONG value = 0;
            if (HidP_GetUsageValue(
                    HidP_Input,
                    cap.UsagePage,
                    cap.LinkCollection,
                    usage,
                    &value,
                    preparsed,
                    reinterpret_cast<PCHAR>(const_cast<BYTE*>(report)),
                    reportLength) != HIDP_STATUS_SUCCESS) {
                continue;
            }

            state.buttonKnown[SDL_GAMEPAD_BUTTON_DPAD_UP] = true;
            state.buttonKnown[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = true;
            state.buttonKnown[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = true;
            state.buttonKnown[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = true;

            const int position = static_cast<int>(value) - cap.LogicalMin;
            if (position < 0 || position > 7)
                return;

            state.buttons[SDL_GAMEPAD_BUTTON_DPAD_UP] =
                position == 0 || position == 1 || position == 7;
            state.buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] =
                position == 1 || position == 2 || position == 3;
            state.buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN] =
                position == 3 || position == 4 || position == 5;
            state.buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT] =
                position == 5 || position == 6 || position == 7;
            return;
        }
    }

    void ParseAllyAxes(
        DeviceContext& device,
        const BYTE* report,
        ULONG reportLength,
        ControllerState& state)
    {
        if (reportLength < 11)
            return;

        state.axisKnown[SDL_GAMEPAD_AXIS_LEFTX] = true;
        state.axisKnown[SDL_GAMEPAD_AXIS_LEFTY] = true;
        state.axisKnown[SDL_GAMEPAD_AXIS_RIGHTX] = true;
        state.axisKnown[SDL_GAMEPAD_AXIS_RIGHTY] = true;
        state.axisKnown[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] = true;
        state.axisKnown[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = true;

        state.axes[SDL_GAMEPAD_AXIS_LEFTX] = NormalizeStick(report[2]);
        state.axes[SDL_GAMEPAD_AXIS_LEFTY] = NormalizeStick(report[4]);
        state.axes[SDL_GAMEPAD_AXIS_RIGHTX] = NormalizeStick(report[6]);
        state.axes[SDL_GAMEPAD_AXIS_RIGHTY] = NormalizeStick(report[8]);

        // The Ally compatibility HID report combines both triggers on byte 10.
        // Neutral is 0x80. One trigger moves toward 0x00; the other moves toward
        // 0xFF. Split the two halves into SDL's independent trigger axes.
        const int combinedTrigger = static_cast<int>(report[10]);
        const int leftMagnitude = combinedTrigger < 128
            ? 128 - combinedTrigger
            : 0;
        const int rightMagnitude = combinedTrigger > 128
            ? combinedTrigger - 128
            : 0;

        state.axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] =
            NormalizeTriggerMagnitude(leftMagnitude, 128);
        state.axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] =
            NormalizeTriggerMagnitude(rightMagnitude, 127);

        constexpr int motionThreshold = 1024;
        const bool stickMoved =
            std::abs(static_cast<int>(state.axes[SDL_GAMEPAD_AXIS_LEFTX])) > motionThreshold ||
            std::abs(static_cast<int>(state.axes[SDL_GAMEPAD_AXIS_LEFTY])) > motionThreshold ||
            std::abs(static_cast<int>(state.axes[SDL_GAMEPAD_AXIS_RIGHTX])) > motionThreshold ||
            std::abs(static_cast<int>(state.axes[SDL_GAMEPAD_AXIS_RIGHTY])) > motionThreshold;

        if (stickMoved && !device.firstStickLogged) {
            device.firstStickLogged = true;
            Log::writeLine(
                QString(
                    "[RawInput] First direct Ally stick state: "
                    "rawLX=%1 rawLY=%2 rawRX=%3 rawRY=%4")
                    .arg(report[2])
                    .arg(report[4])
                    .arg(report[6])
                    .arg(report[8]));
        }

    const bool sticksNearNeutral =
        std::abs(static_cast<int>(report[2]) - 128) <= 6 &&
        std::abs(static_cast<int>(report[4]) - 128) <= 6 &&
        std::abs(static_cast<int>(report[6]) - 128) <= 6 &&
        std::abs(static_cast<int>(report[8]) - 128) <= 6;

    bool reportChanged = !device.hasLastDiagnosticReport;
    if (!reportChanged) {
        for (ULONG index = 0; index < reportLength && index < 16; ++index) {
            if (device.lastDiagnosticReport[index] != report[index]) {
                reportChanged = true;
                break;
            }
        }
    }

    if (sticksNearNeutral && reportChanged) {
        QString bytes;
        for (ULONG index = 0; index < reportLength && index < 16; ++index) {
            if (!bytes.isEmpty())
                bytes += " ";
            bytes += QString("%1").arg(report[index], 2, 16, QChar('0'));
            device.lastDiagnosticReport[index] = report[index];
        }
        device.hasLastDiagnosticReport = true;

        Log::writeLine(
            QString(
                "[RawInput] Ally neutral-stick report bytes=[%1] combined=%2 LT=%3 RT=%4")
                .arg(bytes)
                .arg(combinedTrigger)
                .arg(state.axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER])
                .arg(state.axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER]));
    }
    }

    bool LoadDevice(HANDLE handle, DeviceContext& output)
    {
        RID_DEVICE_INFO info{};
        info.cbSize = sizeof(info);
        UINT infoSize = sizeof(info);

        if (GetRawInputDeviceInfoW(
                handle,
                RIDI_DEVICEINFO,
                &info,
                &infoSize) == static_cast<UINT>(-1) ||
            info.dwType != RIM_TYPEHID ||
            info.hid.dwVendorId != kAsusVendorId ||
            info.hid.dwProductId != kAllyControllerProductId) {
            return false;
        }

        const QString path = DevicePath(handle);
        if (!path.contains("IG_00", Qt::CaseInsensitive))
            return false;

        UINT preparsedSize = 0;
        if (GetRawInputDeviceInfoW(
                handle,
                RIDI_PREPARSEDDATA,
                nullptr,
                &preparsedSize) == static_cast<UINT>(-1) ||
            preparsedSize == 0) {
            return false;
        }

        output = {};
        output.handle = handle;
        output.path = path;
        output.preparsedData.resize(preparsedSize);

        if (GetRawInputDeviceInfoW(
                handle,
                RIDI_PREPARSEDDATA,
                output.preparsedData.data(),
                &preparsedSize) == static_cast<UINT>(-1)) {
            return false;
        }

        auto preparsed = reinterpret_cast<PHIDP_PREPARSED_DATA>(
            output.preparsedData.data());
        if (HidP_GetCaps(preparsed, &output.caps) != HIDP_STATUS_SUCCESS)
            return false;

        USHORT buttonCount = output.caps.NumberInputButtonCaps;
        output.buttonCaps.resize(buttonCount);
        if (buttonCount > 0) {
            if (HidP_GetButtonCaps(
                    HidP_Input,
                    output.buttonCaps.data(),
                    &buttonCount,
                    preparsed) != HIDP_STATUS_SUCCESS) {
                output.buttonCaps.clear();
            }
            else {
                output.buttonCaps.resize(buttonCount);
            }
        }

        USHORT valueCount = output.caps.NumberInputValueCaps;
        output.valueCaps.resize(valueCount);
        if (valueCount > 0) {
            if (HidP_GetValueCaps(
                    HidP_Input,
                    output.valueCaps.data(),
                    &valueCount,
                    preparsed) != HIDP_STATUS_SUCCESS) {
                output.valueCaps.clear();
            }
            else {
                output.valueCaps.resize(valueCount);
            }
        }

        Log::writeLine(
            QString(
                "[RawInput] Loaded Ally controller path=\"%1\" "
                "reportBytes=%2 buttonCaps=%3 valueCaps=%4")
                .arg(output.path)
                .arg(output.caps.InputReportByteLength)
                .arg(output.buttonCaps.size())
                .arg(output.valueCaps.size()));
        return true;
    }

    DeviceContext* GetDevice(HANDLE handle)
    {
        const auto key = reinterpret_cast<std::uintptr_t>(handle);
        if (const auto found = devices.find(key); found != devices.end())
            return &found->second;

        DeviceContext device;
        if (!LoadDevice(handle, device))
            return nullptr;

        auto [inserted, unused] = devices.emplace(key, std::move(device));
        return &inserted->second;
    }

    void Publish(const ControllerState& state)
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        currentState = state;
        currentState.available = true;
    }

    void ProcessRawInput(HRAWINPUT rawInputHandle)
    {
        UINT requiredSize = 0;
        if (GetRawInputData(
                rawInputHandle,
                RID_INPUT,
                nullptr,
                &requiredSize,
                sizeof(RAWINPUTHEADER)) != 0 ||
            requiredSize == 0) {
            return;
        }

        std::vector<BYTE> storage(requiredSize);
        UINT actualSize = requiredSize;
        if (GetRawInputData(
                rawInputHandle,
                RID_INPUT,
                storage.data(),
                &actualSize,
                sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) {
            return;
        }

        const auto* input = reinterpret_cast<const RAWINPUT*>(storage.data());
        if (input->header.dwType != RIM_TYPEHID)
            return;

        DeviceContext* device = GetDevice(input->header.hDevice);
        if (!device)
            return;

        const ULONG reportLength = input->data.hid.dwSizeHid;
        const ULONG reportCount = input->data.hid.dwCount;
        for (ULONG reportIndex = 0; reportIndex < reportCount; ++reportIndex) {
            const BYTE* report = input->data.hid.bRawData +
                static_cast<size_t>(reportIndex) * reportLength;

            ControllerState state;
            ParseButtons(*device, report, reportLength, state);
            ParseDPad(*device, report, reportLength, state);
            ParseAllyAxes(*device, report, reportLength, state);
            Publish(state);
        }
    }

    LRESULT CALLBACK WindowProcedure(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
    {
        switch (message) {
        case WM_INPUT:
            ProcessRawInput(reinterpret_cast<HRAWINPUT>(lParam));
            return DefWindowProcW(window, message, wParam, lParam);

        case WM_INPUT_DEVICE_CHANGE:
            if (wParam == GIDC_REMOVAL) {
                const auto key = reinterpret_cast<std::uintptr_t>(
                    reinterpret_cast<HANDLE>(lParam));
                devices.erase(key);
            }
            else {
                GetDevice(reinterpret_cast<HANDLE>(lParam));
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(window, message, wParam, lParam);
        }
    }

    void ThreadMain()
    {
        const HINSTANCE instance = GetModuleHandleW(nullptr);

        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = WindowProcedure;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = kWindowClass;

        if (!RegisterClassW(&windowClass) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            Log::writeLine(
                "[RawInput] Failed to register Ally input window. Error=" +
                QString::number(GetLastError()));
            return;
        }

        HWND window = CreateWindowExW(
            0,
            kWindowClass,
            L"WoWpadX Ally Raw Input",
            0,
            0,
            0,
            0,
            0,
            HWND_MESSAGE,
            nullptr,
            instance,
            nullptr);

        if (!window) {
            Log::writeLine(
                "[RawInput] Failed to create Ally input window. Error=" +
                QString::number(GetLastError()));
            return;
        }

        RAWINPUTDEVICE registrations[] = {
            { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_JOYSTICK,
              RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window },
            { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_GAMEPAD,
              RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window },
            { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER,
              RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window },
        };

        if (!RegisterRawInputDevices(
                registrations,
                static_cast<UINT>(std::size(registrations)),
                sizeof(RAWINPUTDEVICE))) {
            Log::writeLine(
                "[RawInput] Failed to register Ally controller. Error=" +
                QString::number(GetLastError()));
            DestroyWindow(window);
            return;
        }

        Log::writeLine(
            "[RawInput] Registered dedicated Ally button, stick, and trigger backend.");

        UINT deviceCount = 0;
        if (GetRawInputDeviceList(
                nullptr,
                &deviceCount,
                sizeof(RAWINPUTDEVICELIST)) != static_cast<UINT>(-1)) {
            std::vector<RAWINPUTDEVICELIST> deviceList(deviceCount);
            if (GetRawInputDeviceList(
                    deviceList.data(),
                    &deviceCount,
                    sizeof(RAWINPUTDEVICELIST)) != static_cast<UINT>(-1)) {
                for (const RAWINPUTDEVICELIST& entry : deviceList) {
                    if (entry.dwType == RIM_TYPEHID)
                        GetDevice(entry.hDevice);
                }
            }
        }

        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}

namespace RawInputGamepad
{
    void EnsureStarted()
    {
        if (started.exchange(true))
            return;

        std::thread(ThreadMain).detach();
    }

    bool TryGetButton(SDL_GamepadButton button, bool& pressed)
    {
        if (button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT)
            return false;

        std::lock_guard<std::mutex> lock(stateMutex);
        if (!currentState.available || !currentState.buttonKnown[button])
            return false;

        pressed = currentState.buttons[button];
        return true;
    }

    bool TryGetAxis(SDL_GamepadAxis axis, Sint16& value)
    {
        if (axis < 0 || axis >= SDL_GAMEPAD_AXIS_COUNT)
            return false;

        std::lock_guard<std::mutex> lock(stateMutex);
        if (!currentState.available || !currentState.axisKnown[axis])
            return false;

        value = currentState.axes[axis];
        return true;
    }

    bool HasUsableState()
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        return currentState.available;
    }
}

static void StartRawInputBackend()
{
    RawInputGamepad::EnsureStarted();
}

Q_COREAPP_STARTUP_FUNCTION(StartRawInputBackend)
