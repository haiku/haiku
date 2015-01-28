/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 */
#ifndef NETWORK_SETUP_WINDOW_H
#define NETWORK_SETUP_WINDOW_H


#include "NetworkSetupAddOn.h"

#include <map>

#include <Window.h>


typedef std::map<int, NetworkSetupAddOn*> NetworkAddOnMap;

class NetworkSetupWindow;
class BTabView;
class BButton;
class BMenu;


class NetworkSetupWindow : public BWindow {
public:
	static	const uint32		kMsgProfileSelected = 'prof';
	static	const uint32		kMsgProfileManage = 'mngp';
	static	const uint32		kMsgProfileNew = 'newp';
	static	const uint32		kMsgApply = 'aply';
	static	const uint32		kMsgRevert = 'rvrt';
	static	const uint32		kMsgToggleReplicant = 'trep';

public:
								NetworkSetupWindow();
	virtual						~NetworkSetupWindow();

			bool				QuitRequested();
			void				MessageReceived(BMessage* message);

private:
	typedef	BWindow				inherited;

			void				_BuildProfilesMenu(BMenu* menu, int32 what);
			void				_BuildShowTabView();

			bool				_IsReplicantInstalled();
			void				_ShowReplicant(bool show);

private:
			BButton*			fRevertButton;
			BButton*			fApplyButton;

			NetworkAddOnMap		fNetworkAddOnMap;

			BTabView*			fPanel;
			BView*				fAddOnView;
			int					fAddOnCount;
};


#endif // NETWORK_SETUP_WINDOW_H
