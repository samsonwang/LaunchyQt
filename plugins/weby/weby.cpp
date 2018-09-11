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
#include "weby.h"
#include "IconCache.h"
#include "gui.h"


int Suggest::currentId = 0;

Suggest::Suggest()
{
	connect(&http, SIGNAL(done(bool)), this, SLOT(httpGetFinished(bool)));
}


void Suggest::run(QString url, QString query)
{
	this->query = query;
	url.replace("%s", QUrl::toPercentEncoding(query));
	QUrl u = QUrl::fromPercentEncoding(url.toAscii());

	http.setHost(u.host(), u.port(80));
	http.get(u.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority));
	id = ++currentId;
	loop.exec();
}


void Suggest::httpGetFinished(bool error)
{
	if (id == currentId)
	{
		if (query.count() > 0)
		{
			results << query;
		}

		if (!error)
		{
			QRegExp regex("\\[.*\\[(.*)\\]\\]");
			QRegExp rx("\"((?:[^\\\\\"]|\\\\\")*)\"");

			QString text = http.readAll();
			if (regex.indexIn(text) != -1)
			{
				QString csv = regex.cap(1);

				int pos = 0;
				while ((pos = rx.indexIn(csv, pos)) != -1)
				{
					QString result = rx.cap(1);
					if (result.count() > 0)
						results << result;

					pos += rx.matchedLength();
				}
			}
		}
		loop.exit();
	}
}




void WebyPlugin::init()
{
	QSettings* set = *settings;

	// get config / settings directory (base for 'temporary' icon cache dir)
	QString iniFilename = set->fileName();
	QFileInfo info(iniFilename);
	iconCachePath = info.absolutePath() + "/weby-icon-cache/";
	iconCache = new IconCache(iconCachePath);
	iconCache->setParent(this);
	connect(iconCache, SIGNAL(findIcon(QUrl)), iconCache, SLOT(query(QUrl)));

	double version = set->value("weby/version",0.0).toDouble();

	if ( version == 0.0 )
	{
		int i = 0;
		set->beginWriteArray("weby/sites");

		set->setArrayIndex(i++);
		set->setValue("name", "Google");
		set->setValue("query", "http://www.google.com/search?source=launchy&q=%1");
		//set->setValue("suggest", "http://suggestqueries.google.com/complete/search?output=firefox&q=%1");
		//set->setValue("default", true);

		set->setArrayIndex(i++);
		set->setValue("name", "Bing");
		set->setValue("query", "http://www.bing.com/search?q=%1");

		set->setArrayIndex(i++);
		set->setValue("name", "Yahoo");
		set->setValue("query", "http://search.yahoo.com/search?p=%1");
		//set->setValue("suggest", "http://ff.search.yahoo.com/gossip?output=fxjson&command=%1");

		set->setArrayIndex(i++);
		set->setValue("name", "Weather");
		set->setValue("query", "http://www.weather.com/weather/local/%1");	

		set->setArrayIndex(i++);
		set->setValue("name", "Amazon");
		set->setValue("query", "http://www.amazon.com/gp/search?keywords=%1&index=blended");

		set->setArrayIndex(i++);
		set->setValue("name", "YouTube");
		set->setValue("query", "http://www.youtube.com/results?search_query=%1");

		set->setArrayIndex(i++);
		set->setValue("name", "Wikipedia");
		set->setValue("query", "http://en.wikipedia.org/wiki/Special:Search?search=%1&fulltext=Search");
		//set->setValue("suggest", "http://en.wikipedia.org/w/api.php?action=opensearch&search=%1");

		set->setArrayIndex(i++);
		set->setValue("name", "Dictionary");
		set->setValue("query", "http://dictionary.reference.com/browse/%1");		

		set->setArrayIndex(i++);
		set->setValue("name", "Thesaurus");
		set->setValue("query", "http://thesaurus.reference.com/browse/%1");		

		set->setArrayIndex(i++);
		set->setValue("name", "Netflix");
		set->setValue("query", "http://www.netflix.com/Search?v1=%1");		

		set->setArrayIndex(i++);
		set->setValue("name", "MSDN");
		set->setValue("query", "http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=%1");

		set->setArrayIndex(i++);
		set->setValue("name", "E-Mail");
		set->setValue("query", "mailto:%1");

		set->setArrayIndex(i++);
		set->setValue("name", "IMDB");
		set->setValue("query", "http://www.imdb.com/find?s=all&q=%1");

		set->setArrayIndex(i++);
		set->setValue("name", "Maps");
		set->setValue("query", "http://maps.google.com/maps?f=q&hl=en&geocode=&q=%1&ie=UTF8&z=12&iwloc=addr&om=1");

		set->endArray();
	}

	// Read in the array of websites
	sites.clear();

	int count = set->beginReadArray("weby/sites");
	for (int i = 0; i < count; ++i)
	{
		set->setArrayIndex(i);
		WebySite s;
		s.name = set->value("name").toString();
		s.query = set->value("query").toString();
		s.suggest = set->value("suggest").toString();
		s.def = set->value("default", false).toBool();

		// Ditched the 'base' value between 1.0 and 2.0
		// Also replaced %s with %1,%2,%3...
		if (version == 2.0) {
			s.query.replace("%s","%1");
			s.query = set->value("base").toString() + s.query;			
/*
			Out of scope for 2.2
			if (s.name == "Google")
				s.suggest = "http://suggestqueries.google.com/complete/search?output=firefox&q=%1";
			else if (s.name == "Yahoo")
				s.suggest = "http://ff.search.yahoo.com/gossip?output=fxjson&command=%1";
			else if (s.name == "Wikipedia")
				s.suggest = "http://en.wikipedia.org/w/api.php?action=opensearch&search=%1";
*/
		}
		
		sites.push_back(s);
	}
	set->endArray();

	// Save any upgrades we made from 2.0
	if (version == 2.0) {
		set->beginWriteArray("weby/sites");
		for(int i = 0; i < sites.count(); i++) {
			set->setArrayIndex(i);
			set->setValue("name", sites[i].name);
			set->setValue("query", sites[i].query);
			set->setValue("suggest", sites[i].suggest);
			set->setValue("default", sites[i].def);
		}
		set->endArray();
	}
	
	set->setValue("weby/version", 2.2);
}

