#include "Log.h"

#include <QCoreApplication>
#include <QString>

#include <Windows.h>
#include <hidsdi.h>
#include <setupapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
    constexpr USHORT kAsusVendorId = 0x0B05;
    constexpr USHORT kAllyControllerProductId = 0x1B4C;
    constexpr unsigned kMaximumLoggedChangesPerInterface = 200;

    std::atomic<bool> probeStarted = false;

    QString HexBytes(const std::vector<BYTE>& bytes, DWORD count)
    {
        std::ostringstream stream;
        stream << std::hex << std::setfill('0');
        const DWORD capped = std::min<DWORD>(count, 64);
        for (DWORD index = 0; index < capped; ++index) {
            if (index > 0)
                stream << ' ';
            stream << std::setw(2) << static_cast<unsigned>(bytes[index]);
        }
        if (count > capped)
            stream << " ...";
        return QString::fromStdString(stream.str());
    }

    QString InterfaceLabel(const QString& path, const HIDP_CAPS& caps)
    {
        return QString(
            "path=\"%1\" usagePage=0x%2 usage=0x%3 "
            "inputBytes=%4 outputBytes=%5 featureBytes=%6")
            .arg(path)
            .arg(caps.UsagePage, 0, 16)
            .arg(caps.Usage, 0, 16)
            .arg(caps.InputReportByteLength)
            .arg(caps.OutputReportByteLength)
            .arg(caps.FeatureReportByteLength);
    }

    bool ReadCaps(HANDLE handle, HIDP_CAPS& caps)
    {
        PHIDP_PREPARSED_DATA preparsed = nullptr;
        if (!HidD_GetPreparsedData(handle, &preparsed) || !preparsed)
            return false;

        const NTSTATUS status = HidP_GetCaps(preparsed, &caps);
        HidD_FreePreparsedData(preparsed);
        return status == HIDP_STATUS_SUCCESS;
    }

    void ReaderThread(QString path, HIDP_CAPS caps)
    {
        HANDLE handle = CreateFileW(
            reinterpret_cast<LPCWSTR>(path.utf16()),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

        if (handle == INVALID_HANDLE_VALUE) {
            Log::writeLine(
                "[NativeHidProbe] Read open failed error=" +
                QString::number(GetLastError()) + " " +
                InterfaceLabel(path, caps));
            return;
        }

        Log::writeLine(
            "[NativeHidProbe] Reading " + InterfaceLabel(path, caps));

        const DWORD reportLength = std::max<DWORD>(
            1, static_cast<DWORD>(caps.InputReportByteLength));
        std::vector<BYTE> report(reportLength);
        std::vector<BYTE> previous;
        unsigned loggedChanges = 0;
        auto lastLog = std::chrono::steady_clock::time_point{};

        while (loggedChanges < kMaximumLoggedChangesPerInterface) {
            DWORD bytesRead = 0;
            std::fill(report.begin(), report.end(), 0);
            if (!ReadFile(handle, report.data(), reportLength,
                    &bytesRead, nullptr)) {
                Log::writeLine(
                    "[NativeHidProbe] Read failed error=" +
                    QString::number(GetLastError()) + " path=\"" +
                    path + "\"");
                break;
            }

            if (bytesRead == 0 || report == previous)
                continue;

            const auto now = std::chrono::steady_clock::now();
            const bool nativeReportCandidate = report[0] == 0x0B;
            const bool enoughTimePassed =
                lastLog.time_since_epoch().count() == 0 ||
                now - lastLog >= std::chrono::milliseconds(40);

            if (nativeReportCandidate || enoughTimePassed) {
                Log::writeLine(
                    QString(
                        "[NativeHidProbe] reportId=0x%1 bytesRead=%2 "
                        "path=\"%3\" bytes=[%4]")
                        .arg(report[0], 0, 16)
                        .arg(bytesRead)
                        .arg(path)
                        .arg(HexBytes(report, bytesRead)));
                ++loggedChanges;
                lastLog = now;
            }

            previous.assign(report.begin(), report.begin() + bytesRead);
        }

        CloseHandle(handle);
        Log::writeLine(
            "[NativeHidProbe] Reader stopped path=\"" + path + "\"");
    }

    void ProbeThread()
    {
        GUID hidGuid{};
        HidD_GetHidGuid(&hidGuid);

        HDEVINFO deviceInfo = SetupDiGetClassDevsW(
            &hidGuid,
            nullptr,
            nullptr,
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (deviceInfo == INVALID_HANDLE_VALUE) {
            Log::writeLine(
                "[NativeHidProbe] SetupDiGetClassDevs failed error=" +
                QString::number(GetLastError()));
            return;
        }

        unsigned matchingInterfaces = 0;
        for (DWORD index = 0;; ++index) {
            SP_DEVICE_INTERFACE_DATA interfaceData{};
            interfaceData.cbSize = sizeof(interfaceData);
            if (!SetupDiEnumDeviceInterfaces(
                    deviceInfo,
                    nullptr,
                    &hidGuid,
                    index,
                    &interfaceData)) {
                if (GetLastError() != ERROR_NO_MORE_ITEMS) {
                    Log::writeLine(
                        "[NativeHidProbe] SetupDiEnumDeviceInterfaces failed error=" +
                        QString::number(GetLastError()));
                }
                break;
            }

            DWORD requiredSize = 0;
            SetupDiGetDeviceInterfaceDetailW(
                deviceInfo,
                &interfaceData,
                nullptr,
                0,
                &requiredSize,
                nullptr);
            if (requiredSize == 0)
                continue;

            std::vector<BYTE> detailStorage(requiredSize);
            auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(
                detailStorage.data());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(
                    deviceInfo,
                    &interfaceData,
                    detail,
                    requiredSize,
                    nullptr,
                    nullptr)) {
                continue;
            }

            const QString path = QString::fromWCharArray(detail->DevicePath);
            HANDLE metadataHandle = CreateFileW(
                detail->DevicePath,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);
            if (metadataHandle == INVALID_HANDLE_VALUE)
                continue;

            HIDD_ATTRIBUTES attributes{};
            attributes.Size = sizeof(attributes);
            const bool attributesAvailable =
                HidD_GetAttributes(metadataHandle, &attributes) != FALSE;
            if (!attributesAvailable ||
                attributes.VendorID != kAsusVendorId ||
                attributes.ProductID != kAllyControllerProductId) {
                CloseHandle(metadataHandle);
                continue;
            }

            HIDP_CAPS caps{};
            const bool capsAvailable = ReadCaps(metadataHandle, caps);
            CloseHandle(metadataHandle);
            if (!capsAvailable)
                continue;

            ++matchingInterfaces;
            Log::writeLine(
                QString(
                    "[NativeHidProbe] Found Ally HID interface #%1 %2")
                    .arg(matchingInterfaces)
                    .arg(InterfaceLabel(path, caps)));

            if (caps.InputReportByteLength > 0)
                std::thread(ReaderThread, path, caps).detach();
        }

        SetupDiDestroyDeviceInfoList(deviceInfo);
        Log::writeLine(
            "[NativeHidProbe] Enumeration complete. Matching interfaces=" +
            QString::number(matchingInterfaces));
    }
}

static void StartNativeHidProbe()
{
    if (probeStarted.exchange(true))
        return;
    std::thread(ProbeThread).detach();
}

Q_COREAPP_STARTUP_FUNCTION(StartNativeHidProbe)
