#ifndef EMAILSERVLET_H
#define EMAILSERVLET_H

#include "servlet.h"
#include "servletmanager.h"

// #include "emailservlet.cpp"

class EmailServlet : public Servlet
{
public:
	 // BigTableClient bigtable(
  //     grpc::CreateChannel("localhost:8001",
  //                         grpc::InsecureChannelCredentials())); 
    EmailServlet(QObject *parent = 0);
    void doPost(HttpRequest* request, HttpResponse* response) override;
    void doGet(HttpRequest* request, HttpResponse* response) override;
};

#endif // EMAILSERVLET_H
