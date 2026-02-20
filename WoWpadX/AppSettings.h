#pragma once

#include <QObject>
#include <QJsonObject>
#include <QDateTime>
#include <QVector>
#include <QString>
#include <QStandardPaths>

class AppSettings : public QObject {
    Q_OBJECT
        Q_PROPERTY(bool disableDonationButton READ disableDonationButton WRITE setDisableDonationButton NOTIFY disableDonationButtonChanged)
        Q_PROPERTY(bool runInBackground READ runInBackground WRITE setRunInBackground NOTIFY runInBackgroundChanged)
        Q_PROPERTY(bool autoUpdate READ autoUpdate WRITE setAutoUpdate NOTIFY autoUpdateChanged)
        Q_PROPERTY(QString settingsVersion READ settingsVersion WRITE setSettingsVersion NOTIFY settingsVersionChanged)
        Q_PROPERTY(bool exportBindings READ exportBindings WRITE setExportBindings NOTIFY exportBindingsChanged)
        Q_PROPERTY(int modifierStyle READ modifierStyle WRITE setModifierStyle NOTIFY modifierStyleChanged)
        Q_PROPERTY(bool customBindings READ customBindings WRITE setCustomBindings NOTIFY customBindingsChanged)
        Q_PROPERTY(int walkThreshold READ walkThreshold WRITE setWalkThreshold NOTIFY walkThresholdChanged)
        Q_PROPERTY(bool hideAtStartup READ hideAtStartup WRITE setHideAtStartup NOTIFY hideAtStartupChanged)
        Q_PROPERTY(quint8 triggerThresholdLeft READ triggerThresholdLeft WRITE setTriggerThresholdLeft NOTIFY triggerThresholdLeftChanged)
        Q_PROPERTY(quint8 triggerThresholdRight READ triggerThresholdRight WRITE setTriggerThresholdRight NOTIFY triggerThresholdRightChanged)
        Q_PROPERTY(int cursorDeadzone READ cursorDeadzone WRITE setCursorDeadzone NOTIFY cursorDeadzoneChanged)
        Q_PROPERTY(int cursorSpeed READ cursorSpeed WRITE setCursorSpeed NOTIFY cursorSpeedChanged)
        Q_PROPERTY(int cursorCurve READ cursorCurve WRITE setCursorCurve NOTIFY cursorCurveChanged)
        Q_PROPERTY(bool enableMemoryReading READ enableMemoryReading WRITE setEnableMemoryReading NOTIFY enableMemoryReadingChanged)
        Q_PROPERTY(bool swapSticks READ swapSticks WRITE setSwapSticks NOTIFY swapSticksChanged)
        Q_PROPERTY(int movementThreshold READ movementThreshold WRITE setMovementThreshold NOTIFY movementThresholdChanged)
        Q_PROPERTY(bool inputDirectKeyboard READ inputDirectKeyboard WRITE setInputDirectKeyboard NOTIFY inputDirectKeyboardChanged)
        Q_PROPERTY(bool inputHardwareMouse READ inputHardwareMouse WRITE setInputHardwareMouse NOTIFY inputHardwareMouseChanged)
        Q_PROPERTY(bool memoryOverrideMenu READ memoryOverrideMenu WRITE setMemoryOverrideMenu NOTIFY memoryOverrideMenuChanged)
        Q_PROPERTY(bool memoryOverrideAoeCast READ memoryOverrideAoeCast WRITE setMemoryOverrideAoeCast NOTIFY memoryOverrideAoeCastChanged)
        Q_PROPERTY(bool memoryAutoWalk READ memoryAutoWalk WRITE setMemoryAutoWalk NOTIFY memoryAutoWalkChanged)
        Q_PROPERTY(bool memoryAutoCenter READ memoryAutoCenter WRITE setMemoryAutoCenter NOTIFY memoryAutoCenterChanged)
        Q_PROPERTY(bool memoryAutoCancel READ memoryAutoCancel WRITE setMemoryAutoCancel NOTIFY memoryAutoCancelChanged)
        Q_PROPERTY(bool memoryVibrationDamage READ memoryVibrationDamage WRITE setMemoryVibrationDamage NOTIFY memoryVibrationDamageChanged)
        Q_PROPERTY(bool memoryLightbar READ memoryLightbar WRITE setMemoryLightbar NOTIFY memoryLightbarChanged)
        Q_PROPERTY(int buttonStyle READ buttonStyle WRITE setButtonStyle NOTIFY buttonStyleChanged)
        Q_PROPERTY(bool enableLogging READ enableLogging WRITE setEnableLogging NOTIFY enableLoggingChanged)
        Q_PROPERTY(int memoryAutoCenterDelay READ memoryAutoCenterDelay WRITE setMemoryAutoCenterDelay NOTIFY memoryAutoCenterDelayChanged)
        Q_PROPERTY(bool memoryVibrationHealing READ memoryVibrationHealing WRITE setMemoryVibrationHealing NOTIFY memoryVibrationHealingChanged)
        Q_PROPERTY(QDateTime bindingsModified READ bindingsModified WRITE setBindingsModified NOTIFY bindingsModifiedChanged)
        Q_PROPERTY(bool enableOverlay READ enableOverlay WRITE setEnableOverlay NOTIFY enableOverlayChanged)
        Q_PROPERTY(bool enableOverlayCrosshair READ enableOverlayCrosshair WRITE setEnableOverlayCrosshair NOTIFY enableOverlayCrosshairChanged)
        Q_PROPERTY(bool enableOverlayConnection READ enableOverlayConnection WRITE setEnableOverlayConnection NOTIFY enableOverlayConnectionChanged)
        Q_PROPERTY(bool enableOverlayBattery READ enableOverlayBattery WRITE setEnableOverlayBattery NOTIFY enableOverlayBatteryChanged)
        Q_PROPERTY(int notificationH READ notificationH WRITE setNotificationH NOTIFY notificationHChanged)
        Q_PROPERTY(int notificationV READ notificationV WRITE setNotificationV NOTIFY notificationVChanged)
        Q_PROPERTY(bool enableTouchpad READ enableTouchpad WRITE setEnableTouchpad NOTIFY enableTouchpadChanged)
        Q_PROPERTY(bool memoryTouchpadCursorOnly READ memoryTouchpadCursorOnly WRITE setMemoryTouchpadCursorOnly NOTIFY memoryTouchpadCursorOnlyChanged)
        Q_PROPERTY(int touchpadMode READ touchpadMode WRITE setTouchpadMode NOTIFY touchpadModeChanged)
        Q_PROPERTY(int memoryAoeConfirm READ memoryAoeConfirm WRITE setMemoryAoeConfirm NOTIFY memoryAoeConfirmChanged)
        Q_PROPERTY(int memoryAoeCancel READ memoryAoeCancel WRITE setMemoryAoeCancel NOTIFY memoryAoeCancelChanged)
        Q_PROPERTY(bool memoryInvertTurn READ memoryInvertTurn WRITE setMemoryInvertTurn NOTIFY memoryInvertTurnChanged)
        Q_PROPERTY(bool memoryOverrideLogin READ memoryOverrideLogin WRITE setMemoryOverrideLogin NOTIFY memoryOverrideLoginChanged)
        Q_PROPERTY(QStringList gameProcessNames READ gameProcessNames WRITE setGameProcessNames NOTIFY gameProcessNamesChanged)
        Q_PROPERTY(bool simpleRadial READ simpleRadial WRITE setSimpleRadial NOTIFY simpleRadialChanged)
public:
    static AppSettings* instance(); // singleton access

