// Copyright 2009 Fabian Hofsaess

#ifndef FHOREG_H
#define FHOREG_H


// implement a convenient class instead of an aggregation of static semi-convenient methods!
class FhoReg {
	private:
		static const DWORD maxSize = 256;

	public:
		static HKEY OpenKey(HKEY baseKey, QString &subKeyName, DWORD options);
		static void CloseKey(HKEY key);

		static QString GetKeyValue(HKEY key, QString &valueName);
		static QString GetKeyValue(HKEY parentKey, QString &parentSubKeyName, QString &valueName);
		static QString GetKeyDefaultValue(HKEY key);
		static QString GetKeyDefaultValue(HKEY parentKey, QString &parentSubKeyName);

		static QStringList* EnumValues(HKEY parentKey, QString &parentSubKeyName);
		static QStringList* EnumSubKeys(HKEY key);
		static QStringList* EnumSubKeys(HKEY parentKey, QString &parentSubKeyName);

};


#endif
