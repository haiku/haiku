//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _DIAL_UP_VIEW__H
#define _DIAL_UP_VIEW__H

#include <Message.h>
#include <View.h>

#include <PPPInterfaceListener.h>

class GeneralAddon;


class DialUpView : public BView {
	public:
		DialUpView(BRect frame);
		virtual ~DialUpView();
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		
		void UpDownThread();
		
		// used by ppp_server
		bool SelectInterfaceNamed(const char *name);
		BView *AuthenticationView() const;
		BView *StatusView() const;
		BView *ConnectButton() const;

	private:
		void GetPPPDirectories(BDirectory *settingsDirectory,
			BDirectory *profileDirectory) const;
		
		void HandleReportMessage(BMessage *message);
		void CreateTabs();
		
		void UpdateStatus(int32 code);
		void WatchInterface(ppp_interface_id ID);
		
		bool LoadSettings(bool isNew);
		void IsModified(bool *settings, bool *profile);
		bool SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary);
		bool SaveSettingsToFile();
		
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
		BButton *fConnectButton;
		BPopUpMenu *fInterfaceMenu;
		BMenuField *fMenuField;
		BStringView *fStringView;
			// shows "No interfaces found..." notice
		BTabView *fTabView;
};


#endif
