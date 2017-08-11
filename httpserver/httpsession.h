#ifndef HTTPSESSION_H
#define HTTPSESSION_H
#include <QUuid>
#include <QDateTime>
#include <QHash>

class HttpSession
{
public:
    HttpSession();
    HttpSession HttpSessionGet(QString session_name);

    QString getId();
    void setInvalid();
    void setValid();

    bool isValid();
    void setIsNew(bool status);
    bool isNew();
    void setLastAccessTime();
    void setMaxInterval(qint64 max_interval);

    void setAttribute(QString key, QString value);
    QString getAttribute(QString key);
private:
	QUuid sessionId;
	bool isValid_status = true;
	bool isNew_status = true;
	QDateTime createTime;
    QDateTime lastAccessTime;
	qint64 maxInterval;
    QHash<QString, QString> attributes;
};

#endif // HTTPSESSION_H
