/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DIAL_UP_VIEW__H
#define _DIAL_UP_VIEW__H

#include <Message.h>
#include <View.h>

#include <PPPInterfaceListener.h>

class BMenuItem;
class BStringView;
class BButton;
class BPopUpMenu;
class BMenuField;
class BTabView;
class GeneralAddon;


class DialUpView : public BView {
	public:
		DialUpView();
		virtual ~DialUpView();

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

		void UpDownThread();

		// used by ppp_up application
		bool SelectInterfaceNamed(const char *name);
		bool NeedsRequest() const;
		BView *StatusView() const;
		BView *ConnectButton() const;
		bool SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary);
		bool SaveSettingsToFile();

	private:
		void GetPPPDirectories(BDirectory *settingsDirectory,
			BDirectory *profileDirectory) const;

		void HandleReportMessage(BMessage *message);
		void CreateTabs();

		void UpdateStatus(int32 code);
		void WatchInterface(ppp_interface_id ID);

		bool LoadSettings(bool isNew);
		void IsModified(bool *settings, bool *profile);

		void LoadInterfaces();
		void LoadAddons();

		void AddInterface(const char *name, bool isNew = false);
		void SelectInterface(int32 index, bool isNew = false);
		int32 CountInterfaces() const;

		void UpdateControls();

	private:
		PPPInterfaceListener fListener;

		thread_id fUpDownThread;

		BMessage fAddons, fSettings, fProfile;
		driver_settings *fDriverSettings;
		BMenuItem *fCurrentItem, *fDeleterItem;
		ppp_interface_id fWatching;

		GeneralAddon *fGeneralAddon;
		bool fKeepLabel;
		BStringView *fStatusView;
		BButton *fConnectButton, *fCreateNewButton;
		BPopUpMenu *fInterfaceMenu;
		BMenuField *fMenuField;
		BStringView *fStringView;
			// shows "No interfaces found..." notice
		BTabView *fTabView;
};


#endif
