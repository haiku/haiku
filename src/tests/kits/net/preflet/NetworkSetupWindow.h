/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#ifndef NETWORKSETUPWINDOW_H
#define NETWORKSETUPWINDOW_H

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

			BTabView*			fPanel;
			BView*				fAddonView;
			BRect				fMinAddonViewRect;
};


#endif // ifdef NETWORKSETUPWINDOW_H

