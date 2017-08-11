#ifndef TCPWORKER_H
#define TCPWORKER_H

#include <QObject>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include "httprequest.h"
#include "httpresponse.h"
#include "servletmanager.h"
#include "servlet.h"

class TcpWorker : public QThread
{
    Q_OBJECT
public:
    explicit TcpWorker(qintptr ID, QObject *parent = 0);
    void run();
signals:
    void error(QTcpSocket::SocketError socketerror);
public slots:
    void readyRead();
    void disconnect();

private:
    QTcpSocket* socket;
    qintptr socketDescriptor;
    QObject *parent;
    HttpRequest* request;
    HttpResponse* response;
};

#endif // TCPWORKER_H
