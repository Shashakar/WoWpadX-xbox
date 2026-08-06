#include "Log.h"

#include <QCoreApplication>
#include <QString>

#include <Windows.h>
#include <hidsdi.h>
#include <setupapi.h>

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <thread>
#include <vector>

namespace
{
    constexpr USHORT kAsusVendorId = 0x0B05;
    constexpr USHORT kAllyControllerProductId = 0x1B4C;
    constexpr unsigned kMaximumLoggedReports = 500;

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

    bool IsPaddleCandidate(const QString& path, const HIDP_CAPS& caps)
    {
        return path.contains("mi_04", Qt::CaseInsensitive) &&
            caps.UsagePage == 0xFF82 && caps.Usage == 0x00CF &&
            caps.InputReportByteLength == 17;
    }

    void PaddleReaderThread(QString path, HIDP_CAPS caps)
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
                "[PaddleProbe] Read open failed error=" +
                QString::number(GetLastError()) + " " +
                InterfaceLabel(path, caps));
            return;
        }

        Log::writeLine(
            "[PaddleProbe] Reading ASUS MI_04 paddle candidate " +
            InterfaceLabel(path, caps));

        const DWORD reportLength = std::max<DWORD>(
            1, static_cast<DWORD>(caps.InputReportByteLength));
        std::vector<BYTE> report(reportLength);
        std::vector<BYTE> previous;
        unsigned loggedReports = 0;

        while (loggedReports < kMaximumLoggedReports) {
            DWORD bytesRead = 0;
            std::fill(report.begin(), report.end(), 0);
            if (!ReadFile(handle, report.data(), reportLength,
                    &bytesRead, nullptr)) {
                Log::writeLine(
                    "[PaddleProbe] Read failed error=" +
                    QString::number(GetLastError()) + " path=\"" +
                    path + "\"");
                break;
            }

            if (bytesRead == 0)
                continue;

            const std::vector<BYTE> current(
                report.begin(), report.begin() + bytesRead);
            if (current == previous)
                continue;

            Log::writeLine(
                QString(
                    "[PaddleProbe] change=%1 reportId=0x%2 bytesRead=%3 "
                    "bytes=[%4]")
                    .arg(loggedReports + 1)
                    .arg(report[0], 0, 16)
                    .arg(bytesRead)
                    .arg(HexBytes(report, bytesRead)));

            previous = current;
            ++loggedReports;
        }

        CloseHandle(handle);
        Log::writeLine("[PaddleProbe] Reader stopped.");
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
                "[PaddleProbe] SetupDiGetClassDevs failed error=" +
                QString::number(GetLastError()));
            return;
        }

        unsigned matchingInterfaces = 0;
        unsigned paddleCandidates = 0;
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
                        "[PaddleProbe] SetupDiEnumDeviceInterfaces failed error=" +
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
            if (!IsPaddleCandidate(path, caps))
                continue;

            ++paddleCandidates;
            Log::writeLine(
                QString("[PaddleProbe] Found candidate #%1 %2")
                    .arg(paddleCandidates)
                    .arg(InterfaceLabel(path, caps)));
            std::thread(PaddleReaderThread, path, caps).detach();
        }

        SetupDiDestroyDeviceInfoList(deviceInfo);
        Log::writeLine(
            QString(
                "[PaddleProbe] Enumeration complete. ASUS interfaces=%1 "
                "paddleCandidates=%2")
                .arg(matchingInterfaces)
                .arg(paddleCandidates));
    }
}

static void StartPaddleProbe()
{
    if (probeStarted.exchange(true))
        return;
    std::thread(ProbeThread).detach();
}

Q_COREAPP_STARTUP_FUNCTION(StartPaddleProbe)
