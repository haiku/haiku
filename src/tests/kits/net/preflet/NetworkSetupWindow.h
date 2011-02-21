/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 */
#ifndef NETWORKSETUPWINDOW_H
#define NETWORKSETUPWINDOW_H


#include "NetworkSetupAddOn.h"

#include <map>


typedef std::map<int, NetworkSetupAddOn*> NetworkAddOnMap;


class NetworkSetupWindow;

#include <Window.h>

class BTabView;
class BButton;
class BMenu;

class NetworkSetupWindow : public BWindow
{
	public:
								NetworkSetupWindow(const char *title);
								~NetworkSetupWindow();

	typedef	BWindow				inherited;

	static	const uint32		kMsgAddonShow = 'show';
	static	const uint32		kMsgProfileSelected = 'prof';
	static	const uint32		kMsgProfileManage = 'mngp';
	static	const uint32		kMsgProfileNew = 'newp';
	static	const uint32		kMsgApply = 'aply';
	static	const uint32		kMsgRevert = 'rvrt';

			bool				QuitRequested();
			void				MessageReceived(BMessage* msg);

	private:
			void				_BuildProfilesMenu(BMenu* menu, int32 msg);
			void				_BuildShowTabView(int32 msg);

			BButton*			fHelpButton;
			BButton*			fRevertButton;
			BButton*			fApplyButton;

			NetworkAddOnMap		fNetworkAddOnMap;

			BTabView*			fPanel;
			BView*				fAddonView;
			int					fAddonCount;
			BRect				fMinAddonViewRect;
};


#endif // ifdef NETWORKSETUPWINDOW_H

