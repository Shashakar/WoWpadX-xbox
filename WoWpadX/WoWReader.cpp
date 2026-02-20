#include "WoWReader.h"
#include "Log.h"

HWND WoWReader::windowHandle = nullptr;
std::atomic<uint32_t> WoWReader::atomicColor(0);
std::atomic<bool> WoWReader::synced(false);
std::atomic<bool> WoWReader::focused(false);
POINT WoWReader::beaconOffset = { 0, 0 };
POINT WoWReader::screenCenter = { 0, 0 };
POINT WoWReader::beaconCheckPoint = { 0, 0 };
std::thread WoWReader::workerThread;
std::atomic<bool> WoWReader::threadRunning(false);
std::chrono::steady_clock::time_point WoWReader::lastSeenBeacon = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point WoWReader::lastHandleCheck = std::chrono::steady_clock::now();

bool WoWReader::init(HWND wowHwnd) {
    if (!wowHwnd || !IsWindow(wowHwnd)) return false;

    // If the thread is already running, just update the handle
    if (threadRunning) {
        windowHandle = wowHwnd;
        synced = false;
        return true;
    }

    windowHandle = wowHwnd;
    start();
    return true;
}

void WoWReader::close() {
    stop();
    windowHandle = nullptr;
    synced = false;
    focused = false;
}

void WoWReader::start() {
    if (threadRunning) return;
    threadRunning = true;
    workerThread = std::thread(&WoWReader::threadLoop);
}

void WoWReader::stop() {
    threadRunning = false;
    if (workerThread.joinable()) workerThread.join();
}

void WoWReader::updateWindowMetrics() {
    auto now = std::chrono::steady_clock::now();

    // 1. Validate the handle every 2 seconds or if it becomes invalid
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastHandleCheck).count() >= 2) {
        lastHandleCheck = now;
        if (!windowHandle || !IsWindow(windowHandle)) {
            synced = false;
            focused = false;
            // Handle is dead; the thread remains running but waits for ProcessManager
            // to call WoWReader::init again with the new HWND.
            return;
        }
    }

    // Cache focus state
    bool currentlyFocused = (GetForegroundWindow() == windowHandle);
    focused.store(currentlyFocused);

    // Cache center coordinates only if focused to save resources
    if (currentlyFocused || screenCenter.x == 0){
        RECT rect;
        if (GetClientRect(windowHandle, &rect)) {
            POINT pt = { rect.right / 2, rect.bottom / 2 };

            if (IsHungAppWindow(windowHandle)) {
                synced = false; // Immediately drop sync if game hangs
                return;
            }

            ClientToScreen(windowHandle, &pt);
            screenCenter = pt;
        }
    }
}

void WoWReader::threadLoop() {
    auto lastCalibrationTry = std::chrono::steady_clock::now();

    while (threadRunning) {
        if (!windowHandle || !IsWindow(windowHandle)) {
            synced = false;
            focused = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        updateWindowMetrics();
        auto now = std::chrono::steady_clock::now();

        // 1. If focused, try to sync or sample
        if (focused.load()) {
            if (!synced.load()) {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCalibrationTry).count() >= 250) {
                    lastCalibrationTry = now;
                    if (findBeacon()) {
                        synced = true;
                        Log::writeLine("Pixel Bridge: Synced with client.");
                        lastSeenBeacon = now;
                    }
                }
            }
            else {
                sampleData();
                lastSeenBeacon = now;
            }
        }

        // 2. GLOBAL TIMEOUT (This runs even when Alt-Tabbed!)
        // If we haven't successfully sampled data in 3 seconds, drop sync.
        if (synced.load()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastSeenBeacon).count();
            if (elapsed >= 3) {
                synced = false;
                atomicColor.store(0);
                Log::writeLine("Pixel Bridge: Sync lost (timeout).");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

bool WoWReader::findBeacon() {
    if (!windowHandle || !IsWindow(windowHandle) || IsHungAppWindow(windowHandle)) return false;

    HDC hScreen = GetDC(windowHandle);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, 100, 100);
    HGDIOBJ oldObj = SelectObject(hDC, hBitmap);

    BitBlt(hDC, 0, 0, 100, 100, hScreen, 0, 0, SRCCOPY);

    bool found = false;
    for (int y = 0; y < 100 && !found; y++) {
        for (int x = 0; x < 100; x++) {
            COLORREF check = GetPixel(hDC, x, y);

            // 1. We found the START of the Beacon (Magenta)
            if (GetRValue(check) > 240 && GetGValue(check) < 20 && GetBValue(check) > 240) {
                int walkX = x;

                // 2. WALK RIGHT through the magenta until it ends
                // This bypasses any size issues caused by Windowed vs Fullscreen scaling
                while (walkX < 98) {
                    COLORREF walkColor = GetPixel(hDC, walkX, y);
                    bool stillMagenta = (GetRValue(walkColor) > 200 && GetBValue(walkColor) > 200);

                    if (!stillMagenta) {
                        // 3. We are now at the FIRST pixel of the Data block.
                        // We add +1 or +2 just to be deep inside the data block for safety.
                        beaconOffset.x = walkX + 1;
                        beaconOffset.y = y;

                        // We also need a point to check the Beacon state in sampleData.
                        // We use the 'x' coordinate where we first found Magenta.
                        beaconCheckPoint.x = x;

                        found = true;
                        break;
                    }
                    walkX++;
                }
                if (found) break;
            }
        }
    }

    SelectObject(hDC, oldObj);
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(windowHandle, hScreen);
    return found;
}

void WoWReader::sampleData() {
    if (IsHungAppWindow(windowHandle)) return;

    HDC hScreen = GetDC(windowHandle);
    HDC hDC = CreateCompatibleDC(hScreen);

    // Snapping a small area that encompasses the start of magenta and the data pixel
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, 40, 4);
    HGDIOBJ oldObj = SelectObject(hDC, hBitmap);

    // Capture starting at our beaconCheckPoint
    BitBlt(hDC, 0, 0, 40, 4, hScreen, beaconCheckPoint.x, beaconOffset.y, SRCCOPY);

    // 1. Verify Beacon (Relative x=0 because we started the capture there)
    COLORREF bc = GetPixel(hDC, 0, 0);
    bool isMagenta = (GetRValue(bc) > 200 && GetBValue(bc) > 200);
    bool isBlack = (GetRValue(bc) < 20 && GetGValue(bc) < 20 && GetBValue(bc) < 20);

    if (isMagenta || isBlack) {
        lastSeenBeacon = std::chrono::steady_clock::now();

        // 2. Read Data (Relative x is the distance from start of magenta to data)
        int relativeDataX = beaconOffset.x - beaconCheckPoint.x;
        COLORREF freshColor = GetPixel(hDC, relativeDataX, 0);
        atomicColor.store((uint32_t)freshColor);
    }

    SelectObject(hDC, oldObj);
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(windowHandle, hScreen);
}

// Lock-free interpreters for the InputWatcher
bool WoWReader::readGameState() { return synced.load() && (atomicColor.load() != 0); }
bool WoWReader::readMouselook() { return (GetGValue(atomicColor.load()) & 2); }
int  WoWReader::readMovementState() { return (GetGValue(atomicColor.load()) & 1) ? 1 : 0; }
bool WoWReader::readAoeState() { return GetBValue(atomicColor.load()) > 200; }
std::pair<long, long> WoWReader::readPlayerHealth() {
    uint32_t val = atomicColor.load();
    return { (long)GetRValue(val), 255 };
}