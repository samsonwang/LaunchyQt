// Copyright 2009 Fabian Hofsaess

#include "precompiled.h"
#include "ControlPanelItemFinder.h"
#include "fhoenv.h"
#include "fhoreg.h"
#include "fhores.h"

// amongst others, see "Control Panel Items" (ms-help://MS.VSCC.v80/MS.MSDN.v80/MS.WIN32COM.v10.en/shellcc/platform/shell/programmersguide/extensibilty/conpanel.htm)

// define common buffer size
const DWORD DEFAULT_BUFFER_SIZE = 512;

QHash<QString, CachedCplItem*> ControlPanelItemFinder::cplItemCache; 

ControlPanelItemFinder::ControlPanelItemFinder(uint pluginId, FhoIconCreator *pIconGen, QList<CatItem> *pResultList) {
	controlyPluginId = pluginId;
	pIconCreator = pIconGen;
	pItems = pResultList;

	addControlPanelItself = true;
	
	avoidDuplicates = true;
	// currently, avoidDuplicates operates on the last possiblity it has, just avoiding to add items when they are about to be added
	// an optimization would be to exclude these items as soon as possible, thus avoiding to extract unnecessary information (like icon, etc.) and executing unnecessary queries!?

	addControlPanelSubfolderDepth = 0;
	// currently, do not use; always set to 0!
	// if set to 1:
	// good: this will add items in subfolders, like "Services", etc. in "Administrative Tools"
	// bad: this will also add all Fonts (in "Fonts" folder) - takes quite long and adds hundreds of unnecessary items!
}

ControlPanelItemFinder::~ControlPanelItemFinder() {
}
#
void ControlPanelItemFinder::findItems() {
	// CoInitialize has to be called for current thread before calling SHGetFileInfo
	// getResult() and getCatalog() appear in different threads (main thread vs. catalog update thread!)
	HRESULT hres = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hres))
        qWarning("findItems(), CoInitializeEx failed");

	// order in which methods are called affects how details of some items are displayed, e.g. "GUID-path" vs. "normal path" or "GUID-path" vs. exe / shelldll / rundll / CSLID.controly / etc.;
	// adding items twice is avoided by cplItemNameCache, anyways!

	addSystem32CplControlPanelItems(); // use these first as they can handle the Mouse / Keyboard (main.cpl / main.cpl,@1) issue
	addRegistryCplControlPanelItems();

	addShellInfoControlPanelItems(); // do not use this first, it does currently not handle the Mouse / Keyboard (main.cpl / main.cpl,@1) issue correctly!

	addRegistryExpNsControlPanelItems();

	addControlPanel();

	// clear local cache
	foreach (CachedCplItem *pItem, cplItemNameCache) {
		delete pItem;
	}
	cplItemNameCache.clear();

}

/**
	finds Control Panel items (applets / applications) that are implemented as a library
	(in a DLL named *.CPL, supposed to expose the CPlApplet function)
	and reside in the Windows\System32 directory
	(those do not necessarily need to be registered in the registry)
*/
void ControlPanelItemFinder::addSystem32CplControlPanelItems() {

	// Get a list of all CPL files in the system directory
	TCHAR sysDirBuffer[DEFAULT_BUFFER_SIZE];
	if (GetSystemDirectory(sysDirBuffer, DEFAULT_BUFFER_SIZE)) {
		// GetSystemDirectory() function is provided primarily for compatibility, it is recommended to use SHGetFolderPath()!

		QString sysDirPath = QString::fromUtf16(sysDirBuffer);
		QDir sysDir(sysDirPath);

		QFileInfoList cplFiles = sysDir.entryInfoList(QStringList("*.cpl"), QDir::Files, QDir::Unsorted);

		foreach(QFileInfo cplFileInfo, cplFiles) {
			addCplControlPanelItem(&cplFileInfo);
		}
	}
}

