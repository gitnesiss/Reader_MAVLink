import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: parameterPanel
    color: "#34495e"
    radius: 8
    border.color: "#7f8c8d"
    border.width: 2

    property bool expanded: false

    // Анимация для плавного изменения высоты
    Behavior on height {
        NumberAnimation { duration: 300 }
    }

    onVisibleChanged: {
        if (visible) {
            height = 200  // Высота когда видима
            expanded = true
        } else {
            height = 0    // Скрыта
            expanded = false
        }
    }

    Component.onCompleted: {
        height = 0  // Начинаем скрытой
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Text {
            text: "FCU Parameters"
            font.pixelSize: 16
            font.bold: true
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        // Простая настройка частоты
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: "ATT Rate:"
                color: "white"
                font.pixelSize: 12
                Layout.preferredWidth: 80
            }

            TextField {
                id: attitudeRate
                text: "30"
                validator: IntValidator { bottom: 1; top: 100 }
                Layout.fillWidth: true
                background: Rectangle {
                    color: "#2c3e50"
                    border.color: "#7f8c8d"
                    radius: 4
                }
                color: "white"
            }

            Button {
                text: "Apply"
                Layout.preferredWidth: 80
                onClicked: {
                    mavlinkHandler.setStreamRates(parseInt(attitudeRate.text), 5)
                }
                background: Rectangle {
                    color: parent.down ? "#27ae60" : "#2ecc71"
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 12
                }
            }
        }

        // Кнопки управления
        RowLayout {
            Layout.fillWidth: true
            spacing: 5

            Button {
                text: "High Rate"
                Layout.fillWidth: true
                onClicked: {
                    mavlinkHandler.enableHighRateMode()
                }
                background: Rectangle {
                    color: parent.down ? "#c0392b" : "#e74c3c"
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 12
                }
            }

            Button {
                text: "Reset"
                Layout.fillWidth: true
                onClicked: {
                    mavlinkHandler.resetStreamingToDefaults()
                }
                background: Rectangle {
                    color: parent.down ? "#7f8c8d" : "#95a5a6"
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 12
                }
            }
        }
    }
}
