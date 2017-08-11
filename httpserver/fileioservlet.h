#ifndef FILEIOSERVLET_H
#define FILEIOSERVLET_H

#include "servlet.h"

class FileIOServlet : public Servlet
{
public:
    FileIOServlet(QObject *parent = 0);
    void doPost(HttpRequest* request, HttpResponse* response) override;
    void doGet(HttpRequest* request, HttpResponse* response) override;
};

#endif // FILEIOSERVLET_H
