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

#include <NetworkSettings.h>
#include <NetworkSettingsAddOn.h>


using namespace BNetworkKit;

class BButton;
class BMenu;
class InterfaceListItem;
class InterfaceView;


enum {
	kMsgSettingsItemUpdated = 'SIup'
};


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
			BNetworkSettingsItem*
								_SettingsItemFor(BListItem* item);
			void				_SortItemsUnder(BListItem* item);
			BListItem*			_ListItemFor(BNetworkSettingsType type);
			BListItem*			_CreateItem(const char* label);
			void				_SelectItem(BListItem* listItem);
			void				_BroadcastSettingsUpdate(uint32 type);
			void				_BroadcastConfigurationUpdate(
									const BMessage& message);
			void				_UpdateRevertButton();

			bool				_IsReplicantInstalled();
			void				_ShowReplicant(bool show);

	static	const char*			_ItemName(const BListItem* item);
	static	int					_CompareTopLevelListItems(const BListItem* a,
									const BListItem* b);
	static	int					_CompareListItems(const BListItem* a,
									const BListItem* b);

private:
	typedef BObjectList<BNetworkSettingsAddOn> AddOnList;
	typedef std::map<BString, BListItem*> ItemMap;
	typedef std::map<BListItem*, BNetworkSettingsItem*> SettingsMap;

			BNetworkSettings	fSettings;
			AddOnList			fAddOns;

			BOutlineListView*	fListView;
			ItemMap				fInterfaceItemMap;
			BListItem*			fServicesItem;
			BListItem*			fDialUpItem;
			BListItem*			fVPNItem;
			BListItem*			fOtherItem;

			SettingsMap			fSettingsMap;

			InterfaceView*		fInterfaceView;
			BView*				fAddOnShellView;

			BButton*			fRevertButton;
};


extern BMessenger gNetworkWindow;


#endif // NETWORK_WINDOW_H
