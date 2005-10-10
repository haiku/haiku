/*
 * Copyright 2003-2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// IPCPAddon saves the loaded settings.
// IPCPView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _IPCP_ADDON__H
#define _IPCP_ADDON__H

#include <DialUpAddon.h>

#include <CheckBox.h>
#include <String.h>
#include <TextControl.h>

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
		
		virtual int32 Position() const
			{ return 10; }
		
		virtual bool LoadSettings(BMessage *settings, bool isNew);
		virtual void IsModified(bool *settings) const;
		virtual bool SaveSettings(BMessage *settings);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView(BPoint leftTop);

	private:
		int32 FindIPCPProtocol(const BMessage& message, BMessage *protocol) const;

	private:
		bool fIsNew, fIsEnabled, fDeleteView;
		BString fIPAddress, fPrimaryDNS, fSecondaryDNS;
		BMessage *fSettings;
			// saves last settings state
		IPCPView *fIPCPView;
};


class IPCPView : public BView {
	public:
		IPCPView(IPCPAddon *addon, BRect frame);
		
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
