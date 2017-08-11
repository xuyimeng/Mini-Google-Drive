#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QDataStream>
#include <QTcpSocket>
#include <QSet>
#include <QObject>
#include "httpcookie.h"
#include "httpsession.h"

class HttpRequest : public QObject
{
    Q_OBJECT
public:
    explicit HttpRequest(QObject *parent, QTcpSocket* socket);
    QString getMethod();

    QString getURI();
    QString getPath();

    QString getHeaderValue(QString key);
    QString getParameter(QString key);

    QByteArray& getContent();

    void addCookie(QString key, QString value);
    void addCookie(HttpCookie c1);
    QSet<HttpCookie>* getCookies();
    ~HttpRequest();

    HttpSession* HttpGetSession(bool create);
    HttpSession* InternalGetSession();
//    void setSession(HttpSession session);
signals:

private:
    QTcpSocket* socket;
    QString method;
    QString uri;
    QString path;
    QString version;
    QHash<QString, QStringList*> headers;
    QHash<QString, QString> parameter;
    QByteArray content;

    QSet<HttpCookie> cookies;

    void parseCookies();
    void addSingleCookie(QString Cookie_str);
    void parseParameter(QString& uri);

    void parseSession();
    HttpSession* session;
public slots:
};

#endif // HTTPREQUEST_H
