// Copyright 2009 Fabian Hofsaess

#ifndef FHOICON_H
#define FHOICON_H


// implement icon/image cache?
// extracts / generates icons (images, as pixmap objects) from file resources, image files, files and handles
class FhoIcon {
	private:
		static QImage getDefaultIconFromResource(QString fileName);
		static QImage getDefaultIconFromLibrary(HINSTANCE hLib);
		static QImage getDefaultIcon();

		static QImage getIconForFile(LPCTSTR file, UINT basicFlags);
		static QImage getIconForFile(LPCTSTR file, UINT basicFlags, DWORD basicAttributes);

	public:
		static QImage getIconFromHandle(HICON hIcon);
		static QImage getIconFromResource(QString resourceDefinition);
		static QImage getIconFromResource(QString fileName, int iconIdx);
		static QImage getIconFromLibrary(/*HMODULE*/ HINSTANCE hLib, int iconIdx);
		static QImage getIconFromImage(QString fileName);
		static QImage getIconForFile(QString fileName);
		static QImage getIconForFile(LPITEMIDLIST pidl);
		static QImage getIconForFile(IShellFolder *psfParentItem, LPITEMIDLIST pidlChildItem);
		static QImage getIconForExtension(QString fileExtension);

		static QString getImageIdForExtension(QString fileExtension);
		static QString getImageIdForFile(QString fileName);
		static QString getImageIdForFile(LPITEMIDLIST pidl);
		static QString getImageIdForFile(IShellFolder *psfParentItem, LPITEMIDLIST pidlChildItem);
		static QString getImageIdForFile(LPCTSTR file, UINT basicFlags);
		static QString getImageIdForFile(LPCTSTR file, UINT basicFlags, DWORD basicAttributes);
};

// returns file names pointing to icons (image files), creating these image files from file resources or handles if necessary
class FhoIconCreator {
	private:
		QString imagePath;
		QString baseImageFileName;
		QString defaultImageFileName;
		QString imageType;
		QString imageFileExtension;
		bool forceDefaultImage;

		QString generateImageFileName(QString imageId);
		QString saveImage(QImage &image, QString fileName);

	public:
		FhoIconCreator();
		FhoIconCreator(QString path);

		void setImagePath(QString path);
		void setBaseImageFileName(QString name);
		void setDefaultImageFileName(QString fileName);
		void setImageType(QString fileExtension);
		void setImageType(QString type, QString fileExtension);
		void setForceDefaultImage(bool useDefaultImage);

		QString getImageFileName();
		QString getImageFileName(HICON hIcon, QString imageId);
		QString getImageFileName(QString resourceDescription); 
		QString getImageFileName(QString resourceFileName, int resourceIndex);
		QString getImageFileName(LPITEMIDLIST pidl);
		QString getImageFileName(IShellFolder *psfParentItem, LPITEMIDLIST pidlChildItem);
};


#endif
