#include "mavlinkhandler.h"
#include <QDebug>
#include <QtEndian>
#include <QDateTime>

MavlinkHandler::MavlinkHandler(QObject *parent)
    : QObject(parent)
    , m_networkManager(new NetworkManager(this))
    , m_rawData("No data received")
    , m_attitudeFrequency(0)
    , m_lastAttitudeTime(0)
{
    connect(m_networkManager, &NetworkManager::dataReceived,
            this, &MavlinkHandler::onNetworkDataReceived);
    connect(m_networkManager, &NetworkManager::connectedChanged,
            this, &MavlinkHandler::onNetworkConnectedChanged);
    connect(m_networkManager, &NetworkManager::statusChanged,
            this, &MavlinkHandler::onNetworkStatusChanged);

    // Таймер для расчета частоты обновления
    m_frequencyTimer = new QTimer(this);
    connect(m_frequencyTimer, &QTimer::timeout, this, &MavlinkHandler::updateFrequency);
    m_frequencyTimer->start(1000); // Обновляем частоту каждую секунду
}

MavlinkHandler::~MavlinkHandler()
{
    disconnectFromFC();
}

// Добавляем свойство для частоты
int MavlinkHandler::attitudeFrequency() const
{
    return m_attitudeFrequency;
}

bool MavlinkHandler::connected() const
{
    return m_networkManager->connected();
}

QString MavlinkHandler::status() const
{
    return m_networkManager->status();
}

MavlinkAttitude MavlinkHandler::attitude() const
{
    return m_currentAttitude;
}

QString MavlinkHandler::rawData() const
{
    return m_rawData;
}

void MavlinkHandler::connectToFC(const QString &ip, int port)
{
    int actualPort = (port == 5760) ? 14550 : port;
    m_networkManager->connectToFC(ip, actualPort);

    // Через 2 секунды после подключения запрашиваем поток ATTITUDE
    QTimer::singleShot(2000, this, &MavlinkHandler::requestAttitudeStream);
}

void MavlinkHandler::disconnectFromFC()
{
    m_networkManager->disconnectFromFC();
}

void MavlinkHandler::clearData()
{
    m_rawData.clear();
    emit rawDataChanged(m_rawData);
}

void MavlinkHandler::onNetworkDataReceived(const QByteArray &data)
{
    // Add to buffer for parsing
    m_buffer.append(data);

    // Update raw data display
    m_rawData = QString::fromLatin1(data.toHex(' '));
    emit rawDataChanged(m_rawData);

    // Parse MAVLink messages
    parseMavlinkMessage(m_buffer);

    // Keep buffer reasonable size
    if (m_buffer.size() > 4096) {
        m_buffer = m_buffer.right(2048);
    }
}

void MavlinkHandler::onNetworkConnectedChanged(bool connected)
{
    emit connectedChanged(connected);
}

void MavlinkHandler::onNetworkStatusChanged(const QString &status)
{
    emit statusChanged(status);
}