    // Getters and Setters (generated in AppSettings.cpp)
#define DECLARE_SETTING(setter, name, type) \
        type name() const; \
        void setter(type value); 

    DECLARE_SETTING(setDisableDonationButton, disableDonationButton, bool)
    DECLARE_SETTING(setRunInBackground, runInBackground, bool)
    DECLARE_SETTING(setAutoUpdate, autoUpdate, bool)
    DECLARE_SETTING(setSettingsVersion, settingsVersion, QString)
    DECLARE_SETTING(setExportBindings, exportBindings, bool)
    DECLARE_SETTING(setModifierStyle, modifierStyle, int)
    DECLARE_SETTING(setCustomBindings, customBindings, bool)
    DECLARE_SETTING(setWalkThreshold, walkThreshold, int)
    DECLARE_SETTING(setHideAtStartup, hideAtStartup, bool)
    DECLARE_SETTING(setTriggerThresholdLeft, triggerThresholdLeft, quint8)
    DECLARE_SETTING(setTriggerThresholdRight, triggerThresholdRight, quint8)
    DECLARE_SETTING(setCursorDeadzone, cursorDeadzone, int)
    DECLARE_SETTING(setCursorSpeed, cursorSpeed, int)
    DECLARE_SETTING(setCursorCurve, cursorCurve, int)
    DECLARE_SETTING(setEnableMemoryReading, enableMemoryReading, bool)
    DECLARE_SETTING(setSwapSticks, swapSticks, bool)
    DECLARE_SETTING(setMovementThreshold, movementThreshold, int)
    DECLARE_SETTING(setInputDirectKeyboard, inputDirectKeyboard, bool)
    DECLARE_SETTING(setInputHardwareMouse, inputHardwareMouse, bool)
    DECLARE_SETTING(setMemoryOverrideMenu, memoryOverrideMenu, bool)
    DECLARE_SETTING(setMemoryOverrideAoeCast, memoryOverrideAoeCast, bool)
    DECLARE_SETTING(setMemoryAutoWalk, memoryAutoWalk, bool)
    DECLARE_SETTING(setMemoryAutoCenter, memoryAutoCenter, bool)
    DECLARE_SETTING(setMemoryAutoCancel, memoryAutoCancel, bool)
    DECLARE_SETTING(setMemoryVibrationDamage, memoryVibrationDamage, bool)
    DECLARE_SETTING(setMemoryLightbar, memoryLightbar, bool)
    DECLARE_SETTING(setButtonStyle, buttonStyle, int)
    DECLARE_SETTING(setEnableLogging, enableLogging, bool)
    DECLARE_SETTING(setMemoryAutoCenterDelay, memoryAutoCenterDelay, int)
    DECLARE_SETTING(setMemoryVibrationHealing, memoryVibrationHealing, bool)
    DECLARE_SETTING(setBindingsModified, bindingsModified, QDateTime)
    DECLARE_SETTING(setEnableOverlay, enableOverlay, bool)
    DECLARE_SETTING(setEnableOverlayCrosshair, enableOverlayCrosshair, bool)
    DECLARE_SETTING(setEnableOverlayConnection, enableOverlayConnection, bool)
    DECLARE_SETTING(setEnableOverlayBattery, enableOverlayBattery, bool)
    DECLARE_SETTING(setNotificationH, notificationH, int)
    DECLARE_SETTING(setNotificationV, notificationV, int)
    DECLARE_SETTING(setEnableTouchpad, enableTouchpad, bool)
    DECLARE_SETTING(setMemoryTouchpadCursorOnly, memoryTouchpadCursorOnly, bool)
    DECLARE_SETTING(setTouchpadMode, touchpadMode, int)
    DECLARE_SETTING(setMemoryAoeConfirm, memoryAoeConfirm, int)
    DECLARE_SETTING(setMemoryAoeCancel, memoryAoeCancel, int)
    DECLARE_SETTING(setMemoryInvertTurn, memoryInvertTurn, bool)
    DECLARE_SETTING(setMemoryOverrideLogin, memoryOverrideLogin, bool)
    DECLARE_SETTING(setGameProcessNames, gameProcessNames, QStringList)
    DECLARE_SETTING(setSimpleRadial, simpleRadial, bool)

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();
    Q_INVOKABLE void reset();

