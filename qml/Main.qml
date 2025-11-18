import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 800
    height: 600
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

            // Left panel - Connection and controls
            ConnectionPanel {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
            }

            // Right panel - Data display
            DataDisplay {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
