#include "tcpworker.h"

TcpWorker::TcpWorker(qintptr ID, QObject *parent) :
    QThread(parent) {
    this->parent = parent;
    this->socketDescriptor = ID;
    response = NULL;
}

void TcpWorker::run() {
//    qDebug() << "Worker Thread Started";

    socket = new QTcpSocket();

    if(!socket->setSocketDescriptor(this->socketDescriptor)) {
           // something's wrong, we just emit a signal
           emit error(socket->error());
           return;
    }
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnect()));

    // qDebug() << socketDescriptor << "Client connected";
    exec();
}

void TcpWorker::readyRead() {
    // qDebug() << "READY READ";
    request = new HttpRequest(0, socket);
    response = new HttpResponse(0, socket, request);

    ServletManager* manager = ServletManager::getInstance();
    Servlet* servlet = manager->getMethod(request->getPath());
    // qDebug() << request->getPath();
    // qDebug() << request->getMethod();
    if (servlet != 0) {
        // qDebug() << request->getMethod();
        if (QString::compare(request->getMethod(), "get", Qt::CaseInsensitive) == 0)
            servlet->doGet(request, response);
        else if (QString::compare(request->getMethod(), "post", Qt::CaseInsensitive) == 0) {
            servlet->doPost(request, response);
        } else {
            response->setError(404, "Service Not Found");
            // qDebug() << "Not get Or post";
            socket->close();
        }

        if (!response->check_committed()) {
            response->commit();
            socket->close();
            // qDebug() <<  "Socket descriptor: " << socketDescriptor;
        }
    } else {
        response->setError(404, "Service Not Found");
        qDebug() << "Service 404";
        socket->close();
    }
}

void TcpWorker::disconnect() {
    // qDebug() << socketDescriptor << "Disconnected";
    if (response != NULL)
        delete(response);
    // socket->close();
    socket->deleteLater();
    exit(0);
}
