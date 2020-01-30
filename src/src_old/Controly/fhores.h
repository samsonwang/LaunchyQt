// Copyright 2009 Fabian Hofsaess

#ifndef FHORES_H
#define FHORES_H


class FhoRes {

	public:

		static QString getResourceString(QString &resourceIdentifier);
		static QString getResourceString(QString &resourceName, int resourceId);
		static QString getResourceString(HINSTANCE hLib, int resourceId);

};


#endif
