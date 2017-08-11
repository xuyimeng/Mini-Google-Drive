#include "httpsession.h"

HttpSession::HttpSession() {
	sessionId = QUuid::createUuid();

	isNew_status = true;
	createTime = QDateTime::currentDateTime();
	lastAccessTime = createTime;
    attributes = QHash<QString, QString>();
}

QString HttpSession::getId() {
	return sessionId.toString();
}

void HttpSession::setAttribute(QString key, QString value) {
    attributes.insert(key, value);
}

QString HttpSession::getAttribute(QString key) {
    return attributes.value(key);
}

void HttpSession::setInvalid() {
	isValid_status = false;
}

void HttpSession::setValid() {
    isValid_status = true;
}

bool HttpSession::isNew() {
	return isNew_status;
}

bool HttpSession::isValid() {
	if (isValid_status) {
		if (lastAccessTime.secsTo(createTime) > maxInterval) {
			setInvalid();
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

void HttpSession::setIsNew(bool status) {
	isNew_status = status;
}

void HttpSession::setLastAccessTime() {
	lastAccessTime = QDateTime::currentDateTime();
}

void HttpSession::setMaxInterval(qint64 max_interval) {
	maxInterval = max_interval;
}
