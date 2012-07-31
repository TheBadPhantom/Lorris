/**********************************************
**    This file is part of Lorris
**    http://tasssadar.github.com/Lorris/
**
**    See README and COPYING
***********************************************/

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QHash>
#include <QTcpServer>
#include <QSignalMapper>

class QTcpSocket;

class TcpServer : public QTcpServer
{
    Q_OBJECT

Q_SIGNALS:
    void newData(const QByteArray& data);
    void newConnection(QTcpSocket *socket, quint32 id);
    void removeConnection(quint32 id);

public:
    typedef QHash<quint32, QTcpSocket*> socketMap;

    TcpServer(QObject *parent = NULL);
    ~TcpServer();

    bool listen(const QString& address, quint16 port);
    void stopListening();

    QString getAddress();

public slots:
    void SendData(const QByteArray& data);

private slots:
    void newConnection();
    void disconnected(int con);
    void readyRead(int con);

private:
    QSignalMapper m_disconnect_map;
    QSignalMapper m_ready_map;
    socketMap m_socket_map;

    quint32 m_con_counter;
};

#endif // TCPSERVER_H
