#include "Log.h"

#include <QCoreApplication>
#include <QChar>
#include <QString>

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr wchar_t kRawInputWindowClass[] = L"WoWpadXRawInputDiagnostics";
    constexpr unsigned kMaxLoggedReportChanges = 200;

    std::atomic<bool> rawInputThreadStarted = false;
    std::unordered_map<std::uintptr_t, std::vector<BYTE>> lastReports;
    std::unordered_map<std::uintptr_t, QString> deviceDescriptions;
    unsigned loggedReportChanges = 0;

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

    QString DescribeDevice(HANDLE device)
    {
        const auto key = reinterpret_cast<std::uintptr_t>(device);
        const auto cached = deviceDescriptions.find(key);
        if (cached != deviceDescriptions.end())
            return cached->second;

        RID_DEVICE_INFO info{};
        info.cbSize = sizeof(info);
        UINT infoSize = sizeof(info);

        QString description =
            "handle=" + HandleText(device) +
            " path=\"" + ReadDevicePath(device) + "\"";

        if (GetRawInputDeviceInfoW(
                device,
                RIDI_DEVICEINFO,
                &info,
                &infoSize) != static_cast<UINT>(-1)) {
            description += " type=" + QString::number(info.dwType);

            if (info.dwType == RIM_TYPEHID) {
                description +=
                    " vid=0x" + QString::number(info.hid.dwVendorId, 16) +
                    " pid=0x" + QString::number(info.hid.dwProductId, 16) +
                    " version=0x" + QString::number(info.hid.dwVersionNumber, 16) +
                    " usagePage=0x" + QString::number(info.hid.usUsagePage, 16) +
                    " usage=0x" + QString::number(info.hid.usUsage, 16);
            }
        }

        deviceDescriptions.emplace(key, description);
        return description;
    }

    QString BytesToHex(const BYTE* bytes, size_t byteCount)
    {
        constexpr size_t kMaxBytesPerLog = 96;
        const size_t displayedCount = std::min(byteCount, kMaxBytesPerLog);

        QString output;
        output.reserve(static_cast<int>(displayedCount * 3));

        for (size_t index = 0; index < displayedCount; ++index) {
            if (index > 0)
                output += ' ';

            output += QString("%1")
                .arg(static_cast<unsigned>(bytes[index]), 2, 16, QChar('0'));
        }

        if (displayedCount < byteCount)
            output += " ...";

        return output;
    }

    void EnumerateRawInputDevices()
    {
        UINT deviceCount = 0;
        if (GetRawInputDeviceList(
                nullptr,
                &deviceCount,
                sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1)) {
            Log::writeLine(
                "[RawInput] GetRawInputDeviceList(count) failed. Error=" +
                QString::number(GetLastError()));
            return;
        }

        std::vector<RAWINPUTDEVICELIST> devices(deviceCount);
        if (deviceCount > 0 &&
            GetRawInputDeviceList(
                devices.data(),
                &deviceCount,
                sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1)) {
            Log::writeLine(
                "[RawInput] GetRawInputDeviceList(data) failed. Error=" +
                QString::number(GetLastError()));
            return;
        }

        unsigned hidCount = 0;
        for (const RAWINPUTDEVICELIST& device : devices) {
            if (device.dwType != RIM_TYPEHID)
                continue;

            ++hidCount;
            Log::writeLine("[RawInput] Enumerated HID " + DescribeDevice(device.hDevice));
        }

        Log::writeLine(
            "[RawInput] Enumeration complete. HID devices=" +
            QString::number(hidCount));
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

        const size_t reportBytes =
            static_cast<size_t>(input->data.hid.dwSizeHid) *
            static_cast<size_t>(input->data.hid.dwCount);
        if (reportBytes == 0)
            return;

        const BYTE* reports = input->data.hid.bRawData;
        const auto key = reinterpret_cast<std::uintptr_t>(input->header.hDevice);
        std::vector<BYTE> current(reports, reports + reportBytes);

        const auto previous = lastReports.find(key);
        if (previous != lastReports.end() && previous->second == current)
            return;

        lastReports[key] = current;

        if (loggedReportChanges >= kMaxLoggedReportChanges)
            return;

        ++loggedReportChanges;
        Log::writeLine(
            "[RawInput] HID report change " +
            DescribeDevice(input->header.hDevice) +
            " reportSize=" + QString::number(input->data.hid.dwSizeHid) +
            " reportCount=" + QString::number(input->data.hid.dwCount) +
            " bytes=[" + BytesToHex(reports, reportBytes) + "]");
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
            deviceDescriptions.erase(reinterpret_cast<std::uintptr_t>(device));
            lastReports.erase(reinterpret_cast<std::uintptr_t>(device));

            Log::writeLine(
                QString("[RawInput] Device %1: ")
                    .arg(wParam == GIDC_ARRIVAL ? "arrived" : "removed") +
                DescribeDevice(device));
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
            L"WoWpadX Raw Input Diagnostics",
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
            { 0x01, 0x04, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window }, // Joystick
            { 0x01, 0x05, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window }, // Game pad
            { 0x01, 0x08, RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, window }, // Multi-axis controller
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
            "[RawInput] Registered joystick/gamepad HID input sink. "
            "Changed reports will be logged before and after the Xbox/Desktop transition.");
        EnumerateRawInputDevices();

        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    void StartRawInputDiagnostics()
    {
        if (rawInputThreadStarted.exchange(true))
            return;

        std::thread(RawInputThreadMain).detach();
    }
}

Q_COREAPP_STARTUP_FUNCTION(StartRawInputDiagnostics)
