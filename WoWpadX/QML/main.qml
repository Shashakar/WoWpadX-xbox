import QtQuick.Controls.Material
import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import QtQuick.Window 6.2

ApplicationWindow {
    id: mainWindow
    width: 300
    height: 550
    visible: true
    title: "WoWpadXx"
    flags: Qt.Window | Qt.FramelessWindowHint
    Material.theme: Material.Dark
    Material.primary: Material.DeepPurple
    Material.accent: Material.DeepPurple

    property bool showSettings: false

    // Center on screen
    Component.onCompleted: {
        mainWindow.x = (Screen.width - mainWindow.width) / 2
        mainWindow.y = (Screen.height - mainWindow.height) / 2
    }

    Rectangle {
        anchors.fill: parent
        color: "#1E1E1E"

        border.color: Material.dividerColor
        border.width: 0.5

    ColumnLayout {
        anchors.fill: parent

        TitleBar {
            id: titleBar
            Layout.fillWidth: true
            onSettingsClicked: showSettings = !showSettings
        }

        // Page Stack with Crossfade
        StackLayout {
            id: pageStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: showSettings ? 1 : 0

            MainPage {
                opacity: showSettings ? 0 : 1
                Behavior on opacity {
                    NumberAnimation { duration: 250; easing.type: Easing.OutCirc }
                }
            }

            SettingsPanel {
                opacity: showSettings ? 1 : 0
                Behavior on opacity {
                    NumberAnimation { duration: 250; easing.type: Easing.OutCirc }
                }
            }
        }
    }
    }
}
