#include "httpresponse.h"
#include "httpsession.h"
#include "httpcookie.h"
#include "servletmanager.h"

HttpResponse::HttpResponse(QObject *parent, QTcpSocket* socket, HttpRequest* request) : QObject(parent)
{
    this->socket = socket;
    this->request = request;

    contentCache = QByteArray();
    headerCache = QHash<QString, QStringList*>();


    httpVersion = "HTTP/1.1";
    statusMap = QHash<int, QString>();
	statusMap[200] = "OK";
	statusMap[400] = "Bad Request";
	statusMap[405] = "Method Not Allowed";
	statusMap[505] = "HTTP Version Not Supported";
	statusMap[415] = "Unsupported Media Type";
	statusMap[403] = "Forbidden";
	statusMap[404] = "Not Found";
	statusMap[304] = "Not Modified";
	statusMap[412] = "Precondition Failed";
	statusMap[307] = "Temporary Redirect";
	statusMap[302] = "FOUND";
	statusMap[500] = "Internal Server Error";

	setDefaultHeader();
}

void HttpResponse::setDefaultHeader() {
	addHeader("Server", "QtHttp");
    addHeader("Connection", "close");
    addHeader("Date", getCurrentTime());
}

QString HttpResponse::getCurrentTime() {
	QDateTime current_time = QDateTime::currentDateTimeUtc();
    return current_time.toString("ddd, dd MMM yyyy h:m:s") + " GMT";
}

void HttpResponse::addHeader(QString key, int value) {
	checkCommitted();

	addHeader(key, QString::number(value));
}

void HttpResponse::addHeader(QString key, QString value) {
	checkCommitted();

	QStringList* list;
	if (headerCache.contains(key)) {
		list = headerCache.value(key);
	} else {
		list = new QStringList();
		headerCache.insert(key, list);
	}
	(*list) << value;
}

HttpResponse::~HttpResponse() {
    QHashIterator<QString, QStringList*> iter(headerCache);
	while (iter.hasNext()) {
		iter.next();
		delete(iter.value());
	}
}

void HttpResponse::setRedirect(QString path) {
	checkCommitted();

	reset();

    setStatus(302);
    if (path.startsWith("/")) {
        addHeader("location", path);
    } else {
        addHeader("location", request->getPath() + "/" + path);
    }

	commit();
}

void HttpResponse::setError(int status, QString ErrorMessage) {
	checkCommitted();

	reset();

   	setStatus(status);
   	addContent(ErrorMessage);

	commit();
}

void HttpResponse::reset() {
	checkCommitted();

	contentCache = QByteArray();
    headerCache = QHash<QString, QStringList*>();
    setDefaultHeader();
    status_Message = "";
    status = 404;
}

void HttpResponse::setStatus(int status) {
	checkCommitted();

	this->status = status;
}

void HttpResponse::setStatus(QString status_message) {
	checkCommitted();

	this->status_Message = status_message;
}

void HttpResponse::addContent(QByteArray content) {
	checkCommitted();

	contentCache.append(content);
}

void HttpResponse::addContent(QString content) {
	checkCommitted();

	addContent(content.toUtf8());
}

bool HttpResponse::check_committed() {
	return this->is_Committed;
}

void HttpResponse::setContentLength(int length) {
	checkCommitted();

	content_length = length;
}

void HttpResponse::checkCommitted() {
	if (is_Committed) {
		qDebug() << "HTTP response Wrong Action";
	}
}

void HttpResponse::setContentType(QString type) {
	checkCommitted();

    headerCache.remove("content-type");
    addHeader("content-type", type);
}

void HttpResponse::addCookie(QString key, QString value) {
	request->addCookie(key, value);
}

void HttpResponse::addCookie(HttpCookie c1) {
    request->addCookie(c1);
}

void HttpResponse::Cookie_Commit() {
    QSet<HttpCookie>* cookies = request->getCookies();

    QSet<HttpCookie>::iterator i;
    for (i = cookies->begin(); i != cookies->end(); i++) {
        if (i->getName().compare("JSESSIONID") == 0) {
            HttpSession* session = request->InternalGetSession();
            if (session != NULL) {
                if (!session->isValid()) {
                    HttpCookie cur_cookie = *i;
                    cur_cookie.setMaxAge(0);

                    if (!session->isNew()) {
                        addHeader("Set-Cookie", cur_cookie.toString());
                    }

                    ServletManager* manager = ServletManager::getInstance();
                    manager->deleteSession(session->getId());
                } else {
                    HttpCookie cur_cookie = *i;
                    addHeader("Set-Cookie", cur_cookie.toString());
                }
            }
        } else {
            HttpCookie cur_cookie = *i;
            addHeader("Set-Cookie", cur_cookie.toString());
        }
    }
}

void HttpResponse::commit() {
	checkCommitted();

    HttpSession* session = request->InternalGetSession();
    if (session != NULL && session->isNew() && session->isValid()) {
        addCookie("JSESSIONID", session->getId());
    }

	// add related headers
	// add cookies
	Cookie_Commit();
	// add content length
	if (content_length == 0) {
		addHeader("Content-Length", contentCache.size());
	}

	// add status message
	if (status_Message == NULL) {
		status_Message = statusMap[status];
	}
	// do initial line
    QString init_line = httpVersion + " " + QString::number(status) + " " + status_Message + "\r\n";
	
	// do header
	QString header;
    QHashIterator<QString, QStringList*> iter(headerCache);
    while (iter.hasNext()) {
		iter.next();
		foreach (const QString& str, (*iter.value())) {
			header += iter.key() + ": " + str + "\r\n";
		}
	}
	header += "\r\n";

    socket->write(init_line.toUtf8());
    socket->write(header.toUtf8());
    socket->write(contentCache);

    socket->waitForBytesWritten(-1);

    // set committed
    is_Committed = true;
}
