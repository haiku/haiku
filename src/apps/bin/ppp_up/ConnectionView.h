/*
 * Copyright 2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef CONNECTION_VIEW__H
#define CONNECTION_VIEW__H

#include <View.h>
#include <PPPDefs.h>
#include "PPPUpAddon.h"
#include <PTPSettings.h>


#define MSG_UPDATE		'MUPD'
#define MSG_REPLY		'MRPY'


class ConnectionView : public BView {
		friend class ConnectionWindow;

	public:
		ConnectionView(BRect rect, const char *name, ppp_interface_id id,
			thread_id thread);
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		
		// for PPPUpAddon
		PPPUpAddon *Addon() const
			{ return fAddon; }
		void Reload();
		const char *Username() const
			{ return fUsername->Text(); }
		const char *Password() const
			{ return fPassword->Text(); }
		bool DoesSavePassword() const
			{ return fSavePassword->Value(); }
		
		bool HasTemporaryProfile() const
			{ return !DoesSavePassword(); }
		void IsDeviceModified(bool *settings, bool *profile) const
			{ if(settings) *settings = false; if(profile) *profile = false; }

	private:
		void Connect();
		void Cancel();
		void CleanUp();
		
		BString AttemptString() const;
		void UpdateStatus(int32 code);

	private:
		PPPUpAddon *fAddon;
		BString fInterfaceName;
		ppp_interface_id fID;
		thread_id fReportThread;
		
		BTextControl *fUsername, *fPassword;
		BCheckBox *fSavePassword;
		BStringView *fAttemptView, *fStatusView;
		BButton *fConnectButton, *fCancelButton;
		
		bool fConnecting, fKeepLabel, fReplyRequested;
		PTPSettings fSettings;
};


#endif
