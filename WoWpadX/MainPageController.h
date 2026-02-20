#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class MainPageController : public QObject
{
    Q_OBJECT
        Q_PROPERTY(QString controllerStatus1 READ controllerStatus1 NOTIFY controllerStatus1Changed)
        Q_PROPERTY(QString controllerStatus2 READ controllerStatus2 NOTIFY controllerStatus2Changed)
        Q_PROPERTY(QString controllerStatus3 READ controllerStatus3 NOTIFY controllerStatus3Changed)
        Q_PROPERTY(bool controllerWarningVisible READ controllerWarningVisible NOTIFY controllerWarningVisibleChanged)

        Q_PROPERTY(QString wowStatus1 READ wowStatus1 NOTIFY wowStatus1Changed)
        Q_PROPERTY(QString wowStatus2 READ wowStatus2 NOTIFY wowStatus2Changed)

        Q_PROPERTY(QString updateStatus READ updateStatus NOTIFY updateStatusChanged)
        Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged)
        Q_PROPERTY(QString updateStatusColor READ updateStatusColor NOTIFY updateStatusColorChanged)

        Q_PROPERTY(QString updateIconSource READ updateIconSource NOTIFY updateIconSourceChanged)
        Q_PROPERTY(QString donateButtonSource READ donateButtonSource NOTIFY donateButtonSourceChanged)
        Q_PROPERTY(QString appVersion READ getAppVersion CONSTANT)
        Q_PROPERTY(bool overlayFilesPresent READ overlayFilesPresent NOTIFY overlayFilesPresentChanged)

public:
    explicit MainPageController(QObject* parent = nullptr);

    QString controllerStatus1() const;
    QString controllerStatus2() const;
    QString controllerStatus3() const;
    bool controllerWarningVisible() const;
    
    bool overlayFilesPresent() const;

    QString wowStatus1() const;
    QString wowStatus2() const;

    QString updateStatus() const;
    bool updateAvailable() const;
    QString updateStatusColor() const;
    QString updateIconSource() const;

    QString donateButtonSource() const;

    static QString appVersion() { return "1.0.0"; }

    QString getAppVersion() const { return MainPageController::appVersion(); }

public slots: 
    void updateStatusClick();
    void checkForUpdates();
    void donateButtonClick();
    void donateButtonReaction(int entered);
    void showTestNotification();

signals:
    void controllerStatus1Changed();
    void controllerStatus2Changed();
    void controllerStatus3Changed();
    void controllerWarningVisibleChanged();
    void wowStatus1Changed();
    void wowStatus2Changed();
    void updateStatusChanged();
    void updateAvailableChanged();
    void updateStatusColorChanged();
    void updateIconSourceChanged();
    void donateButtonSourceChanged();
    void overlayFilesPresentChanged();

private slots:
    void onUiTimerElapsed(); 
    void onUpdateReply(QNetworkReply* reply);

private:
    QString m_controllerStatus1;
    QString m_controllerStatus2;
    QString m_controllerStatus3;
    bool m_controllerWarningVisible;

    QString m_wowStatus1;
    QString m_wowStatus2;

    QString m_updateStatus;
    bool m_updateAvailable;
    QString m_updateStatusColor;
    QString m_updateIconSource;

    QString m_downloadUrl;

    QString m_donateButtonSource;

    QTimer m_uiUpdateTimer;
    QNetworkAccessManager _networkManager;
};