void MavlinkHandler::parseMavlinkMessage(const QByteArray &data)
{
    qDebug() << "🔍 Parsing" << data.size() << "bytes of data";

    if (data.size() > 0) {
        QByteArray hexPreview = data.left(16).toHex(' ');
        qDebug() << "📊 Data hex preview:" << hexPreview;
    }

    int i = 0;
    while (i < data.size()) {
        quint8 start_byte = static_cast<quint8>(data[i]);

        // MAVLink 2.0
        if (start_byte == 0xFD && (i + 12) < data.size()) {
            quint8 payload_len = static_cast<quint8>(data[i + 1]);
            quint8 incompat_flags = static_cast<quint8>(data[i + 2]);
            quint8 compat_flags = static_cast<quint8>(data[i + 3]);
            quint8 seq = static_cast<quint8>(data[i + 4]);
            quint8 sysid = static_cast<quint8>(data[i + 5]);
            quint8 compid = static_cast<quint8>(data[i + 6]);

            // Message ID - 3 bytes little endian
            quint32 msg_id = static_cast<quint32>(data[i + 7]) |
                             (static_cast<quint32>(data[i + 8]) << 8) |
                             (static_cast<quint32>(data[i + 9]) << 16);

            int total_len = 10 + payload_len + 2; // Header + payload + checksum

            // Проверяем, что сообщение полностью в буфере
            if (i + total_len <= data.size()) {
                qDebug() << "🎯 MAVLink 2.0 message - ID:" << msg_id << "Length:" << payload_len;

                if (msg_id == 30) { // ATTITUDE
                    qDebug() << "🎉 Found ATTITUDE message!";
                    MavlinkAttitude attitude = parseAttitudeMessage(data, i + 10);
                    if (attitude.timestamp != 0) {
                        m_currentAttitude = attitude;
                        emit attitudeChanged(m_currentAttitude);

                        QString msg = QString("ATTITUDE: Roll=%1°, Pitch=%2°, Yaw=%3°")
                                          .arg(attitude.roll, 0, 'f', 2)
                                          .arg(attitude.pitch, 0, 'f', 2)
                                          .arg(attitude.yaw, 0, 'f', 2);
                        emit newMessage(msg);
                        qDebug() << msg;
                    }
                } else if (msg_id == 0) { // HEARTBEAT
                    qDebug() << "💓 HEARTBEAT from system" << sysid;
                } else if (msg_id == 1) { // SYS_STATUS
                    qDebug() << "📊 SYS_STATUS message";
                } else {
                    qDebug() << "📨 Other MAVLink 2.0 message, ID:" << msg_id;
                }

                i += total_len; // Переходим к следующему сообщению
                continue;
            }
        }
        // MAVLink 1.0
        else if (start_byte == 0xFE && (i + 6) < data.size()) {
            quint8 payload_len = static_cast<quint8>(data[i + 1]);
            quint8 seq = static_cast<quint8>(data[i + 2]);
            quint8 sysid = static_cast<quint8>(data[i + 3]);
            quint8 compid = static_cast<quint8>(data[i + 4]);
            quint8 msg_id = static_cast<quint8>(data[i + 5]);

            int total_len = 6 + payload_len + 2; // Header + payload + checksum

            if (i + total_len <= data.size()) {
                qDebug() << "🎯 MAVLink 1.0 message - ID:" << msg_id << "Length:" << payload_len;

                if (msg_id == 30) { // ATTITUDE
                    qDebug() << "🎉 Found ATTITUDE message!";
                    MavlinkAttitude attitude = parseAttitudeMessage(data, i + 6);
                    if (attitude.timestamp != 0) {
                        m_currentAttitude = attitude;
                        emit attitudeChanged(m_currentAttitude);

                        QString msg = QString("ATTITUDE: Roll=%1°, Pitch=%2°, Yaw=%3°")
                                          .arg(attitude.roll, 0, 'f', 2)
                                          .arg(attitude.pitch, 0, 'f', 2)
                                          .arg(attitude.yaw, 0, 'f', 2);
                        emit newMessage(msg);
                        qDebug() << msg;
                    }
                }

                i += total_len;
                continue;
            }
        }

        i++; // Продолжаем поиск
    }

    // Сохраняем необработанные данные для следующего вызова
    if (i < data.size()) {
        m_buffer = data.mid(i);
    } else {
        m_buffer.clear();
    }
}





// В методе parseAttitudeMessage добавляем подсчет частоты
MavlinkAttitude MavlinkHandler::parseAttitudeMessage(const QByteArray &data, int startPos)
{
    MavlinkAttitude attitude;

    if (startPos + 28 <= data.size()) { // ATTITUDE message is 28 bytes
        // Parse time_boot_ms (uint32_t, bytes 0-3)
        attitude.timestamp = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(data.constData() + startPos));

        // Parse roll (float, bytes 4-7)
        float roll = 0.0f;
        memcpy(&roll, data.constData() + startPos + 4, sizeof(float));
        attitude.roll = static_cast<double>(qFromLittleEndian<float>(roll)) * 180.0 / M_PI;

        // Parse pitch (float, bytes 8-11)
        float pitch = 0.0f;
        memcpy(&pitch, data.constData() + startPos + 8, sizeof(float));
        attitude.pitch = static_cast<double>(qFromLittleEndian<float>(pitch)) * 180.0 / M_PI;

        // Parse yaw (float, bytes 12-15)
        float yaw = 0.0f;
        memcpy(&yaw, data.constData() + startPos + 12, sizeof(float));
        attitude.yaw = static_cast<double>(qFromLittleEndian<float>(yaw)) * 180.0 / M_PI;

        // Подсчитываем частоту
        m_attitudeCount++;
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        // Логируем только каждое 10-е сообщение чтобы не засорять консоль
        if (m_attitudeCount % 10 == 0) {
            qDebug() << "✅ ATTITUDE #" << m_attitudeCount << "roll=" << attitude.roll
                     << "pitch=" << attitude.pitch << "yaw=" << attitude.yaw;
        }
    } else {
        qDebug() << "❌ ATTITUDE message too short:" << (data.size() - startPos) << "bytes";
    }

    return attitude;
}

// MavlinkAttitude MavlinkHandler::parseAttitudeMessage(const QByteArray &data, int startPos)
// {
//     MavlinkAttitude attitude;

//     if (startPos + 28 <= data.size()) { // ATTITUDE message is 28 bytes
//         // Parse time_boot_ms (uint32_t, bytes 0-3)
//         attitude.timestamp = qFromLittleEndian<quint32>(
//             reinterpret_cast<const uchar*>(data.constData() + startPos));

