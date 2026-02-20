import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import QtQuick.Shapes 6.2  
import WoWpadX

Item {
    id: titleBar
    height: 40
    width: parent.width

    signal settingsClicked()

    MouseArea {
        id: dragArea
        anchors.fill: parent
        drag.target: null
        acceptedButtons: Qt.LeftButton
        onPressed: {
            if (!closeMouse.containsMouse &&
                !minimizeMouse.containsMouse &&
                !settingsIcon.containsMouse)
            {
                mainWindow.startSystemMove()
            }
        }
    }


    Rectangle {
        anchors.fill: parent
        color: "blueviolet"
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 10

        // Left: App Icon + Title
        RowLayout {
            Layout.alignment: Qt.AlignLeft
            spacing: 5

            Image {
                source: "qrc:/Resources/wowpadx.png"
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
            }

            Text {
                text: "WOWPADX"
                font.pixelSize: 13
                color: "white"
            }
        }

        // Right: Buttons
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 5

            // Settings Button
            Item {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16

                Image {
                    id: settingsIcon
                    source: "qrc:/Resources/settings.png"
                    anchors.fill: parent
                    opacity: mouseArea.containsMouse ? 1 : 0.7

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: titleBar.settingsClicked()
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 15
                color: "white"
                opacity: 0.7
            }

            // Minimize Button
            Button {
                flat: true
                Layout.preferredWidth: 46
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: minimizeMouse.containsMouse ? "#44AAAAAA" : "transparent"
                }

                contentItem: Shape {
                    anchors.centerIn: parent
                    width: 10
                    height: 10
                    transform: Translate {
                        x: 12
                        y: 2
                    } 
                    ShapePath {
                        strokeWidth: 0
                        fillColor: "white"  
                        scale: Qt.size(0.005,0.005)

                        PathSvg {
                            path: "M2048 1229v-205h-2048v205h2048z"
                        }
                    }
                }

                MouseArea {
                    id: minimizeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: Qt.callLater(() => mainWindow.showMinimized())
                }
            }

            // Maximize Button (hidden)
            Button {
                visible: false
                flat:true
                Layout.preferredWidth: 46
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: "transparent"
                }

                contentItem: Shape {
                    anchors.centerIn: parent
                    width: 10
                    height: 10
                    transform: Translate {
                        x: 12
                        y: 0
                    } 
                    ShapePath {
                        strokeWidth: 0
                        fillColor: "white"  
                        scale: Qt.size(0.005,0.005)

                        PathSvg {
                            path: "M2048 2048v-2048h-2048v2048h2048zM1843 1843h-1638v-1638h1638v1638z"
                        }
                    }
                }
            }

            // Close Button
            Button {
                Layout.preferredWidth: 46
                Layout.preferredHeight: 30
                flat: true
                background: Rectangle {
                    color: closeMouse.containsMouse ? "red" : "transparent"
                }

                contentItem: Shape {
                    anchors.centerIn: parent
                    width: 10
                    height: 10
                    transform: Translate {
                        x: 12
                        y: 0
                    } 
                    ShapePath {
                        strokeWidth: 0
                        fillColor: closeMouse.containsMouse ? "wheat" : "white"
                        scale: Qt.size(0.005,0.005)

                        PathSvg {
                            path: "M1169 1024l879 -879l-145 -145l-879 879l-879 -879l-145 145l879 879l-879 879l145 145l879 -879l879 879l145 -145z"
                        }
                    }
                }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: AppSettings.runInBackground ? TrayManager.minimizeToTray() : Qt.quit()
                }
            }
        }
    }
}