/**
	finds Control Panel items (applets / applications) that are implemented as a library
	(in a DLL named *.CPL, supposed to expose the CPlApplet function)
	and that are registered in the registry
	(they may reside in any directory)
*/
void ControlPanelItemFinder::addRegistryCplControlPanelItems() {

	// addRegistryCplControlPanelItems() and addSystem32CplControlPanelItems() may both find the same items
	// The local cache (cplItemNameCache) will take care to skip double entries!

	// 1. items available for all users:
	// HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Control Panel\Cpls 
	// -> name with REG_EXPAND_SZ value representing the path

	QStringList *cplPaths = FhoReg::EnumValues(HKEY_LOCAL_MACHINE, QString("Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls"));

	for (int i = 0; i < cplPaths->size(); i++) {
		QString cplPath = (*cplPaths)[i];
		QFileInfo cplFileInfo(cplPath);
				
		if ((!cplFileInfo.isFile()) || (!cplFileInfo.exists())) {
			// some items are given without their full path
			// (e.g. 'firewall.cpl', 'netsetup.cpl' instead of e.g. 'C:\Programme\Gemeinsame Dateien\Microsoft Shared\Speech\sapi.cpl'
			// we skip them, assuming they reside in system32 and are already handled with the system32 Cpl items above
		} else {
			addCplControlPanelItem(&cplFileInfo);
		}
	}

	delete cplPaths;

	// on Vista, you can specify 
	// HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Control Panel\ExtendedProperties\System.Software.AppId 
	// -> %ProgramFiles%\MyCorp\MyApp\MyCpl.cpl={A newly generated GUID}
	// is this additionally (meaning we find this items with the 'xp method' here anyways) or an alternative (meaning we would have to check this additionally)?

	// 2. items available per user:
	// same with HKEY_CURRENT_USER instead of HKEY_LOCAL_MACHINE!?
	// is this necessary?
}

