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

    bool connected() const;
    QString status() const;
    MavlinkAttitude attitude() const;
    QString rawData() const;

public slots:
    void connectToFC(const QString &ip, int port = 5760);
    void disconnectFromFC();
    void clearData();

    void requestAttitudeStream();

signals:
    void connectedChanged(bool connected);
    void statusChanged(const QString &status);
    void attitudeChanged(const MavlinkAttitude &attitude);
    void rawDataChanged(const QString &rawData);
    void newMessage(const QString &message);

private slots:
    void onNetworkDataReceived(const QByteArray &data);
    void onNetworkConnectedChanged(bool connected);
    void onNetworkStatusChanged(const QString &status);

private:
    void parseMavlinkMessage(const QByteArray &data);
    MavlinkAttitude parseAttitudeMessage(const QByteArray &data, int startPos);

    NetworkManager *m_networkManager;
    MavlinkAttitude m_currentAttitude;
    QString m_rawData;
    QByteArray m_buffer;
};

#endif // MAVLINKHANDLER_H
