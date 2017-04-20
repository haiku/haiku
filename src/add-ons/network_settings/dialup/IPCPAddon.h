/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// IPCPAddon saves the loaded settings.
// IPCPView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _IPCP_ADDON__H
#define _IPCP_ADDON__H

#include "DialUpAddon.h"

#include <CheckBox.h>
#include <String.h>
#include <TextControl.h>

class BButton;
class IPCPView;


class IPCPAddon : public DialUpAddon {
	public:
		IPCPAddon(BMessage *addons);
		virtual ~IPCPAddon();

		bool IsNew() const
			{ return fIsNew; }

		bool IsEnabled() const
			{ return fIsEnabled; }
		const char *IPAddress() const
			{ return fIPAddress.String(); }
		const char *PrimaryDNS() const
			{ return fPrimaryDNS.String(); }
		const char *SecondaryDNS() const
			{ return fSecondaryDNS.String(); }

		BMessage *Settings() const
			{ return fSettings; }
		BMessage *Profile() const
			{ return fProfile; }

		virtual int32 Position() const
			{ return 10; }

		virtual bool LoadSettings(BMessage *settings, BMessage *profile, bool isNew);
		virtual void IsModified(bool *settings, bool *profile) const;
		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
			bool saveTemporary);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView();

	private:
		int32 FindIPCPProtocol(const BMessage& message, BMessage *protocol) const;

	private:
		bool fIsNew, fIsEnabled;
		BString fIPAddress, fPrimaryDNS, fSecondaryDNS;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		IPCPView *fIPCPView;
};


class IPCPView : public BView {
	public:
		IPCPView(IPCPAddon *addon);

		IPCPAddon *Addon() const
			{ return fAddon; }
		void Reload();

		bool IsEnabled() const
			{ return fIsEnabled->Value(); }
		const char *IPAddress() const
			{ return fIPAddress->Text(); }
		const char *PrimaryDNS() const
			{ return fPrimaryDNS->Text(); }
		const char *SecondaryDNS() const
			{ return fSecondaryDNS->Text(); }

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		void UpdateControls();

	private:
		IPCPAddon *fAddon;
		BCheckBox *fIsEnabled;
		BButton *fCancelButton, *fOKButton;
		BTextControl *fIPAddress, *fPrimaryDNS, *fSecondaryDNS;
};


#endif
