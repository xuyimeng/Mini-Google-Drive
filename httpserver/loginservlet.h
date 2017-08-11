#ifndef LOGINSERVLET_H
#define LOGINSERVLET_H

#include "servlet.h"
#include "servletmanager.h"

class LoginServlet : public Servlet
{
public:
    LoginServlet(QObject *parent = 0);
    void doPost(HttpRequest* request, HttpResponse* response) override;
    void doGet(HttpRequest* request, HttpResponse* response) override;
};

#endif // LOGINSERVLET_H
