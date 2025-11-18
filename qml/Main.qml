import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1200  // Увеличим ширину окна для лучшего отображения
    height: 700
    title: "MAVLink Reader - SpeedyBee F405 Wing"
    visible: true

    // Background gradient
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#2c3e50" }
            GradientStop { position: 1.0; color: "#34495e" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Header
        Text {
            Layout.fillWidth: true
            text: "MAVLink Reader - SpeedyBee F405 Wing"
            font.pixelSize: 24
            font.bold: true
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            padding: 10
        }

        // Connection status
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: mavlinkHandler.connected ? "#27ae60" : "#e74c3c"
            radius: 5

            Text {
                anchors.centerIn: parent
                text: mavlinkHandler.connected ? "CONNECTED" : "DISCONNECTED"
                font.pixelSize: 16
                font.bold: true
                color: "white"
            }
        }

        // Status text
        Text {
            Layout.fillWidth: true
            text: "Status: " + mavlinkHandler.status
            font.pixelSize: 14
            color: "white"
            padding: 5
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // Левая панель - Connection и Parameters (фиксированная ширина)
            Rectangle {
                Layout.preferredWidth: 350  // Фиксированная ширина левой панели
                Layout.preferredHeight: 600
                // Layout.fillHeight: true
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    ConnectionPanel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 400  // Фиксированная высота для ConnectionPanel
                    }

                    // Кнопка для показа/скрытия advanced settings
                    Button {
                        Layout.fillWidth: true
                        text: "Advanced Settings ▼"
                        onClicked: {
                            parameterPanel.visible = !parameterPanel.visible
                            text = parameterPanel.visible ? "Advanced Settings ▲" : "Advanced Settings ▼"
                        }
                        background: Rectangle {
                            color: parent.down ? "#7f8c8d" : "#95a5a6"
                            radius: 5
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    ParameterPanel {
                        id: parameterPanel
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0  // Начинаем с нулевой высоты
                        visible: false
                    }

                    // Spacer чтобы занять оставшееся место
                    Item {
                        Layout.fillHeight: true
                    }
                }
            }

            // Правая панель - Data display (занимает всё оставшееся пространство)
            DataDisplay {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
