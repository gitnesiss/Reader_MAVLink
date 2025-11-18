import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: dataDisplay
    color: "#34495e"
    radius: 8
    border.color: "#7f8c8d"
    border.width: 2

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        Text {
            text: "Flight Data"
            font.pixelSize: 18
            font.bold: true
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        // Attitude display
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160  // Увеличили высоту для нового поля
            color: "#2c3e50"
            radius: 6
            border.color: "#7f8c8d"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Text {
                    text: "Attitude (Euler Angles)"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#3498db"
                    Layout.alignment: Qt.AlignHCenter
                }

                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 2
                    rowSpacing: 5
                    columnSpacing: 10

                    Text { text: "Roll:"; font.pixelSize: 14; color: "white"; Layout.alignment: Qt.AlignRight }
                    Text {
                        text: mavlinkHandler.attitude.roll.toFixed(2) + "°"
                        font.pixelSize: 14; font.bold: true; color: "#e74c3c"
                    }

                    Text { text: "Pitch:"; font.pixelSize: 14; color: "white"; Layout.alignment: Qt.AlignRight }
                    Text {
                        text: mavlinkHandler.attitude.pitch.toFixed(2) + "°"
                        font.pixelSize: 14; font.bold: true; color: "#2ecc71"
                    }

                    Text { text: "Yaw:"; font.pixelSize: 14; color: "white"; Layout.alignment: Qt.AlignRight }
                    Text {
                        text: mavlinkHandler.attitude.yaw.toFixed(2) + "°"
                        font.pixelSize: 14; font.bold: true; color: "#f39c12"
                    }

                    Text { text: "Timestamp:"; font.pixelSize: 14; color: "white"; Layout.alignment: Qt.AlignRight }
                    Text {
                        text: mavlinkHandler.attitude.timestamp + " ms"
                        font.pixelSize: 14; color: "#bdc3c7"
                    }

                    // НОВОЕ: Отображение частоты
                    Text { text: "Frequency:"; font.pixelSize: 14; color: "white"; Layout.alignment: Qt.AlignRight }
                    Text {
                        text: mavlinkHandler.attitudeFrequency + " Hz"
                        font.pixelSize: 14; font.bold: true; color: "#9b59b6"
                    }
                }
            }
        }

        // Message log
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            color: "#2c3e50"
            radius: 6
            border.color: "#7f8c8d"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Text {
                    text: "Message Log"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#3498db"
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        id: messageLog
                        readOnly: true
                        font.pixelSize: 12
                        font.family: "Courier New"
                        color: "#ecf0f1"
                        background: Rectangle { color: "transparent" }
                    }
                }
            }
        }

        // Raw data display
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#2c3e50"
            radius: 6
            border.color: "#7f8c8d"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Text {
                    text: "Raw MAVLink Data"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#3498db"
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        id: rawDataDisplay
                        text: mavlinkHandler.rawData
                        readOnly: true
                        font.pixelSize: 10
                        font.family: "Courier New"
                        color: "#bdc3c7"
                        background: Rectangle { color: "transparent" }
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }
    }

    // Connect to new messages
    Connections {
        target: mavlinkHandler
        function onNewMessage(message) {
            var timestamp = new Date().toLocaleTimeString();
            messageLog.text += "[" + timestamp + "] " + message + "\n";

            // Auto-scroll to bottom
            messageLog.cursorPosition = messageLog.text.length;
        }

        function onAttitudeFrequencyChanged(frequency) {
            // Можно добавить дополнительную логику при изменении частоты
            console.log("Attitude frequency changed to:", frequency + "Hz");
        }
    }
}
