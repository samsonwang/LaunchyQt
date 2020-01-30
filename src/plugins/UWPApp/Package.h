#pragma once

#include <iostream>
#include <string>

#include <collection.h>
#include <Shlwapi.h>
#include <io.h>
#include <fcntl.h>
#include <AppxPackaging.h>
#include <Shobjidl.h>
#include <Dwmapi.h>

#include <QVector>
#include <QList>
#include <QString>

#include "PluginInterface.h"
#include "CatalogItem.h"

#include "Application.h"

class Package {
public:
    Package();
    Package(uint pluginId, QList<launchy::CatItem>* items, QString defaultIconPath);
    ~Package();

public:
    int findPackages();
    void packageInfo(Windows::ApplicationModel::Package^ package);
    HRESULT getManifestReader(_In_ LPCWSTR manifestFilePath, _Outptr_ IAppxManifestReader** reader);
    HRESULT readManifest(LPCWSTR manifestFilePath);
    void getXmlNamespaces(LPCWSTR path);
    HRESULT readManifestApplications(_In_ IAppxManifestReader* manifestReader);
    std::wstring getLogoKey();

private:
    uint pluginId;
    QList<launchy::CatItem>* items;

    QString name;
    QString fullName;
    QString installedLocation;
    QString defaultIconPath;
    QString namespaces;
    QVector<Application> apps;
};

