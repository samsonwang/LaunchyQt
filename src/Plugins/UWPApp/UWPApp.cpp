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

#include "UWPApp.h"

#include <windows.h>
#include <Shobjidl.h>
#include <objbase.h>
#include <propvarutil.h>

#include <string>

#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QXmlStreamReader>

#include "LaunchyLib/PluginMsg.h"

DEFINE_GUID(BHID_EnumItems, 0x94f60519, 0x2850, 0x4924, 0xaa, 0x5a, 0xd1, 0x5e, 0x84, 0x86, 0x80, 0x39);
DEFINE_GUID(BHID_PropertyStore, 0x0384e1a4, 0x1523, 0x439c, 0xa4, 0xc8, 0xab, 0x91, 0x10, 0x52, 0xf5, 0x86);

static const char* PLUGIN_NAME = "UWPApp";

namespace {

// ---------------------------------------------------------------------------
// Property store access
// ---------------------------------------------------------------------------

// PSGetPropertyKeyFromName leaves the key zeroed when it does not know the
// requested name (an old Windows, or a typo). Reading through such a key would
// return whatever happens to live there, so every read checks it first.
bool isPropertyKeySet(const PROPERTYKEY& key) {
    static const GUID kUnsetKey = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
    return !IsEqualGUID(key.fmtid, kUnsetKey);
}

PROPERTYKEY lookupPropertyKey(const wchar_t* name) {
    PROPERTYKEY key;
    ZeroMemory(&key, sizeof(key));
    if (FAILED(PSGetPropertyKeyFromName(name, &key))) {
        qWarning() << "UWPApp, unknown shell property:"
                   << QString::fromWCharArray(name);
    }
    return key;
}

// Read a property as a string. PropVariantToStringAlloc allocates exactly the
// buffer that is needed, which removes the fixed 512 character scratch buffer
// used before: a long package path was silently truncated there, and the
// failure of the conversion was not checked at all.
bool readPropertyString(IPropertyStore* store, const PROPERTYKEY& key, QString* value) {
    if (store == nullptr || value == nullptr || !isPropertyKeySet(key)) {
        return false;
    }

    PROPVARIANT prop;
    PropVariantInit(&prop);

    bool ok = false;
    if (SUCCEEDED(store->GetValue(key, &prop))) {
        PWSTR text = nullptr;
        if (SUCCEEDED(PropVariantToStringAlloc(prop, &text)) && text != nullptr) {
            *value = QString::fromWCharArray(text);
            CoTaskMemFree(text);
            ok = !value->isEmpty();
        }
    }

    PropVariantClear(&prop);
    return ok;
}

// ---------------------------------------------------------------------------
// Package layout
// ---------------------------------------------------------------------------

// Resolve the package root from the package full name. Used when the entry does
// not publish "System.AppUserModel.PackageInstallPath", which happens for
// packaged desktop apps (MSIX / desktop bridge). Loaded dynamically because the
// API is Windows 8+: a hard import would stop the plugin from loading at all on
// an older system, where it can still report an empty catalog.
typedef LONG (WINAPI *GetPackagePathByFullNameFn)(PCWSTR, UINT32*, PWSTR);

QString packageRootFromFullName(const QString& packageFullName) {
    static GetPackagePathByFullNameFn getPackagePathByFullName =
        reinterpret_cast<GetPackagePathByFullNameFn>(
            ::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"),
                             "GetPackagePathByFullName"));
    if (getPackagePathByFullName == nullptr) {
        return QString();
    }

    const std::wstring fullName = packageFullName.toStdWString();

    UINT32 length = 0;
    if (getPackagePathByFullName(fullName.c_str(), &length, nullptr) != ERROR_INSUFFICIENT_BUFFER) {
        return QString();
    }

    std::wstring buffer(length, L'\0');
    if (getPackagePathByFullName(fullName.c_str(), &length, &buffer[0]) != ERROR_SUCCESS) {
        return QString();
    }