/**
	adds Control Panel items (applets / applications) that are implemented as a library
	(in a DLL named *.CPL, supposed to expose the CPlApplet function)
*/
void ControlPanelItemFinder::addCplControlPanelItem(QFileInfo *pCplFileInfo) {

	// on Vista, some CPLs are still DLLs, but do not export CPlApplet any more
	// (e.g. Software/Add Remove Programs/Programs and Functions (appwiz.cpl), Firewall, etc.)!
	// these will be handled by other methods and have to be skipped here!

	QString cplFileName = pCplFileInfo->fileName();
	QString cplFullPath = QDir::toNativeSeparators(pCplFileInfo->absoluteFilePath());

	if (cplItemCache.contains(cplFileName)) {
		// file already indexed before, info stored in cache

		// remember to handle all items stored in the library, not only the first one...
		int i = 0;
		bool itemFound;
		do {
			QString fileName;
			QString fullPath;

			if (i == 0) {
				// first item (dialog) implemented in library (application)
				fileName = cplFileName;
				fullPath = cplFullPath;
			} else {
				// other items (dialogs) implemented in library (application)
				fileName = cplFileName + ",@" + QString::number(i);
				fullPath = cplFullPath + ",@" + QString::number(i);
			}

			if (cplItemCache.contains(fileName)) {
				// add the item from the cache, continue searching for sub-items
				addControlPanelItem(fileName);
				itemFound = true;
			} else {
				// stop searching for sub-items
				itemFound = false;
			}
			i++;
		} while (itemFound);
		
		return;
	} // item already in cache

	bool itemAdded = false; // has at least one item been successfully added?
	
	// Library Handle to *.cpl file
	HINSTANCE hCplLib = LoadLibrary(cplFullPath.utf16());
	if (hCplLib) {

		// Pointer to CPlApplet() function
		APPLET_PROC CplCall = (APPLET_PROC) GetProcAddress(hCplLib, "CPlApplet");
		if (CplCall) {

			if (CplCall(NULL, CPL_INIT, 0, 0)) {
				int cplCount = CplCall(NULL, CPL_GETCOUNT, 0, 0);

				for (int i = 0; i < cplCount; i++) {
					int pluginId;

					QString fileName;
					QString fullPath;

					if (i == 0) {
						// first item (dialog) implemented in library (application)
						fileName = cplFileName;
						fullPath = cplFullPath;
						pluginId = 0; // execution can be directly handled by Launchy main
					} else {
						// other items (dialogs) implemented in library (application)
						fileName = cplFileName + ",@" + QString::number(i);
						fullPath = cplFullPath + ",@" + QString::number(i);
						pluginId = controlyPluginId; // execute has to be handled by plugin itself!
					}

					QString name;
					QString iconPath;

					// prefer INQIURE over NEWINQUIRE, do only use NEWINQUIRE if information (icon / name) is not returned (empty or CPL_DYNAMIC_RES)
					// this is the preferred way according to Microsoft documentation (for performance), although NEWINQUIRE was initially intended to replace INQUIRE, it has already been deprecated!
					// but be aware that some apps may return string ids that do not yield a result (although being returned)

					// For the result of CplApplet() function (CplCall()) with CPL_INQUIRE and CPL_NEWINQUIRE,
					// the MS documentation states that 0 *should* be returned on success, 
					// but other part of the documentation states that the return value is ignored.
					// Thus, we ignore the return code (there are items that return 0 on success, others return 1 on success, and so on)

					// Case #1, try CPL_INQUIRE first
					CPLINFO cplInfo;
					CplCall(NULL, CPL_INQUIRE, i, (LPARAM)&cplInfo);

					if (cplInfo.idName != CPL_DYNAMIC_RES) {
						name = FhoRes::getResourceString(hCplLib, cplInfo.idName);
					}
					if (cplInfo.idIcon != CPL_DYNAMIC_RES) {
						iconPath = pIconCreator->getImageFileName(cplFullPath, -abs(cplInfo.idIcon)/*, file*/);
					}

					if ((name.isEmpty()) || (iconPath.isEmpty())) {

						union { 
							NEWCPLINFOA NewCplInfoA;
							NEWCPLINFOW NewCplInfoW; 
						} cplNewInfo;

						cplNewInfo.NewCplInfoA.dwSize = 0;
						cplNewInfo.NewCplInfoA.dwFlags = 0;

						CplCall(NULL, CPL_NEWINQUIRE, i, (LPARAM)&cplNewInfo);
						if (cplNewInfo.NewCplInfoA.dwSize == sizeof(NEWCPLINFOW)) {
							// Case #2a, CPL_NEWINQUIRE has returned an Unicode String

							if (iconPath.isEmpty()) {
								HICON hIcon = cplNewInfo.NewCplInfoW.hIcon;
								if (hIcon != NULL) {
									iconPath = pIconCreator->getImageFileName(hIcon, fileName);
								}
							}
							if (name.isEmpty()) {
								name = QString::fromUtf16(cplNewInfo.NewCplInfoW.szName);
							}

						} else if (cplNewInfo.NewCplInfoA.dwSize == sizeof(NEWCPLINFOA)) {
							// Case #2b, CPL_NEWINQUIRE has returned an ANSI String

							if (iconPath.isEmpty()) {
								HICON hIcon = cplNewInfo.NewCplInfoA.hIcon;
								if (hIcon != NULL) {
									iconPath = pIconCreator->getImageFileName(hIcon, fileName);
								}
							}
							if (name.isEmpty()) {
								name = QString(cplNewInfo.NewCplInfoA.szName);
							}

						} // else Case #2c, CPL_NEWINQUIRE did not succeed!

					} // name or iconPath still empty after CPL_INQUIRE

					if ((name.isEmpty()) || (iconPath.isEmpty())) {
						// fallback if name or iconPath are still empty after CPL_INQUIRE and CPL_NEWINQUIRE
						if (iconPath.isEmpty()) {
							// use Controly default icon
							iconPath = pIconCreator->getImageFileName();
						}
						if (name.isEmpty()) {
							name = fileName;
						}
					}

					addControlPanelItem(fullPath, name, pluginId, iconPath, fileName);
					itemAdded = true;

				} // for cplCount

				CplCall(NULL, CPL_EXIT, 0, 0);

			} // if CPL_INIT
		} // if CplCall

		FreeLibrary(hCplLib);
	} // if hCplLib

	if (!itemAdded) {
		// mark the item as invalid in the cache (getting the info failed, do not retry to get the information - it will probably fail in the future, also)
		if (!cplItemCache.contains(cplFileName)) {
			cplItemCache[cplFileName] = NULL;
		}
	}
}

