#include "traymanager.h"
#include <QGuiApplication>
#include <QFile>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDebug>

TrayManager::TrayManager(QObject* parent) : QObject(parent) {
    qApp->installNativeEventFilter(this);
}

void TrayManager::setMainWindow(QWindow* window) {
    mainWindow = window;
    hwnd = (HWND)window->winId();
    createTrayIcon();
}

void TrayManager::minimizeToTray() {
    if (mainWindow) {
        mainWindow->hide();
        showTrayMessage("Minimized", "WoWpadX is running in the system tray.");
    }
}

static HICON qtImageToHICON(const QImage& image) {
    int width = image.width();
    int height = image.height();

    // Create a bitmap header
    BITMAPV5HEADER bi;
    ZeroMemory(&bi, sizeof(BITMAPV5HEADER));
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = width;
    bi.bV5Height = -height; // Negative for top-down DIB
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    // Create DIB section
    void* bits = nullptr;
    HDC hdc = GetDC(nullptr);
    HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdc);

    if (!hBitmap || !bits) return nullptr;

    // Copy image data
    memcpy(bits, image.bits(), width * height * 4);

    // Create mask bitmap (not used, but required)
    HBITMAP hMonoMask = CreateBitmap(width, height, 1, 1, nullptr);

    // Create icon
    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = hBitmap;
    iconInfo.hbmMask = hMonoMask;

    HICON hIcon = CreateIconIndirect(&iconInfo);

    // Clean up
    DeleteObject(hBitmap);
    DeleteObject(hMonoMask);

    return hIcon;
}

void TrayManager::createTrayIcon() {
    QPixmap pixmap(":/Resources/wowpadx.png");
    QImage image = pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation).toImage().convertToFormat(QImage::Format_ARGB32);

    HICON hIcon = qtImageToHICON(image); 

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = hIcon;
    wcscpy_s(nid.szTip, L"WoWpadX");

    Shell_NotifyIcon(NIM_ADD, &nid);

    // Create context menu
    hMenu = CreatePopupMenu();
    InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, 1, L"WoWpadX");
    InsertMenu(hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
    InsertMenu(hMenu, 2, MF_BYPOSITION | MF_STRING, 2, L"Quit");
}

void TrayManager::removeTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
    if (hMenu) {
        DestroyMenu(hMenu);
        hMenu = nullptr;
    }
}

void TrayManager::showTrayMessage(const QString& title, const QString& message) {
    nid.uFlags |= NIF_INFO;
    wcscpy_s(nid.szInfo, message.toStdWString().c_str());
    wcscpy_s(nid.szInfoTitle, title.toStdWString().c_str());
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

bool TrayManager::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_USER + 1) {
        if (msg->lParam == WM_LBUTTONUP && mainWindow) {
            mainWindow->showNormal();
            mainWindow->raise();
            mainWindow->requestActivate();
        }
        else if (msg->lParam == WM_RBUTTONUP && hMenu) {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
            if (cmd == 1 && mainWindow) {
                mainWindow->showNormal();
                mainWindow->raise();
                mainWindow->requestActivate();
            }
            else if (cmd == 2) {
                QGuiApplication::quit();
            }
        }
    }
    return false;
}