void WebyPlugin::getID(uint* id)
{
	*id = HASH_WEBY;
}

void WebyPlugin::getName(QString* str)
{
	*str = "Weby";
}

void WebyPlugin::getLabels(QList<InputData>* inputData)
{
	if (inputData->count() > 1)
		return;

	// Apply a "website" label if we think it's a website
	const QString & text = inputData->last().getText();

	QString defaultMatchExpression = "^(http|https|ftp)://|^www.|.com|.co.[a-z]{2,}|.net|.org";
	QString matchExpression = (*settings)->value("weby/UrlRegExp", defaultMatchExpression).toString();
	QRegExp regex(matchExpression);
	if (!regex.isValid())
	{
		qDebug() << QString("Settings match expression \"%1\" is invalid. Using default.").arg(matchExpression);
		regex = QRegExp(defaultMatchExpression);
	}
	if (regex.indexIn(text) != -1)
	{
		inputData->last().setLabel(HASH_WEBSITE);
	}
}



void WebyPlugin::getResults(QList<InputData>* inputData, QList<CatItem>* results)
{
	if (inputData->last().hasLabel(HASH_WEBSITE))
	{
		const QString & text = inputData->last().getText();
		// This is a website, create an entry for it
		if (!text.trimmed().isEmpty())
		{
			results->push_front(CatItem(text + ".weby", text, HASH_WEBY, getIcon()));
		}
	}

	if (inputData->count() > 1 && (unsigned int)inputData->first().getTopResult().id == HASH_WEBY)
	{
		const QString & text = inputData->last().getText();
		// This is user search text, create an entry for it
		QString suggestUrl;
		CatItem* item = &inputData->first().getTopResult();

		foreach(WebySite site, sites)
		{
			if (item->shortName == site.name)
			{
				suggestUrl = site.suggest;
				break;
			}
		}

		if (!suggestUrl.isEmpty() && !text.trimmed().isEmpty())
		{
			// query the web
			Suggest suggest;
			suggest.run(suggestUrl, text);

			foreach(QString res, suggest.results)
			{
				results->push_back(CatItem(res + ".weby", res, HASH_WEBY, item->icon));
			}
		}
		else
		{
			results->push_front(CatItem(text + ".weby", text, HASH_WEBY, item->icon));
		}
	}

	// If we don't have any results, add default
	if (results->size() == 0 && inputData->count() <= 1)
	{
		const QString & text = inputData->last().getText();
		if (!text.trimmed().isEmpty())
		{
			QString name = getDefault().name;
			if (name != "")
			{
				inputData->first().setLabel(HASH_DEFAULTSEARCH);
				results->push_back(CatItem(text + ".weby", name, HASH_WEBY, getIcon()));
			}
		}
	}
}


