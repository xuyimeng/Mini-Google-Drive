#include "servlet.h"
#include <QByteArray>
#include <QFile>


Servlet::Servlet(QObject *parent) : QObject(parent) {

}

void Servlet::doPost(HttpRequest* request, HttpResponse* response) {
}

void Servlet::doGet(HttpRequest* request, HttpResponse* response) {
    response->setError(404, "Sorry Not Found");
    // QFile file("/Users/yimengxu/git/505-Project/httpserver/html/send_email.html");

    // if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    //        return;
    // response->addContent(file.readAll());
    // response->setStatus(200);

}

