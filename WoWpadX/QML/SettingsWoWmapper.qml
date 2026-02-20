import QtQuick.Controls.Material
import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import WoWpadX 1.0

Item {
    id: settingsWoWmapper
    width: 313
    height: 421

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 10


        // General Section
        Label { text: "General"; font.pixelSize: 16 }
        Rectangle { height: 1; width: parent.width - 15; color: "#ccc" }
/*
        CheckBox {
            id: checkExportBindings
            text: "Automatically sync settings to ConsolePort" 
            checked: appSettings.exportBindings
            onToggled: appSettings.exportBindings = checked

        }
*/
        CheckBox {
            text: "Close WoWpadX to the notification icon"
            checked: AppSettings.runInBackground
            onToggled: AppSettings.runInBackground = checked
        }

        CheckBox {
            text: "Hide application window at startup"
            checked: AppSettings.hideAtStartup
            onToggled: AppSettings.hideAtStartup = checked
        }

        CheckBox {
            text: "Output log file to WoWpadX folder"
            checked: AppSettings.enableLogging
            onToggled: AppSettings.enableLogging = checked
        }

        CheckBox {
            text: "Automatically download and install updates"
            enabled: false
            checked: false // AppSettings.autoUpdate
            visible: true
            onToggled: AppSettings.autoUpdate = checked
        }


        Button {
            text: "Reset all settings"
            Layout.preferredWidth: parent.width
            onClicked: AppSettings.reset()
        }

        ColumnLayout {
            Layout.leftMargin: 75
            spacing: 5
            Label {
                id: versionText
                text: "WoWpadX Version " + MainPageController.appVersion
                color: "#7F7F7F"
                horizontalAlignment: Qt.AlignHCenter
            }
        }
    }
}
