//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// IPCPAddon saves the loaded settings.
// IPCPView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _IPCP_ADDON__H
#define _IPCP_ADDON__H

#include <DialUpAddon.h>

#include <String.h>
#include <TextControl.h>
#include <Window.h>

class IPCPWindow;


class IPCPAddon : public DialUpAddon {
	public:
		IPCPAddon(BMessage *addons);
		virtual ~IPCPAddon();
		
		bool IsNew() const
			{ return fIsNew; }
		
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
		
		virtual const char *FriendlyName() const;
		virtual const char *TechnicalName() const;
		virtual const char *KernelModuleName() const;
		
		virtual bool LoadSettings(BMessage *settings, BMessage *profile, bool isNew);
		virtual void IsModified(bool& settings, bool& profile) const;
		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
			bool saveTemporary);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView(BPoint leftTop);

	private:
		int32 FindIPCPProtocol(BMessage& message, BMessage& protocol) const;

	private:
		bool fIsNew;
		BString fIPAddress, fPrimaryDNS, fSecondaryDNS;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		IPCPWindow *fIPCPWindow;
};


class IPCPWindow : public BWindow {
	public:
		IPCPWindow(IPCPAddon *addon, BRect frame);
		
		IPCPAddon *Addon() const
			{ return fAddon; }
		void Reload();
		
		const char *IPAddress() const
			{ return fIPAddress->Text(); }
		const char *PrimaryDNS() const
			{ return fPrimaryDNS->Text(); }
		const char *SecondaryDNS() const
			{ return fSecondaryDNS->Text(); }
		
		virtual void MessageReceived(BMessage *message);

	private:
		IPCPAddon *fAddon;
		BButton *fCancelButton, *fOKButton;
		BTextControl *fIPAddress, *fPrimaryDNS, *fSecondaryDNS;
		BString fPreviousIPAddress, fPreviousPrimaryDNS, fPreviousSecondaryDNS;
};


#endif
