//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// PPPoEAddon saves the loaded settings.
// PPPoEView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _PPPoE_ADDON__H
#define _PPPoE_ADDON__H

#include <DialUpAddon.h>

#include <CheckBox.h>
#include <Message.h>
#include <String.h>
#include <TextControl.h>

class PPPoEView;


class PPPoEAddon : public DialUpAddon {
	public:
		PPPoEAddon(BMessage *addons);
		virtual ~PPPoEAddon();
		
		bool IsNew() const
			{ return fIsNew; }
		
		const char *InterfaceName() const
			{ return fInterfaceName.String(); }
		const char *ACName() const
			{ return fACName.String(); }
		const char *ServiceName() const
			{ return fServiceName.String(); }
		
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
		bool fIsNew;
		BString fInterfaceName, fACName, fServiceName;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		PPPoEView *fPPPoEView;
		float fHeight;
			// height of PPPoEView
};


class PPPoEView : public BView {
	public:
		PPPoEView(PPPoEAddon *addon, BRect frame);
		virtual ~PPPoEView();
		
		PPPoEAddon *Addon() const
			{ return fAddon; }
		void Reload();
		
		const char *InterfaceName() const
			{ return fInterfaceName->Text(); }
		const char *ACName() const
			{ return fACName->Text(); }
		const char *ServiceName() const
			{ return fServiceName->Text(); }
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		PPPoEAddon *fAddon;
		BButton *fServiceButton, *fCancelButton, *fOKButton;
		BTextControl *fInterfaceName, *fACName, *fServiceName;
		BWindow *fServiceWindow;
		BString fPreviousACName, fPreviousServiceName;
};


#endif