/**
	finds Control Panel items (applets / applications) (implemented as a executables, for example)
	that are registered under the 'Explorer Namespace' in the registry
	(e.g. via their GUID / CLSID)
*/
void ControlPanelItemFinder::addRegistryExpNsControlPanelItems() {
	/*
	  HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ControlPanel\NameSpace\{0052D9FC-6764-4D29-A66F-2F3BD9E2BB40}
	   -> (Default) = 'My Settings' (name for control panel item, optional)
	  HKEY_CLASSES_ROOT\CLSID\{0052D9FC-6764-4D29-A66F-2F3BD9E2BB40}
	  *-> (Default) = [REG_SZ] 'My Settings' (display name of control panel item)
	  *-> LocalizedString= [REG_EXPAND_SZ] @%ProgramFiles%\MyApp.exe,-9 (module + string table id of localized name, optional)
	   -> InfoTip= [REG_EXPAND_SZ] @%ProgramFiles%\MyApp.exe,-5 (description / tooltip)
	   -> System.ApplicationName= [REG_SZ] MyCompany.MySettings (canonical name to open app)
	   -> System.ControlPanel.Category= [REG_SZ] 1,8
	   -> System.Software.TasksFileUrl= [REG_SZ or REG_EXPAND_SZ] "%ProgramFiles%\MyApp\MyTaskLinks.xml
	  HKEY_CLASSES_ROOT\CLSID\{0052D9FC-6764-4D29-A66F-2F3BD9E2BB40}\DefaultIcon
	   -> (Default) = [REG_EXPAND_SZ] %ProgramFiles%\MyApp.exe,-2 (path to file that contains icon, including resource id of icon image in that file)
	  HKEY_CLASSES_ROOT\CLSID\{0052D9FC-6764-4D29-A66F-2F3BD9E2BB40}\Shell\Open\Command
	  *-> (Default) = [REG_EXPAND_SZ] %ProgramFiles%\MyApp.exe /Settings (command to open / launch item)
	*/

	// do we have to check the 'don't load' registry key in getRegistryDLLControlPanelItems / getSystem32ControlPanelItems!?

	// some items do not store a guid/clsid but some human readable identifier - but these won't find something under HKCR\CLDSI later...

	QStringList *clsidSubKeyNames = FhoReg::EnumSubKeys(HKEY_LOCAL_MACHINE, QString("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace"));
	
	for (int i = 0; i < clsidSubKeyNames->size(); i++) {
		QString guid = (*clsidSubKeyNames)[i];

		if (cplItemCache.contains(guid)) {
			// file already indexed before, info stored in cache

			addControlPanelItem(guid);
			continue;
		}

		// there are also some name value pairs under this key
		// for some items, the standard value is the name,
		// for others there is a 'Name' name where the value is the name of the item,
		// some items have additional information
		// we currently ignore all of these values here

		bool itemAdded = false; // has an item been successfully added?

		HKEY hkClsidFolder = FhoReg::OpenKey(HKEY_CLASSES_ROOT, QString("CLSID\\" + guid), KEY_QUERY_VALUE);

		if (hkClsidFolder != 0) {
			QString displayName = FhoReg::GetKeyDefaultValue(hkClsidFolder);
			QString localizedNameResource = FhoReg::GetKeyValue(hkClsidFolder, QString("LocalizedString")); // e.g. '@%SystemRoot%\system32\SHELL32.dll,-32517'
			QString localizedName = FhoRes::getResourceString(localizedNameResource);

			HKEY hkIconFolder = FhoReg::OpenKey(hkClsidFolder, QString("DefaultIcon"), KEY_QUERY_VALUE);
			QString iconResourceName;
			if (hkIconFolder != 0) {
				iconResourceName = FhoReg::GetKeyDefaultValue(hkIconFolder);
			}

			HKEY hkCmdFolder = FhoReg::OpenKey(hkClsidFolder, QString("Shell\\Open\\Command"), KEY_QUERY_VALUE);

			if (hkCmdFolder == 0) {
				// Under Windows Vista, some dll cpls (not only exe cpls) are defined this way
				// however, they do not store an executable under Shell\Open\Command, but a library under InProcServer32
				// for example, this applies to appwiz.cpl (which resides in the system32 directory),
				// but it does not expose the CPlApplet function and therefore fails to load 'the normal way'
				// (it would normally have been handled by addSystem32CplControlPanelItems() and/or addRegistryCplControlPanelItems)!
				// Even WinXP finds some items this way, however these are plain .dlls and not .cpls.
				// (e.g. Fonts, Administrative Tools, -> handled below
				// Scheduled Tasks, Scanner and Kameras, [Printer and Fax], [Mail] -> handled using other methods)

				hkCmdFolder = FhoReg::OpenKey(hkClsidFolder, QString("InProcServer32"), KEY_QUERY_VALUE);
			}
			
			if (hkCmdFolder != 0) {
				QString itemLaunchCommand = FhoReg::GetKeyDefaultValue(hkCmdFolder);

				QString path = FhoEnv::expand(itemLaunchCommand); // expand, some items are stored with %SystemRoot% placeholders, for example (e.g. on Vista)
				QString desc = localizedName; // prefer to use the localized display to the 'normal' display name (doesn't really seem to matter on XP for most items, but makes a big difference on Vista)
				if (desc.isEmpty()) {
					desc = displayName;
				}

				int pluginId;
				bool addItem = true;

				QStringList spl = path.split(".exe ");
				if (spl.size() == 1) {
					// just the executable itself (or a cpl, or a simple dll that we currently don't handle)
					pluginId = 0; // can probably be executed directly

					if (!path.contains(".exe") && !path.contains(".cpl")) {
						// .exe and .cpl are added, others (like .dll) are not!

						// if only a dll (no exe or cpl) is given:
						// as a last resort, try to launch it via the application name (e.g. 'control.exe /name Microsoft.WindowsUpdate')
						QString applicationName = FhoReg::GetKeyValue(hkClsidFolder, QString("System.ApplicationName"));
						if (!applicationName.isEmpty()) {
							pluginId = controlyPluginId; // execute has to be handled by plugin itself!
							path = "control.exe /name " + applicationName;
						} else {
							// check if we have a shell extension object / shell namespace extension (special shell folder extension, "virtual folder") that is listed in the control panel (fonts or admin tools)
							
							QString sf = FhoReg::GetKeyValue(hkClsidFolder, QString("Instance\\InitPropertyBag"), QString("TargetSpecialFolder"));
							if (!sf.isEmpty()) {
								// shell instance object
								pluginId = controlyPluginId;
								// Constant special item ID list (CSIDL) / csidl / specialfolder / shellfolder / specialshellfolder / shellinstance
								path = "csidl:" + sf + ".controly";
							} else {

								// we are still missing some items referenced in the control panel
								// e.g. on XP:
								// ("Scheduled Tasks", "{D6277990-4C6A-11CF-8D87-00AA0060F5BF}", "C:\WINDOWS\System32\mstask.dll") -> found via getShellInfoControlPanelItems
								// ("Scanner and Cameras", "{E211B736-43FD-11D1-9EFB-0000F8757FCD}", "wiashext.dll") -> found via getShellInfoControlPanelItems
								// ("Network Connections", "{7007ACC7-3202-11D1-AAD2-00805FC1270E}", "C:\WINDOWS\system32\NETSHELL.dll") -> found via ncpa.cpl
								// ("Printers and Fax") -> found via getShellInfoControlPanelItems
								// ("Mail") -> found via getShellInfoControlPanelItems
								// e.g. on Vista:
								// ("Indesig options")  -> found via getShellInfoControlPanelItems
								// ("Text-to-Speech") -> found via getShellInfoControlPanelItems
								// ("Mail") -> found via getShellInfoControlPanelItems
								// -> These items are found using other methods

								addItem = false;
							}
						}
					}
				} else if (spl.size() == 2) {
					// the executable with command line parameters
					// be aware that we currently assume that the path to the executable does not contain blanks!
					pluginId = controlyPluginId; // execute has to be handled by plugin itself!
				} else {
					// not expected - currently not handled
					addItem = false; // ?
				}
				
				if (addItem) {
					QString iconPath = pIconCreator->getImageFileName(iconResourceName/*, file*/);
					addControlPanelItem(path, desc, pluginId, iconPath, guid);
					itemAdded = true;
				}

				FhoReg::CloseKey(hkCmdFolder);
			} // hkCmdFolder

			FhoReg::CloseKey(hkClsidFolder);

		} // hkClsidFolder

		if (!itemAdded) {
			// mark the item as invalid in the cache (getting the info failed, do not retry to get the information - it will probably fail in the future, also)
			if (!cplItemCache.contains(guid)) {
				cplItemCache[guid] = NULL;
			}
		}

	} // for clsidSubKeyNames

	delete clsidSubKeyNames;
}

