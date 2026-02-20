#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QStandardPaths>
#include <QObject>
#include <QDateTime>

#include "AppSettings.h"
#include "KeybindDefaults.h" 
#include "Log.h"

class BindManager : public QObject {
    Q_OBJECT

public:
    static inline QString keybindFile = "keybinds.json";
    static inline QString userPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + keybindFile;
    static inline std::vector<Keybind> currentKeybinds;

    static int getKey(GamepadBinding button);
    static void setKey(GamepadBinding button, int key);
    static void loadBindings();
    static void saveBindings();
    static void resetDefaults(int profile);

signals:
    void bindingsChanged();

private:
    static BindManager& instance();
};