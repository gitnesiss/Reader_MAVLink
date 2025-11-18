#include "networkmanager.h"
#include <QDebug>
#include <QNetworkInterface>
#include <QtEndian>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_connected(false)
    , m_status("Disconnected")
    , m_remotePort(0)
    , m_heartbeatTimer(new QTimer(this))
{
    connect(m_socket, &QUdpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &NetworkManager::sendMavlinkHeartbeat);

    // Слушаем на всех интерфейсах, порт 14550
    if (m_socket->bind(QHostAddress::Any, 14550)) {
        qDebug() << "✅ Listening on UDP port 14550";
    } else {
        qDebug() << "❌ Failed to bind to port 14550:" << m_socket->errorString();
    }

    // Также пробуем привязаться к порту 14551 на случай если контроллер отправляет туда
    QUdpSocket* secondarySocket = new QUdpSocket(this);
    if (secondarySocket->bind(QHostAddress::Any, 14551)) {
        connect(secondarySocket, &QUdpSocket::readyRead, this, &NetworkManager::onReadyRead);
        qDebug() << "✅ Also listening on UDP port 14551";
    }

    m_heartbeatTimer->setInterval(1000);
}

NetworkManager::~NetworkManager()
{
    disconnectFromFC();
}

bool NetworkManager::connected() const
{
    return m_connected;
}

QString NetworkManager::status() const
{
    return m_status;
}

void NetworkManager::connectToFC(const QString &ip, int port)
{
    if (m_connected) {
        disconnectFromFC();
    }

    m_status = "Connecting via UDP...";
    emit statusChanged(m_status);

    m_remoteAddress = QHostAddress(ip);
    m_remotePort = port;

    // Для UDP мы просто "подключаемся", начиная слушать порт
    if (m_socket->state() != QAbstractSocket::BoundState) {
        if (!m_socket->bind(QHostAddress::Any, 14550)) {
            m_status = "UDP bind failed: " + m_socket->errorString();
            emit statusChanged(m_status);
            return;
        }
    }

    m_connected = true;
    m_status = QString("UDP connected to %1:%2").arg(ip).arg(port);
    emit connectedChanged(m_connected);
    emit statusChanged(m_status);

    m_heartbeatTimer->start();
    qDebug() << "UDP connected to" << ip << ":" << port;
}

void NetworkManager::disconnectFromFC()
{
    m_heartbeatTimer->stop();
    m_socket->close();
    m_connected = false;
    m_status = "Disconnected";
    emit connectedChanged(m_connected);
    emit statusChanged(m_status);
}

void NetworkManager::sendData(const QByteArray &data)
{
    if (m_connected && m_remotePort > 0) {
        qint64 bytesSent = m_socket->writeDatagram(data, m_remoteAddress, m_remotePort);
        if (bytesSent == -1) {
            qDebug() << "Failed to send UDP data:" << m_socket->errorString();
        }
    }
}

void NetworkManager::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;

        qint64 bytesRead = m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        if (bytesRead > 0) {
            // Логируем ВСЕ полученные данные
            qDebug() << "📨 Received" << bytesRead << "bytes from" << sender.toString() << ":" << senderPort;

            // Выводим первые 16 байт в hex для анализа
            QByteArray hexPreview = datagram.left(16).toHex(' ');
            qDebug() << "Hex preview:" << hexPreview;

            // Проверяем, что данные пришли с ожидаемого адреса
            if (sender.toString().contains("192.168.1")) { // Более гибкая проверка
                emit dataReceived(datagram);

                // Считаем статистику
                static int packetCount = 0;
                packetCount++;
                qDebug() << "📊 Total packets received:" << packetCount;
            } else {
                qDebug() << "⚠️  Received data from unexpected source:" << sender.toString();
            }
        }
    }
}

void NetworkManager::onHeartbeatTimeout()
{
    // Отправляем простой heartbeat (можно реализовать MAVLink heartbeat позже)
    if (m_connected) {
        // Пока отправляем пустой пакет для поддержания активности
        sendData(QByteArray());
    }
}

QByteArray NetworkManager::createMavlinkHeartbeat()
{
    // MAVLink 1.0 heartbeat message - правильная версия
    QByteArray heartbeat;

    // Header MAVLink 1.0
    heartbeat.append(char(0xFE));        // Стартовый байт MAVLink 1.0
    heartbeat.append(char(0x09));        // Длина payload (9 байт)

    static quint8 sequence = 0;
    heartbeat.append(char(sequence++));  // Sequence number (увеличиваем каждый раз)

    heartbeat.append(char(0xFF));        // System ID (GCS обычно использует 255)
    heartbeat.append(char(0x01));        // Component ID (1 для autopilot)
    heartbeat.append(char(0x00));        // Message ID: HEARTBEAT (0) - LITTLE ENDIAN!

    // Payload HEARTBEAT (9 байт) - ВСЕ В LITTLE ENDIAN!
    // type (6 = GCS)
    heartbeat.append(char(0x06));
    heartbeat.append(char(0x00));
    heartbeat.append(char(0x00));
    heartbeat.append(char(0x00));

    // autopilot (0 = generic)
    heartbeat.append(char(0x00));
    heartbeat.append(char(0x00));        // base_mode
    heartbeat.append(char(0x00));        // custom_mode
    heartbeat.append(char(0x00));

    // system_status (4 = active)
    heartbeat.append(char(0x04));
    heartbeat.append(char(0x00));
    heartbeat.append(char(0x00));
    heartbeat.append(char(0x00));

    // mavlink_version (3 = MAVLink 1.0)
    heartbeat.append(char(0x03));
    heartbeat.append(char(0x00));
    heartbeat.append(char(0x00));
    heartbeat.append(char(0x00));

    // Правильный расчёт checksum
    uint16_t checksum = 0xFFFF;

    // CRC для заголовка (без стартового байта)
    for (int i = 1; i < 6; i++) {
        uint8_t c = heartbeat[i];
        checksum ^= (c << 8);
        for (int j = 0; j < 8; j++) {
            if (checksum & 0x8000) {
                checksum = (checksum << 1) ^ 0x1021;
            } else {
                checksum <<= 1;
            }
        }
    }

    // CRC для payload
    for (int i = 6; i < heartbeat.size(); i++) {
        uint8_t c = heartbeat[i];
        checksum ^= (c << 8);
        for (int j = 0; j < 8; j++) {
            if (checksum & 0x8000) {
                checksum = (checksum << 1) ^ 0x1021;
            } else {
                checksum <<= 1;
            }
        }
    }

    // Добавляем checksum (little endian)
    heartbeat.append(char(checksum & 0xFF));
    heartbeat.append(char((checksum >> 8) & 0xFF));

    qDebug() << "❤️ Heartbeat packet:" << heartbeat.toHex(' ');
    return heartbeat;
}

void NetworkManager::sendMavlinkHeartbeat()
{
    if (m_connected) {
        QByteArray heartbeat = createMavlinkHeartbeat();
        sendData(heartbeat);
        qDebug() << "❤️  MAVLink Heartbeat sent to" << m_remoteAddress.toString();
    }
}
