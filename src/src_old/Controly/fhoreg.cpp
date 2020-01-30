// Copyright 2009 Fabian Hofsaess

#include "precompiled.h"
#include "fhoreg.h"


// currently, we assume that all reg values are returned as strings

HKEY FhoReg::OpenKey(HKEY baseKey, QString &subKeyName, DWORD options) {
	HKEY k;
	
	LONG l = RegOpenKeyEx(baseKey,
						  subKeyName.utf16(),
						  0,
						  options,
						  &k);
	
	if (l == ERROR_SUCCESS) {
		return k;
	}
	return 0;
}

void FhoReg::CloseKey(HKEY key) {
	if (key != 0) {
		RegCloseKey(key);
	}
}

QStringList* FhoReg::EnumValues(HKEY parentKey, QString &parentSubKeyName) {
	QStringList *resultList = new QStringList();

	HKEY k = OpenKey(parentKey, parentSubKeyName, KEY_QUERY_VALUE);
	
	if (k != 0) {
		DWORD idx = 0;
		TCHAR valueName[maxSize];
		BYTE valueData[maxSize];

		LONG l;
		do {
			DWORD size = maxSize;
			DWORD size2 = maxSize;
			DWORD type;
			l = RegEnumValue(k,
							 idx,
							 valueName,
							 &size,
							 NULL,
							 &type,
							 valueData,
							 &size2);

			if (l == ERROR_SUCCESS) {
				QString name = QString::fromUtf16(valueName);
				QString data = QString::fromUtf16((const ushort*) valueData);

				resultList->append(data);
			}

			idx++;
		} while (l != ERROR_NO_MORE_ITEMS);

		CloseKey(k);
	}

	return resultList;
}

QStringList* FhoReg::EnumSubKeys(HKEY key) {
	QStringList *resultList = new QStringList();

	if (key != 0) {
		DWORD idx = 0;
		TCHAR keyName[maxSize];

		LONG l;
		do {
			DWORD size = maxSize;
			l = RegEnumKeyEx(key,
				 		     idx,
							 keyName,
							 &size,
							 NULL,
							 NULL,
							 NULL,
							 NULL);

			if (l == ERROR_SUCCESS) {
				QString subKeyName = QString::fromUtf16(keyName);

				resultList->append(subKeyName);
			}

			idx++;
		} while (l != ERROR_NO_MORE_ITEMS);
	}

	return resultList;
}

QStringList* FhoReg::EnumSubKeys(HKEY parentKey, QString &parentSubKeyName) {
	QStringList *resultList;

	HKEY k = OpenKey(parentKey, parentSubKeyName, KEY_ENUMERATE_SUB_KEYS);
	
	if (k != 0) {
		resultList = EnumSubKeys(k);

		CloseKey(k);
	} else {
		resultList = new QStringList();
	}

	return resultList;
}

QString FhoReg::GetKeyValue(HKEY key, QString &valueName) {
	DWORD type;
	BYTE keyVal[maxSize];
	DWORD sz = maxSize;

	LONG l = RegQueryValueEx(key,
							 (!valueName.isEmpty()) ? valueName.utf16() : NULL,
							 NULL,
							 &type,
							 keyVal,
							 &sz);

	if (l == ERROR_SUCCESS) {
		QString keyValue = QString::fromUtf16((const ushort*) keyVal);
		return keyValue;
	}
	return NULL;
}

QString FhoReg::GetKeyValue(HKEY parentKey, QString &parentSubKeyName, QString &valueName) {
	HKEY k = OpenKey(parentKey, parentSubKeyName, KEY_QUERY_VALUE);

	QString value;

	if (k != 0) {
		value = GetKeyValue(k, valueName);

		CloseKey(k);
	}

	return value;
}

QString FhoReg::GetKeyDefaultValue(HKEY key) {
	return GetKeyValue(key, QString());
}

QString FhoReg::GetKeyDefaultValue(HKEY parentKey, QString &parentSubKeyName) {
	HKEY k = OpenKey(parentKey, parentSubKeyName, KEY_QUERY_VALUE);

	QString value;

	if (k != 0) {
		value = GetKeyDefaultValue(k);

		CloseKey(k);
	}

	return value;
}


