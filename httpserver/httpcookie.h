#ifndef HTTPCOOKIE_H
#define HTTPCOOKIE_H
#include <QString>
#include <QHash>
#include <QDateTime>

class HttpCookie {
public:
    HttpCookie(QString name, QString value);
    QString getName() const;
    QString getValue() const;
    void setMaxAge(int age);
    int getMaxAge();
    QString toString();

    inline bool operator==(const HttpCookie &that) const
    {
        return (this->name.compare(that.name) == 0);
    }
private:
    int maxAge;
	QString name;
	QString value;

    inline QString DateTime2Str(QDateTime& time) {
        return time.toString("ddd, dd MMM yyyy h:m:s") + " GMT";
    }
};


inline uint qHash(const HttpCookie &c1, uint seed = 0)
{
    return qHash(c1.getName(), seed);
}

#endif // HTTPCOOKIE_H
