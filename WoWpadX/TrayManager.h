#pragma once

#include <QObject>
#include <QWindow>
#include <QAbstractNativeEventFilter>
#include <windows.h>
#include <shellapi.h>

class TrayManager : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    explicit TrayManager(QObject* parent = nullptr);
    void setMainWindow(QWindow* window);

    Q_INVOKABLE void minimizeToTray();

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    void createTrayIcon();
    void removeTrayIcon();
    void showTrayMessage(const QString& title, const QString& message);

    HWND hwnd = nullptr;
    NOTIFYICONDATA nid = {};
    QWindow* mainWindow = nullptr;
    HMENU hMenu = nullptr;
};