import QtQuick.Controls.Material
import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import WoWpadX 1.0

Item {
    id: settingsOverlay
    width: 313
    height: 421

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width

    ColumnLayout {
        id: mainCol
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label
        {
            text: "Overlay"
            font.pixelSize: 16
        }

        Rectangle { height: 1; width: parent.width; color: "#ccc" }

        Label {
            id: warningText
            text: "Warning: This feature injects a overlay\nlibrary into the client to interact with the\ngraphics stack and display custom popups.\nWhile this is a common method used by many\noverlays, it still modifies the client process\nand may be flagged by anti-cheat systems. If\nyou're concerned about triggering a ban,\nI strongly recommend that you do NOT\nenable this feature."
            font.bold: true
            wrapMode: Text.Wrap
            color: "red"
        }

        Label {
            id: featureText
            text: "This feature will only work when playing in \nwindowed and windowed fullscreen modes."
            font.bold: true
            wrapMode: Text.Wrap
            color: "white"
        }

        CheckBox {
            id: checkEnableOverlay
            text: "Enable WoWpadX in-game overlay"
            checked: AppSettings.enableOverlay
            onToggled: AppSettings.enableOverlay = checked
        }

        Rectangle { height: 1; width: parent.width; color: "#ccc" }

        TabBar {
            id: tabBar
            width: parent.width
            enabled: AppSettings.enableOverlay

            TabButton { text: qsTr("Notifications") }
            // Add more TabButton entries here if needed
        }

        StackLayout {
            width: parent.width
            currentIndex: tabBar.currentIndex
            enabled: AppSettings.enableOverlay

            Item {
                implicitHeight: columnContent.implicitHeight
                // Notifications Tab Content
                ColumnLayout {
                    id: columnContent       
                    spacing: 10
                    Layout.fillWidth: true

                    GridLayout {
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 5

                        Label { text: "Position" }

                        RowLayout {
                            spacing: 10
                            ComboBox {
                                id: comboVertical
                                enabled: false
                                model: ["Top", "Middle", "Bottom"]
                                currentIndex: 2 //AppSettings.notificationV
                                //onCurrentIndexChanged: AppSettings.notificationV = currentIndex
                                Layout.preferredWidth: 80
                            }

                            ComboBox {
                                id: comboHorizontal
                                enabled: false
                                model: ["Left", "Center", "Right"]
                                currentIndex: 2 //AppSettings.notificationH
                                //onCurrentIndexChanged: AppSettings.notificationH = currentIndex
                                Layout.preferredWidth: 80
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 5
                        CheckBox {
                            text: "Notify on controller connect/disconnect"
                            checked: AppSettings.enableOverlayConnection
                            onToggled: AppSettings.enableOverlayConnection = checked
                        }

                        CheckBox {
                            text: "Notify on low battery"
                            checked: AppSettings.enableOverlayBattery
                            onToggled: AppSettings.enableOverlayBattery = checked
                        }
                    }

                    Button {
                        text: "Show Test Notification"
                        Layout.preferredWidth: mainCol.width
                        onClicked: MainPageController.showTestNotification();
                    }
                }
            }
        }
    }
}
}