    Q_INVOKABLE QVariant getProperty(const QString& name);
    Q_INVOKABLE void setPropertyValue(const QString& name, const QVariant& value);

signals:
#define DECLARE_SIGNAL(name) void name##Changed();
    DECLARE_SIGNAL(disableDonationButton)
    DECLARE_SIGNAL(runInBackground)
    DECLARE_SIGNAL(autoUpdate)
    DECLARE_SIGNAL(settingsVersion)
    DECLARE_SIGNAL(exportBindings)
    DECLARE_SIGNAL(modifierStyle)
    DECLARE_SIGNAL(customBindings)
    DECLARE_SIGNAL(walkThreshold)
    DECLARE_SIGNAL(hideAtStartup)
    DECLARE_SIGNAL(triggerThresholdLeft)
    DECLARE_SIGNAL(triggerThresholdRight)
    DECLARE_SIGNAL(cursorDeadzone)
    DECLARE_SIGNAL(cursorSpeed)
    DECLARE_SIGNAL(cursorCurve)
    DECLARE_SIGNAL(enableMemoryReading)
    DECLARE_SIGNAL(swapSticks)
    DECLARE_SIGNAL(movementThreshold)
    DECLARE_SIGNAL(inputDirectKeyboard)
    DECLARE_SIGNAL(inputHardwareMouse)
    DECLARE_SIGNAL(memoryOverrideMenu)
    DECLARE_SIGNAL(memoryOverrideAoeCast)
    DECLARE_SIGNAL(memoryAutoWalk)
    DECLARE_SIGNAL(memoryAutoCenter)
    DECLARE_SIGNAL(memoryAutoCancel)
    DECLARE_SIGNAL(memoryVibrationDamage)
    DECLARE_SIGNAL(memoryLightbar)
    DECLARE_SIGNAL(buttonStyle)
    DECLARE_SIGNAL(enableLogging)
    DECLARE_SIGNAL(memoryAutoCenterDelay)
    DECLARE_SIGNAL(memoryVibrationHealing)
    DECLARE_SIGNAL(bindingsModified)
    DECLARE_SIGNAL(enableOverlay)
    DECLARE_SIGNAL(enableOverlayCrosshair)
    DECLARE_SIGNAL(enableOverlayConnection)
    DECLARE_SIGNAL(enableOverlayBattery)
    DECLARE_SIGNAL(notificationH)
    DECLARE_SIGNAL(notificationV)
    DECLARE_SIGNAL(enableTouchpad)
    DECLARE_SIGNAL(memoryTouchpadCursorOnly)
    DECLARE_SIGNAL(touchpadMode)
    DECLARE_SIGNAL(memoryAoeConfirm)
    DECLARE_SIGNAL(memoryAoeCancel)
    DECLARE_SIGNAL(memoryInvertTurn)
    DECLARE_SIGNAL(memoryOverrideLogin)
    DECLARE_SIGNAL(gameProcessNames)
    DECLARE_SIGNAL(simpleRadial)

