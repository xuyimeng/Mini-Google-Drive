#include "fileioservlet.h"

FileIOServlet::FileIOServlet(QObject *parent) : Servlet(parent)
{

}

void FileIOServlet::doGet(HttpRequest* request, HttpResponse* response) {
    if (QString::compare(request->getPath(), "/session1") == 0) {
        HttpSession* session = request->HttpGetSession(true);
        session->setAttribute("username","xuyimeng");

        response->setContentType("text/html");
        response->setStatus(200);

        response->addContent(QString("<HTML><HEAD><TITLE>Session Servlet 1</TITLE></HEAD><BODY>"));
        response->addContent(QString("<P>TestAttribute set to 12345.</P>"));
        response->addContent(QString("<P>Continue to <A HREF=\"session2\">Session Servlet 2</A>.</P>"));
        response->addContent(QString("</BODY></HTML>"));

        return;
    } else if (QString::compare(request->getPath(), "/session2") == 0) {
        response->setContentType("text/html");
        response->setStatus(200);
        HttpSession* session = request->HttpGetSession(false);
        if (session != NULL) {
            if (session->isValid()) {
                QString username = session->getAttribute("username");
                response->addContent(QString("Session is valid")+username);
            } else {
                response->addContent(QString("Session is not valid"));
            }
            session->setInvalid();
        }

        response->addContent(QString("Session invalidated"));
        response->addContent(QString("<P>Continue to <A HREF=\"session3\">Session Servlet 2</A>.</P>"));
    } else if (QString::compare(request->getPath(), "/session3") == 0) {

        HttpSession* session = request->HttpGetSession(false);
        response->setContentType("text/html");

        if (session == NULL) {
            response->addContent(QString("Session is not valid"));
        } else {
            response->addContent(QString("Session is valid"));
        }
    }
}

void FileIOServlet::doPost(HttpRequest* request, HttpResponse* response) {

}