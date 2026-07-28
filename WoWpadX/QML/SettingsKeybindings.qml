import QtQuick.Controls.Material
import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import WoWpadX 1.0

Item {
    id: settingsKeybindings
    width: 313
    height: 421

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width

        ColumnLayout {
            anchors.margins: 10
            spacing: 10
            width: parent.width

            Label { text: "Controller Settings"; font.pixelSize: 16 }
            Rectangle { height: 1; width: parent.width }

            ColumnLayout {
                Layout.preferredWidth: parent.width
                spacing: 5
                Label { text: "Modifier Buttons" }
                ComboBox {
                    Layout.preferredWidth: parent.width - 50
                    model: [
                        "Default (L1+L2)",
                        "Triggers (L2+R2)",
                        "Reversed (R1+R2)",
                        "Shoulders (L1+R1)"
                    ]
                    currentIndex: AppSettings.modifierStyle
                    onActivated: {
                        // A selected preset overrides stale hidden custom bindings.
                        AppSettings.customBindings = false
                        AppSettings.modifierStyle = currentIndex
                        AppSettings.save()
                    }
                }
            }

            ColumnLayout {
                Layout.preferredWidth: parent.width
                spacing: 5
                Label { text: "Displayed Icons" }
                ComboBox {
                    enabled: false
                    Layout.preferredWidth: parent.width - 50
                    model: ["Auto Detect", "PlayStation", "Xbox"]
                    currentIndex: AppSettings.buttonStyle
                    onCurrentIndexChanged: AppSettings.buttonStyle = currentIndex
                }
            }

            /*

            I won't implement that, at least for now... I don't see why changing keyboard bindings makes sense and I
            don't think I need to expose real keyboard mappings to the user, just let ConsolePortLK handle the default ones...


            Label { text: "Binding Settings"; font.pixelSize: 16 }
            Rectangle { height: 1; width: parent.width }

            Label {
                text: appSettings.syncMessage
                wrapMode: Text.Wrap
            }

            CheckBox {
                text: "Send key binds directly to WoW"
                checked: appSettings.inputDirectKeyboard
                onToggled: appSettings.inputDirectKeyboard = checked
            }

            RowLayout {
                spacing: 10
                CheckBox {
                    text: "Manually define key bindings"
                    checked: AppSettings.customBindings
                    onToggled: AppSettings.customBindings = checked
                }

                Button {
                    width: 24
                    height: 24
                    padding: 3
                    ToolTip.text: "Reload default bindings"
                    onClicked: KeybindingsController.resetBindings()
                    contentItem: Image {
                        source: "qrc:/Resources/reloadbinds.png"
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }

            ListView {
                id: keybindList
                model: KeybindingsController.keybindModel
                delegate: keybindDelegate
                spacing: 2
                leftMargin: 8
                width: parent.width - 8
                height: Math.min(KeybindingsController.keybindModel.count * 42, 200)
                clip: true
            }

            */
        }
    }

    Component {
        id: keybindDelegate

        Rectangle {
            width: parent.width
            height: 40
            color: Material.dividerColor
            opacity: 1

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onDoubleClicked: {
                    // Handle double-click logic here
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
                    Layout.alignment: Qt.AlignVCenter
                }

                Label {
                    text: bindType
                    Layout.preferredWidth: 80
                    font.pixelSize: 14
                    color: "white"
                }

                Label {
                    text: name
                    Layout.preferredWidth: 80
                    font.pixelSize: 14
                    color: "white"
                }

                Label {
                    text: key
                    Layout.preferredWidth: 60
                    font.pixelSize: 14
                    color: "white"
                }
            }
        }
    }
}
