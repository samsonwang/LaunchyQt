#include "precompiled.h"
#include "IconCache.h"


IconCache::IconCache(const QString& path) :
	cachePath(path)
{
	nam.setParent(this);	
	connect(&nam, SIGNAL(finished(QNetworkReply*)), SLOT(finished(QNetworkReply*)));

	QFileInfo info(path);
	if (!info.exists()) {
		QDir d;
		d.mkdir(path);
	}
}




QString IconCache::getIconPath(const QString& site)
{

	QString cachedName = site;
	if (site.contains("http"))
		cachedName = QUrl(site).host();

	qDebug() << cachedName;

	//cachedName = cachedName.replace("http:", "").replace("https:", "").replace("/", "");
	QFileInfo info;
	info.setFile(cachePath, cachedName + ".png");

	if (info.exists())
		return info.size() > 0 ? info.absoluteFilePath() : QString();
	info.setFile(cachePath, cachedName + ".ico");
	if (info.exists())
		return info.size() > 0 ? info.absoluteFilePath() : QString();

	if (!site.startsWith("http"))
		return "";


	// Call the main thread to grab the icon in the background
	emit findIcon(QUrl("http://" + QUrl(site).host() + "/favicon.ico"));

	return "";
}

void IconCache::query(QUrl url)
{
	qDebug() << "Going for" << url;
	QNetworkRequest request;
	request.setUrl(url);
	request.setAttribute(QNetworkRequest::User, url);
	
	nam.get(request);

}

void IconCache::finished(QNetworkReply* reply)
{

	QUrl url = reply->url();
	if (reply && reply->error() == QNetworkReply::NoError)
	{
		QFile file(cachePath + url.host() + ".ico");
		if (!file.open(QIODevice::WriteOnly))
		{
			qDebug() << "Could not open icon for writing";
			return;
		}
		QByteArray ba = reply->readAll();
		file.write(ba);
	}	
	reply->deleteLater();

	/*
	QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	if (redirectionTarget.isValid())
	{
		delete reply;
		reply = depth < 5 ? query(redirectionTarget.toUrl(), depth + 1) : NULL;
	}
	*/

}
