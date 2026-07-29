#include "BindManager.h"
#include <QDir>

int BindManager::getKey(GamepadBinding button) {
    try {
        const auto& defaults = KeybindDefaults::getDefault(
            AppSettings::instance()->modifierStyle());
        for (const auto& bind : defaults) {
            if (bind.bindType == button)
                return bind.virtualKey;
        }
    }
    catch (...) {}

    return 0;
}

void BindManager::setKey(GamepadBinding button, int key) {
    for (auto& bind : currentKeybinds) {
        if (bind.bindType == button) {
            bind.virtualKey = key;
            break;
        }
    }
    saveBindings();
    emit instance().bindingsChanged();

    AppSettings::instance()->setBindingsModified(QDateTime::currentDateTime()); 
}

void BindManager::loadBindings() {
    if (!QFile::exists(keybindFile) && !QFile::exists(userPath)) {
        resetDefaults(0);
        return;
    }

    QString pathToLoad = QFile::exists(keybindFile) ? keybindFile : userPath;

    QFile file(pathToLoad);
    if (!file.open(QIODevice::ReadOnly)) {
        Log::writeLine("Failed to open keybinds file");
        resetDefaults(0);
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        Log::writeLine("Invalid JSON structure for keybinds");
        resetDefaults(0);
        return;
    }

    QJsonArray arr = doc.object()["Bindings"].toArray();
    currentKeybinds.clear();

    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        GamepadBinding btn = static_cast<GamepadBinding>(obj["bindType"].toInt());
        int key = { obj["key"].toInt() };
        currentKeybinds.push_back({ btn, key });
    }
}

void BindManager::saveBindings() {
    QJsonArray arr;
    for (const auto& bind : currentKeybinds) {
        QJsonObject obj;
        obj["bindType"] = static_cast<int>(bind.bindType);
        obj["key"] = static_cast<int>(bind.virtualKey);
        arr.append(obj);
    }

    QJsonObject root;
    root["Bindings"] = arr;

    QJsonDocument doc(root);

    QString savePath = keybindFile;
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        savePath = userPath;
        file.setFileName(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            Log::writeLine("Failed to save keybinds to both locations");
            return;
        }
    }

    file.write(doc.toJson());
}

void BindManager::resetDefaults(int profile) {
    currentKeybinds = KeybindDefaults::getDefault(AppSettings::instance()->modifierStyle());
    AppSettings::instance()->setModifierStyle(profile);
    saveBindings();
    emit instance().bindingsChanged();
}

BindManager& BindManager::instance() {
    static BindManager inst;
    return inst;
}
