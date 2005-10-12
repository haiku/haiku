/*
 * Copyright 2003-2005, Waldemar Kornewald <wkornew@gmx.net>
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

	private:
		void BringUpOrDown();
		
		void HandleReportMessage(BMessage *message);
		void CreateTabs();
		
		void UpdateStatus(int32 code);
		void WatchInterface(ppp_interface_id ID);
		
		void LoadInterfaces();
		
		void AddInterface(const char *name, bool isNew = false);
		void SelectInterface(int32 index, bool isNew = false);
		int32 CountInterfaces() const;
		BMenuItem *FindInterface(BString name);
		
		void UpdateControls();
		void UpdateDefaultInterface();

	private:
		PPPInterfaceListener fListener;
		PTPSettings fSettings;
		
		thread_id fUpDownThread;
		
		BMenuItem *fCurrentItem, *fDeleterItem;
		
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
