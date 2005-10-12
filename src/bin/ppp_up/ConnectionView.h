/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef CONNECTION_VIEW__H
#define CONNECTION_VIEW__H

#include <CheckBox.h>
#include <Message.h>
#include <TextControl.h>

#include <PPPInterfaceListener.h>

class BButton;
class BStringView;


class ConnectionView : public BView {
		friend class ConnectionWindow;

	public:
		ConnectionView(BRect rect, const BString& interfaceName);
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		
		const char *Username() const
			{ return fUsername->Text(); }
		const char *Password() const
			{ return fPassword->Text(); }
		bool DoesSavePassword() const
			{ return fSavePassword->Value(); }

	private:
		void Reload();
		
		void Connect();
		void Cancel();
		void CleanUp();
		
		BString AttemptString() const;
		void HandleReportMessage(BMessage *message);
		void UpdateStatus(int32 code);
		void WatchInterface(ppp_interface_id ID);

	private:
		PPPInterfaceListener fListener;
		BString fInterfaceName;
		
		BTextControl *fUsername, *fPassword;
		BCheckBox *fSavePassword;
		BStringView *fAttemptView, *fStatusView;
		BButton *fConnectButton, *fCancelButton;
		
		BMessage fSettings;
		bool fKeepLabel, fHasUsername, fHasPassword, fAskBeforeConnecting;
};


#endif
