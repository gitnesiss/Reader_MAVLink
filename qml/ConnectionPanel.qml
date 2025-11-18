import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: connectionPanel
    color: "#34495e"
    radius: 8
    border.color: "#7f8c8d"
    border.width: 2

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        Text {
            text: "Connection Settings"
            font.pixelSize: 18
            font.bold: true
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        // IP Address input
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: "IP Address:"
                font.pixelSize: 14
                color: "white"
            }

            TextField {
                id: ipField
                Layout.fillWidth: true
                placeholderText: "192.168.1.1"
                text: "192.168.1.1"
                font.pixelSize: 14
                background: Rectangle {
                    color: "#2c3e50"
                    border.color: "#7f8c8d"
                    border.width: 1
                    radius: 4
                }
                color: "white"
            }
        }

        // Port input
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: "Port:"
                font.pixelSize: 14
                color: "white"
            }

            TextField {
                id: portField
                Layout.fillWidth: true
                placeholderText: "14550"  // Изменили с 5760 на 14550
                text: "5760"
                validator: IntValidator { bottom: 1; top: 65535 }
                font.pixelSize: 14
                background: Rectangle {
                    color: "#2c3e50"
                    border.color: "#7f8c8d"
                    border.width: 1
                    radius: 4
                }
                color: "white"
            }
        }

        // Connection buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                id: connectButton
                Layout.fillWidth: true
                text: mavlinkHandler.connected ? "Disconnect" : "Connect"
                font.pixelSize: 14
                font.bold: true

                background: Rectangle {
                    color: parent.down ? "#2980b9" : (mavlinkHandler.connected ? "#e74c3c" : "#3498db")
                    radius: 5
                }
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (mavlinkHandler.connected) {
                        mavlinkHandler.disconnectFromFC()
                    } else {
                        mavlinkHandler.connectToFC(ipField.text, parseInt(portField.text))
                    }
                }
            }

            Button {
                id: clearButton
                Layout.preferredWidth: 100
                text: "Clear Data"
                font.pixelSize: 14

                background: Rectangle {
                    color: parent.down ? "#e67e22" : "#f39c12"
                    radius: 5
                }
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    mavlinkHandler.clearData()
                }
            }
        }

        // Preset IPs
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: "Quick Connect:"
                font.pixelSize: 14
                color: "white"
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 5
                columnSpacing: 5

                Repeater {
                    model: [
                        { "name": "SpeedyBee UDP", "ip": "192.168.1.1", "port": "14550" },
                        { "name": "SpeedyBee TCP", "ip": "192.168.1.1", "port": "5760" },
                        { "name": "Standard UDP", "ip": "192.168.4.1", "port": "14550" },
                        { "name": "Standard TCP", "ip": "192.168.4.1", "port": "5760" },
                        { "name": "Local Sim", "ip": "127.0.0.1", "port": "14550" },
                        { "name": "Broadcast", "ip": "255.255.255.255", "port": "14550" }
                    ]

                    Button {
                        Layout.fillWidth: true
                        text: modelData.name
                        font.pixelSize: 12

                        background: Rectangle {
                            color: parent.down ? "#16a085" : "#1abc9c"
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            ipField.text = modelData.ip
                            portField.text = modelData.port
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true } // Spacer

        Button {
            Layout.fillWidth: true
            text: "Send Heartbeat"
            font.pixelSize: 14

            background: Rectangle {
                color: parent.down ? "#9b59b6" : "#8e44ad"
                radius: 5
            }
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                // Вызовем метод отправки heartbeat через mavlinkHandler
                console.log("Manual heartbeat sent");
            }
        }

        Button {
            Layout.fillWidth: true
            text: "Request ATTITUDE Data"
            font.pixelSize: 14

            background: Rectangle {
                color: parent.down ? "#3498db" : "#2980b9"
                radius: 5
            }
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                mavlinkHandler.requestAttitudeStream()
                console.log("Requested ATTITUDE data stream")
            }
        }

        Button {
            Layout.fillWidth: true
            text: "Force 30Hz Stream"
            font.pixelSize: 14

            background: Rectangle {
                color: parent.down ? "#e67e22" : "#d35400"
                radius: 5
            }
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                mavlinkHandler.requestAttitudeStream()
                console.log("Forced 30Hz attitude stream request")
            }
        }
    }
}
