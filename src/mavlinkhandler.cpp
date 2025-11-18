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
    , m_retryCount(0)
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
    m_frequencyTimer->start(1000);

    // Таймер для повторных запросов потока данных
    m_streamRequestTimer = new QTimer(this);
    connect(m_streamRequestTimer, &QTimer::timeout, this, &MavlinkHandler::ensureAttitudeStream);
    m_streamRequestTimer->setInterval(2000); // Увеличили частоту проверок
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

    // Запускаем таймер для обеспечения потока данных
    QTimer::singleShot(2000, this, [this]() {
        requestAttitudeStream();
        m_streamRequestTimer->start();
    });
}

// void MavlinkHandler::connectToFC(const QString &ip, int port)
// {
//     int actualPort = (port == 5760) ? 14550 : port;
//     m_networkManager->connectToFC(ip, actualPort);

//     // Через 2 секунды после подключения запрашиваем поток ATTITUDE
//     QTimer::singleShot(2000, this, &MavlinkHandler::requestAttitudeStream);
// }

void MavlinkHandler::disconnectFromFC()
{
    m_streamRequestTimer->stop();
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

        // Логируем только каждое 30-е сообщение чтобы не засорять консоль
        if (m_attitudeCount % 30 == 0) {
            qDebug() << "✅ ATTITUDE #" << m_attitudeCount << "roll=" << attitude.roll
                     << "pitch=" << attitude.pitch << "yaw=" << attitude.yaw
                     << "freq=" << m_attitudeFrequency << "Hz";
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

    // Также отправляем команду для отключения оптимизации (если поддерживается)
    sendStreamOptimizationCommand();

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

    // Если частота низкая, увеличиваем счетчик повторных запросов
    if (m_attitudeFrequency < 25 && connected()) {
        m_retryCount++;
        if (m_retryCount >= 3) {
            qDebug() << "⚠️ Low attitude frequency (" << m_attitudeFrequency << "Hz), re-requesting stream...";
            requestAttitudeStream();
            m_retryCount = 0;
        }
    } else {
        m_retryCount = 0;
    }
}

void MavlinkHandler::ensureAttitudeStream()
{
    if (connected()) {
        if (m_attitudeFrequency < 25) {
            qDebug() << "🔄 Low frequency (" << m_attitudeFrequency << "Hz), re-requesting streams...";
            requestAllStreams();

            // Если частота очень низкая, попробуем более агрессивные настройки
            if (m_attitudeFrequency < 10) {
                qDebug() << "🚀 Very low frequency, enabling high rate mode";
                enableHighRateMode();
            }
        }
    }
}

void MavlinkHandler::sendStreamOptimizationCommand()
{
    // Команда для отключения оптимизации потоков данных
    // MAV_CMD_SET_MESSAGE_INTERVAL для других важных сообщений

    // Запрашиваем также SYS_STATUS для поддержания активности
    QByteArray sysStatusCommand;

    sysStatusCommand.append(char(0xFD)); // start byte
    sysStatusCommand.append(char(20));   // payload length
    sysStatusCommand.append(char(0));    // incompat flags
    sysStatusCommand.append(char(0));    // compat flags

    static quint8 sequence = 0;
    sysStatusCommand.append(char(sequence++)); // sequence

    sysStatusCommand.append(char(0xFF)); // system ID (GCS)
    sysStatusCommand.append(char(0x01)); // component ID

    // Message ID: 511 (MAV_CMD_SET_MESSAGE_INTERVAL)
    sysStatusCommand.append(char(0xFF)); // 511 = 0x01FF little endian
    sysStatusCommand.append(char(0x01));
    sysStatusCommand.append(char(0x00));

    // Запрашиваем SYS_STATUS с частотой 5 Гц
    quint32 sys_status_id = 1; // SYS_STATUS
    float sys_status_interval = 200000.0f; // 5 Hz
    quint8 target_system = 1;
    quint8 target_component = 1;

    sysStatusCommand.append(reinterpret_cast<const char*>(&sys_status_id), 4);
    sysStatusCommand.append(reinterpret_cast<const char*>(&sys_status_interval), 4);
    sysStatusCommand.append(target_system);
    sysStatusCommand.append(target_component);

    for (int i = 0; i < 8; i++) {
        sysStatusCommand.append(char(0));
    }

    sysStatusCommand.append(char(0));
    sysStatusCommand.append(char(0));

    m_networkManager->sendData(sysStatusCommand);
    qDebug() << "⚙️ Requested SYS_STATUS stream at 5 Hz to maintain connection";
}

void MavlinkHandler::setStreamRates(int attitudeHz, int sysStatusHz)
{
    qDebug() << "🔄 Setting stream rates - ATTITUDE:" << attitudeHz << "Hz, SYS_STATUS:" << sysStatusHz << "Hz";

    // Устанавливаем частоту для ATTITUDE
    QByteArray attitudeCommand;
    attitudeCommand.append(char(0xFD));
    attitudeCommand.append(char(20));
    attitudeCommand.append(char(0));
    attitudeCommand.append(char(0));

    static quint8 sequence = 0;
    attitudeCommand.append(char(sequence++));

    attitudeCommand.append(char(0xFF));
    attitudeCommand.append(char(0x01));
    attitudeCommand.append(char(0xFF));
    attitudeCommand.append(char(0x01));
    attitudeCommand.append(char(0x00));

    quint32 attitude_msg_id = 30;
    float attitude_interval = 1000000.0f / attitudeHz; // Convert Hz to microseconds
    quint8 target_system = 1;
    quint8 target_component = 1;

    attitudeCommand.append(reinterpret_cast<const char*>(&attitude_msg_id), 4);
    attitudeCommand.append(reinterpret_cast<const char*>(&attitude_interval), 4);
    attitudeCommand.append(target_system);
    attitudeCommand.append(target_component);

    for (int i = 0; i < 8; i++) {
        attitudeCommand.append(char(0));
    }

    attitudeCommand.append(char(0));
    attitudeCommand.append(char(0));

    m_networkManager->sendData(attitudeCommand);

    // Устанавливаем частоту для SYS_STATUS
    QByteArray sysStatusCommand;
    sysStatusCommand.append(char(0xFD));
    sysStatusCommand.append(char(20));
    sysStatusCommand.append(char(0));
    sysStatusCommand.append(char(0));

    sysStatusCommand.append(char(sequence++));

    sysStatusCommand.append(char(0xFF));
    sysStatusCommand.append(char(0x01));
    sysStatusCommand.append(char(0xFF));
    sysStatusCommand.append(char(0x01));
    sysStatusCommand.append(char(0x00));

    quint32 sys_status_msg_id = 1;
    float sys_status_interval = 1000000.0f / sysStatusHz;

    sysStatusCommand.append(reinterpret_cast<const char*>(&sys_status_msg_id), 4);
    sysStatusCommand.append(reinterpret_cast<const char*>(&sys_status_interval), 4);
    sysStatusCommand.append(target_system);
    sysStatusCommand.append(target_component);

    for (int i = 0; i < 8; i++) {
        sysStatusCommand.append(char(0));
    }

    sysStatusCommand.append(char(0));
    sysStatusCommand.append(char(0));

    m_networkManager->sendData(sysStatusCommand);

    emit newMessage(QString("Set stream rates: ATTITUDE=%1Hz, SYS_STATUS=%2Hz").arg(attitudeHz).arg(sysStatusHz));
}

void MavlinkHandler::setArduPilotParameters(int sr1_ext_stat, int sr1_extra1, int sr1_extra2, int sr1_extra3)
{
    qDebug() << "🔧 Setting ArduPilot stream parameters";

    // SR1_ parameters control stream rates in ArduPilot
    setParameter("SR1_EXT_STAT", sr1_ext_stat);
    setParameter("SR1_EXTRA1", sr1_extra1);  // This controls ATTITUDE stream rate
    setParameter("SR1_EXTRA2", sr1_extra2);
    setParameter("SR1_EXTRA3", sr1_extra3);

    emit newMessage(QString("Set ArduPilot params: SR1_EXTRA1=%1").arg(sr1_extra1));
}

void MavlinkHandler::setParameter(const QString &paramName, float value)
{
    QByteArray paramSet;

    paramSet.append(char(0xFD));
    paramSet.append(char(25));
    paramSet.append(char(0));
    paramSet.append(char(0));

    static quint8 sequence = 0;
    paramSet.append(char(sequence++));

    paramSet.append(char(0xFF));
    paramSet.append(char(0x01));
    paramSet.append(char(0x17));
    paramSet.append(char(0x00));
    paramSet.append(char(0x00));

    paramSet.append(char(0x01));
    paramSet.append(char(0x01));

    // Parameter ID (16 bytes)
    QByteArray paramId = paramName.toUtf8();
    paramId.resize(16);
    paramSet.append(paramId);

    // Parameter value
    paramSet.append(reinterpret_cast<const char*>(&value), 4);

    // Parameter type (MAV_PARAM_TYPE_REAL32 = 9)
    paramSet.append(char(9));

    // Checksum
    paramSet.append(char(0));
    paramSet.append(char(0));

    m_networkManager->sendData(paramSet);

    qDebug() << "📝 Set parameter" << paramName << "to" << value;
}

void MavlinkHandler::enableHighRateMode()
{
    qDebug() << "🚀 Enabling high rate mode";

    // Aggressive stream rates
    setStreamRates(50, 10); // 50 Hz attitude, 10 Hz sys_status

    // Set ArduPilot parameters for high rates
    setArduPilotParameters(10, 50, 20, 10); // High rates for all streams

    // Request multiple data streams
    requestAllStreams();

    emit newMessage("Enabled high rate mode (50Hz ATTITUDE)");
}

void MavlinkHandler::resetStreamingToDefaults()
{
    qDebug() << "🔄 Resetting streaming to defaults";

    setStreamRates(30, 5);
    setArduPilotParameters(5, 10, 5, 2);

    emit newMessage("Reset streaming to defaults");
}

void MavlinkHandler::requestAllStreams()
{
    // Request multiple data streams to ensure constant data flow
    QVector<uint32_t> streamIds = {30, 1, 33, 74}; // ATTITUDE, SYS_STATUS, GLOBAL_POSITION, VFR_HUD

    foreach (uint32_t streamId, streamIds) {
        QByteArray command;
        command.append(char(0xFD));
        command.append(char(20));
        command.append(char(0));
        command.append(char(0));

        static quint8 sequence = 0;
        command.append(char(sequence++));

        command.append(char(0xFF));
        command.append(char(0x01));
        command.append(char(0xFF));
        command.append(char(0x01));
        command.append(char(0x00));

        float interval = 100000.0f; // 10 Hz for other streams

        if (streamId == 30) { // ATTITUDE gets higher rate
            interval = 33333.0f; // 30 Hz
        }

        command.append(reinterpret_cast<const char*>(&streamId), 4);
        command.append(reinterpret_cast<const char*>(&interval), 4);
        command.append(char(0x01)); // target_system
        command.append(char(0x01)); // target_component

        for (int i = 0; i < 8; i++) {
            command.append(char(0));
        }

        command.append(char(0));
        command.append(char(0));

        m_networkManager->sendData(command);
    }

    qDebug() << "📡 Requested multiple data streams";
}
