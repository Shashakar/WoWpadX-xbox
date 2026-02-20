#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <iostream>
#include <cstring>
#include <vector>

//OpenProcess
typedef HANDLE(WINAPI* o_proc)(DWORD, BOOL, DWORD);
//CreateRemoteThread
typedef HANDLE(WINAPI* c_r_t)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID,
    DWORD, LPDWORD);
//WriteProcessMemory
typedef BOOL(WINAPI* w_p_m)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
//VirtualAllocEx
typedef LPVOID(WINAPI* v_a_ex)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
//VirtualFreeEx
typedef BOOL(WINAPI* v_f_ex)(HANDLE, LPVOID, SIZE_T, DWORD);

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

FARPROC resolve_obf_function(HMODULE module, const char* obfName, uint64_t key)
{
    if (!module || !obfName)
        return nullptr;

    char tmp[256];
    size_t len = strlen(obfName);
    if (len >= sizeof(tmp))
        return nullptr; // name too long

    memcpy(tmp, obfName, len + 1);
    deobfuscate_inplace(tmp, key);

    return GetProcAddress(module, tmp);
}

bool LoadOverlay(DWORD processID, const std::wstring& dllPath, DWORD& errorCode) {
    static const char opp[] = "Orhg\\zlig\x7fw\0"; //OpenProcess
    static const char crt[] = "CphhxmQoocpj[c|caf\0"; // CreateRemoteThread
    static const char wpm[] = "Wpd}iXqeaiw|Bncir{\0"; //WriteProcessMemory
    static const char vaex[] = "Vk\x7f}yioKn`klJs\0"; //VirtualAllocEx
    static const char vfex[] = "Vk\x7f}yioLpiaJw\0"; //VirtualFreeEx
    static const char ldlbw[] = "Lmlm@aaxc~}X\0"; //LoadLibraryW
    uint64_t uKey = 0x6EBFF4C2A38C9D20ULL;

    HMODULE kern = GetModuleHandleW(L"kernel32.dll");

    auto openProc = reinterpret_cast<o_proc>(resolve_obf_function(kern, opp, uKey));
    auto createRemThread = reinterpret_cast<c_r_t>(resolve_obf_function(kern, crt, uKey));
    auto writeProcMem = reinterpret_cast<w_p_m>(resolve_obf_function(kern, wpm, uKey));
    auto virtAllocEx = reinterpret_cast<v_a_ex>(resolve_obf_function(kern, vaex, uKey));
    auto virtFreeEx = reinterpret_cast<v_f_ex>(resolve_obf_function(kern, vfex, uKey));
    auto ldLibraryW = resolve_obf_function(kern, ldlbw, uKey);

    if (!openProc || !createRemThread || !writeProcMem || !virtAllocEx || !virtFreeEx || !ldLibraryW) { 
        std::wcerr << L"Failed to get necessary functions: " << GetLastError() << std::endl;
        errorCode = 0x40;
        return false;
    }


    HANDLE hProcess = openProc(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess) {
        std::wcerr << L"Failed to open process: " << GetLastError() << std::endl;
        errorCode = 0x10;
        return false;
    }

    // Allocate space for DLL path in target process
    LPVOID allocMem = virtAllocEx(hProcess, nullptr, (dllPath.size() + 1) * sizeof(wchar_t),
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocMem) {
        std::wcerr << L"Failed to allocate library path in target process: " << GetLastError() << std::endl;
        errorCode = 0x20;
        CloseHandle(hProcess);
        return false;
    }

    // Write DLL path into target process memory
    if (!writeProcMem(hProcess, allocMem, dllPath.c_str(),
        (dllPath.size() + 1) * sizeof(wchar_t), nullptr)) {
        std::wcerr << L"Failed to write into the process memory: " << GetLastError() << std::endl;
        errorCode = 0x30;
        virtFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Create remote thread to load DLL
    HANDLE hThread = createRemThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)ldLibraryW, allocMem, 0, nullptr);
    if (!hThread) {
        std::wcerr << L"Failed to create a remote thread: " << GetLastError() << std::endl;
        errorCode = 0x50;
        virtFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait until the DLL is loaded
    WaitForSingleObject(hThread, INFINITE);

    // Cleanup
    virtFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return true;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 5) {
        std::wcout << L"Usage: WoWpadXOverlayLoader.exe --pid <ProcessID> --ld <DLL Path>\n";
        return 0x8;
    }

    DWORD pid = 0;
    std::wstring dllPath;

    for (int i = 1; i < argc; i++) {
        if (_wcsicmp(argv[i], L"--pid") == 0 && i + 1 < argc) {
            pid = _wtoi(argv[++i]);
        }
        else if (_wcsicmp(argv[i], L"--ld") == 0 && i + 1 < argc) {
            dllPath = argv[++i];
        }
    }

    if (pid == 0 || dllPath.empty()) {
        std::wcerr << L"Invalid arguments.\n";

        return 0xC;
    }

    // If DLL path is just a filename, build full path in same directory as EXE
    if (dllPath.find(L"\\") == std::wstring::npos && dllPath.find(L"/") == std::wstring::npos) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);

        wchar_t* lastSlash = wcsrchr(exePath, L'\\');
        if (lastSlash) *(lastSlash + 1) = L'\0';

        dllPath = std::wstring(exePath) + dllPath;
    }
    DWORD errorCode = 0;

    if (LoadOverlay(pid, dllPath, errorCode)) {
        std::wcout << L"Successfully loaded: " << dllPath << L"\n";
        return 0;
    }
    else {
        std::wcerr << L"Failed to load.\n";
        return errorCode;
    }
}
