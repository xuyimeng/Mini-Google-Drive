#include "httprequest.h"
#include "servletmanager.h"

QString HttpRequest::getMethod() {
    return method;
}

// QString HttpRequest::getURI() {
//     return uri;
// }
QString HttpRequest::getURI() {
    QByteArray ret = QByteArray::fromPercentEncoding(uri.toUtf8());
    // QByteArray value = QByteArray::fromPercentEncoding(key_value.value(1).toUtf8());
    return QString(ret);
}

QString HttpRequest::getPath() {
    return path;
}

QString HttpRequest::getHeaderValue(QString key) {
    if (!headers.contains(key)) {
        return "";
    } else {
        return headers.value(key)->value(0);
    }
}

QString HttpRequest::getParameter(QString key) {
    if (!parameter.contains(key)) {
        return "";
    } else {
        return parameter.value(key);
    }
}

QByteArray& HttpRequest::getContent() {
    return content;
}

void HttpRequest::addSingleCookie(QString Cookie_str) {
    if (Cookie_str.contains('=')) {
        QString key = Cookie_str.section('=', 0, 0).trimmed();
        QString value = Cookie_str.section('=',1).trimmed();
        HttpCookie cookie(key, value);
        cookies.insert(cookie);
    } else {
        qDebug() << "ERROR: Parsing Cookie Error";
    }
}

void HttpRequest::parseCookies() {
    if (headers.contains("cookie")) {
        QStringList* cookieheader = headers.value("cookie");
        for (int i = 0; i < cookieheader->size(); i++) {
            if (cookieheader->value(i).contains(";")) {
                QStringList current_cookies = cookieheader->value(i).split(";");
                for (int j = 0; j < current_cookies.size(); j++) {
                    addSingleCookie(current_cookies.value(j).trimmed());
                }
            } else if (cookieheader->value(i).contains(",")) {
                QStringList current_cookies = cookieheader->value(i).split(",");
                for (int j = 0; j < current_cookies.size(); j++) {
                    addSingleCookie(current_cookies.value(j).trimmed());
                }
            } else {
                addSingleCookie(cookieheader->value(i).trimmed());
            }
        }
    }
}

HttpSession* HttpRequest::InternalGetSession() {
    return session;
}


HttpSession* HttpRequest::HttpGetSession(bool create) {
    if (create) {
        if (this->session == NULL) {
            HttpSession* session = new HttpSession();
            session->setValid();

            ServletManager* manager = ServletManager::getInstance();
            manager->PutSession(session->getId(), session);

            this->session = session;
        }
    }

    return this->session;
}

//void HttpRequest::setSession(HttpSession session) {
//    ServletManager* manager = ServletManager::getInstance();
//    manager->PutSession(session.getId(), session);
//    this->session = session;
//}

void HttpRequest::parseSession() {
    QSet<HttpCookie>::iterator i = cookies.begin();

    for (i = cookies.begin(); i != cookies.end(); ++i) {
        if(QString::compare(i->getName(), "JSESSIONID") == 0) {
            QString session_name = i->getValue();
            // qDebug() << "In request parseSession"<<session_name;
            ServletManager* manager = ServletManager::getInstance();
            session = manager->HttpGetSession(session_name.trimmed());
            return;
        } 
    }
    // No session found
    session = NULL;
}

HttpRequest::~HttpRequest() {
    QHashIterator<QString, QStringList*> iter(headers);
    while (iter.hasNext()) {
        iter.next();
        delete(iter.value());
    }
}

HttpRequest::HttpRequest(QObject *parent, QTcpSocket* socket) : QObject(parent) {
    enum ParsingStatus {
        ParsingError    = -1,
        AwaitingRequest =  0,
        AwaitingHeaders =  1,
        AwaitingContent =  2,
        RequestParsed   =  3
    };

    this->socket = socket;

    headers = QHash<QString, QStringList*>();
    content = QByteArray();
    ParsingStatus status = AwaitingRequest;

    while (socket->bytesAvailable()) {
        switch (status) {
            case AwaitingRequest: {
                QByteArray line = socket->readLine();
            	QString str = QString::fromUtf8(line).trimmed();
            	QStringList parts = str.split(' ', QString::SkipEmptyParts);
            	if (parts.size() == 3) {
            		method = parts.at(0).toLower();
            		uri = parts.at(1).toLower();
            		version = parts.at(2).toLower();

            		status = AwaitingHeaders;
            	} else {
            		status = ParsingError;
            	}
            	break;
            }
            case AwaitingHeaders: {
                QByteArray line = socket->readLine();
            	QString raw = QString::fromUtf8(line).trimmed();
            	if (!raw.isEmpty()) {
            		int pos = raw.indexOf(':');
            		if (pos > 0) {
            			QString key = raw.left(pos).trimmed().toLower();
            			QString value = raw.mid(pos + 1).trimmed();

                        QStringList* list;
                        if (headers.contains(key)) {
                            list = headers.value(key);
                        } else {
                            list = new QStringList();
                            headers.insert(key, list);
                        }
                        (*list) << value;
            		} else {
            			status = ParsingError;
            		}
            	} else {
            		// end of headers
                    if (headers.contains("content-length")) {
            			status = AwaitingContent;
            		} else {
            			status = RequestParsed;
            		}
            	}

            	break;
            }
            case AwaitingContent: {
                int requested_content_length = getHeaderValue("content-length").toLong();
                while (content.size() < requested_content_length) {
                    if (!socket->bytesAvailable())
                        socket->waitForReadyRead(30000);
                    content.append(socket->readAll());
                }
                status = RequestParsed;
            	break;
            }
    	}
        // after parsing
    	if (status == ParsingError || status == RequestParsed) {
        	break;
        }
    }

    if (status == ParsingError) {
        qDebug() << "ERROR: HTTP Request Parsing Error";
    } else {
        // parse URL parameter:
        if (uri.contains('?')) {
            parseParameter(uri);
        } else {
            path = uri;
        }
        // parse cookies
        parseCookies();
        parseSession();
    }
}

void HttpRequest::addCookie(QString key, QString value) {
    cookies.insert(HttpCookie(key, value));
}

void HttpRequest::addCookie(HttpCookie c1) {
    cookies.remove(c1);
    cookies.insert(c1);
}

QSet<HttpCookie>* HttpRequest::getCookies() {
    return &(cookies);
}

void HttpRequest::parseParameter(QString& uri) {
    QStringList parts = uri.split("?");
    this->path = parts.value(0);

    QString parameters = parts.value(1);
    QStringList parameters_list = parameters.split('&');
    for (int i = 0; i < parameters_list.size(); i++) {
        QStringList key_value = parameters_list.value(i).split('=');

        QByteArray key = QByteArray::fromPercentEncoding(key_value.value(0).toUtf8());
        QByteArray value = QByteArray::fromPercentEncoding(key_value.value(1).toUtf8());

        parameter.insert(QString(key), QString(value));
    }
}

