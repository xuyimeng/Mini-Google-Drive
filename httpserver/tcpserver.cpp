#include "tcpserver.h"
#include "tcpworker.h"
#include <QTcpServer>
#include <QTcpSocket>

TcpServer::TcpServer(QObject *parent) :
    QTcpServer(parent) {



}

void TcpServer::startServer() {
    int port = 8000;
    if (!this->listen(QHostAddress::Any, port)) {
        qCritical() << "Error: Cannot setup listen server";
    } else {
        qDebug() << "Listen on port: " << port;
    }
}

void TcpServer::incomingConnection(qintptr socketDescriptor) {
    qDebug() << "=====\nServer Connecting..." << socketDescriptor;
    TcpWorker* worker = new TcpWorker(socketDescriptor, this);
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    worker->start();
}