/**
	finds Control Panel items (applets / applications) 
	by browsing the ControlPanel virtual folder
	(there seem to be some exceptions / small gaps, but this approach alone should *theoretically* find all items!)
*/
void ControlPanelItemFinder::addShellInfoControlPanelItems() {

	LPITEMIDLIST pidlCplAbs;

	HRESULT hres = SHGetFolderLocation(NULL, CSIDL_CONTROLS, NULL, 0, &pidlCplAbs);
	if (hres == S_OK) {
		IShellFolder *psfDeskTop = NULL;
		hres = SHGetDesktopFolder(&psfDeskTop);
		if (hres == NOERROR) {
			if (addControlPanelItself) {
				// add control panel itself and the subitems contained within

				// try to get relative pidl from absolute pidl
				IShellFolder *psfParent = NULL;
				LPCITEMIDLIST pidlCplRel;
				hres = SHBindToParent(pidlCplAbs, IID_IShellFolder, (LPVOID *) &psfParent, &pidlCplRel);
				if (hres == S_OK) {
					// if possible, use relative pidl (as done in all following requests)
					addShellInfoItem((LPITEMIDLIST)pidlCplRel, psfParent, true, addControlPanelSubfolderDepth+1, psfDeskTop);
					psfParent->Release();
				} else {
					// fallback (we probably never run into this): use absolute pidl (all following requests will use relative pidl)
					addShellInfoItem(pidlCplAbs, psfDeskTop, false, addControlPanelSubfolderDepth+1, psfDeskTop);
				}
			} else {
				// add control panel contents, but not the item itself
				IShellFolder *psfCpl = NULL;
				hres = psfDeskTop->BindToObject(pidlCplAbs, NULL, IID_IShellFolder, (LPVOID *) &psfCpl);
				if (hres == S_OK) {
					addShellInfoItems(psfCpl, addControlPanelSubfolderDepth, psfDeskTop);
					// do not do this! psfCpl->Release();
				}
			}

			psfDeskTop->Release();
		}

		CoTaskMemFree(pidlCplAbs);
	}
}

