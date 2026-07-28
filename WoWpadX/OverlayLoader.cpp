#include <windows.h> 
#include <iostream>
#include <vector>
#include "OverlayLoader.h"
#include <thread>
#include <qstring.h>
#include "Log.h"
#include <qfileinfo.h>
#include <qdir.h>
#include <QCoreApplication>

static const char opp[] = "BpgbX}`ijq\x7f\0";           // OpenProcess
static const char crt[] = "Nrgm|j]obmxf_axmld\0";       // CreateRemoteThread
static const char wpm[] = "Zrkxm_}elg\x7fpFlgg\x7fy\0"; // WriteProcessMemory
static const char vaex[] = "[ipx}ncKcnc`Nq\0";          // VirtualAllocEx
static const char vfex[] = "[ipx}ncL}giFs\0";           // VirtualFreeEx
static const char ldlba[] = "AochDfmxnpuB\0";           // LoadLibraryA
static uint64_t uKey = 0x8A9B3C2FAFF8C20DULL;

static void deobfuscate_inplace(char* buf, uint64_t key)
{
    uint8_t keyBytes[8];
    for (int i = 0; i < 8; ++i)
        keyBytes[i] = static_cast<uint8_t>((key >> (8 * i)) & 0xFF);

    int cycle = 0; // cycles every 16 bytes
    int idx = 0;

    while (buf[idx] != '\0') {
        int pos = cycle / 2;            // which key byte to use
        bool even = (cycle % 2) == 0;   // which half-byte to use

        uint8_t kb = keyBytes[pos % 8];
        uint8_t half = even ? (kb & 0x0F) : ((kb >> 4) & 0x0F);

        buf[idx] ^= half;

        ++idx;
        ++cycle;
        if (cycle >= 16) cycle = 0; // restart after 16 bytes
    }
}

std::string resolve_obf_string(const char* obf, uint64_t key)
{
    if (!obf)
        return "";

    char tmp[256];
    size_t len = strlen(obf);
    if (len >= sizeof(tmp))
        return "";

    memcpy(tmp, obf, len + 1);
    deobfuscate_inplace(tmp, key);

    return tmp;
}


bool OverlayLoader::Load(DWORD pid, const std::string& dllPath) {
    Log::writeLine("Preparing to launch standalone Overlay Loader...");

    // 1. Define the Loader path (Application folder instead of Temp)
    QString loaderPath = QCoreApplication::applicationDirPath() + "/WoWpadXOverlayLoader.exe";

    // 2. Validate that the Loader exists
    if (!QFile::exists(loaderPath)) {
        Log::writeLine("Overlay Loader not found at: " + loaderPath);
        return false;
    }
     
    QFileInfo dllInfo(QString::fromStdString(dllPath));
    if (!dllInfo.exists()) {
        Log::writeLine("The target DLL does not exist: " + QString::fromStdString(dllPath));
        return false;
    }
    QString actualDllPath = dllInfo.absoluteFilePath(); 

    QString args = QString("--pid %1 --ld \"%2\"").arg(pid).arg(actualDllPath);
    QString fullCmd = QString("\"%1\" %2").arg(loaderPath, args);
     
    std::wstring fullCmdStr = fullCmd.toStdWString();
    LPWSTR cmdLine = &fullCmdStr[0];

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
 
    if (!CreateProcessW(nullptr, cmdLine, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        Log::writeLine("Failed to launch standalone loader. Error: " + QString::number(GetLastError()));
        return false;
    }
     
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        Log::writeLine("Failed to retrieve loader exit code.");
    }
    else {
        QString returnText = "Overlay loader finished (Code 0x%1): %2";

        switch (exitCode) {
        case 0:    Log::writeLine("Overlay injection successful."); break;
        case 0x8:  Log::writeLine(returnText.arg("8", "Incorrect argument count")); break;
        case 0xC:  Log::writeLine(returnText.arg("C", "Invalid arguments")); break;
        case 0x10: Log::writeLine(returnText.arg("10", QString("%1 failed").arg(resolve_obf_string(opp, uKey).c_str()))); break;
        case 0x20: Log::writeLine(returnText.arg("20", QString("%1 failed").arg(resolve_obf_string(vaex, uKey).c_str()))); break;
        case 0x30: Log::writeLine(returnText.arg("30", QString("%1 failed").arg(resolve_obf_string(wpm, uKey).c_str()))); break;
        case 0x40: Log::writeLine(returnText.arg("40", "GetProcAddress failed")); break;
        case 0x50: Log::writeLine(returnText.arg("50", QString("%1 failed").arg(resolve_obf_string(crt, uKey).c_str()))); break;
        default:   Log::writeLine("Loader exited with unknown code: " + QString::number(exitCode, 16));
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
     
    return (exitCode == 0);
}
