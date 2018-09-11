/*
Launchy: Application Launcher
Copyright (C) 2007  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "precompiled.h"
#include "gcalc.h"


int Process::currentId = 0;


Process::Process(QString url, QString matchExpression) :
	url(url),
	matchExpression(matchExpression)
{
}


void Process::run()
{
	if (query.length() > 0)
	{
		QString webQuery = "/search?source=launchy&q=";
		query = QUrl::toPercentEncoding(query);
		webQuery += query;
		//	qDebug() << webQuery;
		resBuffer.open(QIODevice::ReadWrite);

		connect(&http, SIGNAL(done(bool)), this, SLOT(httpGetFinished(bool)));
		http.setHost("www.google.com");
		http.get(webQuery, &resBuffer);
		id = ++currentId;
		loop.exec();
	}
}


void Process::httpGetFinished(bool error)
{
	if (id != currentId)
	{
		result.clear();
	}
	else if (!error)
	{
		result = resBuffer.data();
		QRegExp regex_res(matchExpression, Qt::CaseInsensitive);
		regex_res.setMinimal(true);
		if (regex_res.indexIn(result) != -1)
		{
			result = regex_res.cap(regex_res.numCaptures());
			result = result.trimmed();
		}
		else
		{
			result = tr("Unknown");
		}
	}
	else
	{
		result = tr("Error");
	}
	loop.exit();
}


void gcalcPlugin::init()
{
	// Ensure settings are written to ini file
	QString url = (*settings)->value("gcalc/url", "/search?source=launchy&q=").toString();
	(*settings)->setValue("gcalc/url", url);

	QString matchExpression = (*settings)->value("gcalc/matchExpression", "<h2 class=r style=\"font-size:\\d+%\"><b>(.*)</b>").toString();
	(*settings)->setValue("gcalc/matchExpression", matchExpression);
}


void gcalcPlugin::getCatalog(QList<CatItem>* items)
{
	items->push_back(CatItem("GCalc.gcalc", "GCalc", HASH_gcalc, getIcon()));
}


void gcalcPlugin::getID(uint* id)
{
	*id = HASH_gcalc;
}


void gcalcPlugin::getName(QString* str)
{
	*str = "GCalc";
}


void gcalcPlugin::getResults(QList<InputData>* id, QList<CatItem>* results)
{
	if (id->count() != 2)
		return;

	const QString & text = id->first().getText();

	if (!text.contains("gcalc", Qt::CaseInsensitive))
		return;

	const QString & query = id->last().getText();

	QString url = (*settings)->value("gcalc/url", "/search?source=launchy&q=").toString();
	QString matchExpression = (*settings)->value("gcalc/matchExpression", "<h2 class=r style=\"font-size:\\d+%\"><b>(.*)</b>").toString();
	Process p(url, matchExpression);
	p.query = query;
	p.run();

	if (p.result.length() > 0)
	{
		for (int i = 0; i < results->count(); ++i)
		{
                        if (results->at(i).id == (int) HASH_gcalc)
			{
				results->removeAt(i);
				break;
			}
		}
		results->push_front(CatItem(p.result + ".gcalc", p.result, HASH_gcalc, getIcon()));
	}
}


QString gcalcPlugin::getIcon()
{
	return libPath + "/icons/calcy.png";
}


void gcalcPlugin::setPath(QString * path)
{
	libPath = *path;
}


int gcalcPlugin::msg(int msgId, void* wParam, void* lParam)
{
	bool handled = false;
	switch (msgId)
	{		
	case MSG_INIT:
		init();
		handled = true;
		break;
	case MSG_GET_ID:
		getID((uint*) wParam);
		handled = true;
		break;
	case MSG_GET_NAME:
		getName((QString*) wParam);
		handled = true;
		break;
	case MSG_GET_RESULTS:
		getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
		handled = true;
		break;
	case MSG_GET_CATALOG:
		getCatalog((QList<CatItem>*) wParam);
		handled = true;
		break;
	case MSG_PATH:
		setPath((QString *) wParam);
	default:
		break;
	}

	return handled;
}

Q_EXPORT_PLUGIN2(gcalc, gcalcPlugin) 