#ifdef Q_WS_WIN

QString GetShellDirectory(int type)
{
	wchar_t buffer[_MAX_PATH];
	SHGetFolderPath(NULL, type, NULL, 0, buffer);
	return QString::fromUtf16(buffer);
}


void WebyPlugin::indexIE(QString path, QList<CatItem>* items)
{
	QDir qd(path);
	QString dir = qd.absolutePath();
	QStringList dirs = qd.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

	for (int i = 0; i < dirs.count(); ++i)
	{
		QString cur = dirs[i];
		if (cur.contains(".lnk"))
			continue;
		indexIE(dir + "/" + dirs[i],items);
	}	

	QStringList files = qd.entryList(QStringList("*.url"), QDir::Files, QDir::Unsorted);
	for (int i = 0; i < files.count(); ++i)
	{
		items->push_back(CatItem(dir + "/" + files[i], files[i].mid(0,files[i].size()-4)));
	}
}

#endif


QString WebyPlugin::getFirefoxPath()
{
	QString path;
	QString osPath;

#ifdef Q_WS_WIN
	osPath = GetShellDirectory(CSIDL_APPDATA) + "/Mozilla/Firefox/";
#endif

#ifdef Q_WS_X11
	osPath = QDir::homePath() + "/.mozilla/firefox/";
#endif

#ifdef Q_WS_MAC
        osPath = QDir::homePath() + "/Library/Application Support/Firefox/";
#endif
	QString iniPath = osPath + "profiles.ini";

	QFile file(iniPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return "";
	bool isRel = false;
	while (!file.atEnd())
	{
		QString line = file.readLine();
		if (line.contains("IsRelative"))
		{
			QStringList spl = line.split("=");
			isRel = spl[1].toInt() != 0;
		}
		if (line.contains("Path"))
		{
			QStringList spl = line.split("=");
			if (isRel)
				path = osPath;
			path += spl[1].mid(0,spl[1].count()-1) + "/bookmarks.html";
			break;
		}
	}

	return path;
}


QString WebyPlugin::getIcon()
{
	return libPath + "/icons/weby.png";
}


void WebyPlugin::indexFirefox(QString path, QList<CatItem>* items)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	marks.clear();

	QRegExp regex_url("<a href=\"([^\"]*)\"", Qt::CaseInsensitive);
	QRegExp regex_urlname("\">([^<]*)</A>", Qt::CaseInsensitive);
	QRegExp regex_shortcut("SHORTCUTURL=\"([^\"]*)\"");
	QRegExp regex_postdata("POST_DATA", Qt::CaseInsensitive);

	while (!file.atEnd())
	{
		QString line = QString::fromUtf8(file.readLine());
		if (regex_url.indexIn(line) != -1)
		{
			Bookmark mark;
			mark.url = regex_url.cap(1);

			if (regex_urlname.indexIn(line) != -1)
			{
				mark.name = regex_urlname.cap(1);

				if (regex_postdata.indexIn(line) != -1)
					continue;
				if (regex_shortcut.indexIn(line) != -1)
				{
					mark.shortcut = regex_shortcut.cap(1);
					marks.push_back(mark);
					items->push_back(CatItem(mark.url + ".shortcut", mark.shortcut, HASH_WEBY, getIcon()));
				}
				else
				{
					items->push_back(CatItem(mark.url, mark.name, 0, getIcon()));
				}
			}
		}
	}
}


