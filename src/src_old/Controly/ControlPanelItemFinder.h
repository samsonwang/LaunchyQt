// Copyright 2009 Fabian Hofsaess

#pragma once

#include "catalog.h"
#include "fhoicon.h"


struct CachedCplItem {
	QString path; // path / "launch command"
	QString name; // (short) name / description
	QString icon; // path to icon
	int pluginId; // plugin id

	CachedCplItem(QString p, QString n, QString i, int id) {
		path = p;
		name = n;
		icon = i;
		pluginId = id;
	}
};

class ControlPanelItemFinder {

	private:
		QList<CatItem> *pItems; // list to add the items to
		uint controlyPluginId;

		// "local cache" (for single scan, temporary): avoid adding same control panel item / element (that are found by different search / browse methods) multiple times
		// maps display name to boolean indicating if such an item has already been found
		QHash<QString, CachedCplItem*> cplItemNameCache; 
		bool avoidDuplicates; // avoid adding same item multiple times if found by different search strategies?

		// "global cache" (shared between all scans, performance optimization): avoid repeating expensive analysis (loading, icon extraction, etc.) of cpl dlls - do it only for the very first scan after program startup and stick to the information for repeated scans; 
		// maps file name to cached item information belonging to this file name!
		static QHash<QString, CachedCplItem*> cplItemCache; 

		FhoIconCreator *pIconCreator;

		bool addControlPanelItself;
		int addControlPanelSubfolderDepth;

	public:
		ControlPanelItemFinder(uint pluginId, FhoIconCreator *pIconGen, QList<CatItem> *pResultList);
		~ControlPanelItemFinder();

		void findItems();

	private:
		void addControlPanelItem(QString &cacheId);
		void addControlPanelItem(QString &path, QString &name, int pluginId, QString &iconPath, QString &cacheId);

		void addCplControlPanelItem(QFileInfo *pCplFileInfo);
		void addSystem32CplControlPanelItems();
		void addRegistryCplControlPanelItems();

		void addRegistryExpNsControlPanelItems();

		void addShellInfoControlPanelItems();
		void addShellInfoItem(LPITEMIDLIST pidlItems, IShellFolder *psfParentItem, bool isRelativePidl, int addSubItemDepths, IShellFolder *psfDeskTop);
		void addShellInfoItems(IShellFolder *psfParentItem, int addSubItemDepths, IShellFolder *psfDeskTop);

		void addControlPanel();
};
