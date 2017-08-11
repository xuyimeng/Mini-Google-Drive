#ifndef SERVLET_H
#define SERVLET_H

#include <QObject>
#include "httprequest.h"
#include "httpresponse.h"

#include <stdlib.h>
#include <stdio.h>
using namespace std;
using std::string;

class Servlet : public QObject
{
    Q_OBJECT
public:
    QString rootDir = "../httpserver";
    explicit Servlet(QObject *parent = 0);
    virtual void doPost(HttpRequest* request, HttpResponse* response);
    virtual void doGet(HttpRequest* request, HttpResponse* response);

signals:

public slots:
};

#endif