    // The API appends a trailing separator, which QDir handles fine
    return QString::fromWCharArray(buffer.c_str());
}

// Turn the logo reference of a manifest ("Assets\Square44x44Logo.png",
// "Assets/Square44x44Logo.png" or "ms-appx:///Assets/Square44x44Logo.png") into
// a relative path using the '/' separator the rest of the code works with.
QString normalizedAssetReference(const QString& raw) {
    QString path = raw.trimmed();

    const QString scheme = QStringLiteral("ms-appx:///");
    if (path.startsWith(scheme, Qt::CaseInsensitive)) {
        path = path.mid(scheme.length());
    }

    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (path.startsWith(QLatin1Char('/'))) {
        path.remove(0, 1);
    }
    return path;
}

// ---------------------------------------------------------------------------
// Manifest reading
// ---------------------------------------------------------------------------

// Read the logo reference from the package manifest. The shell does not publish
// "System.Tile.SmallLogoPath" for every entry (packaged desktop apps and a few
// inbox apps), and those entries used to end up without any icon.
QString logoReferenceFromManifest(const QString& packageRoot, const QString& aumid) {
    if (packageRoot.isEmpty()) {
        return QString();
    }

    QFile manifest(QDir(packageRoot).filePath(QStringLiteral("AppxManifest.xml")));
    if (!manifest.open(QIODevice::ReadOnly)) {
        qWarning() << "UWPApp, fail to open the manifest of" << packageRoot;
        return QString();
    }

    // The AUMID is "<PackageFamilyName>!<Application Id>", which selects the
    // matching <Application> in packages that declare more than one.
    const int separator = aumid.indexOf(QLatin1Char('!'));
    const QString applicationId = separator >= 0 ? aumid.mid(separator + 1) : QString();

    QString currentApplicationId;
    QString matchedSquare44x44;
    QString matchedSquare150x150;
    QString anySquare44x44;
    QString anySquare150x150;
    QString propertiesLogo;

    QXmlStreamReader reader(&manifest);
    while (!reader.atEnd() && !reader.hasError()) {
        const QXmlStreamReader::TokenType token = reader.readNext();

        if (token == QXmlStreamReader::EndElement
            && reader.name() == QLatin1String("Application")) {
            currentApplicationId.clear();
            continue;
        }
        if (token != QXmlStreamReader::StartElement) {
            continue;
        }

        const QStringRef element = reader.name();

        if (element == QLatin1String("Application")) {
            currentApplicationId = reader.attributes().value(QLatin1String("Id")).toString();
            continue;
        }

        if (element == QLatin1String("Logo")) {
            // <Properties><Logo>Assets\StoreLogo.png</Logo></Properties>
            const QString value = reader.readElementText().trimmed();
            if (propertiesLogo.isEmpty()) {
                propertiesLogo = value;
            }
            continue;
        }

        if (element != QLatin1String("VisualElements")) {
            continue;
        }

        // Note: not "small"/"large" - windows.h defines those as macros
        const QXmlStreamAttributes attributes = reader.attributes();
        const QString logo44x44 = attributes.value(QLatin1String("Square44x44Logo")).toString().trimmed();
        const QString logo150x150 = attributes.value(QLatin1String("Square150x150Logo")).toString().trimmed();

        // Prefer the VisualElements of the application this AUMID points at and
        // fall back to the first one in the manifest.
        const bool matchesApplication = applicationId.isEmpty()
            || currentApplicationId.isEmpty()
            || currentApplicationId == applicationId;
        if (matchesApplication) {
            if (matchedSquare44x44.isEmpty()) {
                matchedSquare44x44 = logo44x44;
            }
            if (matchedSquare150x150.isEmpty()) {
                matchedSquare150x150 = logo150x150;
            }
        }
        if (anySquare44x44.isEmpty()) {
            anySquare44x44 = logo44x44;
        }
        if (anySquare150x150.isEmpty()) {
            anySquare150x150 = logo150x150;
        }
    }

    if (reader.hasError()) {
        qWarning() << "UWPApp, fail to parse the manifest of" << packageRoot
                   << reader.errorString();
    }

    // "Square44x44Logo" is what the Start menu uses. "Properties/Logo" is the
    // store logo Windows falls back to, and "Square150x150Logo" is mandatory for
    // UWP packages, so it is the last useful resort.
    const QString candidates[] = {
        matchedSquare44x44, propertiesLogo, matchedSquare150x150,
        anySquare44x44, anySquare150x150,
    };
    for (const QString& candidate : candidates) {
        if (!candidate.isEmpty()) {
            return candidate;
        }
    }
    return QString();
}

