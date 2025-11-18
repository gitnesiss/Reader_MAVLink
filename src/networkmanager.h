#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

    bool connected() const;
    QString status() const;

public slots:
    void connectToFC(const QString &ip, int port);
    void disconnectFromFC();
    void sendData(const QByteArray &data);

signals:
    void connectedChanged(bool connected);
    void statusChanged(const QString &status);
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onHeartbeatTimeout();

private:
    QUdpSocket *m_socket;
    bool m_connected;
    QString m_status;
    QHostAddress m_remoteAddress;
    quint16 m_remotePort;
    QTimer *m_heartbeatTimer;

    QByteArray createMavlinkHeartbeat();
    void sendMavlinkHeartbeat();

    // поддержка TCP
    // QTcpSocket *m_tcpSocket;
    // bool m_useTcp;
};

#endif // NETWORKMANAGER_H
