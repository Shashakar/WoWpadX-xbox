import QtQuick.Controls.Material
import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import WoWpadX 1.0



Item {
    id: mainPage
    width: 300
    height: 500

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Controller Section
        Label { text: "Controller"; font.pixelSize: 20; }
        Rectangle {  Layout.preferredHeight: 2;  Layout.preferredWidth: parent.width; color: "#ccc" }

        ColumnLayout {
            spacing: 5
            Layout.leftMargin: 20
            Label { id: controllerStatus1; text: MainPageController.controllerStatus1 }
            Label { id: controllerStatus2; text: MainPageController.controllerStatus2 }
            Label {
                id: controllerStatus3
                text: MainPageController.controllerStatus3
                visible: MainPageController.controllerWarningVisible
                color: "red" // override for warning
            }
        }

        // WoW Section
        Label { text: "World of Warcraft"; font.pixelSize: 20; }
        Rectangle {  Layout.preferredHeight: 2;  Layout.preferredWidth: parent.width; color: "#ccc" }

        ColumnLayout {
                spacing: 5
                Layout.leftMargin: 20
            Label { id: wowStatus1; text: MainPageController.wowStatus1; }
            Label { id: wowStatus2; text: MainPageController.wowStatus2; }
        }

        // Updates Section
        Label { text: "Updates"; font.pixelSize: 20; }
        Rectangle {  Layout.preferredHeight: 2;  Layout.preferredWidth: parent.width; color: "#ccc" }

        RowLayout {
            spacing: 5
            Layout.leftMargin: 20
            Label {
                id: updateStatus1
                text: MainPageController.updateStatus
                color: MainPageController.updateStatusColor 
                font.underline: MainPageController.updateAvailable 
                MouseArea {
                    anchors.fill: parent 
                    cursorShape: MainPageController.updateAvailable ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: MainPageController.updateStatusClick()
                }
            }
            Image {
                id: updateIcon
                source: MainPageController.updateIconSource
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
            }
        }


        Rectangle {  Layout.preferredHeight: 2;  Layout.preferredWidth: parent.width; color: "#ccc" }

        // Donation Panel
        ColumnLayout {
            Layout.leftMargin: 40
            spacing: 5

            Label {
                text: "Thank you for using WoWpadX!"
                wrapMode: Text.Wrap
                horizontalAlignment: Qt.AlignHCenter
                Layout.preferredWidth: 200
            }

            Label {
                id: infoStatus1
                text: "Good luck, and happy adventuring!"
                wrapMode: Text.Wrap
                horizontalAlignment: Qt.AlignHCenter
                Layout.topMargin: 6
            }
        }

        // Bottom Section
        ColumnLayout { 
            spacing: 10

            Image {
                id: donateButton
                source: MainPageController.donateButtonSource
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.leftMargin: 124
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onEntered: MainPageController.donateButtonReaction(1)
                    onExited: MainPageController.donateButtonReaction(0)
                    onClicked: MainPageController.donateButtonClick()
                }
            }

            ColumnLayout {
                Layout.leftMargin: 73
                spacing: 5 
                Label { text: "Please report any issues!"; horizontalAlignment: Qt.AlignHCenter; }
            }
            
            ColumnLayout {
                Layout.leftMargin: 15
                spacing: 5 

                Label {
                    text: "https://github.com/leoaviana/WoWpadX/issues"
                    color: "cornflowerblue"
                    font.pixelSize: 12
                    font.underline: true
                    wrapMode: Text.Wrap
                    horizontalAlignment: Qt.AlignHCenter
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: gitLinkClick()
                    }
                }
            }

            ColumnLayout {
                Layout.leftMargin: 72
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

    function gitLinkClick() {
        Qt.openUrlExternally("https://github.com/leoaviana/WoWpadX/issues")
    }
}