    void settingChanged(QString name, QVariant newValue);

private:
    explicit AppSettings(QObject* parent = nullptr);
    static AppSettings* m_instance;

    bool m_disableDonationButton = false;
    bool m_runInBackground = false;
    bool m_autoUpdate = false;
    QString m_settingsVersion = "0.0.0.0";
    bool m_exportBindings = true;
    int  m_modifierStyle = 0;
    bool m_customBindings = false;
    int  m_walkThreshold = 70;
    bool m_hideAtStartup = false;
    qint8 m_triggerThresholdLeft = 80;
    qint8 m_triggerThresholdRight = 80;
    int  m_cursorDeadzone = 20;
    int  m_cursorSpeed = 16;
    int  m_cursorCurve = 4;
    bool m_enableMemoryReading = false;
    bool m_swapSticks = false;
    int  m_movementThreshold = 40;
    bool m_inputDirectKeyboard = true;
    bool m_inputHardwareMouse = false;
    bool m_memoryOverrideMenu = true;
    bool m_memoryOverrideAoeCast = true;
    bool m_memoryAutoWalk = true;
    bool m_memoryAutoCenter = true;
    bool m_memoryAutoCancel = true;
    bool m_memoryVibrationDamage = true;
    bool m_memoryLightbar = true;
    int m_buttonStyle = 0;
    bool m_enableLogging = true;
    int m_memoryAutoCenterDelay = 1000;
    bool m_memoryVibrationHealing = true;
    QDateTime m_bindingsModified = QDateTime(QDate(1970, 1, 1), QTime(0,0));
    bool m_enableOverlay = false;
    bool m_enableOverlayCrosshair = true;
    bool m_enableOverlayConnection = true;
    bool m_enableOverlayBattery = true;
    int m_notificationH = 2;
    int m_notificationV = 2;
    bool m_enableTouchpad = true;
    bool m_memoryTouchpadCursorOnly = false;
    int m_touchpadMode = 0;
    int m_memoryAoeConfirm = 0; // GamepadBindings::South
    int m_memoryAoeCancel = 1; // GamepadBindings::East
    bool m_memoryInvertTurn = false;
    bool m_memoryOverrideLogin = true;
    QStringList m_gameProcessNames = { "wow", "wow-64", "wowt", "wowt-64", "wowb", "wowb-64", "ascension" };
    bool m_simpleRadial = false;


    const QString settingsPath = "settings.json";
    const QString userPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.json";

    QJsonObject defaultSettings() const;
};
