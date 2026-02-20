import QtQuick.Controls.Material
import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import WoWpadX 1.0

Item {
    id: settingsDevices
    width: 313
    height: 421

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width

        ColumnLayout {
            anchors.margins: 10
            spacing: 10
            width: parent.width

            // Selected Device Section
            Label { text: "Selected Device"; font.pixelSize: 16; color: "white" }
            Rectangle { height: 1; width: parent.width; color: "white" }

            Item {
                Layout.preferredHeight: 40
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.preferredWidth: parent.width - 16

                ListView {
                    id: selectedDeviceLoader
                    anchors.fill: parent
                    model: DevicesController.selectedDeviceModel
                    delegate: selectedDeviceComponent
                    visible: DevicesController.selectedDeviceModel.count > 0
                }

                Label {
                    text: "No Active Controller"
                    anchors.centerIn: parent
                    visible: DevicesController.selectedDeviceModel.count === 0
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // Available Devices Section
            Label { text: "Available Devices"; font.pixelSize: 16; color: "white" }

            ListView {
                id: availableDevicesList
                currentIndex: -1
                model: DevicesController.availableDevicesModel
                delegate: controllerDelegate
                clip: true
                spacing: 2
                leftMargin: 8
                rightMargin: 8
                width: parent.width - 14
                Layout.preferredHeight: Math.min(DevicesController.availableDevicesModel.count * 42, 200)
            }

            Button {
                text: "Use Selected Controller"
                Layout.alignment: Qt.AlignHCenter
                onClicked: DevicesController.selectDevice(availableDevicesList.currentIndex)
            }
        }
    }

    // Controller Item Template
    Component {
        id: controllerDelegate

        Rectangle {
            id: itemRect
            width: parent.width
            height: 40
            color: ListView.isCurrentItem ? Material.scrollBarPressedColor : Material.dividerColor
            opacity: 1

            MouseArea {
                hoverEnabled: true
                anchors.fill: parent
                onEntered: { itemRect.opacity = 0.5 }
                onExited: { itemRect.opacity = 1 }
                onClicked: { 
                    availableDevicesList.currentIndex = index
                }
            }


            RowLayout {
                anchors.fill: parent
                spacing: 10

                Image {
                    source: image
                    width: 24
                    height: 24
                    fillMode: Image.PreserveAspectFit
                }

                Label {
                    text: type
                    Layout.preferredWidth: 80
                    verticalAlignment: Text.AlignVCenter
                }

                Label {
                    text: name
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    Component {
        id: selectedDeviceComponent

        Rectangle {
            width: parent.width
            height: 40
            color: Material.dividerColor
            opacity: 1

            RowLayout {
                anchors.fill: parent
                spacing: 10

                Image {
                    source: image
                    width: 24
                    height: 24
                    fillMode: Image.PreserveAspectFit
                }

                Label {
                    text: type
                    Layout.preferredWidth: 80
                    verticalAlignment: Text.AlignVCenter
                }

                Label {
                    text: name
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
