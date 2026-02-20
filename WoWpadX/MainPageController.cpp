#include "MainPageController.h"
#include "ControllerManager.h"
#include "ProcessManager.h"
#include "AppSettings.h"
#include "WoWReader.h"
#include "OverlayClient.h"
#include <QDesktopServices>
#include <QVersionNumber>
#include <QUrl>
#include <QCoreApplication>
#include <QDebug>
#include "Log.h"

MainPageController::MainPageController(QObject* parent) : QObject(parent)
{
    m_controllerStatus1 = "Controller connection status";
    m_controllerStatus2 = "Controller battery status";
    m_controllerStatus3 = "Controller status warning";
    m_controllerWarningVisible = false;

    m_wowStatus1 = "Game process status";
    m_wowStatus2 = "Pixel bridge status";

    m_updateStatus = "Checking for updates...";
    m_updateAvailable = false;
    m_updateStatusColor = "white";
    m_updateIconSource = "qrc:/Resources/update-check.png";
    m_donateButtonSource = "qrc:/Resources/donate.png";

    m_downloadUrl = "";


    connect(&_networkManager, &QNetworkAccessManager::finished, this, &MainPageController::onUpdateReply);

    m_uiUpdateTimer.setInterval(1000); 
    connect(&m_uiUpdateTimer, &QTimer::timeout, this, &MainPageController::onUiTimerElapsed);
    m_uiUpdateTimer.start();

    checkForUpdates();
}

QString MainPageController::controllerStatus1() const { return m_controllerStatus1; }
QString MainPageController::controllerStatus2() const { return m_controllerStatus2; }
QString MainPageController::controllerStatus3() const { return m_controllerStatus3; }
bool MainPageController::controllerWarningVisible() const { return m_controllerWarningVisible; }

QString MainPageController::wowStatus1() const { return m_wowStatus1; }
QString MainPageController::wowStatus2() const { return m_wowStatus2; }

QString MainPageController::updateStatus() const { return m_updateStatus; }
bool MainPageController::updateAvailable() const { return m_updateAvailable; }
QString MainPageController::updateStatusColor() const { return m_updateStatusColor; }
QString MainPageController::updateIconSource() const { return m_updateIconSource; }


QString MainPageController::donateButtonSource() const { return m_donateButtonSource; }

void MainPageController::onUiTimerElapsed()
{
    auto activeDevice = ControllerManager::instance()->getActiveController();

    if (activeDevice != NULL)
    {
        auto batteryLevel = ControllerManager::instance()->GetBatteryLevel(activeDevice);

        m_controllerStatus1 = QString(SDL_GetGamepadName(activeDevice)) + " connected.";
        
        m_controllerStatus2 = QString("Battery level is at %1%").arg(batteryLevel);
    }
    else
    {
        m_controllerStatus1 = "No active controller";
        m_controllerStatus2 = "No information available";
    }

    m_wowStatus1 = ProcessManager::isGameRunning()
        ? "World of Warcraft is running"
        : "World of Warcraft is not running";


    if (AppSettings::instance()->enableMemoryReading())
        m_wowStatus2 = WoWReader::isSynced()
        ? "Pixel Bridge is enabled"
        : "Pixel Bridge is waiting sync";
    else
        m_wowStatus2 = "Pixel Bridge is disabled";
    
    emit controllerStatus1Changed();
    emit controllerStatus2Changed();
    emit wowStatus1Changed();
    emit wowStatus2Changed();
}

void MainPageController::checkForUpdates()
{
    QNetworkRequest request(QUrl("https://api.github.com/repos/leoaviana/WoWpadX/releases/latest"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "WoWpadX");
    _networkManager.get(request);
}


void MainPageController::onUpdateReply(QNetworkReply* reply)
{
    int hasError = 0;

    if (reply->error() != QNetworkReply::NoError) {
        Log::writeLine("Error checking for updates: " + reply->errorString()); 

        m_updateStatus = QString("Error checking for updates!");
        m_updateIconSource = "qrc:/Resources/update-failed.png";
        emit updateStatusChanged();
        emit updateIconSourceChanged();

        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (!jsonDoc.isObject()) {
        Log::writeLine("Error checking for updates: Invalid JSON response");

        m_updateStatus = QString("Error checking for updates!");
        m_updateIconSource = "qrc:/Resources/update-failed.png";
        emit updateStatusChanged();
        emit updateIconSourceChanged();

        reply->deleteLater();
        return;
    }

    QJsonObject root = jsonDoc.object();
    QString latestVersion = root.value("tag_name").toString();
    QString releaseName = root.value("name").toString();
    QString releaseNotes = root.value("body").toString();

    QString currentVersionStr = QCoreApplication::applicationVersion();
    QVersionNumber currentVersion = QVersionNumber::fromString(currentVersionStr);
    QVersionNumber latest = QVersionNumber::fromString(latestVersion);

    if (QVersionNumber::compare(latest, currentVersion) > 0) {
        Log::writeLine("New version available: " + latestVersion);

        m_updateStatus = QString("Version %1 is available now!").arg(latestVersion);
        m_updateIconSource = "qrc:/Resources/update-available.png";
        m_updateStatusColor = "cornflowerblue";
        m_updateAvailable = true;
        emit updateStatusChanged();
        emit updateAvailableChanged();
        emit updateStatusColorChanged();
        emit updateIconSourceChanged();

        m_downloadUrl = "https://github.com/leoaviana/WoWpadX/releases/latest";
    }
    else {
        m_updateStatus = QString("You have the latest version.");
        m_updateIconSource = "qrc:/Resources/update-ok.png";
        emit updateStatusChanged();
        emit updateIconSourceChanged();
    }

    reply->deleteLater();
}


void MainPageController::updateStatusClick()
{
    if(m_downloadUrl != "")
        QDesktopServices::openUrl(QUrl(m_downloadUrl));
}

void MainPageController::donateButtonReaction(int entered)
{
    if (entered == 1)
        m_donateButtonSource = "qrc:/Resources/donate-hover.png";
    else
        m_donateButtonSource = "qrc:/Resources/donate.png";

    emit donateButtonSourceChanged();
}


void MainPageController::donateButtonClick()
{
    QDesktopServices::openUrl(QUrl("https://www.paypal.com/donate/?hosted_button_id=CSQHQU3DNCRYU"));
}

// I won't create a new controller just for overlay test notification, so i'll put it in here.
void MainPageController::showTestNotification()
{    
    OverlayClient::SendNotification("This is a test",
        "This is a test notification. You clicked a button, and this is the notification that appeared. That's it.");
}

bool MainPageController::overlayFilesPresent() const {
    QString appPath = QCoreApplication::applicationDirPath();
    bool dllExists = QFile::exists(appPath + "/WoWpadXOverlay.dll");
    bool loaderExists = QFile::exists(appPath + "/WoWpadXOverlayLoader.exe");

    if (!(dllExists && loaderExists))
    {
        AppSettings::instance()->setEnableOverlay(false); // disable overlay if files are not present
    }

    return dllExists && loaderExists;
}