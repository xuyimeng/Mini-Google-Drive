#include <QCoreApplication>
#include <QDebug>
#include "tcpserver.h"
#include "servletmanager.h"
#include "servlet.h"
#include "loginservlet.h"
#include "emailservlet.h"
#include "fileioservlet.h"
#include "storageservlet.h"
#include "consoleservlet.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    TcpServer server;

    ServletManager::setUp();
    ServletManager* manager = ServletManager::getInstance();
    Servlet* servlet = new Servlet(manager);
    Servlet* login_servlet = new LoginServlet(manager);
    Servlet* email_servlet = new EmailServlet(manager);
    Servlet* fileio_servlet = new FileIOServlet(manager);
    Servlet* storage_servlet = new StorageServlet(manager);
    Servlet* console_servlet = new ConsoleServlet(manager);
    
    manager->setMethod("/", servlet);
    manager->setMethod("/login", login_servlet);
    manager->setMethod("/signup", login_servlet);
    manager->setMethod("/home",login_servlet);
    manager->setMethod("/logout",login_servlet);

    manager->setMethod("/email",email_servlet);
    manager->setMethod("/new_email",email_servlet);
    manager->setMethod("/receive_email",email_servlet);
    manager->setMethod("/sent_email",email_servlet);
    manager->setMethod("/trash_email",email_servlet);
    manager->setMethod("/show_email",email_servlet);
    manager->setMethod("/email_delete",email_servlet);
    manager->setMethod("/email_reply",email_servlet);
    manager->setMethod("/email_forward",email_servlet);
    manager->setMethod("/complete_delete",email_servlet);

    manager->setMethod("/session1", fileio_servlet);
    manager->setMethod("/session2", fileio_servlet);
    manager->setMethod("/session3", fileio_servlet);

    manager->setMethod("/storage",storage_servlet);
    manager->setMethod("/upload_file",storage_servlet);
    manager->setMethod("/create_folder",storage_servlet);
    manager->setMethod("/show_file",storage_servlet);

    manager->setMethod("/console",console_servlet);
    manager->setMethod("/terminate",console_servlet);
    manager->setMethod("/show_rawdata",console_servlet);
    manager->setMethod("/showdata",console_servlet);

    server.startServer();
    return a.exec();
}
