#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>

class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = 0);
    void startServer();
signals:

protected:
    void incomingConnection(qintptr socketDescriptor);
public slots:

};

#endif // TCPSERVER_H
