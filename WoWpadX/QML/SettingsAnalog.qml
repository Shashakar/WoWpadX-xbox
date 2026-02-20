import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import WoWpadX 1.0

Item {
    id: settingsAnalog
    width: 313
    height: 421

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Trigger Sensitivity
        Label { text: "Trigger Sensitivity"; font.pixelSize: 16; }
        Rectangle { height: 1; width: parent.width - 12; color: "#ccc" }

        Repeater {
            model: [
                { label: "Left", value: "triggerThresholdLeft", min: 20, max: 250, tip: "The trigger sensitivity for the left trigger." },
                { label: "Right", value: "triggerThresholdRight", min: 20, max: 250, tip: "The trigger sensitivity for the right trigger." }
            ]
            delegate: GridLayout {
                columns: 3
                columnSpacing: 10
                Layout.fillWidth: true

                Label { text: modelData.label; Layout.preferredWidth: 45}

                Slider {
                    id: sliderTrigger
                    from: modelData.min
                    to: modelData.max
                    value: AppSettings.getProperty(modelData.value)
                    onValueChanged: AppSettings.setPropertyValue(modelData.value, value)

                    ToolTip.visible: hovered
                    ToolTip.delay: 500 
                    ToolTip.text: modelData.tip
                }

                Label {
                    text: Math.round(sliderTrigger.value).toString()
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // Cursor and Camera
        Label { text: "Cursor and Camera"; font.pixelSize: 16; color: "white" }
        Rectangle { height: 1; width: parent.width - 12; color: "#ccc" }

        Repeater {
            model: [
                { label: "Deadzone", value: "cursorDeadzone", min: 1, max: 100, tip: "The radius from the center of the axis which will not cause cursor movement."},
                { label: "Speed", value: "cursorSpeed", min: 1, max: 30, tip: "The overall rate at which the cursor will move. Increase this to make the cursor move faster."},
                { label: "Curve", value: "cursorCurve", min: 1, max: 10, tip: "The input curve of the analog stick. Increase this to speed up cursor movement when less input is applied."}
            ]
            delegate: GridLayout {
                columns: 3
                columnSpacing: 10
                Layout.fillWidth: true

                Label { text: modelData.label; Layout.preferredWidth: 45 }

                Slider {
                    id: sliderCursor
                    from: modelData.min
                    to: modelData.max
                    value: AppSettings.getProperty(modelData.value)
                    onValueChanged: AppSettings.setPropertyValue(modelData.value, value)

                    ToolTip.visible: hovered
                    ToolTip.delay: 500 
                    ToolTip.text: modelData.tip
                }

                Label {
                    text: Math.round(sliderCursor.value).toString()
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // Touchpad
        //
        // I can't implement that right now because I don't have a gamepad which has touchpad, so I cannot test it, maybe soon...
        //
        Label { text: "Touchpad"; font.pixelSize: 16 }
        Rectangle { height: 1; width: parent.width - 12; color: "#ccc" }

        CheckBox {
            text: "Enable touchpad cursor control (untested)"
            enabled: true
            checked: AppSettings.enableTouchpad
            onToggled: AppSettings.enableTouchpad = checked

            ToolTip.visible: hovered
            ToolTip.delay: 500 
            ToolTip.text: "Enables or disables your controller touchpad (if present)"
        }

        RowLayout {
            spacing: 10
            Label { text: "Touchpad Buttons" }

            ComboBox {
                enabled: AppSettings.enableTouchpad
                model: ["Left/Right Click", "Share/Options"]
                currentIndex: AppSettings.touchpadMode
                onCurrentIndexChanged: AppSettings.touchpadMode = currentIndex
                Layout.preferredWidth: 140

            }
        }

        // Miscellaneous
        Label { text: "Miscellaneous"; font.pixelSize: 16; }
        Rectangle { height: 1; width: parent.width - 12; color: "#ccc" }

        CheckBox {
            text: "Orthodox movement controls (8-way)" 
            checked: AppSettings.simpleRadial
            onToggled: AppSettings.simpleRadial = checked
        }

        CheckBox {
            text: "Swap Sticks" 
            checked: AppSettings.swapSticks
            onToggled: AppSettings.swapSticks = checked
        }

        CheckBox {
            text: "Simulate hardware mouse input"
            enabled: false
            checked: true //AppSettings.inputHardwareMouse
            //onToggled: AppSettings.inputHardwareMouse = checked
        }
    } }
}
