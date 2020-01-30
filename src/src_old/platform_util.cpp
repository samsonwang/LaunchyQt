/*
#include <QPluginLoader>
#include <QDebug>
#include "plugin_interface.h"
#include "platform_util.h"
#include "globals.h"
#include "main.h"


PlatformBase * loadPlatform()
{
	QList<QString> files;
	QString file;

	#ifdef Q_WS_WIN
	// Look for the platform file in the same directory as the executable
	wchar_t module_name[MAX_PATH+1];
	GetModuleFileNameW(0, module_name, MAX_PATH);
	module_name[MAX_PATH] = 0;
	QFileInfo fileInfo = QString::fromUtf16((ushort*)module_name);
	QString filePath = fileInfo.path();
	SetCurrentDirectory(filePath.utf16());
	files += "platform_win.dll";
	#endif
	#ifdef Q_WS_X11
	files += "libplatform_unix.so";
	files += QString(PLATFORMS_PATH) + "/libplatform_unix.so";
	#endif

	QObject * plugin = NULL;
	foreach(QString file, files)
	{
		qDebug() << file;
		QPluginLoader loader(file);
		plugin = loader.instance();
		if (plugin)
			break;
		qDebug() << loader.errorString();
	}
	
	if (!plugin)
	{
		qDebug() << "Could not load platform file!";
		exit(1);
	}
	
	return qobject_cast<PlatformBase*>(plugin);
}
*/