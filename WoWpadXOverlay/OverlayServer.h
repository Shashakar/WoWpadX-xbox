#pragma once
#include <string>
#include <thread>
#include <atomic>


class OverlayServer {
public:
    static void Start();
    static void Stop();

private:
    static void ListenLoop();
    static void HandleMessage(const std::string& message);

    static inline std::thread listenerThread;
    static inline std::atomic<bool> running = false;
    static inline const std::string pipeName = R"(\\.\pipe\WoWpadX_IPC)";
};