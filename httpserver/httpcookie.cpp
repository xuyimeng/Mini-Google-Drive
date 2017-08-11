#include "httpcookie.h"

HttpCookie::HttpCookie(QString name, QString value)
{
    this->name = name;
    this->value = value;
    this->setMaxAge(-1);
}

QString HttpCookie::getName() const {
	return name;
}

QString HttpCookie::getValue() const {
	return value;
}

QString HttpCookie::toString() {
	if (this->getMaxAge() < 0) {
        return QString("%1=%2").arg(this->getName(), this->getValue());
	} else if (this->getMaxAge() == 0) {
        QDateTime current = QDateTime::currentDateTimeUtc();
        return QString("%1=%2; expires=%3").arg(this->getName(), this->getValue(), DateTime2Str(current));
	} else {
        QDateTime current = QDateTime::currentDateTimeUtc();
        current = current.addSecs(this->getMaxAge());
        return QString("%1=%2; expires=%3").arg(this->getName(), this->getValue(), DateTime2Str(current));
	}
}

void HttpCookie::setMaxAge(int age) {
    this->maxAge = age;
}

int HttpCookie::getMaxAge() {
    return this->maxAge;
}
