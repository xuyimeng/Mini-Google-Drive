#ifndef STORAGESERVLET_H
#define STORAGESERVLET_H

#include "servlet.h"
#include "servletmanager.h"


class StorageServlet : public Servlet
{
public:
    StorageServlet(QObject *parent = 0);
    void doPost(HttpRequest* request, HttpResponse* response) override;
    void doGet(HttpRequest* request, HttpResponse* response) override;
};

#endif // STORAGESERVLET_H