// ---------------------------------------------------------------------------
// Asset selection
// ---------------------------------------------------------------------------

// Qualifiers a package appends to the asset name from the manifest. The
// unplated variants are listed first: the plated ones bake in an opaque theme
// coloured background plate that looks wrong inside Launchy. Move them below
// the ".scale-*" entries if a light skin makes the transparent logos hard to
// see. The empty entry is last so that the asset exactly as the manifest refers
// to it is still found.
const char* const kAssetQualifiers[] = {
    ".targetsize-256_altform-unplated",
    ".targetsize-48_altform-unplated",
    ".targetsize-44_altform-unplated",
    ".altform-unplated_targetsize-256",
    ".altform-unplated_targetsize-48",
    ".altform-unplated_targetsize-44",
    ".altform-unplated",
    ".scale-200",
    ".scale-400",
    ".scale-125",
    ".scale-150",
    ".scale-100",
    ".targetsize-256",
    ".targetsize-48",
    ".targetsize-44",
    ".targetsize-24",
    ".targetsize-16",
    "",
};

// Score a candidate file so the best variant wins. Higher is better, and the
// plate rank dominates the size rank.
int scoreAssetFile(const QString& fileName, const QString& stem, const QString& extension) {
    const QString lower = fileName.toLower();

    int plate = 1;
    if (lower.contains(QStringLiteral("altform-unplated"))) {
        plate = 4;
    }
    else if (lower.contains(QStringLiteral("altform-lightunplated"))
             || lower.contains(QStringLiteral("_unplated"))) {
        plate = 3;
    }
    else if (lower.contains(QStringLiteral("contrast-"))) {
        plate = 0;      // high contrast / monochrome variants come last
    }

    // Express both qualifier families as an approximate pixel size so they can
    // be compared: "targetsize-256" is 256 px, "scale-400" is 4x the base asset
    // (about 100 px for a 25 px base), which puts a 256 px target first.
    int size = 0;
    const int targetSize = lower.indexOf(QStringLiteral("targetsize-"));
    if (targetSize >= 0) {
        size = lower.mid(targetSize + 11).section(QLatin1Char('.'), 0, 0)
                    .section(QLatin1Char('_'), 0, 0).toInt();
    }
    else {
        const int scale = lower.indexOf(QStringLiteral("scale-"));
        if (scale >= 0) {
            size = lower.mid(scale + 6).section(QLatin1Char('.'), 0, 0)
                        .section(QLatin1Char('_'), 0, 0).toInt() / 4;
        }
    }

    if (fileName.compare(stem + extension, Qt::CaseInsensitive) == 0) {
        size += 5;      // the unqualified asset: assume the plain 1x image
    }

    return plate * 10000 + size;
}

