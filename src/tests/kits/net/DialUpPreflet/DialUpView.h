/*
 * Copyright 2003-2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef _DIAL_UP_VIEW__H
#define _DIAL_UP_VIEW__H

#include <Message.h>
#include <View.h>

#include <PPPInterfaceListener.h>
#include <PTPSettings.h>


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
		
		void LoadInterfaces();
		
		void AddInterface(const char *name, bool isNew = false);
		void SelectInterface(int32 index, bool isNew = false);
		int32 CountInterfaces() const;
		
		void UpdateControls();

	private:
		PPPInterfaceListener fListener;
		PTPSettings fSettings;
		
		thread_id fUpDownThread;
		
		BMenuItem *fCurrentItem, *fDeleterItem;
		ppp_interface_id fWatching;
		
		bool fKeepLabel;
		BStringView *fStatusView;
		BButton *fConnectButton, *fCreateNewButton;
		BPopUpMenu *fInterfaceMenu;
		BMenuField *fMenuField;
		BCheckBox *fDefaultInterface;
		BStringView *fStringView;
			// shows "No interfaces found..." notice
		BTabView *fTabView;
};


#endif
