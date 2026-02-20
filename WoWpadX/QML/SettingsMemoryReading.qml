import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import QtQuick.Controls.Material
import WoWpadX 1.0

Item {
    id: settingsMemoryReading
    width: 323
    height: 500

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width

        ColumnLayout {
            anchors.margins: 5
            spacing: 10
            width: parent.width


            Label {
                text: "Pixel Bridge"
                font.pixelSize: 16
                Layout.leftMargin: 5

            }

            Rectangle {
                height: 1
                width: parent.width
                color: "white"
                Layout.margins: 5
            }

            // Warning Section
            ColumnLayout {
                id: warningColumn
                spacing: 5
                Layout.preferredHeight: 110
                Layout.leftMargin: 5
                Layout.rightMargin: 5

                Label {
                    text: "This feature reads information from \nWorld of Warcraft's window to assist with controller\ngameplay."
                    wrapMode: Text.Wrap
                }

                Label {
                    text: "This feature is experimental but safe to use.\nIt requires ConsolePortLK's Pixel Bridge\nfeature enabled in the addon settings."
                    wrapMode: Text.Wrap
                }
            }

            ColumnLayout {
                id: warningSection2
                spacing: 5
                Layout.preferredHeight: 60
                Layout.leftMargin: 5
                Layout.rightMargin: 5

                Label {
                    id: featureText
                    text: "This feature will only work when playing in \nwindowed and windowed fullscreen modes."
                    font.bold: true
                    wrapMode: Text.Wrap
                    color: "white"
                }

                Label {
                    text: "More information about this feature"
                    font.pixelSize: 13
                    color: "cornflowerblue"
                    //textDecoration: Text.Underline
                    horizontalAlignment: Text.AlignHCenter
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: MemoryReadingController.textMoreInfoClicked()
                    }
                }
            }

            // Content Section
            ColumnLayout {
                spacing: 10

                CheckBox {
                    text: "Enable Pixel Bridge"
                    checked: AppSettings.enableMemoryReading
                    onToggled: AppSettings.enableMemoryReading = checked
                    onCheckedChanged: {
                        AppSettings.enableMemoryReading = checked
                        checked ? warningColumn.visible = false : warningColumn.visible = true
                    }
                }

                Rectangle {
                    height: 1
                    width: parent.width
                    color: "white"
                    Layout.margins: 5
                }


                ColumnLayout {
                    spacing: 10
                    width: parent.width

                    TabBar {
                        id: tabBar
                        enabled: AppSettings.enableMemoryReading
                        Layout.preferredWidth: parent.width - 30

                        TabButton { text: "General" }
                        TabButton { text: "Input" }
                        TabButton { text: "Rumble" }
                        //TabButton { text: "Debug" }
                    }

                    StackLayout {
                        id: tabStack
                        enabled: AppSettings.enableMemoryReading
                        currentIndex: tabBar.currentIndex
                        Layout.fillWidth: true

                        ColumnLayout {
                            spacing: 5

                            CheckBox {
                                text: "Cancel mouselook when alt-tabbed"
                                checked: AppSettings.memoryAutoCancel
                                onToggled: AppSettings.memoryAutoCancel = checked

                                ToolTip.visible: hovered
                                ToolTip.delay: 500 
                                ToolTip.text: "Automatically cancels mouselook in-game when the game window loses focus."
                            }

                            CheckBox {
                                text: "Show cursor position on overlay during\nmouselook"
                                checked: AppSettings.enableOverlayCrosshair
                                onToggled: AppSettings.enableOverlayCrosshair = checked
                                enabled: AppSettings.enableOverlay
                                visible: AppSettings.enableOverlay

                                ToolTip.visible: hovered
                                ToolTip.delay: 500 
                                ToolTip.text: "Shows a crosshair on the overlay that indicates the cursor position while mouselooking."
                            }
                        }

                        ColumnLayout {
                            spacing: 5

                            CheckBox {
                                text: "Auto-center mouse cursor after mouselook"
                                checked: AppSettings.memoryAutoCenter
                                onToggled: AppSettings.memoryAutoCenter = checked

                                ToolTip.visible: hovered
                                ToolTip.delay: 500 
                                ToolTip.text: "Moves the mouse cursor to the center of the screen after mouselook has been active for the configured minimum duration."
                            }

                            RowLayout {
                                spacing: 5
                                Label { text: "Trigger after" }
                                Label { text: AppSettings.memoryAutoCenterDelay.toString() }
                                Label { text: "ms" }
                            }

                            Slider {
                                from: 0
                                to: 10000
                                value: AppSettings.memoryAutoCenterDelay                                
                                onValueChanged: AppSettings.memoryAutoCenterDelay = value
                                stepSize: 1
                                width: 150
                            }


                                RowLayout {
                                    CheckBox {
                                        text: "Auto-toggle run/walk at"
                                        checked: AppSettings.memoryAutoWalk                                        
                                        onToggled: AppSettings.memoryAutoWalk = checked

                                        ToolTip.visible: hovered
                                        ToolTip.delay: 500 
                                        ToolTip.text: "Confirm and cancel casting targeted AoE spells using buttons instead of mouse clicks."
                                    }
                                    Label { text: AppSettings.walkThreshold.toString() }
                                    Label { text: "%" }

                                }

                            Slider {
                                from: 35
                                to: 120
                                value: AppSettings.walkThreshold                                
                                onValueChanged: AppSettings.walkThreshold = value
                                stepSize: 1
                                width: 100
                                enabled: AppSettings.memoryAutoWalk
                            }

                            CheckBox {
                                text: "Invert left/right turn while mouselooking"
                                checked: AppSettings.memoryInvertTurn
                                onToggled: AppSettings.memoryInvertTurn = checked
                            }

                            CheckBox {
                                text: "Disable touchpad while mouselook is active"
                                enabled: true
                                checked: AppSettings.memoryTouchpadCursorOnly
                                onToggled: AppSettings.memoryTouchpadCursorOnly = checked
                            }

                            CheckBox {
                                text: "Override buttons when casting AoE spells"
                                checked: AppSettings.memoryOverrideAoeCast
                                onToggled: AppSettings.memoryOverrideAoeCast = checked

                                ToolTip.visible: hovered
                                ToolTip.delay: 500       
                                ToolTip.text: "Confirm and cancel casting targeted AoE spells using buttons instead of mouse clicks."
                            }

                            ColumnLayout {
                                spacing: 5
                                Label { text: "Cast Spell" }
                                ComboBox {
                                    id: aoeConfirm
                                    model: MemoryReadingController.aoeConfirmModel
                                    currentIndex: MemoryReadingController.aoeConfirmIndex
                                    textRole: "label"
                                    contentItem: RowLayout {
                                        spacing: 5
                                        Image {
                                            source: MemoryReadingController.aoeConfirmModel.get(aoeConfirm.currentIndex).buttonImg
                                            width: 18; height: 18
                                        }
                                        Label {
                                            text: MemoryReadingController.aoeConfirmModel.get(aoeConfirm.currentIndex).label
                                        }
                                    }
                                    delegate: ItemDelegate {
                                        width: parent.width
                                        RowLayout {
                                            spacing: 5
                                            Image {
                                                source: buttonImg
                                                width: 18; height: 18
                                            }
                                            Label { text: label }
                                        }
                                    }
                                    onCurrentIndexChanged: { 
                                        MemoryReadingController.aoeOverrideChanged(currentIndex, aoeCancel.currentIndex)
                                    }
                                }

                                Label { text: "Cancel" }
                                ComboBox {
                                    id: aoeCancel
                                    model: MemoryReadingController.aoeCancelModel
                                    currentIndex: MemoryReadingController.aoeCancelIndex
                                    textRole: "label"
                                    contentItem: RowLayout {
                                        spacing: 5
                                        Image {
                                            source: MemoryReadingController.aoeCancelModel.get(aoeCancel.currentIndex).buttonImg
                                            width: 18; height: 18
                                        }
                                        Label {
                                            text: MemoryReadingController.aoeCancelModel.get(aoeCancel.currentIndex).label
                                        }
                                    }
                                    delegate: ItemDelegate {
                                        width: parent.width
                                        RowLayout {
                                            spacing: 5
                                            Image {
                                                source: buttonImg
                                                width: 18; height: 18
                                            }
                                            Label { text: label }
                                        }
                                    }
                                    onCurrentIndexChanged: MemoryReadingController.aoeOverrideChanged(aoeConfirm.currentIndex, currentIndex)
                                }
                            }
                        }

                        // Feedback Tab
                        ColumnLayout {
                            spacing: 5

                            CheckBox {
                                text: "Vibrate controller when taking damage"
                                checked: AppSettings.memoryVibrationDamage
                                onToggled: AppSettings.memoryVibrationDamage = checked

                                ToolTip.visible: hovered
                                ToolTip.delay: 500       
                                ToolTip.text: "Vibrates the controller based on the amount of damage your character is taking."
                            } 

                            CheckBox {
                                text: "Colour lightbar by health percentage"
                                checked: AppSettings.memoryLightbar
                                onToggled: AppSettings.memoryLightbar = checked
                                enabled: false
                                visible: false

                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                                ToolTip.text: "Colours the DualShock 4 lightbar based on the amount of health you have remaining."
                            }
                        }


                        /*
                        ColumnLayout {
                            spacing: 5

                            Item {
                                width: 320
                                Layout.preferredHeight: 30 + (MemoryReadingController.debugModel.count * 30)

                                Row {
                                    spacing: 1
                                    Rectangle {
                                        width: 100; height: 30; color: "#333"
                                        Text { anchors.centerIn: parent; text: "Name"; color: "white" }
                                    }
                                    Rectangle {
                                        width: 90; height: 30; color: "#333"
                                        Text { anchors.centerIn: parent; text: "Value"; color: "white" }
                                    }
                                }

                                ListView {
                                    id: listDebug
                                    anchors.top: parent.top
                                    anchors.topMargin: 35
                                    model: MemoryReadingController.debugModel
                                    width: parent.width
                                    height: parent.height - 35
                                    clip: true
                                    delegate: Row {
                                        spacing: 1
                                        Rectangle {
                                            width: 100; height: 30; color: "#555"
                                            Text { anchors.centerIn: parent; text: model.name; color: "white" }
                                        }
                                        Rectangle {
                                            width: 90; height: 30; color: "#555"
                                            Text { anchors.centerIn: parent; text: model.value; color: "white" }
                                        }
                                    }
                                }
                                ListModel {
                                    id: debugModel
                                    ListElement { name: "Health"; value: "100" }
                                    ListElement { name: "Mana"; value: "50" }
                                    ListElement { name: "Stamina"; value: "75" }
                                }
                            }

                            Button {
                                text: "Refresh Values"
                                onClicked: MemoryReadingController.buttonRefreshValuesClicked()
                            }
                        }
                        */
                    }
                }
            }
        }
    }
}