// Return the best variant of "stem + extension" inside one directory, or an
// empty string when there is no match at all.
QString bestAssetFile(const QString& directory,
                      const QString& stem,
                      const QString& extension,
                      bool recursive) {
    const QDir dir(directory);
    if (stem.isEmpty() || !dir.exists()) {
        return QString();
    }

    // No name filter is used on purpose: QDir wildcard matching is case
    // sensitive depending on the platform, and package asset casing is not
    // guaranteed. Filtering by hand also lets the dot check below run.
    QDirIterator it(directory,
                    QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                    recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);

    QString best;
    int bestScore = -1;
    while (it.hasNext()) {
        it.next();
        const QString fileName = it.fileName();

        if (!fileName.endsWith(extension, Qt::CaseInsensitive)
            || !fileName.startsWith(stem, Qt::CaseInsensitive)) {
            continue;
        }

        // A qualified variant always continues after a dot
        // ("Square44x44Logo.scale-200.png"). Requiring that keeps the search
        // from latching onto an unrelated logo that only shares the prefix.
        if (fileName.compare(stem, Qt::CaseInsensitive) != 0
            && fileName.at(stem.length()) != QLatin1Char('.')) {
            continue;
        }

        const int score = scoreAssetFile(fileName, stem, extension);
        if (score > bestScore) {
            bestScore = score;
            best = it.filePath();
        }
    }

    return best;
}

// Resolve the manifest asset reference to a real file inside the package.
QString resolveAssetFile(const QString& packageRoot, const QString& assetReference) {
    if (packageRoot.isEmpty()) {
        return QString();
    }

    const QString relative = normalizedAssetReference(assetReference);
    if (relative.isEmpty()) {
        return QString();
    }

    const QFileInfo info(relative);
    if (info.isAbsolute()) {
        // Nothing to join the package root with; such a reference is not
        // something the shell ever produces.
        return QString();
    }

    const QString stem = info.suffix().isEmpty() ? info.fileName() : info.completeBaseName();
    // Some manifests omit the extension
    const QString extension = QStringLiteral(".")
        + (info.suffix().isEmpty() ? QStringLiteral("png") : info.suffix());
    if (stem.isEmpty()) {
        return QString();
    }

    const QDir packageDir(packageRoot);
    const QString relativeDir = info.path();
    const QString directory = relativeDir.isEmpty()
        ? packageDir.absolutePath()
        : packageDir.filePath(relativeDir);

    // 1. Probes by name. Reading a file whose exact path is known keeps working
    //    even when the package folder cannot be enumerated, so this is tried
    //    before any directory listing.
    for (const char* qualifier : kAssetQualifiers) {
        const QString path = directory + QLatin1Char('/') + stem
            + QLatin1String(qualifier) + extension;
        if (QFile::exists(path)) {
            return path;
        }
    }

    // 2. Whatever the package actually ships, ranked by the scoring above.
    QString found = bestAssetFile(directory, stem, extension, false);
    if (!found.isEmpty()) {
        return found;
    }

    // 3. Same, but covering sub folders of the asset directory. Never the
    //    package root itself: that holds every DLL and every language folder.
    found = bestAssetFile(directory, stem, extension, true);
    if (!found.isEmpty()) {
        return found;
    }

    // 4. Some packages keep the visuals in a differently named folder than the
    //    manifest says, or the manifest folder does not exist at all.
    const char* const kFallbackDirs[] = { "Assets", "assets", "images", "Images", "VisualAssets" };
    for (const char* name : kFallbackDirs) {
        found = bestAssetFile(packageDir.filePath(QLatin1String(name)), stem, extension, true);
        if (!found.isEmpty()) {
            return found;
        }
    }

    qWarning() << "UWPApp, no asset matches" << assetReference << "under" << packageRoot;
    return QString();
}

// ---------------------------------------------------------------------------
// Shell rendered fallback
// ---------------------------------------------------------------------------

