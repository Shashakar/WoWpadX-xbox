#include "RawInputGamepad.h"

#include "Log.h"

#include <QCoreApplication>
#include <QChar>
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
    constexpr wchar_t kRawInputWindowClass[] = L"WoWpadXRawInputGamepad";
    constexpr USHORT kAsusVendorId = 0x0B05;
    constexpr USHORT kAllyControllerProductId = 0x1B4C;

    struct ParsedState
    {
        std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> buttons{};
        std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> buttonKnown{};
        std::array<Sint16, SDL_GAMEPAD_AXIS_COUNT> axes{};
        std::array<bool, SDL_GAMEPAD_AXIS_COUNT> axisKnown{};
    };

    struct DeviceContext
    {
        HANDLE handle = nullptr;
        RID_DEVICE_INFO rawInfo{};
        QString path;
        QString description;
        std::vector<BYTE> preparsedStorage;
        HIDP_CAPS caps{};
        std::vector<HIDP_BUTTON_CAPS> buttonCaps;
        std::vector<HIDP_VALUE_CAPS> valueCaps;
        int selectionScore = 0;
        bool usable = false;
    };

    struct SharedState
    {
        ParsedState state;
        bool available = false;
        std::uintptr_t activeDevice = 0;
        int activeScore = std::numeric_limits<int>::min();
        bool firstNonNeutralLogged = false;
    };

    std::atomic<bool> rawInputThreadStarted = false;
    std::mutex sharedStateMutex;
    SharedState sharedState;

    // The device map is owned exclusively by the Raw Input message thread.
    std::unordered_map<std::uintptr_t, DeviceContext> devices;

    QString HandleText(HANDLE handle)
    {
        return QString("0x%1")
            .arg(reinterpret_cast<quintptr>(handle), 0, 16);
    }

    QString ReadDevicePath(HANDLE device)
    {
        UINT characterCount = 0;
        if (GetRawInputDeviceInfoW(
                device,
                RIDI_DEVICENAME,
                nullptr,
                &characterCount) == static_cast<UINT>(-1) ||
            characterCount == 0) {
            return "unknown";
        }

        std::vector<wchar_t> buffer(characterCount + 1, L'\0');
        if (GetRawInputDeviceInfoW(
                device,
                RIDI_DEVICENAME,
                buffer.data(),
                &characterCount) == static_cast<UINT>(-1)) {
            return "unknown";
        }

        return QString::fromWCharArray(buffer.data());
    }

    QString DescribeRawDevice(HANDLE device, const RID_DEVICE_INFO& info)
    {
        QString description =
            "handle=" + HandleText(device) +
            " path=\"" + ReadDevicePath(device) + "\"" +
            " type=" + QString::number(info.dwType);

        if (info.dwType == RIM_TYPEHID) {
            description +=
                " vid=0x" + QString::number(info.hid.dwVendorId, 16) +
                " pid=0x" + QString::number(info.hid.dwProductId, 16) +
                " version=0x" + QString::number(info.hid.dwVersionNumber, 16) +
                " usagePage=0x" + QString::number(info.hid.usUsagePage, 16) +
                " usage=0x" + QString::number(info.hid.usUsage, 16);
        }

        return description;
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

    LONG DecodeLogicalValue(ULONG value, const HIDP_VALUE_CAPS& cap)
    {
        if (cap.LogicalMin >= 0 || cap.BitSize == 0 || cap.BitSize >= 32)
            return static_cast<LONG>(value);

        const ULONG signBit = 1UL << (cap.BitSize - 1);
        if ((value & signBit) == 0)
            return static_cast<LONG>(value);

        const ULONG valueMask = (1UL << cap.BitSize) - 1UL;
        return static_cast<LONG>(value | ~valueMask);
    }

    Sint16 NormalizeSignedAxis(
        LONG value,
        LONG logicalMin,
        LONG logicalMax)
    {
        if (logicalMax <= logicalMin)
            return 0;

        const double normalized =
            (static_cast<double>(value - logicalMin) /
             static_cast<double>(logicalMax - logicalMin)) * 2.0 - 1.0;

        const double clamped = std::clamp(normalized, -1.0, 1.0);
        return static_cast<Sint16>(std::lround(clamped * 32767.0));
    }

    Sint16 NormalizeTrigger(
        LONG value,
        LONG logicalMin,
        LONG logicalMax)
    {
        if (logicalMax <= logicalMin)
            return 0;

        const double normalized =
            static_cast<double>(value - logicalMin) /
            static_cast<double>(logicalMax - logicalMin);

        const double clamped = std::clamp(normalized, 0.0, 1.0);
        return static_cast<Sint16>(std::lround(clamped * 32767.0));
    }

    bool IsReportForCap(const HIDP_VALUE_CAPS& cap, const BYTE* report, ULONG reportLength)
    {
        return cap.ReportID == 0 ||
            (reportLength > 0 && report[0] == cap.ReportID);
    }

    bool IsReportForCap(const HIDP_BUTTON_CAPS& cap, const BYTE* report, ULONG reportLength)
    {
        return cap.ReportID == 0 ||
            (reportLength > 0 && report[0] == cap.ReportID);
    }

    std::vector<USAGE> CapUsages(const HIDP_VALUE_CAPS& cap)
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

    std::vector<USAGE> CapUsages(const HIDP_BUTTON_CAPS& cap)
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

    void MarkKnownButtonUsages(
        const HIDP_BUTTON_CAPS& cap,
        ParsedState& parsed)
    {
        if (cap.UsagePage != HID_USAGE_PAGE_BUTTON)
            return;

        for (USAGE usage : CapUsages(cap)) {
            const auto mapped = MapButtonUsage(usage);
            if (mapped)
                parsed.buttonKnown[*mapped] = true;
        }
    }

    void ParseButtons(
        DeviceContext& device,
        const BYTE* report,
        ULONG reportLength,
        ParsedState& parsed)
    {
        PHIDP_PREPARSED_DATA preparsed =
            reinterpret_cast<PHIDP_PREPARSED_DATA>(device.preparsedStorage.data());

        for (const HIDP_BUTTON_CAPS& cap : device.buttonCaps) {
            if (!IsReportForCap(cap, report, reportLength))
                continue;

            MarkKnownButtonUsages(cap, parsed);
            if (cap.UsagePage != HID_USAGE_PAGE_BUTTON)
                continue;

            ULONG usageCount = static_cast<ULONG>(CapUsages(cap).size());
            if (usageCount == 0)
                continue;

            std::vector<USAGE> activeUsages(usageCount);
            NTSTATUS status = HidP_GetUsages(
                HidP_Input,
                cap.UsagePage,
                cap.LinkCollection,
                activeUsages.data(),
                &usageCount,
                preparsed,
                reinterpret_cast<PCHAR>(const_cast<BYTE*>(report)),
                reportLength);

            if (status != HIDP_STATUS_SUCCESS)
                continue;

            for (ULONG index = 0; index < usageCount; ++index) {
                const auto mapped = MapButtonUsage(activeUsages[index]);
                if (mapped)
                    parsed.buttons[*mapped] = true;
            }
        }
    }

    struct LogicalReading
    {
        bool available = false;
        LONG value = 0;
        LONG logicalMin = 0;
        LONG logicalMax = 0;
    };

    void StoreLogicalReading(
        std::unordered_map<USAGE, LogicalReading>& readings,
        USAGE usage,
        ULONG rawValue,
        const HIDP_VALUE_CAPS& cap)
    {
        LogicalReading reading;
        reading.available = true;
        reading.value = DecodeLogicalValue(rawValue, cap);
        reading.logicalMin = cap.LogicalMin;
        reading.logicalMax = cap.LogicalMax;
        readings[usage] = reading;
    }

    void ApplyHatSwitch(const LogicalReading& reading, ParsedState& parsed)
    {
        parsed.buttonKnown[SDL_GAMEPAD_BUTTON_DPAD_UP] = true;
        parsed.buttonKnown[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = true;
        parsed.buttonKnown[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = true;
        parsed.buttonKnown[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = true;

        if (!reading.available ||
            reading.value < reading.logicalMin ||
            reading.value > reading.logicalMax) {
            return;
        }

        const int position = static_cast<int>(reading.value - reading.logicalMin);
        if (position < 0 || position > 7)
            return;

        parsed.buttons[SDL_GAMEPAD_BUTTON_DPAD_UP] =
            position == 0 || position == 1 || position == 7;
        parsed.buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] =
            position == 1 || position == 2 || position == 3;
        parsed.buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN] =
            position == 3 || position == 4 || position == 5;
        parsed.buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT] =
            position == 5 || position == 6 || position == 7;
    }

    void ApplySignedAxis(
        const LogicalReading& reading,
        SDL_GamepadAxis axis,
        ParsedState& parsed)
    {
        if (!reading.available)
            return;

        parsed.axisKnown[axis] = true;
        parsed.axes[axis] = NormalizeSignedAxis(
            reading.value,
            reading.logicalMin,
            reading.logicalMax);
    }

    void ApplyTrigger(
        const LogicalReading& reading,
        SDL_GamepadAxis axis,
        ParsedState& parsed)
    {
        if (!reading.available)
            return;

        parsed.axisKnown[axis] = true;
        parsed.axes[axis] = NormalizeTrigger(
            reading.value,
            reading.logicalMin,
            reading.logicalMax);
    }

    void ParseValues(
        DeviceContext& device,
        const BYTE* report,
        ULONG reportLength,
        ParsedState& parsed)
    {
        PHIDP_PREPARSED_DATA preparsed =
            reinterpret_cast<PHIDP_PREPARSED_DATA>(device.preparsedStorage.data());

        std::unordered_map<USAGE, LogicalReading> genericReadings;

        for (const HIDP_VALUE_CAPS& cap : device.valueCaps) {
            if (!IsReportForCap(cap, report, reportLength) ||
                cap.UsagePage != HID_USAGE_PAGE_GENERIC) {
                continue;
            }

            for (USAGE usage : CapUsages(cap)) {
                ULONG rawValue = 0;
                NTSTATUS status = HidP_GetUsageValue(
                    HidP_Input,
                    cap.UsagePage,
                    cap.LinkCollection,
                    usage,
                    &rawValue,
                    preparsed,
                    reinterpret_cast<PCHAR>(const_cast<BYTE*>(report)),
                    reportLength);

                if (status == HIDP_STATUS_SUCCESS)
                    StoreLogicalReading(genericReadings, usage, rawValue, cap);
            }
        }

        const auto reading = [&](USAGE usage) -> LogicalReading {
            const auto found = genericReadings.find(usage);
            return found == genericReadings.end()
                ? LogicalReading{}
                : found->second;
        };

        ApplySignedAxis(
            reading(HID_USAGE_GENERIC_X),
            SDL_GAMEPAD_AXIS_LEFTX,
            parsed);
        ApplySignedAxis(
            reading(HID_USAGE_GENERIC_Y),
            SDL_GAMEPAD_AXIS_LEFTY,
            parsed);

        const LogicalReading rx = reading(HID_USAGE_GENERIC_RX);
        const LogicalReading ry = reading(HID_USAGE_GENERIC_RY);
        const LogicalReading z = reading(HID_USAGE_GENERIC_Z);
        const LogicalReading rz = reading(HID_USAGE_GENERIC_RZ);
        const LogicalReading slider = reading(HID_USAGE_GENERIC_SLIDER);
        const LogicalReading dial = reading(HID_USAGE_GENERIC_DIAL);

        // Xbox-style HID descriptors expose Rx/Ry as the right stick and
        // Z/Rz as independent triggers. Older DirectInput-style descriptors
        // may use Z/Rz for the right stick and Slider/Dial for triggers.
        if (rx.available && ry.available) {
            ApplySignedAxis(rx, SDL_GAMEPAD_AXIS_RIGHTX, parsed);
            ApplySignedAxis(ry, SDL_GAMEPAD_AXIS_RIGHTY, parsed);
            ApplyTrigger(z, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, parsed);
            ApplyTrigger(rz, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, parsed);
        }
        else {
            ApplySignedAxis(z, SDL_GAMEPAD_AXIS_RIGHTX, parsed);
            ApplySignedAxis(rz, SDL_GAMEPAD_AXIS_RIGHTY, parsed);
            ApplyTrigger(slider, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, parsed);
            ApplyTrigger(dial, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, parsed);
        }

        ApplyHatSwitch(reading(HID_USAGE_GENERIC_HATSWITCH), parsed);
    }

    bool IsNonNeutral(const ParsedState& parsed)
    {
        for (size_t index = 0; index < parsed.buttons.size(); ++index) {
            if (parsed.buttonKnown[index] && parsed.buttons[index])
                return true;
        }

        constexpr Sint16 axisNoiseThreshold = 1024;
        for (size_t index = 0; index < parsed.axes.size(); ++index) {
            if (parsed.axisKnown[index] &&
                std::abs(static_cast<int>(parsed.axes[index])) > axisNoiseThreshold) {
                return true;
            }
        }

        return false;
    }

    QString DescribeParsedState(const ParsedState& parsed)
    {
        quint32 buttonMask = 0;
        for (size_t index = 0; index < parsed.buttons.size() && index < 32; ++index) {
            if (parsed.buttons[index])
                buttonMask |= (1u << index);
        }

        return QString("buttons=0x%1 LX=%2 LY=%3 RX=%4 RY=%5 LT=%6 RT=%7")
            .arg(buttonMask, 0, 16)
            .arg(parsed.axes[SDL_GAMEPAD_AXIS_LEFTX])
            .arg(parsed.axes[SDL_GAMEPAD_AXIS_LEFTY])
            .arg(parsed.axes[SDL_GAMEPAD_AXIS_RIGHTX])
            .arg(parsed.axes[SDL_GAMEPAD_AXIS_RIGHTY])
            .arg(parsed.axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER])
            .arg(parsed.axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER]);
    }

    void PublishState(DeviceContext& device, const ParsedState& parsed)
    {
        const auto key = reinterpret_cast<std::uintptr_t>(device.handle);
        bool selectedNow = false;
        bool logFirstInput = false;

        {
            std::lock_guard<std::mutex> lock(sharedStateMutex);

            if (sharedState.activeDevice != 0 &&
                sharedState.activeDevice != key &&
                device.selectionScore < sharedState.activeScore) {
                return;
            }

            if (sharedState.activeDevice != key) {
                sharedState.activeDevice = key;
                sharedState.activeScore = device.selectionScore;
                sharedState.firstNonNeutralLogged = false;
                selectedNow = true;
            }

            sharedState.state = parsed;
            sharedState.available = true;

            if (!sharedState.firstNonNeutralLogged && IsNonNeutral(parsed)) {
                sharedState.firstNonNeutralLogged = true;
                logFirstInput = true;
            }
        }

        if (selectedNow) {
            Log::writeLine(
                "[RawInput] Selected controller backend device: " +
                device.description);
        }

        if (logFirstInput) {
            Log::writeLine(
                "[RawInput] First non-neutral Xbox-mode HID state: " +
                DescribeParsedState(parsed));
        }
    }

    bool BuildDeviceContext(HANDLE handle, DeviceContext& context)
    {
        context = {};
        context.handle = handle;
        context.path = ReadDevicePath(handle);
        context.rawInfo.cbSize = sizeof(context.rawInfo);

        UINT infoSize = sizeof(context.rawInfo);
        if (GetRawInputDeviceInfoW(
                handle,
                RIDI_DEVICEINFO,
                &context.rawInfo,
                &infoSize) == static_cast<UINT>(-1) ||
            context.rawInfo.dwType != RIM_TYPEHID) {
            return false;
        }

        context.description = DescribeRawDevice(handle, context.rawInfo);

        UINT preparsedSize = 0;
        if (GetRawInputDeviceInfoW(
                handle,
                RIDI_PREPARSEDDATA,
                nullptr,
                &preparsedSize) == static_cast<UINT>(-1) ||
            preparsedSize == 0) {
            return false;
        }

        context.preparsedStorage.resize(preparsedSize);
        if (GetRawInputDeviceInfoW(
                handle,
                RIDI_PREPARSEDDATA,
                context.preparsedStorage.data(),
                &preparsedSize) == static_cast<UINT>(-1)) {
            return false;
        }

        PHIDP_PREPARSED_DATA preparsed =
            reinterpret_cast<PHIDP_PREPARSED_DATA>(context.preparsedStorage.data());
        if (HidP_GetCaps(preparsed, &context.caps) != HIDP_STATUS_SUCCESS)
            return false;

        const bool isGameController =
            context.caps.UsagePage == HID_USAGE_PAGE_GENERIC &&
            (context.caps.Usage == HID_USAGE_GENERIC_GAMEPAD ||
             context.caps.Usage == HID_USAGE_GENERIC_JOYSTICK ||
             context.caps.Usage == HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER);

        if (!isGameController)
            return false;

        USHORT buttonCapCount = context.caps.NumberInputButtonCaps;
        context.buttonCaps.resize(buttonCapCount);
        if (buttonCapCount > 0 &&
            HidP_GetButtonCaps(
                HidP_Input,
                context.buttonCaps.data(),
                &buttonCapCount,
                preparsed) != HIDP_STATUS_SUCCESS) {
            context.buttonCaps.clear();
        }
        else {
            context.buttonCaps.resize(buttonCapCount);
        }

        USHORT valueCapCount = context.caps.NumberInputValueCaps;
        context.valueCaps.resize(valueCapCount);
        if (valueCapCount > 0 &&
            HidP_GetValueCaps(
                HidP_Input,
                context.valueCaps.data(),
                &valueCapCount,
                preparsed) != HIDP_STATUS_SUCCESS) {
            context.valueCaps.clear();
        }
        else {
            context.valueCaps.resize(valueCapCount);
        }

        context.selectionScore =
            context.caps.Usage == HID_USAGE_GENERIC_GAMEPAD ? 100 : 50;

        if (context.rawInfo.hid.dwVendorId == kAsusVendorId)
            context.selectionScore += 50;
        if (context.rawInfo.hid.dwProductId == kAllyControllerProductId)
            context.selectionScore += 100;
        if (context.path.contains("IG_00", Qt::CaseInsensitive))
            context.selectionScore += 25;

        context.usable = !context.valueCaps.empty() || !context.buttonCaps.empty();

        Log::writeLine(
            "[RawInput] Parsed gamepad descriptor " + context.description +
            " inputReportBytes=" + QString::number(context.caps.InputReportByteLength) +
            " buttonCaps=" + QString::number(context.buttonCaps.size()) +
            " valueCaps=" + QString::number(context.valueCaps.size()) +
            " score=" + QString::number(context.selectionScore));

        return context.usable;
    }

    DeviceContext* GetOrCreateDevice(HANDLE handle)
    {
        const auto key = reinterpret_cast<std::uintptr_t>(handle);
        const auto existing = devices.find(key);
        if (existing != devices.end())
            return &existing->second;

        DeviceContext context;
        if (!BuildDeviceContext(handle, context))
            return nullptr;

        auto [inserted, _] = devices.emplace(key, std::move(context));
        return &inserted->second;
    }

    void RemoveDevice(HANDLE handle)
    {
        const auto key = reinterpret_cast<std::uintptr_t>(handle);
        devices.erase(key);

        std::lock_guard<std::mutex> lock(sharedStateMutex);
        if (sharedState.activeDevice == key) {
            sharedState = {};
            sharedState.activeScore = std::numeric_limits<int>::min();
            Log::writeLine("[RawInput] Active controller device was removed.");
        }
    }

    void EnumerateRawInputDevices()
    {
        UINT deviceCount = 0;
        if (GetRawInputDeviceList(
                nullptr,
                &deviceCount,
                sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1)) {
            return;
        }

        std::vector<RAWINPUTDEVICELIST> rawDevices(deviceCount);
        if (deviceCount > 0 &&
            GetRawInputDeviceList(
                rawDevices.data(),
                &deviceCount,
                sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1)) {
            return;
        }

        unsigned parsedControllers = 0;
        for (const RAWINPUTDEVICELIST& rawDevice : rawDevices) {
            if (rawDevice.dwType != RIM_TYPEHID)
                continue;

            if (GetOrCreateDevice(rawDevice.hDevice))
                ++parsedControllers;
        }

        Log::writeLine(
            "[RawInput] Gamepad enumeration complete. Parsed controllers=" +
            QString::number(parsedControllers));
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

        const RAWINPUT* input =
            reinterpret_cast<const RAWINPUT*>(storage.data());
        if (input->header.dwType != RIM_TYPEHID)
            return;

        DeviceContext* device = GetOrCreateDevice(input->header.hDevice);
        if (!device)
            return;

        const ULONG reportLength = input->data.hid.dwSizeHid;
        const ULONG reportCount = input->data.hid.dwCount;
        if (reportLength == 0 || reportCount == 0)
            return;

        for (ULONG reportIndex = 0; reportIndex < reportCount; ++reportIndex) {
            const BYTE* report =
                input->data.hid.bRawData +
                static_cast<size_t>(reportIndex) * reportLength;

            ParsedState parsed;
            ParseButtons(*device, report, reportLength, parsed);
            ParseValues(*device, report, reportLength, parsed);
            PublishState(*device, parsed);
        }
    }

    LRESULT CALLBACK RawInputWindowProcedure(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
    {
        switch (message) {
        case WM_INPUT:
            ProcessRawInput(reinterpret_cast<HRAWINPUT>(lParam));
            return DefWindowProcW(window, message, wParam, lParam);

        case WM_INPUT_DEVICE_CHANGE: {
            HANDLE device = reinterpret_cast<HANDLE>(lParam);
            if (wParam == GIDC_ARRIVAL) {
                GetOrCreateDevice(device);
                Log::writeLine(
                    "[RawInput] Controller device arrived: " +
                    HandleText(device));
            }
            else {
                RemoveDevice(device);
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(window, message, wParam, lParam);
        }
    }

    void RawInputThreadMain()
    {
        const HINSTANCE instance = GetModuleHandleW(nullptr);

        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = RawInputWindowProcedure;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = kRawInputWindowClass;

        if (!RegisterClassW(&windowClass) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            Log::writeLine(
                "[RawInput] RegisterClassW failed. Error=" +
                QString::number(GetLastError()));
            return;
        }

        HWND window = CreateWindowExW(
            0,
            kRawInputWindowClass,
            L"WoWpadX Raw Input Gamepad",
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
                "[RawInput] CreateWindowExW failed. Error=" +
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
                "[RawInput] RegisterRawInputDevices failed. Error=" +
                QString::number(GetLastError()));
            DestroyWindow(window);
            return;
        }

        Log::writeLine(
            "[RawInput] Registered background HID gamepad backend.");
        EnumerateRawInputDevices();

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
        if (rawInputThreadStarted.exchange(true))
            return;

        std::thread(RawInputThreadMain).detach();
    }

    bool TryGetButton(SDL_GamepadButton button, bool& pressed)
    {
        if (button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT)
            return false;

        std::lock_guard<std::mutex> lock(sharedStateMutex);
        if (!sharedState.available || !sharedState.state.buttonKnown[button])
            return false;

        pressed = sharedState.state.buttons[button];
        return true;
    }

    bool TryGetAxis(SDL_GamepadAxis axis, Sint16& value)
    {
        if (axis < 0 || axis >= SDL_GAMEPAD_AXIS_COUNT)
            return false;

        std::lock_guard<std::mutex> lock(sharedStateMutex);
        if (!sharedState.available || !sharedState.state.axisKnown[axis])
            return false;

        value = sharedState.state.axes[axis];
        return true;
    }

    bool HasUsableState()
    {
        std::lock_guard<std::mutex> lock(sharedStateMutex);
        return sharedState.available;
    }
}

static void StartRawInputGamepadAtStartup()
{
    RawInputGamepad::EnsureStarted();
}

Q_COREAPP_STARTUP_FUNCTION(StartRawInputGamepadAtStartup)
