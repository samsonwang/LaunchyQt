// Copyright 2009 Fabian Hofsaess

#include "precompiled.h"
#include "fhoicon.h"
#include "fhoenv.h"


QImage FhoIcon::getIconFromHandle(HICON hIcon) {
	if (hIcon) {
		return QPixmap::fromWinHICON(hIcon).toImage();
	} else {
		return getDefaultIcon();
	}
}

/* creates an image from a loaded module's resources by id */
QImage FhoIcon::getIconFromLibrary(HINSTANCE hLib, int iconIdx) {
	// theory:
	// see getIconFromResource()/ExtractIcon() for a brief description
	// mydll,-3 extracts icon with resource id (name) "3"
	// mydll,3 extracts icon with index 3 (4th icon)
	// in practice:
	// it seems that passing numbers via MAKEINTRESOURCE() returns the icons with the name represented by the (positive) number
	// it seems that passing a string itself does never yield a result!?
	// lib,3 extracts icon with resource id (name) "3" (via MAKEINTRESOURCE)
	// lib,-3 extracts icon with resource id (name) "3" by passing absolute value only! (via MAKEINTRESOURCE)
	// => always loads resource with given name, can currently not load nth icon by index!
	iconIdx = abs(iconIdx); // assure that if name is intended (by passing negative value) the expected result is given; we cannot achieve that if index is intended we will get the correct result!

	if (!hLib) {
		return getDefaultIcon();
	}

	HICON hIcon;
	if (iconIdx > 0) {
		// represents zero-based index of icon, e.g. 3 for fourth icon
		
		hIcon = (HICON) LoadImage(hLib, MAKEINTRESOURCE(abs(iconIdx)), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);

		// passing 0 for sizes with LR_DEFAULTSIZE flag returns default system size (e.g. 32), like LoadIcon does
		// passing 0 for sizes without LR_DEFAULTSIZE flag would return actual resource size (e.g. 48 / 32 / etc.)
	} else if (iconIdx == 0)  {
		// this works quite well for positive and negative ids, but it fails to load a resource if id is 0!?
		// does 0 index (first icon) really not work?
		hIcon = (HICON) LoadImage(hLib, QString::number(abs(iconIdx)).utf16(), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
	} else {
		// represents name of icon, e.g. -3 for icon with id "3"
		// does -1 have a special meaning also, like it has for ExtractIcon()?
		hIcon = (HICON) LoadImage(hLib, QString::number(abs(iconIdx)).utf16(), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
	}
	
	if (hIcon) {
		return getIconFromHandle(hIcon);
	} else {
		// find first icon in library!? take care not to run in an endless loop!
	}

	return getDefaultIcon();
}

QImage FhoIcon::getIconFromResource(QString fileName, int iconIdx) {
	// see ExtractIcon for a brief description.
	// In short:
	// mydll,-3 extracts icon with resource id (name) "3"
	// mydll,3 extracts icon with index 3 (4th icon)

	// iconIndex >= 0:
	// -> zero based index of icon (load by ordinal)
	//    e.g. 0: first icon (index 0)
	//         3: fourth icon (index 3)
	// iconIndex < -1:
	// -> icon with resouce identifier of abs(iconIndex) (load by name)
	//    e.g. -3: icon with resource identifier ('name') "3"
	// iconIndex == -1:
	// -> returns total number of icons in file!
	// => to extract icon with resouce identifier "1" (iconIndex -1), use ExtractIconEx()!

	if (iconIdx == -1) {
		// we want icon with name "1", but parameter -1 will return the icon count!
		// really get icon with name "1" instead of the first icon, e.g. using ExtractIconEx()?!
		return getDefaultIconFromResource(fileName);
	} else {
		// remove leading @
		if (fileName.startsWith('@')) {
			fileName = fileName.right(fileName.length()-1); // fileName.remove("@");
		}

		fileName = FhoEnv::expand(fileName);

		HICON hIcon = ExtractIcon(0, fileName.utf16(), iconIdx);

		if ((hIcon) && ((int)hIcon != 1)) {
			QImage image = getIconFromHandle(hIcon);
				
			DestroyIcon(hIcon);

			return image;
		} else {
			if (iconIdx != 0) {
				// we did not try to get the resource's default icon yet
				return getDefaultIconFromResource(fileName);
			} else {
				// we already tried to get the resource's default icon
				return getDefaultIcon();
			}
		}
	}

	// Alternative method would be to use GetModuleHandle/LoadLibrary and LoadImage
	// we prefer ExtractIcon because this can potentially also handle resource files that cannot be loaded (e.g. because of missing dependencies, etc.)
	// also, it seems that LoadImage can only handle icon names (passed via makeintresource, however) and can not load icons by name!? (documentations states it can!)
	// Does it make sense to try this version first for performance reasons (at least for "name" case that works)?
}

QImage FhoIcon::getDefaultIconFromResource(QString fileName) {
	return getIconFromResource(fileName, 0); // index 0, first icon ("default icon")
}

QImage FhoIcon::getIconFromResource(QString resourceDefinition) {
	if (resourceDefinition.contains(',')) {
		QStringList split = resourceDefinition.split(',', QString::SkipEmptyParts);
		if (split.size() == 2) {
			// e.g. "my.dll,5" or "my.dll,-5"
			QString fileName = split.at(0);
			INT resourceId = split.at(1).toInt();
			
			return getIconFromResource(fileName, resourceId);
		} else {
			return getDefaultIcon();
		}
	} else {
		// e.g. "my.dll"
		QString fileName = resourceDefinition;

		return getDefaultIconFromResource(fileName);
	}
}

QImage FhoIcon::getIconFromImage(QString fileName) {
	fileName = FhoEnv::expand(fileName);

	QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
	QFileInfo fi(fileName);
	QString type = fi.suffix();
	
	if (type.compare("ico", Qt::CaseInsensitive) == 0) {
		// Qt used to have no image loader for windows icons
		// it seems that it has one now (QImageReader::supportedImageFormats() also lists .ico type),
		// but that one seems to load the first icon via QImage(iconFile) (e.g. a small b/w one for an icon containing multiple images),
		// whereas this workaround (via ExtractIcon(0, iconFile, 0)) tries to find an icon matching the current windows default icon size!

		// we could also use LoadImage() with NULL and LR_LOADFROMFILE to load icons, cursors or bitmaps; which method to prefer?

		QImage image = getDefaultIconFromResource(fileName);
		if (!image.isNull()) {
			return image;
		}
		// in case this method fails and returns an empty image, we give QImage(iconFile) a try before potentially returning a default icon finally
	}
	
	if (supportedFormats.contains(type.toAscii())) {
		return QImage(fileName);
	} else {
		return getDefaultIcon();
	}
}

QImage FhoIcon::getIconForExtension(QString fileExtension) {
	if (!fileExtension.startsWith('.')) {
		fileExtension = "." + fileExtension;
	}
	return getIconForFile(fileExtension.utf16(), SHGFI_USEFILEATTRIBUTES, FILE_ATTRIBUTE_NORMAL);
}

QImage FhoIcon::getIconForFile(QString fileName) {
	fileName = FhoEnv::expand(fileName);
	return getIconForFile(fileName.utf16(), 0);
}

QImage FhoIcon::getIconForFile(LPITEMIDLIST pidl) {
	return getIconForFile((LPCTSTR)pidl, SHGFI_PIDL);
}

QImage FhoIcon::getIconForFile(IShellFolder *psfParentItem, LPITEMIDLIST pidlChildItem) {
	HRESULT hres;
	IExtractIcon *pExtractIcon = NULL;

	hres = psfParentItem->GetUIObjectOf(NULL, 1, const_cast<LPCITEMIDLIST *>(&pidlChildItem), IID_IExtractIcon, NULL, reinterpret_cast<LPVOID *>(&pExtractIcon));
	if (SUCCEEDED(hres)) {
		TCHAR str[MAX_PATH] = {0};
		int index;
		UINT flags;

		// Get the file location where the icons are stored.
		hres = pExtractIcon->GetIconLocation(0, str, MAX_PATH, &index, &flags);
		//hres = pExtractIcon->GetIconLocation(GIL_FORSHELL, str, MAX_PATH, &index, &flags);
		if (SUCCEEDED(hres)) {
			QString iconResourceFile = QString::fromUtf16(str);
			
			// do not use 'general' getIconFromResource() -> ExtractIcon()!
			// use 'specific' pExtractIcon->Extract() instead!
			// this is because iconResourceFile returned may be "*" and not refer to a real file! pExtractIcon->Extract() will handle this, ExtractIcon() cannot handle this!
			// "*" is dangerous for caching the imageId! we could check if iconResourceFile refers to an existing file!?
			HICON hIconLarge = NULL;
			UINT nIconSize = MAKELONG(0, 0); // this requests the default size!?
			hres = pExtractIcon->Extract(str, index, &hIconLarge, NULL, nIconSize);
			if (hres == NOERROR) {
				if (hIconLarge) {
					QImage image = getIconFromHandle(hIconLarge);
				
					DestroyIcon(hIconLarge);

					return image;
				} else {
					return getIconFromResource(iconResourceFile, index);
				}
			} else {
				return getIconFromResource(iconResourceFile, index);
			}
		} else {
			return getIconForFile(pidlChildItem);
		}
		pExtractIcon->Release();
	} else {
		return getIconForFile(pidlChildItem);
	}
}

QImage FhoIcon::getIconForFile(LPCTSTR file, UINT basicFlags) {
	return getIconForFile(file, basicFlags, 0);
}

QImage FhoIcon::getIconForFile(LPCTSTR file, UINT basicFlags, DWORD basicAttributes) {
	SHFILEINFO finfo;
	HRESULT hres;

	memset(&finfo, 0, sizeof(SHFILEINFO));
	hres = SHGetFileInfo(file, basicAttributes, &finfo, sizeof(SHFILEINFO), basicFlags | SHGFI_ICONLOCATION);
	if (hres) {
		QString iconResourceFile = QString::fromUtf16(finfo.szDisplayName);
		if (!iconResourceFile.isEmpty()) {
			return getIconFromResource(iconResourceFile, finfo.iIcon);
		} else {
			hres = FALSE;
		}
	} 
	if (!hres) {
		memset(&finfo, 0, sizeof(SHFILEINFO));
		hres = SHGetFileInfo(file, basicAttributes, &finfo, sizeof(SHFILEINFO), basicFlags | SHGFI_SYSICONINDEX);
		if (finfo.iIcon != 0) {
			HICON hIcon = ImageList_GetIcon((HIMAGELIST) hres, finfo.iIcon, ILD_TRANSPARENT);
			if (hIcon != NULL) {
				QImage image = getIconFromHandle(hIcon);
				hres = TRUE;
				DestroyIcon(hIcon);
				return image;
			} else {
				hres = FALSE;
			}
		} else {
			hres = FALSE;
		}
		if (!hres) {
			memset(&finfo, 0, sizeof(SHFILEINFO));
			hres = SHGetFileInfo(file, basicAttributes, &finfo, sizeof(SHFILEINFO), basicFlags | SHGFI_ICON);
			if (hres) {
				QImage image = getIconFromHandle(finfo.hIcon);
				DestroyIcon(finfo.hIcon);
				return image;
			} else {
				return QImage();
			}
		}
	}

	return QImage();
}

QString FhoIcon::getImageIdForExtension(QString fileExtension) {
	if (!fileExtension.startsWith('.')) {
		fileExtension = "." + fileExtension;
	}
	return getImageIdForFile(fileExtension.utf16(), SHGFI_USEFILEATTRIBUTES, FILE_ATTRIBUTE_NORMAL);
}

QString FhoIcon::getImageIdForFile(QString fileName) {
	fileName = FhoEnv::expand(fileName);
	return getImageIdForFile(fileName.utf16(), 0);
}

QString FhoIcon::getImageIdForFile(LPITEMIDLIST pidl) {
	return getImageIdForFile((LPCTSTR)pidl, SHGFI_PIDL);
}

QString FhoIcon::getImageIdForFile(IShellFolder *psfParentItem, LPITEMIDLIST pidlChildItem) {
	HRESULT hres;
	IExtractIcon *pExtractIcon = NULL;

	hres = psfParentItem->GetUIObjectOf(NULL, 1, const_cast<LPCITEMIDLIST *>(&pidlChildItem), IID_IExtractIcon, NULL, reinterpret_cast<LPVOID *>(&pExtractIcon));
	if (SUCCEEDED(hres)) {
		TCHAR str[MAX_PATH] = {0};
		int index;
		UINT flags;

		// Get the file location where the icons are stored.
		hres = pExtractIcon->GetIconLocation(0, str, MAX_PATH, &index, &flags);
		if (SUCCEEDED(hres)) {
			QString iconResourceFile = QString::fromUtf16(str);
			// this may return "*" and does not always refer to a real existing file; see above!
			
			QString imageId = "@" + iconResourceFile + "," + QString::number(index); // did we already expand the resource file name?
			return imageId;
		} else {
			return getImageIdForFile(pidlChildItem);
		}
		pExtractIcon->Release();
	} else {
		return getImageIdForFile(pidlChildItem);
	}
}

QString FhoIcon::getImageIdForFile(LPCTSTR file, UINT basicFlags) {
	return getImageIdForFile(file, basicFlags, 0);
}

QString FhoIcon::getImageIdForFile(LPCTSTR file, UINT basicFlags, DWORD basicAttributes) {
	// has to be kept in sync with getIconForFile(LPITEMIDLIST pidl) -> getIconForFile(LPCTSTR file, UINT basicFlags, DWORD basicAttributes)!
	// harmonize to use the same code!

	QString imageId;

	SHFILEINFO finfo;
	HRESULT hres;

	memset(&finfo, 0, sizeof(SHFILEINFO));
	hres = SHGetFileInfo(file, basicAttributes, &finfo, sizeof(SHFILEINFO), basicFlags | SHGFI_ICONLOCATION);
	if (hres) {
		QString iconResourceFile = QString::fromUtf16(finfo.szDisplayName);
		if (!iconResourceFile.isEmpty()) {
			imageId = "@" + iconResourceFile + "," + QString::number(finfo.iIcon);
			return imageId;
		} else {
			hres = FALSE;
		}
	} 
	if (!hres) {
		memset(&finfo, 0, sizeof(SHFILEINFO));
		hres = SHGetFileInfo(file, basicAttributes, &finfo, sizeof(SHFILEINFO), basicFlags | SHGFI_SYSICONINDEX);
		if (finfo.iIcon != 0) {
			imageId = "[SYSIMAGELIST]:" + QString::number(finfo.iIcon);
			return imageId;
		} else {
			hres = FALSE;
		}
		if (!hres) {
			QString fallbackImageId;
			if (basicFlags & SHGFI_PIDL) {
				int minAsciiId = 33; // '!'
				int maxAsciiId = 126; // '~'
				int numAsciiIds = maxAsciiId - minAsciiId + 1;
				LPITEMIDLIST pidl = (LPITEMIDLIST)file;
				USHORT cb;
				fallbackImageId = "[ITEMIDLIST]:";
				do {
					cb = pidl->mkid.cb;
					if (cb != NULL) {
						BYTE *abID = pidl->mkid.abID;
						for (unsigned int i = 0; i < (cb-sizeof(cb)); i++) {
							int chid = abID[i];
							char ch = (char)(minAsciiId + (chid % numAsciiIds));
							fallbackImageId += ch;
						}
						fallbackImageId += ' ';
						pidl = (LPITEMIDLIST)((int)pidl + cb);
					}
				} while (cb != NULL);
			} else {
				fallbackImageId = QString::fromUtf16(file);
			}
			imageId = fallbackImageId;
		}
	}

	return imageId;
}

QImage FhoIcon::getDefaultIcon() {
	return QImage();
}

FhoIconCreator::FhoIconCreator() {
	forceDefaultImage = false;
	setImageType("png");
}

FhoIconCreator::FhoIconCreator(QString path) {
	setImagePath(path);
}

void FhoIconCreator::setForceDefaultImage(bool useDefaultImage) {
	forceDefaultImage = useDefaultImage;
}

void FhoIconCreator::setImagePath(QString path) {
	imagePath = path;

	if (!QFile::exists(imagePath)) {
		QDir().mkpath(imagePath);
	}
}

void FhoIconCreator::setBaseImageFileName(QString name) {
	baseImageFileName = name;
}

void FhoIconCreator::setDefaultImageFileName(QString fileName) {
	defaultImageFileName = fileName;
}

void FhoIconCreator::setImageType(QString fileExtension) {
	setImageType("", fileExtension);
}

void FhoIconCreator::setImageType(QString type, QString fileExtension) {
	QList<QByteArray> supportedFormats = QImageWriter::supportedImageFormats();

	if (type.isEmpty()) {
		type = fileExtension;
	}

	if (supportedFormats.contains(type.toAscii())) {
		imageType = type;
		imageFileExtension = fileExtension;
	}
}

QString FhoIconCreator::saveImage(QImage &image, QString fileName) {
	if (!image.isNull()) {
		image.save(fileName, imageType.isEmpty() ? 0 : imageType.toAscii());
		return fileName;
	} else {
		return defaultImageFileName;
	}
}

/* generates the file name of the image file */
QString FhoIconCreator::generateImageFileName(QString imageId) {
	if (forceDefaultImage) {
		return defaultImageFileName;
	}

	QString fileName = imagePath + "/" + baseImageFileName + QString::number(qHash(imageId), 16) + "." + imageFileExtension;
	return fileName;
}

/* returns the file name of an image file for a give image id */
QString FhoIconCreator::getImageFileName() {
	return defaultImageFileName;
}

/* creates an icon image from a HICON (if not already cached) and returns the file name */
QString FhoIconCreator::getImageFileName(HICON hIcon, QString imageId) {
	if ((forceDefaultImage) || (hIcon == NULL)) {
		return defaultImageFileName;
	}

	QString fileName = generateImageFileName(imageId);
	if (!QFile::exists(fileName)) {
		// create the icon file if it does not exist yet - otherwise, the existing file will be used!
		QImage image = FhoIcon::getIconFromHandle(hIcon);
		return saveImage(image, fileName);
	}
	return fileName;
}

/* creates an icon image (embedded in a file) from a resource description (like "my.dll,-1") (if not already cached) and returns the file name */
/* creates an icon image for a file or folder (the Icon Windows Explorer would display) */
QString FhoIconCreator::getImageFileName(QString resourceDescription) {
	if ((forceDefaultImage) || (resourceDescription.isEmpty())) {
		return defaultImageFileName;
	}

	QString fileName;

	if ((resourceDescription.startsWith('@')) || (resourceDescription.contains(','))) {
		// return from resource
		QString imageId = resourceDescription; // did we already expand the resource description?
		fileName = generateImageFileName(imageId);
		
		if (!QFile::exists(fileName)) {
			QImage image = FhoIcon::getIconFromResource(resourceDescription);
			return saveImage(image, fileName);
		}
	} else if (resourceDescription.startsWith('.')) {
		// return for extension
		QString imageId = FhoIcon::getImageIdForExtension(resourceDescription);
		fileName = generateImageFileName(imageId);

		if (!QFile::exists(fileName)) {
			QImage image = FhoIcon::getIconForExtension(resourceDescription);
			return saveImage(image, fileName);
		}
	} else {
		// return for file
		QString imageId = FhoIcon::getImageIdForFile(resourceDescription);
		fileName = generateImageFileName(imageId);

		if (!QFile::exists(fileName)) {
			QImage image = FhoIcon::getIconForFile(resourceDescription);
			return saveImage(image, fileName);
		}
	}

	return fileName;
}

/* creates an icon image (embedded in a file) from a file name including the resource (like "my.dll") and an icon index (like -3 or 3) (if not already cached) and returns the file name */
QString FhoIconCreator::getImageFileName(QString resourceFileName, int resourceIndex) {
	if ((forceDefaultImage) || (resourceFileName.isEmpty())) {
		return defaultImageFileName;
	}

	QString imageId = "@" + resourceFileName + "," + QString::number(resourceIndex); // did we already expand the resource file name?
	QString fileName = generateImageFileName(imageId);
	if (!QFile::exists(fileName)) {
		QImage image = FhoIcon::getIconFromResource(resourceFileName, resourceIndex);
		return saveImage(image, fileName);
	}
	return fileName;
}

QString FhoIconCreator::getImageFileName(LPITEMIDLIST pidl) {
	if (forceDefaultImage) {
		return defaultImageFileName;
	}

	QString imageId = FhoIcon::getImageIdForFile(pidl);
	QString fileName = generateImageFileName(imageId);
	if (!QFile::exists(fileName)) {
		QImage image = FhoIcon::getIconForFile(pidl);
		return saveImage(image, fileName);
	}
	return fileName;
}

QString FhoIconCreator::getImageFileName(IShellFolder *psfParentItem, LPITEMIDLIST pidlChildItem) {
	if (forceDefaultImage) {
		return defaultImageFileName;
	}

	QString imageId = FhoIcon::getImageIdForFile(psfParentItem, pidlChildItem);
	QString fileName = generateImageFileName(imageId);
	if (!QFile::exists(fileName)) {
		QImage image = FhoIcon::getIconForFile(psfParentItem, pidlChildItem);
		return saveImage(image, fileName);
	}
	return fileName;
}