// Qt 5 has no HBITMAP loader (QImage::fromHICON only arrived in Qt 6.4), so the
// pixels are pulled out with GetDIBits.
QImage imageFromBitmap(HBITMAP bitmap) {
    BITMAP header;
    if (bitmap == nullptr || GetObject(bitmap, sizeof(header), &header) != sizeof(header)
        || header.bmWidth <= 0 || header.bmHeight <= 0) {
        return QImage();
    }

    BITMAPINFO info;
    ZeroMemory(&info, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = header.bmWidth;
    info.bmiHeader.biHeight = -header.bmHeight;     // negative: top-down rows
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    // The shell hands out premultiplied BGRA, which is exactly the memory layout
    // of Format_ARGB32_Premultiplied.
    QImage image(header.bmWidth, header.bmHeight, QImage::Format_ARGB32_Premultiplied);
    if (image.isNull()) {
        return QImage();
    }

    HDC dc = CreateCompatibleDC(nullptr);
    if (dc == nullptr) {
        return QImage();
    }
    const int rows = GetDIBits(dc, bitmap, 0, header.bmHeight, image.bits(), &info, DIB_RGB_COLORS);
    DeleteDC(dc);

    if (rows != header.bmHeight) {
        qWarning() << "UWPApp, GetDIBits returned" << rows << "of" << header.bmHeight << "rows";
        return QImage();
    }

    return image;
}

bool hasVisiblePixel(const QImage& image) {
    if (!image.hasAlphaChannel()) {
        return true;
    }
    for (int y = 0; y < image.height(); ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(line[x]) != 0) {
                return true;
            }
        }
    }
    return false;
}

// Last resort: let the shell render the icon of the AppsFolder entry itself.
// This covers entries whose assets cannot be matched inside the package - no
// assets folder at all, an unreadable manifest, a generated logo. The result is
// written to "<user temp>/Launchy/icon" rather than a cache: the catalog is
// rebuilt periodically, so the bitmap does not need to outlive one run, and
// there is nothing to manage or prune.
QString extractShellIcon(IShellItem* shellItem, const QString& aumid) {
    if (shellItem == nullptr || aumid.isEmpty()) {
        return QString();
    }

    IShellItemImageFactory* factory = nullptr;
    if (FAILED(shellItem->QueryInterface(IID_PPV_ARGS(&factory)))) {
        return QString();
    }

    // SIIGBF_ICONONLY asks for the icon rather than a thumbnail, and
    // SIIGBF_BIGGERSIZEOK lets the shell return the large image it has instead
    // of scaling a small one up.
    const SIZE size = { 256, 256 };
    HBITMAP bitmap = nullptr;
    const HRESULT hr = factory->GetImage(size, SIIGBF_ICONONLY | SIIGBF_BIGGERSIZEOK, &bitmap);
    factory->Release();

    if (FAILED(hr) || bitmap == nullptr) {
        qWarning() << "UWPApp, fail to render the shell icon of" << aumid
                   << ", HRESULT is" << hr;
        return QString();
    }

    QImage image = imageFromBitmap(bitmap);
    DeleteObject(bitmap);

    if (image.isNull()) {
        return QString();
    }

    if (!hasVisiblePixel(image)) {
        // A shell bitmap without usable alpha data comes back fully transparent
        // and would be saved as an invisible icon; read the same buffer as
        // opaque BGRX instead.
        image = QImage(image.constBits(), image.width(), image.height(),
                       image.bytesPerLine(), QImage::Format_RGB32).copy();
    }

    // Build a file system safe name from the AUMID for the temporary copy.
    QString safe;
    safe.reserve(aumid.size());
    for (QChar ch : aumid) {
        safe += ch.isLetterOrNumber() ? ch : QLatin1Char('_');
    }
    if (safe.isEmpty()) {
        safe = QStringLiteral("unknown");
    }

    // The icons live in "<user temp>/Launchy/icon". Created on demand; the
    // operating system may clean the temp directory at any time, which is
    // fine because the catalog pass recreates the file on the next run.
    const QDir iconDir(QDir::tempPath() + QStringLiteral("/Launchy/icon"));
    if (!iconDir.exists() && !QDir().mkpath(iconDir.absolutePath())) {
        qWarning() << "UWPApp, fail to create the temp icon directory"
                   << iconDir.absolutePath();
        return QString();
    }

    const QString target = iconDir.filePath(
        QStringLiteral("uwp_") + safe + QStringLiteral(".png"));
    if (!image.save(target, "PNG")) {
        qWarning() << "UWPApp, fail to save the shell icon to" << target;
        return QString();
    }

    return target;
}

}   // namespace

UWPApp::UWPApp() {

}

