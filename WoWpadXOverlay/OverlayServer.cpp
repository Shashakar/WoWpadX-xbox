#include "OverlayServer.h"
#include "Hooks.h"
#include <windows.h>
#include <sstream>
#include <iostream>

void OverlayServer::Start() {
    if (running) return;
    running = true;
    listenerThread = std::thread(ListenLoop);
}

void OverlayServer::Stop() {
    running = false;
    if (listenerThread.joinable()) listenerThread.join();
}

void OverlayServer::HandleMessage(const std::string& message) {
    std::istringstream iss(message);
    std::string type;
    std::getline(iss, type, '|');

    if (type == "NOTIFY") {
        std::string idStr, imgStr, durStr, title, content;
        std::getline(iss, idStr, '|');
        std::getline(iss, imgStr, '|');
        std::getline(iss, durStr, '|');
        std::getline(iss, title, '|');
        std::getline(iss, content);

        int id = std::stoi(idStr);
        int img = std::stoi(imgStr);
        int dur = std::stoi(durStr);

        //std::cout << "[Toast] ID: " << id << ", Title: " << title << ", Content: " << content << ", Image: " << img << ", Duration: " << dur << "\n";

        if (Hooks::InitImGui)
        {
            Hooks::SendNotification(title, content, id, dur, img);
        }

    }
    else if (type == "CROSSHAIR") {
        std::string enabledStr, xStr, yStr;
        std::getline(iss, enabledStr, '|');
        std::getline(iss, xStr, '|');
        std::getline(iss, yStr);

        bool enabled = (enabledStr == "1");
        int x = std::stoi(xStr);
        int y = std::stoi(yStr);

        // std::cout << "[Crosshair] Enabled: " << enabled << ", X: " << x << ", Y: " << y << "\n";
        Hooks::renderCrosshair = enabled;
        Hooks::crossX = x;
        Hooks::crossY = y;
    }
    else {
        std::cerr << "Unknown message type: " << type << "\n";
    }
}

void OverlayServer::ListenLoop() {
    while (running) {
        HANDLE pipe = CreateNamedPipeA(
            pipeName.c_str(),
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1, // Only one instance
            1024, 1024,
            0,
            nullptr
        );

        if (pipe == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create pipe.\n";
            Sleep(1000);
            continue;
        }

        if (!ConnectNamedPipe(pipe, nullptr)) {
            CloseHandle(pipe);
            continue;
        }

        char buffer[1024];
        DWORD bytesRead = 0;

        while (running) {
            BOOL success = ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
            if (!success || bytesRead == 0) {
                DWORD err = GetLastError();
                if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA) {
                    // Client disconnected
                    break;
                }

                Sleep(1); // avoid spinning
                continue;
            }

            buffer[bytesRead] = '\0';
            HandleMessage(std::string(buffer));
        }

        // Clean up pipe and prepare for next connection
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}
