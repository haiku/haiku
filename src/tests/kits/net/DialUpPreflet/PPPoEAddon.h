/*
 * Copyright 2003-2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// PPPoEAddon saves the loaded settings.
// PPPoEView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _PPPoE_ADDON__H
#define _PPPoE_ADDON__H

#include <DialUpAddon.h>

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
		const char *ServiceName() const
			{ return fServiceName.String(); }
		
		BMessage *Settings() const
			{ return fSettings; }
		
		virtual const char *FriendlyName() const;
		virtual const char *TechnicalName() const;
		virtual const char *KernelModuleName() const;
		
		virtual bool LoadSettings(BMessage *settings, bool isNew);
		
		virtual void IsModified(bool *settings) const;
		
		virtual bool SaveSettings(BMessage *settings);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView(BPoint leftTop);
		
		void UnregisterView()
			{ fPPPoEView = NULL; }

	private:
		bool fIsNew;
		BString fInterfaceName, fServiceName;
		BMessage *fSettings;
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
			{ return fInterfaceName.String(); }
		const char *ServiceName() const
			{ return fServiceName->Text(); }
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		void ReloadInterfaces();

	private:
		PPPoEAddon *fAddon;
		BMenuField *fInterface;
		BMenuItem *fOtherInterface;
		BString fInterfaceName;
		BTextControl *fServiceName;
};


#endif
