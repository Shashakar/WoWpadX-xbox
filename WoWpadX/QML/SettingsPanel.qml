import QtQuick.Controls.Material
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

Item {
    id: settingsPanel
    width: 313
    height: 500

    ListModel {
        id: settingsMenuModel
        ListElement { name: "General"; pageIndex: 0 }
        ListElement { name: "Controllers"; pageIndex: 1 }
        ListElement { name: "Keybindings"; pageIndex: 2 }
        ListElement { name: "Analog Inputs"; pageIndex: 3 } 
        ListElement { name: "Pixel Bridge"; pageIndex: 4 }
        
        Component.onCompleted: { 
            if (MainPageController.overlayFilesPresent) {
                settingsMenuModel.insert(5, { "name": "Overlay", "pageIndex": 5 });
            }
        }
    }

    Drawer {
        id: leftDrawer
        width: 200
        height: parent.height
        edge: Qt.LeftEdge
        modal: false

        ScrollView {
            anchors.fill: parent
            ColumnLayout {
                width: parent.width
                spacing: 0

                Rectangle {
                    Layout.preferredHeight: 180
                    Layout.fillWidth: true
                    color: "#FF6200EE" // Accent color

                    Image {
                        source: "qrc:/Resources/wowpadx.png"
                        width: 120
                        height: 120
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 16
                    }
                }

                ListView {
                    id: drawerList
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    model: settingsMenuModel
                    delegate: ItemDelegate {
                        width: drawerList.width
                        height: 48
                        padding: 16
                        text: model.name
                        onClicked: {
                            leftDrawer.visible = false
                            pageCarousel.currentIndex = model.pageIndex
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: appBar
        height: 56
        width: parent.width
        color: "#FF3700B3" // PrimaryMid
        z: 1

        RowLayout {
            anchors.fill: parent
            anchors.margins: 16

            ToolButton {
                id: navDrawerSwitch
                checkable: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                contentItem: Item {
                        Shape {
                            width: 24
                            height: 24
                            transform: Translate {
                                x: 0
                                y: 0
                        }
                        anchors.centerIn: parent
                        ShapePath {
                            id: drawerIcon
                            fillColor: "white"
                            strokeWidth: 0
                            PathSvg {
                                path: "M3,6H21V8H3V6 M3,11H21V13H3V11 M3,16H21V18H3V16"
                            }
                        }
                    }
                }

                MouseArea {
                    id: mouseOver
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: leftDrawer.visible = true
                }
            }


            Item { Layout.preferredWidth: 32 } // Spacer

            Text {
                text: "Settings"
                font.pixelSize: 20
                color: "white"
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    SwipeView {
        id: pageCarousel
        anchors.top: appBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: 0
        interactive: false

        SettingsWoWmapper {}
        SettingsDevices {}
        SettingsKeybindings {}
        SettingsAnalog {}
        SettingsMemoryReading {}
        SettingsOverlay {}
    }
}
