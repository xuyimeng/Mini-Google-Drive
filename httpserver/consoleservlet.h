#ifndef CONSOLESERVLET_H
#define CONSOLESERVLET_H

#include "servlet.h"
#include "servletmanager.h"

// #include "emailservlet.cpp"

class ConsoleServlet : public Servlet
{
public:
	 // BigTableClient bigtable(
  //     grpc::CreateChannel("localhost:8001",
  //                         grpc::InsecureChannelCredentials())); 
    ConsoleServlet(QObject *parent = 0);
    void doPost(HttpRequest* request, HttpResponse* response) override;
    void doGet(HttpRequest* request, HttpResponse* response) override;
};

#endif // EMAILSERVLET_H