/**
	Adds a single item (e.g. "file" or "folder", which may be virtual files or folders)
	If specified, also adds all child items in case of a "folder" (if addSubItemDepths > 0)
*/
void ControlPanelItemFinder::addShellInfoItem(LPITEMIDLIST pidlItems, IShellFolder *psfParentItem, bool isRelativePidl, int addSubItemDepths, IShellFolder *psfDeskTop) {
	
	STRRET strDispName;

	HRESULT hres = psfParentItem->GetDisplayNameOf(pidlItems, SHGDN_NORMAL, &strDispName);
	if (hres == S_OK) {

		TCHAR pszDisplayName[MAX_PATH];
		hres = StrRetToBuf(&strDispName, pidlItems, pszDisplayName, MAX_PATH);
		if (hres == S_OK) {
			QString cplDesc; // name
			cplDesc = QString::fromUtf16(pszDisplayName);

			bool itemAdded = false;			

			hres = psfParentItem->GetDisplayNameOf(pidlItems, SHGDN_FORPARSING | SHGDN_NORMAL, &strDispName); // absolute parse name (relative to desktop)
			if (hres == S_OK) {
					
				hres = StrRetToBuf(&strDispName, pidlItems, pszDisplayName, MAX_PATH);
				if (hres == S_OK) {

					QString cplPath; // launch command
					cplPath = QString::fromUtf16(pszDisplayName);

					QString cacheId = cplDesc + "-" + cplPath; // use all info, because e.g. Mouse and Keyboard both have the path 'c:\windows\system32\main.cpl'!

					if (cplItemCache.contains(cacheId)) {
						// already indexed before, info stored in cache

						addControlPanelItem(cacheId);
						itemAdded = true;
					} else {
						LPITEMIDLIST pidlItem = NULL; // absolute pidl
						//ULONG attr = 0;
						//ULONG attr = SFGAO_COMPRESSED;
						hres = psfDeskTop->ParseDisplayName(NULL, NULL, pszDisplayName, NULL, &pidlItem, /*&attr*/ NULL);
						if (hres == S_OK) {
							QString iconPath;

							if (!isRelativePidl) {
								// either use an absolute pidl...
								// (will return 'normal' folder icons for fonts (schriftarten) and for administrative tools (verwaltung)
								iconPath = pIconCreator->getImageFileName(pidlItem);

								// with absolute pidl, could also use SHGetFileInfo() to extract icon!?
							} else {
								// ...or a relative (single-level!?) pidl with parent
								// (will return correct icons also for fonts and for administrative tools, but cannot be used for control panel here if this is passed in as an absolute pidl!)
								iconPath = pIconCreator->getImageFileName(psfParentItem, pidlItems);
							}

							// local cache is checked in this method, but this does not avoid the unnecessary call to the iconCreator!?
							// can we use the global cache here?
							addControlPanelItem(cplPath, cplDesc, 0 /*HASH_controly*/, iconPath, cacheId);
							itemAdded = true;

							CoTaskMemFree(pidlItem);
						} else {
							// currently, the "relative" approach is the fallback; should we prefer it?
							hres = psfParentItem->GetDisplayNameOf(pidlItems, SHGDN_FORPARSING | SHGDN_INFOLDER, &strDispName); // relative parse name (relative to parent folder)
							if (hres == S_OK) {
								hres = StrRetToBuf(&strDispName, pidlItems, pszDisplayName, MAX_PATH);

								// some items just return their name for the full path, e.g. "Mail"
								// these are handled by some of the fallbacks below, but what also could be done is 
								// to build their launch path using the base Control Panel (or all Control Panel Elements) "path" plus their name, e.g.:
								// "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\Mail" on WinXP or
								// "::{26EE0668-A00A-44D7-9371-BEB064C98683}\0\Maus" on Win7

								// we can just use the parse name as the launch command, see above
								if (hres == S_OK) {
									cplPath = QString::fromUtf16(pszDisplayName); // launch command (e.g. full path!?)

									cacheId = cplDesc + "-" + cplPath;

									if (cplItemCache.contains(cacheId)) {
										// already indexed before, info stored in cache

										addControlPanelItem(cacheId);
										itemAdded = true;
									} else {
										hres = psfDeskTop->ParseDisplayName(NULL, NULL, pszDisplayName, NULL, &pidlItem, /*&attr*/ NULL);
										// although this is a relative parse name, it only succeeds beeing executed from desktop (?); the format always seems to be an absolute path to a .cpl file!
										if (hres == S_OK) {
											QString iconPath;
											if (!isRelativePidl) {
												// either use an absolute pidl...
												// (will return 'normal' folder icons for fonts (schriftarten) and for administrative tools (verwaltung)
												iconPath = pIconCreator->getImageFileName(pidlItem);
											} else {
												// ...or a relative pidl with parent
												// (will return correct icons also for fonts and for administrative tools, but cannot be used for control panel here if this is passed in as an absolute pidl!)
												iconPath = pIconCreator->getImageFileName(psfParentItem, pidlItems);
											}

											addControlPanelItem(cplPath, cplDesc, 0 /*HASH_controly*/, iconPath, cacheId);
											itemAdded = true;

											CoTaskMemFree(pidlItem);
										} else {
											// ?
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (addSubItemDepths > 0) {
		ULONG uAttr = SFGAO_FOLDER;
		psfParentItem->GetAttributesOf(1, (LPCITEMIDLIST *) &pidlItems, &uAttr);
		if (uAttr & SFGAO_FOLDER) {
			IShellFolder *psfSubFolder = NULL;
			hres = psfParentItem->BindToObject(pidlItems, NULL, IID_IShellFolder, (LPVOID *) &psfSubFolder);

			if (hres == S_OK) {
				addShellInfoItems(psfSubFolder, addSubItemDepths-1, psfDeskTop);
			}
		}
	}

}

/**
	Adds all child items (e.g. "files" or "folders", which may be virtual files or folders) of a parent "folder"
*/
void ControlPanelItemFinder::addShellInfoItems(IShellFolder *psfParentItem, int addSubItemDepths, IShellFolder *psfDeskTop) {
	// use cache!?

    HRESULT hres;
	LPENUMIDLIST ppenum = NULL;
	ULONG celtFetched;
	LPITEMIDLIST pidlItems = NULL; // relative pidl
	
	hres = psfParentItem->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &ppenum);

	while( ((hres = ppenum->Next(1, &pidlItems, &celtFetched)) == S_OK) && (celtFetched == 1)) {
		addShellInfoItem(pidlItems, psfParentItem, true, addSubItemDepths, psfDeskTop);
		CoTaskMemFree(pidlItems); // pMalloc->Free(pidlItems);
	}
	
	ppenum->Release();
    psfParentItem->Release();

	// the normal execute command (or windows explorer) understand the paths we extract above (including those in the '::{GUID}\{GUID}' format)
}

/** adds control panel itself */
void ControlPanelItemFinder::addControlPanel() {

	// works as expected on XP with "classic control panel view", may have different behavior with XP/Vista/Win7 "category view"

	if (addControlPanelItself) {
		QString cplDesc; // name
		QString cplPath; // launch command

		// Constant special item ID list (CSIDL) / csidl / specialfolder / shellfolder / specialshellfolder / shellinstance
		cplPath = QString("csidl:") + QString("0x%1").arg(CSIDL_CONTROLS, 4, 16, QChar('0')) + ".controly";

		// we could also just use this:
		// cplPath = "control.exe";

		if (cplItemCache.contains(cplPath)) {
			// file already indexed before, info stored in cache
			
			addControlPanelItem(cplPath);
		} else {

			bool itemAdded = false; // has the item been successfully added?

			LPITEMIDLIST pidlCpl;
			HRESULT hres = SHGetFolderLocation(NULL, CSIDL_CONTROLS, NULL, 0, &pidlCpl);
			if (hres == S_OK) {
				SHFILEINFO cplFileInfo;
				memset(&cplFileInfo, 0, sizeof(SHFILEINFO));
				if (SHGetFileInfo((LPCTSTR)pidlCpl, 0, &cplFileInfo, sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_DISPLAYNAME)) {
					cplDesc = QString::fromUtf16(cplFileInfo.szDisplayName);

				} else {
					// similar to addShellInfoItem, fallback may not be necessary!?
					IShellFolder *psfDeskTop = NULL;
					STRRET strDispName;
					hres = SHGetDesktopFolder(&psfDeskTop);
					hres = psfDeskTop->GetDisplayNameOf(pidlCpl, SHGDN_NORMAL, &strDispName);
					psfDeskTop->Release();
					TCHAR pszDisplayName[MAX_PATH];
					hres = StrRetToBuf(&strDispName, pidlCpl, pszDisplayName, MAX_PATH);
					if (hres == S_OK) {
						cplDesc = QString::fromUtf16(pszDisplayName);
					}
				}

				QString iconPath = pIconCreator->getImageFileName(pidlCpl/*, cplPath*/);
				addControlPanelItem(cplPath, cplDesc, controlyPluginId, iconPath, cplPath);
				itemAdded = true;

				CoTaskMemFree(pidlCpl);
			}

			if (!itemAdded) {
				// mark the item as invalid in the cache (getting the info failed, do not retry to get the information - it will probably fail in the future, also)
				if (!cplItemCache.contains(cplPath)) {
					cplItemCache[cplPath] = NULL;
				}
			}
		}
	}
}

void ControlPanelItemFinder::addControlPanelItem(QString &cacheId) {
	// add item from cache!

	if (cplItemCache.contains(cacheId)) {
		CachedCplItem *pCachedItem = cplItemCache[cacheId];
		if (pCachedItem != NULL) {
			addControlPanelItem(pCachedItem->path, pCachedItem->name, pCachedItem->pluginId, pCachedItem->icon, cacheId);
		}
	}
}

void ControlPanelItemFinder::addControlPanelItem(QString &path, QString &name, int pluginId, QString &iconPath, QString &cacheId) {
	// path: the 'full path' or the 'launch command' (used to launch the item)
	// name: the 'short name' or 'description' (displayed to identify the icon)
	// cacheId: the file name or any other id identifying the item in the cache (cache not used if string is empty)

	if (!cacheId.isEmpty()) {
		if (!cplItemCache.contains(cacheId)) {
			cplItemCache[cacheId] = new CachedCplItem(path, name, iconPath, pluginId);
		}
	}

	bool duplicate = cplItemNameCache.contains(name);
	if (!duplicate) { // avoid adding same element multiple times for different control panel browse methods
		// no duplicate: first occurence of item - add in any case
		
		pItems->push_back(CatItem(path, name, pluginId, iconPath));

		cplItemNameCache[name] = new CachedCplItem(path, name, iconPath, pluginId);
		// currently, there is no plugin deinit or unload (?), thus objects created with new will not be deleted!
	} else {
		// duplicate: repeated occurence of item - skip (if avoidDuplicates is enabled) or add again (if avoidDuplicates is disabled)
		if (!avoidDuplicates) {
			pItems->push_back(CatItem(path, name, pluginId, iconPath));
			// see above and decide which actions need to be taken (local cache, global cache, etc.)
			// how to deal with cache in this case?
		}
	}

}
