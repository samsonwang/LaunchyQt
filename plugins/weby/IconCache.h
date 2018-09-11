#ifndef ICONCACHE_H
#define ICONCACHE_H
#include <QtGui>
#include <QtNetwork>
class IconCache : public QObject
{
	Q_OBJECT

public:
	IconCache(const QString& path);

	QString getIconPath(const QString& site);

public slots:
	void finished(QNetworkReply*);
	void query(QUrl url);

signals:
	void findIcon(QUrl url);


private:
	QString cachePath;
	QNetworkAccessManager nam;
//	QEventLoop loop;
};


#endif // ICONCACHE_H
