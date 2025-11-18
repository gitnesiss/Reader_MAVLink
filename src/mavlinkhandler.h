#ifndef MAVLINKHANDLER_H
#define MAVLINKHANDLER_H

#include <QObject>
#include <QTimer>
#include "networkmanager.h"

// Simple MAVLink structures
struct MavlinkAttitude {
    Q_GADGET
    Q_PROPERTY(double roll MEMBER roll)
    Q_PROPERTY(double pitch MEMBER pitch)
    Q_PROPERTY(double yaw MEMBER yaw)
    Q_PROPERTY(quint32 timestamp MEMBER timestamp)

public:
    double roll = 0.0;
    double pitch = 0.0;
    double yaw = 0.0;
    quint32 timestamp = 0;
};

Q_DECLARE_METATYPE(MavlinkAttitude)

class MavlinkHandler : public QObject
{
    Q_OBJECT

public:
    explicit MavlinkHandler(QObject *parent = nullptr);
    ~MavlinkHandler();

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(MavlinkAttitude attitude READ attitude NOTIFY attitudeChanged)
    Q_PROPERTY(QString rawData READ rawData NOTIFY rawDataChanged)
    Q_PROPERTY(int attitudeFrequency READ attitudeFrequency NOTIFY attitudeFrequencyChanged)

    bool connected() const;
    QString status() const;
    MavlinkAttitude attitude() const;
    QString rawData() const;
    int attitudeFrequency() const;

public slots:
    void connectToFC(const QString &ip, int port = 5760);
    void disconnectFromFC();
    void clearData();
    void requestAttitudeStream();

    // Новые методы для настройки параметров
    void setStreamRates(int attitudeHz, int sysStatusHz);
    void setArduPilotParameters(int sr1_ext_stat, int sr1_extra1, int sr1_extra2, int sr1_extra3);
    void enableHighRateMode();
    void resetStreamingToDefaults();

signals:
    void connectedChanged(bool connected);
    void statusChanged(const QString &status);
    void attitudeChanged(const MavlinkAttitude &attitude);
    void rawDataChanged(const QString &rawData);
    void newMessage(const QString &message);
    void attitudeFrequencyChanged(int frequency);

private slots:
    void onNetworkDataReceived(const QByteArray &data);
    void onNetworkConnectedChanged(bool connected);
    void onNetworkStatusChanged(const QString &status);
    void updateFrequency();
    void ensureAttitudeStream();

private:
    void parseMavlinkMessage(const QByteArray &data);
    MavlinkAttitude parseAttitudeMessage(const QByteArray &data, int startPos);
    void sendStreamOptimizationCommand();

    // Новые методы для работы с параметрами
    void setParameter(const QString &paramName, float value);
    void requestAllStreams();
    void sendMavlinkCommand(uint16_t command, const QVector<float> &params);

    NetworkManager *m_networkManager;
    MavlinkAttitude m_currentAttitude;
    QString m_rawData;
    QByteArray m_buffer;

    // Для подсчета частоты
    QTimer *m_frequencyTimer;
    QTimer *m_streamRequestTimer;
    int m_attitudeCount;
    int m_attitudeFrequency;
    int m_retryCount;
    qint64 m_lastAttitudeTime;
};

#endif // MAVLINKHANDLER_H
