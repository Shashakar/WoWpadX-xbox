// ControllerManager.h

#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include <QObject>
#include <SDL3/SDL_gamepad.h>

#include "Gamepad/gamepad.h"

class QTimer;

class ControllerManager : public QObject {
    Q_OBJECT

public:
    std::atomic<float> m_touchDeltaX{ 0 };
    std::atomic<float> m_touchDeltaY{ 0 };
    std::atomic<bool> m_touchButtonDown{ false };
    std::atomic<bool> m_touchSideLeft{ true };

    static ControllerManager* instance();

    int GetBatteryLevel(SDL_Gamepad* pad);
    QString GetButtonIcon(GamepadBinding button);
    QString GetButtonName(GamepadBinding button);

    void SetController(SDL_Gamepad* controller, SDL_JoystickID instanceId);
    void startWatcher();
    void stopWatcher();
    SDL_Gamepad* getActiveController();
    std::vector<SDL_Gamepad*> getControllers();
    bool Rumble(uint16_t low_freq, uint16_t high_freq, uint32_t duration_ms);

signals:
    void activeControllerChanged();
    void controllersChanged();

private:
    static ControllerManager* m_instance;

    std::mutex controllerMutex;
    std::vector<SDL_Gamepad*> controllers;
    std::vector<SDL_Gamepad*> retiredControllers;
    SDL_Gamepad* activeController = nullptr;
    SDL_JoystickID activeInstanceId = -1;

    std::atomic<bool> threadRunning = false;
    std::thread watcherThread;
    QTimer* eventTimer = nullptr;
    bool sdlInitialized = false;

    int lastBatteryWarn = 100;
    bool showedBatteryLow = false;
    bool showedBatteryCritical = false;

    void PumpEvents();
    void ReconcileControllers();
    void ControllerWatcher();
    void CheckBatteryWarnings();
};
