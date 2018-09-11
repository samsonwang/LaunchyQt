// Copyright 2009 Fabian Hofsaess

#include "precompiled.h"
#include "fhores.h"
#include "fhoenv.h"


const DWORD maxSize = 256;

QString FhoRes::getResourceString(HINSTANCE hLib, int resourceId) {
	QString resourceString;

	if (hLib != NULL) {
		TCHAR buffer[maxSize];

		int res = LoadString(hLib, abs(resourceId), buffer, maxSize);
		if (res != 0) {
			resourceString = QString::fromUtf16(buffer);
		}
	}

	return resourceString;
}

QString FhoRes::getResourceString(QString &resourceName, int resourceId) {
	// e.g. '@%SystemRoot%\system32\SHELL32.dll' ,-32517

	QString resourceString;

	QString libraryName =  FhoEnv::expand(resourceName);
	// remove leading @
	if (libraryName.startsWith('@')) {
		libraryName = libraryName.right(libraryName.length()-1);
	}

	// can we do this without loading the library (like for icons via ExtractIcon instead of LoadIcon/LoadImage)?
	HINSTANCE hLib = LoadLibrary((LPCTSTR) libraryName.utf16());
	if (hLib != NULL) {
		resourceString = getResourceString(hLib, resourceId);

		FreeLibrary(hLib);
	}

	return resourceString;
}

QString FhoRes::getResourceString(QString &resourceIdentifier) {
	// e.g. '@%SystemRoot%\system32\SHELL32.dll,-32517'

	QString resourceString;

	QStringList split = resourceIdentifier.split(",", QString::SkipEmptyParts);
	if (split.size() == 2) {
		QString resourceName = split.at(0);
		QString resourceIdStr = split.at(1);
		int resourceId = resourceIdStr.toInt();

		resourceString = getResourceString(resourceName, resourceId);
	}

	return resourceString;
}