//         // Parse roll (float, bytes 4-7)
//         float roll = 0.0f;
//         memcpy(&roll, data.constData() + startPos + 4, sizeof(float));
//         attitude.roll = static_cast<double>(qFromLittleEndian<float>(roll)) * 180.0 / M_PI;

//         // Parse pitch (float, bytes 8-11)
//         float pitch = 0.0f;
//         memcpy(&pitch, data.constData() + startPos + 8, sizeof(float));
//         attitude.pitch = static_cast<double>(qFromLittleEndian<float>(pitch)) * 180.0 / M_PI;

//         // Parse yaw (float, bytes 12-15)
//         float yaw = 0.0f;
//         memcpy(&yaw, data.constData() + startPos + 12, sizeof(float));
//         attitude.yaw = static_cast<double>(qFromLittleEndian<float>(yaw)) * 180.0 / M_PI;

//         qDebug() << "✅ Successfully parsed ATTITUDE: roll=" << attitude.roll
//                  << "pitch=" << attitude.pitch << "yaw=" << attitude.yaw;
//     } else {
//         qDebug() << "❌ ATTITUDE message too short:" << (data.size() - startPos) << "bytes";
//     }

//     return attitude;
// }







void MavlinkHandler::requestAttitudeStream()
{
    // Запрашиваем ATTITUDE с частотой 30 Гц (33333 микросекунды)
    QByteArray command;

    // MAVLink 2.0 заголовок
    command.append(char(0xFD)); // start byte
    command.append(char(20));   // payload length
    command.append(char(0));    // incompat flags
    command.append(char(0));    // compat flags

    static quint8 sequence = 0;
    command.append(char(sequence++)); // sequence

    command.append(char(0xFF)); // system ID (GCS)
    command.append(char(0x01)); // component ID

    // Message ID: 511 (MAV_CMD_SET_MESSAGE_INTERVAL)
    command.append(char(0xFF)); // 511 = 0x01FF little endian
    command.append(char(0x01));
    command.append(char(0x00));

    // Payload: message_id, interval_us, target_system, target_component
    quint32 message_id = 30; // ATTITUDE
    float interval_us = 33333.0f; // 30 Hz (33333 microseconds)
    quint8 target_system = 1;
    quint8 target_component = 1;

    // Добавляем данные (little endian)
    command.append(reinterpret_cast<const char*>(&message_id), 4);
    command.append(reinterpret_cast<const char*>(&interval_us), 4);
    command.append(target_system);
    command.append(target_component);

    // Заполняем остальные параметры нулями
    for (int i = 0; i < 8; i++) {
        command.append(char(0));
    }

    // Checksum (упрощённо)
    command.append(char(0));
    command.append(char(0));

    m_networkManager->sendData(command);
    qDebug() << "📡 Requested ATTITUDE stream at 30 Hz";

    emit newMessage("Requested ATTITUDE data stream at 30 Hz");
}

// void MavlinkHandler::requestAttitudeStream()
// {
//     // MAVLink команда для запроса потока данных
//     // MAV_CMD_SET_MESSAGE_INTERVAL (511)
//     QByteArray command;

//     // MAVLink 2.0 заголовок
//     command.append(char(0xFD)); // start byte
//     command.append(char(20));   // payload length
//     command.append(char(0));    // incompat flags
//     command.append(char(0));    // compat flags

//     static quint8 sequence = 0;
//     command.append(char(sequence++)); // sequence

//     command.append(char(0xFF)); // system ID (GCS)
//     command.append(char(0x01)); // component ID

//     // Message ID: 511 (MAV_CMD_SET_MESSAGE_INTERVAL)
//     command.append(char(0xFF)); // 511 = 0x01FF little endian
//     command.append(char(0x01));
//     command.append(char(0x00));

//     // Payload: message_id, interval_us, target_system, target_component
//     quint32 message_id = 30; // ATTITUDE
//     float interval_us = 100000.0f; // 10 Hz (100000 microseconds)
//     quint8 target_system = 1;
//     quint8 target_component = 1;

//     // Добавляем данные (little endian)
//     command.append(reinterpret_cast<const char*>(&message_id), 4);
//     command.append(reinterpret_cast<const char*>(&interval_us), 4);
//     command.append(target_system);
//     command.append(target_component);

//     // Заполняем остальные параметры нулями
//     for (int i = 0; i < 8; i++) {
//         command.append(char(0));
//     }

//     // Checksum (упрощённо)
//     command.append(char(0));
//     command.append(char(0));

//     m_networkManager->sendData(command);
//     qDebug() << "📡 Requested ATTITUDE stream at 10 Hz";

//     emit newMessage("Requested ATTITUDE data stream");
// }

// Добавляем метод для расчета частоты
void MavlinkHandler::updateFrequency()
{
    m_attitudeFrequency = m_attitudeCount;
    m_attitudeCount = 0;
    emit attitudeFrequencyChanged(m_attitudeFrequency);
}
