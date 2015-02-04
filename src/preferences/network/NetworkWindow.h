/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 */
#ifndef NETWORK_WINDOW_H
#define NETWORK_WINDOW_H


#include <map>

#include <ObjectList.h>
#include <Window.h>

#include "NetworkSettingsAddOn.h"


using namespace BNetworkKit;


class BTabView;
class BButton;
class BMenu;


class NetworkWindow : public BWindow {
public:
								NetworkWindow();
	virtual						~NetworkWindow();

			bool				QuitRequested();
			void				MessageReceived(BMessage* message);

private:
	typedef	BWindow				inherited;

			void				_BuildProfilesMenu(BMenu* menu, int32 what);
			void				_ScanInterfaces();
			void				_ScanAddOns();
			BListItem*			_ItemFor(BNetworkSettingsType type);

			bool				_IsReplicantInstalled();
			void				_ShowReplicant(bool show);

private:
	typedef BObjectList<BNetworkSettingsAddOn> AddOnList;
	typedef BObjectList<BNetworkSettingsItem> ItemList;
	typedef std::map<BString, BListItem*> ItemMap;

			BButton*			fRevertButton;
			BButton*			fApplyButton;

			AddOnList			fAddOns;

			BOutlineListView*	fListView;
			ItemMap				fInterfaceItemMap;
			BListItem*			fServicesItem;
			BListItem*			fDialUpItem;
			BListItem*			fOtherItem;

			ItemList			fItems;

			BView*				fAddOnView;
};


#endif // NETWORK_WINDOW_H