WebySite WebyPlugin::getDefault()
{
	foreach(WebySite site, sites)
	{
		if (site.def)
		{
			return site;
		}
	}
	return WebySite();
}


void WebyPlugin::getCatalog(QList<CatItem>* items)
{
	foreach(WebySite site, sites)
	{
		QString iconName = iconCache->getIconPath(site.query);
		items->push_back(CatItem(site.name + ".weby", site.name, HASH_WEBY,
			iconName.length() > 0 ? iconName : getIcon()));
	}

#ifdef Q_WS_WIN
	if ((*settings)->value("weby/ie", true).toBool())
	{
		QString path = GetShellDirectory(CSIDL_FAVORITES);
		indexIE(path, items);
	}
#endif
	if ((*settings)->value("weby/firefox", true).toBool())
	{
		QString path = getFirefoxPath();
		indexFirefox(path, items);
	}
}


void WebyPlugin::launchItem(QList<InputData>* inputData, CatItem* item)
{
	QString file = "";
	QStringList args;
	
	int i = inputData->count() == 1 && inputData->first().hasLabel(HASH_DEFAULTSEARCH) ? 0 : 1;
	for (; i < inputData->count(); i++)
	{
		QString txt = inputData->at(i).getText();
		args.push_back(QUrl::toPercentEncoding(txt));
	}

	// Is it a Firefox shortcut?
	if (item->fullPath.contains(".shortcut"))
	{
		file = item->fullPath.mid(0, item->fullPath.count()-9);
		file.replace("%s",args[0]);
	}
	else
	{
		// It's a user-specific site
		bool found = false;
		foreach(WebySite site, sites)
		{
			if (item->shortName == site.name)
			{
				found = true;
				file = site.query;
				if (args.count() == 0)
				{
					// if no addition parameters have been entered and the weby URL has placeholders to take parameters,
					// strip the URL down to its root and launch that
					if (file.contains("%1"))
					{
						QRegExp regex("^(([a-z]*://)?([^/]*))");
						if (regex.indexIn(file) != -1)
						{
							file = regex.cap(0);
						}
					}
				}
				else
				{
					// Build the new string
					QString out;
					int curArg = 0;
					for(int i = 0; i < file.size()-1; i++) {
						QChar curchar = file[i];
						QChar nextchar = file[i+1];
						if (curchar == '%' && nextchar >= '0' && nextchar <= '9') {							
							i += 1;
							if (curArg < args.size()) {
								out += args[curArg++];
							}
						} else {
							out += curchar;
						}
					}
					file = out;
				}
				break;
			}
		}

		if (!found)
		{
			file = item->shortName;
			// Make sure we have a protocol
			if (!file.startsWith("http://") && !file.startsWith("https://") && !file.startsWith("ftp://"))
			{
				file = "http://" + file;
			}
		}
	}

	QUrl url(file);
	runProgram(url.toString(), "", false);
}


void WebyPlugin::doDialog(QWidget* parent, QWidget** newDlg)
{
	if (gui != NULL)
		return;
	gui.reset(new Gui(parent, *settings));
	*newDlg = gui.get();
}


void WebyPlugin::endDialog(bool accept)
{
	if (accept)
	{
		gui->writeOptions();
		init();
	}
	gui.reset();
}


void WebyPlugin::setPath(QString * path)
{
	libPath = *path;
}


int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
{
	bool handled = false;
	switch (msgId)
	{		
	case MSG_INIT:
		init();
		handled = true;
		break;
	case MSG_GET_LABELS:
		getLabels((QList<InputData>*) wParam);
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
	case MSG_LAUNCH_ITEM:
		launchItem((QList<InputData>*) wParam, (CatItem*) lParam);
		handled = true;
		break;
	case MSG_HAS_DIALOG:
		handled = true;
		break;
	case MSG_DO_DIALOG:
		doDialog((QWidget*) wParam, (QWidget**) lParam);
		break;
	case MSG_END_DIALOG:
		endDialog(wParam != 0);
		break;
	case MSG_PATH:
		setPath((QString *) wParam);
	default:
		break;
	}

	return handled;
}

Q_EXPORT_PLUGIN2(weby, WebyPlugin)
