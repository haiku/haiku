//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _GENERAL_ADDON__H
#define _GENERAL_ADDON__H

#include <DialUpAddon.h>

#include <View.h>
#include <Message.h>
#include <String.h>
#include <TextControl.h>

class GeneralView;


class GeneralAddon : public DialUpAddon {
	public:
		GeneralAddon(BMessage *addons);
		virtual ~GeneralAddon();
		
		const char *Username() const
			{ return fUsername.String(); }
		const char *Password() const
			{ return fPassword.String(); }
		
		virtual int32 Position() const
			{ return 0; }
		virtual bool LoadSettings(BMessage *settings, BMessage *profile);
		bool LoadDeviceSettings(BMessage *settings, BMessage *profile);
		bool LoadAuthenticationSettings(BMessage *settings, BMessage *profile);
		
		virtual void IsModified(bool& settings, bool& profile) const;
		void IsDeviceModified(bool& settings, bool& profile) const;
		void IsAuthenticationModified(bool& settings, bool& profile) const;
		
		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
			bool saveTemporary);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView(BPoint leftTop);

	private:
		bool GetAuthenticator(const BString& moduleName, BMessage *entry) const;
		bool MarkAuthenticatorAsValid(const BString& moduleName);

	private:
		bool fHasDevice, fHasPassword;
		BString fDevice, fUsername, fPassword;
		int32 fAuthenticatorsCount;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		GeneralView *fGeneralView;
};


class GeneralView : public BView {
	public:
		GeneralView(GeneralAddon *addon, BRect frame);
		virtual ~GeneralView();
		
		GeneralAddon *Addon() const
			{ return fAddon; }
		void Reload();
		
		const char *Username() const
			{ return fUsername->Text(); }
		const char *Password() const
			{ return fPassword->Text(); }
		bool DoesSavePassword() const
			{ return true; }
				// TODO: implement me!
		
		const char *DeviceName() const
			{ return NULL; }
				// TODO: implement me!
		const char *AuthenticatorName() const
			{ return NULL; }
				// TODO: implement me!
		bool IsDeviceModified() const
			{ return false; }
				// TODO: implement me!
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		GeneralAddon *fAddon;
		BBox *fDeviceBox, *fAuthenticationBox;
		BMenuField *fAuthenticator;
		BTextControl *fUsername, *fPassword;
};


#endif
