//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _DIAL_UP_VIEW__H
#define _DIAL_UP_VIEW__H

#include <View.h>
#include <Message.h>

#include <PPPInterfaceListener.h>


class DialUpView : public BView {
	public:
		DialUpView(BRect frame);
		virtual ~DialUpView();
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		
		void UpDownThread();

	private:
		void GetPPPDirectories(BDirectory *settingsDirectory,
			BDirectory *profileDirectory) const;
		
		void HandleReportMessage(BMessage *message);
		void CreateTabs();
		
		void UpdateStatus(int32 code);
		void WatchInterface(ppp_interface_id ID);
		
		void LoadSettings();
		void IsModified(bool& settings, bool& profile);
		bool SaveSettings(BMessage& settings, BMessage& profile, bool saveTemporary);
		bool SaveSettingsToFile();
		
		void AddInterface(const char *name);
		void SelectInterface(int32 index);
		int32 CountInterfaces() const;

	private:
		PPPInterfaceListener fListener;
		
		thread_id fUpDownThread;
		
		BMessage fAddons, fSettings;
		driver_settings *fDriverSettings;
		BMenuItem *fCurrentItem;
		ppp_interface_id fWatching;
		
		bool fKeepLabel;
		BStringView *fStatusView;
		BButton *fConnectButton;
		BPopUpMenu *fInterfaceMenu;
		BMenuField *fMenuField;
		BTabView *fTabView;
};


#endif
