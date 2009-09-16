/* -----------------------------------------------------------------------
 * Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * ----------------------------------------------------------------------- */

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
		
		// used by ppp_up application
		bool SelectInterfaceNamed(const char *name);
		bool NeedsRequest() const;
		BView *AuthenticationView() const;
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
