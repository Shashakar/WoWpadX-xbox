#include "OverlayClient.h"
#include <iostream>
#include <sstream>

bool OverlayClient::Connect() {
    pipeHandle = CreateFileA(
        pipeName.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    return pipeHandle != INVALID_HANDLE_VALUE;
}

void OverlayClient::Disconnect() {
    if (pipeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(pipeHandle);
        pipeHandle = INVALID_HANDLE_VALUE;
    }
}

bool OverlayClient::EnsureConnected() {
    if (pipeHandle == INVALID_HANDLE_VALUE) {
        return Connect();
    }
    return true;
}


bool OverlayClient::SendMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(pipeMutex);

    if (!EnsureConnected()) return false;

    DWORD bytesWritten;
    BOOL success = WriteFile(
        pipeHandle,
        message.c_str(),
        static_cast<DWORD>(message.size()),
        &bytesWritten,
        nullptr
    );

    if (!success || bytesWritten != message.size()) {
        Disconnect(); // Pipe might be broken
        return false;
    }

    return true;
}

bool OverlayClient::SendNotification(const std::string& title, const std::string& content, int uniqueID, int image, int duration) {
    std::ostringstream oss;
    oss << "NOTIFY|" << uniqueID << "|" << image << "|" << duration << "|" << title << "|" << content;
    return SendMessage(oss.str());
}

bool OverlayClient::SetCrosshairState(bool enabled, int x, int y) {
    std::ostringstream oss;
    oss << "CROSSHAIR|" << (enabled ? "1" : "0") << "|" << x << "|" << y;
    return SendMessage(oss.str());
}