UWPApp::~UWPApp() {

}

int UWPApp::msg(int msgId, void* wParam, void* lParam) {
    int handled = 0;
    switch (msgId) {
    case MSG_INIT:
        init();
        handled = 1;
        break;

    case MSG_GET_LABELS:
        getLabels((QList<launchy::InputData>*) wParam);
        handled = 1;
        break;

    case MSG_GET_NAME:
        getName((QString*)wParam);
        handled = 1;
        break;

    case MSG_GET_RESULTS:
        getResults((QList<launchy::InputData>*) wParam, (QList<launchy::CatItem>*) lParam);
        handled = 1;
        break;

    case MSG_GET_CATALOG:
        getCatalog((QList<launchy::CatItem>*) wParam);
        handled = 1;
        break;

    case MSG_LAUNCH_ITEM:
        launchItem((QList<launchy::InputData>*) wParam, (launchy::CatItem*)lParam);
        handled = 1;
        break;

    case MSG_DO_DIALOG:
        // No options page yet: doDialog() below is an empty stub, so it leaves
        // the caller's QWidget* at nullptr and Launchy shows an empty box.
        // doDialog((QWidget*)wParam, (QWidget**)lParam);
        break;

    case MSG_END_DIALOG:
        // endDialog((bool)wParam);
        break;

    default:
        break;
    }

    return handled;
}

void UWPApp::init() {

}

void UWPApp::getName(QString* str) {
    *str = PLUGIN_NAME;
}

