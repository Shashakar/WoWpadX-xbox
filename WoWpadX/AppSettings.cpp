#include "AppSettings.h"
#include "Log.h"
#include <QJsonDocument>
#include <QFile>
#include <QStandardPaths>
#include <QJsonArray>
#include <QDir>

AppSettings* AppSettings::m_instance = nullptr;

AppSettings* AppSettings::instance() {
    if (!m_instance)
        m_instance = new AppSettings();
    return m_instance;
}

AppSettings::AppSettings(QObject* parent) : QObject(parent) {
    this->load();

}

#define DEFINE_SETTING(Name, name, Type) \
    Type AppSettings::name() const { return m_##name; } \
    void AppSettings::Name(Type value) { \
        if (m_##name != value) { \
            m_##name = value; \
            emit name##Changed(); \
            emit settingChanged(QStringLiteral(#name), value); \
        } \
    }

DEFINE_SETTING(setDisableDonationButton, disableDonationButton, bool)
DEFINE_SETTING(setRunInBackground, runInBackground, bool)
DEFINE_SETTING(setAutoUpdate, autoUpdate, bool)
DEFINE_SETTING(setSettingsVersion, settingsVersion, QString)
DEFINE_SETTING(setExportBindings, exportBindings, bool)
DEFINE_SETTING(setModifierStyle, modifierStyle, int)
DEFINE_SETTING(setCustomBindings, customBindings, bool)
DEFINE_SETTING(setWalkThreshold, walkThreshold, int)
DEFINE_SETTING(setHideAtStartup, hideAtStartup, bool)
DEFINE_SETTING(setTriggerThresholdLeft, triggerThresholdLeft, quint8)
DEFINE_SETTING(setTriggerThresholdRight, triggerThresholdRight, quint8)
DEFINE_SETTING(setCursorDeadzone, cursorDeadzone, int)
DEFINE_SETTING(setCursorSpeed, cursorSpeed, int)
DEFINE_SETTING(setCursorCurve, cursorCurve, int)
DEFINE_SETTING(setEnableMemoryReading, enableMemoryReading, bool)
DEFINE_SETTING(setSwapSticks, swapSticks, bool)
DEFINE_SETTING(setMovementThreshold, movementThreshold, int)
DEFINE_SETTING(setInputDirectKeyboard, inputDirectKeyboard, bool)
DEFINE_SETTING(setInputHardwareMouse, inputHardwareMouse, bool)
DEFINE_SETTING(setMemoryOverrideMenu, memoryOverrideMenu, bool)
DEFINE_SETTING(setMemoryOverrideAoeCast, memoryOverrideAoeCast, bool)
DEFINE_SETTING(setMemoryAutoWalk, memoryAutoWalk, bool)
DEFINE_SETTING(setMemoryAutoCenter, memoryAutoCenter, bool)
DEFINE_SETTING(setMemoryAutoCancel, memoryAutoCancel, bool)
DEFINE_SETTING(setMemoryVibrationDamage, memoryVibrationDamage, bool)
DEFINE_SETTING(setMemoryLightbar, memoryLightbar, bool)
DEFINE_SETTING(setButtonStyle, buttonStyle, int)
DEFINE_SETTING(setEnableLogging, enableLogging, bool)
DEFINE_SETTING(setMemoryAutoCenterDelay, memoryAutoCenterDelay, int)
DEFINE_SETTING(setMemoryVibrationHealing, memoryVibrationHealing, bool)
DEFINE_SETTING(setBindingsModified, bindingsModified, QDateTime)
DEFINE_SETTING(setEnableOverlay, enableOverlay, bool)
DEFINE_SETTING(setEnableOverlayCrosshair, enableOverlayCrosshair, bool)
DEFINE_SETTING(setEnableOverlayConnection, enableOverlayConnection, bool)
DEFINE_SETTING(setEnableOverlayBattery, enableOverlayBattery, bool)
DEFINE_SETTING(setNotificationH, notificationH, int)
DEFINE_SETTING(setNotificationV, notificationV, int)
DEFINE_SETTING(setEnableTouchpad, enableTouchpad, bool)
DEFINE_SETTING(setMemoryTouchpadCursorOnly, memoryTouchpadCursorOnly, bool)
DEFINE_SETTING(setTouchpadMode, touchpadMode, int)
DEFINE_SETTING(setMemoryAoeConfirm, memoryAoeConfirm, int)
DEFINE_SETTING(setMemoryAoeCancel, memoryAoeCancel, int)
DEFINE_SETTING(setMemoryInvertTurn, memoryInvertTurn, bool)
DEFINE_SETTING(setMemoryOverrideLogin, memoryOverrideLogin, bool)
DEFINE_SETTING(setGameProcessNames, gameProcessNames, QStringList)
DEFINE_SETTING(setSimpleRadial, simpleRadial, bool)

void AppSettings::load() {
    QFile file(settingsPath);
    if (!file.open(QIODevice::ReadOnly)) {
        file.setFileName(userPath);
        if (!file.open(QIODevice::ReadOnly)) {
            return;
        }
    }


    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    setDisableDonationButton(obj["disableDonationButton"].toBool());
    setRunInBackground(obj["runInBackground"].toBool());
    setAutoUpdate(obj["autoUpdate"].toBool());
    setSettingsVersion(obj["settingsVersion"].toString());
    setExportBindings(obj["exportBindings"].toBool());
    setModifierStyle(obj["modifierStyle"].toInt());
    setCustomBindings(false);
    setWalkThreshold(obj["walkThreshold"].toInt());
    setHideAtStartup(obj["hideAtStartup"].toBool());
    setTriggerThresholdLeft(obj["triggerThresholdLeft"].toInt());
    setTriggerThresholdRight(obj["triggerThresholdRight"].toInt());
    setCursorDeadzone(obj["cursorDeadzone"].toInt());
    setCursorSpeed(obj["cursorSpeed"].toInt());
    setCursorCurve(obj["cursorCurve"].toInt());
    setEnableMemoryReading(obj["enableMemoryReading"].toBool());
    setSwapSticks(obj["swapSticks"].toBool());
    setMovementThreshold(obj["movementThreshold"].toInt());
    setInputDirectKeyboard(obj["inputDirectKeyboard"].toBool());
    setInputHardwareMouse(obj["inputHardwareMouse"].toBool());
    setMemoryOverrideMenu(obj["memoryOverrideMenu"].toBool());
    setMemoryOverrideAoeCast(obj["memoryOverrideAoeCast"].toBool());
    setMemoryAutoWalk(obj["memoryAutoWalk"].toBool());
    setMemoryAutoCenter(obj["memoryAutoCenter"].toBool());
    setMemoryAutoCancel(obj["memoryAutoCancel"].toBool());
    setMemoryVibrationDamage(obj["memoryVibrationDamage"].toBool());
    setMemoryLightbar(obj["memoryLightbar"].toBool());
    setButtonStyle(obj["buttonStyle"].toInt());
    setEnableLogging(obj["enableLogging"].toBool());
    setMemoryAutoCenterDelay(obj["memoryAutoCenterDelay"].toInt());
    setMemoryVibrationHealing(obj["memoryVibrationHealing"].toBool());

    QString dateString = obj["bindingsModified"].toString();
    QDateTime dt = QDateTime::fromString(dateString, Qt::ISODate);
    setBindingsModified(dt);

    setEnableOverlay(obj["enableOverlay"].toBool());
    setEnableOverlayCrosshair(obj["enableOverlayCrosshair"].toBool());
    setEnableOverlayConnection(obj["enableOverlayConnection"].toBool());
    setEnableOverlayBattery(obj["enableOverlayBattery"].toBool());
    setNotificationH(obj["notificationH"].toInt());
    setNotificationV(obj["notificationV"].toInt());
    setEnableTouchpad(obj["enableTouchpad"].toBool());
    setMemoryTouchpadCursorOnly(obj["memoryTouchpadCursorOnly"].toBool());
    setTouchpadMode(obj["touchpadMode"].toInt());
    setMemoryAoeConfirm(obj["memoryAoeConfirm"].toInt());
    setMemoryAoeCancel(obj["memoryAoeCancel"].toInt());
    setMemoryInvertTurn(obj["memoryInvertTurn"].toBool());
    setMemoryOverrideLogin(obj["memoryOverrideLogin"].toBool());
    
    QJsonArray array = obj["gameProcessNames"].toArray();
    QStringList list;
    for (const QJsonValue& value : array) {
        list << value.toString();
    }
    setGameProcessNames(list);
    setSimpleRadial(obj["simpleRadial"].toBool());
    Log::writeLine(QString("[Bindings] Effective modifier profile=%1 customBindings=%2")
        .arg(m_modifierStyle)
        .arg(m_customBindings ? "true" : "false"));
}

void AppSettings::save() {
    QJsonObject obj;

    obj["disableDonationButton"] = m_disableDonationButton;
    obj["runInBackground"] = m_runInBackground;
    obj["autoUpdate"] = m_autoUpdate;


    obj["settingsVersion"] = m_settingsVersion;
    obj["exportBindings"] = m_exportBindings;
    obj["modifierStyle"] = m_modifierStyle;
    obj["customBindings"] = m_customBindings;
    obj["walkThreshold"] = m_walkThreshold;
    obj["hideAtStartup"] = m_hideAtStartup;
    obj["triggerThresholdLeft"] = m_triggerThresholdLeft;
    obj["triggerThresholdRight"] = m_triggerThresholdRight;
    obj["cursorDeadzone"] = m_cursorDeadzone;
    obj["cursorSpeed"] = m_cursorSpeed;
    obj["cursorCurve"] = m_cursorCurve;
    obj["enableMemoryReading"] = m_enableMemoryReading;
    obj["swapSticks"] = m_swapSticks;
    obj["movementThreshold"] = m_movementThreshold;
    obj["inputDirectKeyboard"] = m_inputDirectKeyboard;
    obj["inputHardwareMouse"] = m_inputHardwareMouse;
    obj["memoryOverrideMenu"] = m_memoryOverrideMenu;
    obj["memoryOverrideAoeCast"] =  m_memoryOverrideAoeCast;
    obj["memoryAutoWalk"] = m_memoryAutoWalk;
    obj["memoryAutoCenter"] = m_memoryAutoCenter;
    obj["memoryAutoCancel"] = m_memoryAutoCancel;
    obj["memoryVibrationDamage"] = m_memoryVibrationDamage;
    obj["memoryLightbar"] = m_memoryLightbar;
    obj["buttonStyle"] = m_buttonStyle;
    obj["enableLogging"] = m_enableLogging;
    obj["memoryAutoCenterDelay"] = m_memoryAutoCenterDelay;
    obj["memoryVibrationHealing"] = m_memoryVibrationHealing;

    obj["bindingsModified"] = m_bindingsModified.toString(Qt::ISODate);

    obj["enableOverlay"] = m_enableOverlay;
    obj["enableOverlayCrosshair"] = m_enableOverlayCrosshair;
    obj["enableOverlayConnection"] = m_enableOverlayConnection;
    obj["enableOverlayBattery"] = m_enableOverlayBattery;
    obj["notificationH"] = m_notificationH;
    obj["notificationV"] = m_notificationV;
    obj["enableTouchpad"] = m_enableTouchpad;
    obj["memoryTouchpadCursorOnly"] = m_memoryTouchpadCursorOnly;
    obj["touchpadMode"] = m_touchpadMode;
    obj["memoryAoeConfirm"] = m_memoryAoeConfirm;
    obj["memoryAoeCancel"] = m_memoryAoeCancel;
    obj["memoryInvertTurn"] = m_memoryInvertTurn;
    obj["memoryOverrideLogin"] = m_memoryOverrideLogin;
    QJsonArray array = QJsonArray::fromStringList(m_gameProcessNames);
    obj["gameProcessNames"] = array;
    obj["simpleRadial"] = m_simpleRadial;


    QFile file(settingsPath);
    if (!file.open(QIODevice::WriteOnly)) {
        file.setFileName(userPath);
        if (!file.open(QIODevice::WriteOnly)) {
            return;
        }
    }

    QJsonDocument doc(obj);
    file.write(doc.toJson());
}

void AppSettings::reset() {
    QJsonObject def = defaultSettings();

    setDisableDonationButton(def["disableDonationButton"].toBool());
    setRunInBackground(def["runInBackground"].toBool());
    setAutoUpdate(def["autoUpdate"].toBool());
    setSettingsVersion(def["settingsVersion"].toString());
    setExportBindings(def["exportBindings"].toBool());
    setModifierStyle(def["modifierStyle"].toInt());
    setCustomBindings(def["customBindings"].toBool());
    setWalkThreshold(def["walkThreshold"].toInt());
    setHideAtStartup(def["hideAtStartup"].toBool());
    setTriggerThresholdLeft(static_cast<qint8>(def["triggerThresholdLeft"].toInt()));
    setTriggerThresholdRight(static_cast<qint8>(def["triggerThresholdRight"].toInt()));
    setCursorDeadzone(def["cursorDeadzone"].toInt());
    setCursorSpeed(def["cursorSpeed"].toInt());
    setCursorCurve(def["cursorCurve"].toInt());
    setEnableMemoryReading(def["enableMemoryReading"].toBool());
    setSwapSticks(def["swapSticks"].toBool());
    setMovementThreshold(def["movementThreshold"].toInt());
    setInputDirectKeyboard(def["inputDirectKeyboard"].toBool());
    setInputHardwareMouse(def["inputHardwareMouse"].toBool());
    setMemoryOverrideMenu(def["memoryOverrideMenu"].toBool());
    setMemoryOverrideAoeCast(def["memoryOverrideAoeCast"].toBool());
    setMemoryAutoWalk(def["memoryAutoWalk"].toBool());
    setMemoryAutoCenter(def["memoryAutoCenter"].toBool());
    setMemoryAutoCancel(def["memoryAutoCancel"].toBool());
    setMemoryVibrationDamage(def["memoryVibrationDamage"].toBool());
    setMemoryLightbar(def["memoryLightbar"].toBool());
    setButtonStyle(def["buttonStyle"].toInt());
    setEnableLogging(def["enableLogging"].toBool());
    setMemoryAutoCenterDelay(def["memoryAutoCenterDelay"].toInt());
    setMemoryVibrationHealing(def["memoryVibrationHealing"].toBool());
    setBindingsModified(QDateTime::fromString(def["bindingsModified"].toString(), Qt::ISODate));
    setEnableOverlay(def["enableOverlay"].toBool());
    setEnableOverlayCrosshair(def["enableOverlayCrosshair"].toBool());
    setEnableOverlayConnection(def["enableOverlayConnection"].toBool());
    setEnableOverlayBattery(def["enableOverlayBattery"].toBool());
    setNotificationH(def["notificationH"].toInt());
    setNotificationV(def["notificationV"].toInt());
    setEnableTouchpad(def["enableTouchpad"].toBool());
    setMemoryTouchpadCursorOnly(def["memoryTouchpadCursorOnly"].toBool());
    setTouchpadMode(def["touchpadMode"].toInt());
    setMemoryAoeConfirm(def["memoryAoeConfirm"].toInt());
    setMemoryAoeCancel(def["memoryAoeCancel"].toInt());
    setMemoryInvertTurn(def["memoryInvertTurn"].toBool());
    setMemoryOverrideLogin(def["memoryOverrideLogin"].toBool());
    setGameProcessNames(def["gameProcessNames"].toVariant().toStringList());
    setSimpleRadial(def["simpleRadial"].toBool());
}

QVariant AppSettings::getProperty(const QString& name) {
    return this->property(name.toUtf8());
}

void AppSettings::setPropertyValue(const QString& name, const QVariant& value) {
    setProperty(name.toUtf8(), value);
}

QJsonObject AppSettings::defaultSettings() const {
    return QJsonObject{
        { "disableDonationButton", false },
        { "runInBackground", false },
        { "autoUpdate", false },
        { "settingsVersion", "0.0.0.0" },
        { "exportBindings", true },
        { "modifierStyle", 0 },
        { "customBindings", false },
        { "walkThreshold", 70 },
        { "hideAtStartup", false },
        { "triggerThresholdLeft", 80 },
        { "triggerThresholdRight", 80 },
        { "cursorDeadzone", 20 },
        { "cursorSpeed", 16 },
        { "cursorCurve", 4 },
        { "enableMemoryReading", false },
        { "swapSticks", false },
        { "movementThreshold", 40 },
        { "inputDirectKeyboard", true },
        { "inputHardwareMouse", false },
        { "memoryOverrideMenu", true },
        { "memoryOverrideAoeCast", true },
        { "memoryAutoWalk", true },
        { "memoryAutoCenter", true },
        { "memoryAutoCancel", true },
        { "memoryVibrationDamage", true },
        { "memoryLightbar", true },
        { "buttonStyle", 0 },
        { "enableLogging", true },
        { "memoryAutoCenterDelay", 1000 },
        { "memoryVibrationHealing", true },
        { "bindingsModified", QDateTime(QDate(1970, 1, 1), QTime(0, 0)).toString(Qt::ISODate) },
        { "enableOverlay", false },
        { "enableOverlayCrosshair", true },
        { "enableOverlayConnection", true },
        { "enableOverlayBattery", true },
        { "notificationH", 2 },
        { "notificationV", 2 },
        { "enableTouchpad", true },
        { "memoryTouchpadCursorOnly", false },
        { "touchpadMode", 0 },
        { "memoryAoeConfirm", 0 },
        { "memoryAoeCancel", 1 },
        { "memoryInvertTurn", false },
        { "memoryOverrideLogin", true },
        { "gameProcessNames", QJsonArray{ "wow", "wow-64", "wowt", "wowt-64", "wowb", "wowb-64", "ascension" }},
        { "simpleRadial", false }
    };
}

