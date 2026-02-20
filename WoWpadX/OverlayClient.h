#pragma once
#include <string>
#include <windows.h>
#include <mutex>

class OverlayClient {
public:
    // Static interface methods
    static bool SendNotification(const std::string& title, const std::string& content, int uniqueID = -1, int image = -1, int duration = 5000);
    static bool SetCrosshairState(bool enabled, int x = 0, int y = 0);


private:
    // Internal static helpers
    static bool Connect();
    static void Disconnect();
    static bool EnsureConnected();
    static bool SendMessage(const std::string& message);

    // Static members

    static inline HANDLE pipeHandle = INVALID_HANDLE_VALUE;
    static inline const std::string pipeName = R"(\\.\pipe\WoWpadX_IPC)";
    static inline std::mutex pipeMutex;
};