void UWPApp::getCatalog(QList<launchy::CatItem>* items) {

    qDebug() << "UWPApp::getCatalog, function entry";

    CoInitialize(NULL);

    // The COM smart pointers below are plain raw pointers on purpose: ATL
    // (atlbase.h / atlcomcli.h) is an optional Visual Studio component that
    // build agents do not always carry, and this plugin only needs reference
    // counting plus CoTaskMem strings, both of which the COM runtime provides.
    // They are released from the cleanup after the do/while below, which every
    // exit path reaches.
    IShellItem* appFolder = nullptr;
    IEnumShellItems* enumShellItems = nullptr;

    const PROPERTYKEY pkLauncherAppState = lookupPropertyKey(L"System.Launcher.AppState");
    const PROPERTYKEY pkSmallLogoPath = lookupPropertyKey(L"System.Tile.SmallLogoPath");
    const PROPERTYKEY pkAppUserModelID = lookupPropertyKey(L"System.AppUserModel.ID");
    const PROPERTYKEY pkInstallPath = lookupPropertyKey(L"System.AppUserModel.PackageInstallPath");
    const PROPERTYKEY pkPackageFullName = lookupPropertyKey(L"System.AppUserModel.PackageFullName");

    do {
        if (FAILED(SHCreateItemFromParsingName(L"shell:AppsFolder",
                                               nullptr,
                                               IID_PPV_ARGS(&appFolder)))) {
            qWarning() << "UWPApp::getCatalog, fail to open shell:AppsFolder";
            break;
        }

        qDebug() << "UWPApp::getCatalog, succeed to open shell::AppsFolder";

        if (FAILED(appFolder->BindToHandler(nullptr,
                                            BHID_EnumItems,
                                            IID_PPV_ARGS(&enumShellItems)))) {
            qWarning() << "UWPApp::getCatalog, fail to bind to handler";
            break;
        }

        qDebug() << "UWPApp::getCatalog, succeed to bind to handler";

        qDebug() << "UWPApp::getCatalog, begin while loop";
        IShellItem* shellItem = nullptr;
        while (enumShellItems->Next(1, &shellItem, nullptr) == S_OK) {
            // The enumeration hands over one reference, and that is the only
            // pointer the loop body has to release.
            IPropertyStore* propertyStore = nullptr;
            QString aumid;
            QString shortName;
            QString iconPath;

            do {
                if (FAILED(shellItem->BindToHandler(NULL,
                                                    BHID_PropertyStore,
                                                    IID_PPV_ARGS(&propertyStore)))) {
                    break;
                }

                // A UWP entry always carries a launch state. The check filters
                // out the plain desktop shortcuts that also live in AppsFolder
                // and that ActivateApplication could not start anyway.
                QString appState;
                if (!readPropertyString(propertyStore, pkLauncherAppState, &appState)) {
                    break;
                }

                wchar_t* name = nullptr;
                if (SUCCEEDED(shellItem->GetDisplayName(SIGDN_NORMALDISPLAY, &name))) {
                    shortName = QString::fromWCharArray(name);
                    CoTaskMemFree(name);
                    qDebug() << "name: " << shortName;
                }

                // Without an AppUserModelID the entry cannot be launched
                if (!readPropertyString(propertyStore, pkAppUserModelID, &aumid)) {
                    qWarning() << "UWPApp::getCatalog, entry without an id:" << shortName;
                    break;
                }

                // The package root. The install path property is the fast way,
                // the package full name keeps working for the entries that do
                // not publish it (packaged desktop apps).
                QString packageRoot;
                if (!readPropertyString(propertyStore, pkInstallPath, &packageRoot)) {
                    QString packageFullName;
                    if (readPropertyString(propertyStore, pkPackageFullName, &packageFullName)) {
                        packageRoot = packageRootFromFullName(packageFullName);
                    }
                }
                qDebug() << " id: " << aumid << " install: " << packageRoot;

                // The asset reference. The shell property is the fast way, the
                // manifest is the documented source of truth behind it and works
                // for the entries that do not publish the property.
                QString logoReference;
                if (!readPropertyString(propertyStore, pkSmallLogoPath, &logoReference)) {
                    logoReference = logoReferenceFromManifest(packageRoot, aumid);
                }
                qDebug() << " logo reference: " << logoReference;

                QString assetFile = resolveAssetFile(packageRoot, logoReference);
                if (assetFile.isEmpty()) {
                    // Nothing matched inside the package: fall back to the icon
                    // the shell renders for this entry, which always exists.
                    assetFile = extractShellIcon(shellItem, aumid);
                }
                qDebug() << " logo file: " << assetFile;

                iconPath = assetFile;
                qDebug() << " logo path: " << iconPath;

                items->push_back(launchy::CatItem(aumid,
                                                  shortName,
                                                  PLUGIN_NAME,
                                                  iconPath));
            } while (0);

            if (propertyStore != nullptr) {
                propertyStore->Release();
            }
            shellItem->Release();
        }

        qDebug() << "UWPApp::getCatalog, end while loop";
    } while (0);

    if (enumShellItems != nullptr) {
        enumShellItems->Release();
    }
    if (appFolder != nullptr) {
        appFolder->Release();
    }

    CoUninitialize();
}

void UWPApp::getLabels(QList<launchy::InputData>* inputData) {

}

void UWPApp::getResults(QList<launchy::InputData>* inputData, QList<launchy::CatItem>* results) {

}

void UWPApp::launchItem(QList<launchy::InputData>* inputData, launchy::CatItem* item) {
    // Specify the appropriate COM threading model

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    IApplicationActivationManager* pAAM = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ApplicationActivationManager,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pAAM));
    if (FAILED(hr)) {
        qWarning() << "UWPApp::launchItem, fail to create CoCreateInstance, HR is" << hr;
        return;
    }

    DWORD pid = 0;
    hr = pAAM->ActivateApplication(item->fullPath.toStdWString().c_str(), L"", AO_NONE, &pid);
    if (FAILED(hr)) {
        qWarning() << "UWPApp::launchItem, Error in ActivateApplication call & HR is " << hr;
        return;
    }

    if (hr == 0) {
        qDebug() << "UWPApp::launchItem, Activated " << item->fullPath << " with pid " << pid;
    }

    CoUninitialize();
}

void UWPApp::doDialog(QWidget* parent, QWidget** dialog) {

}

void UWPApp::endDialog(bool accept) {

}
