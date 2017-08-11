#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QObject>
#include <QDataStream>
#include <QTcpSocket>
#include <QString>
#include <QDateTime>
#include <QFile>

#include "httpcookie.h"
#include "httprequest.h"

class HttpResponse : public QObject
{
    Q_OBJECT
public:
    explicit HttpResponse(QObject *parent, QTcpSocket* socket, HttpRequest *request);
    bool check_committed();
    void commit();
    void addHeader(QString key, QString value);
    void addHeader(QString key, int value);
    void setStatus(int status);
    void setStatus(QString status_message);

    void addContent(QByteArray content);
    void addContent(QString content);
    void setContentLength(int length);
    void setContentType(QString type);

    void setRedirect(QString path);
    void setError(int status, QString ErrorMessage);
    void reset();
    ~HttpResponse();

    void addCookie(QString key, QString value);
    void addCookie(HttpCookie c1);
//    void setSession(HttpSession session);
signals:
private:
    HttpRequest* request;

    int status = 404;
    QString status_Message;
    QHash<int, QString> statusMap;
    bool is_Committed = false;

    int content_length = 0;
    QString content_type = "text/html; charset=utf-8";

	QTcpSocket* socket;
    QString httpVersion;
    QHash<QString, QStringList*> headerCache;
    QByteArray contentCache;

    void checkCommitted();
    QString getCurrentTime();
    void setDefaultHeader();

    void Cookie_Commit();
public slots:
};

#endif // HTTPRESPONSE_H
