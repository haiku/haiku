//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// GeneralAddon saves the loaded settings.
// GeneralView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _GENERAL_ADDON__H
#define _GENERAL_ADDON__H

#include <DialUpAddon.h>

#include <CheckBox.h>
#include <Message.h>
#include <String.h>
#include <TextControl.h>

class GeneralView;


class GeneralAddon : public DialUpAddon {
	public:
		GeneralAddon(BMessage *addons);
		virtual ~GeneralAddon();
		
		bool IsNew() const
			{ return fIsNew; }
		
		const char *DeviceName() const
			{ return fDeviceName.String(); }
		const char *Username() const
			{ return fUsername.String(); }
		const char *Password() const
			{ return fPassword.String(); }
		bool HasPassword() const
			{ return fHasPassword; }
		
		DialUpAddon *FindDevice(const BString& moduleName) const;
		DialUpAddon *DeviceAddon() const
			{ return fDeviceAddon; }
		
		int32 CountAuthenticators() const
			{ return fAuthenticatorsCount; }
		
		BMessage *Settings() const
			{ return fSettings; }
		BMessage *Profile() const
			{ return fProfile; }
		
		virtual int32 Position() const
			{ return 0; }
		virtual bool LoadSettings(BMessage *settings, BMessage *profile, bool isNew);
		bool LoadDeviceSettings(BMessage *settings, BMessage *profile);
		bool LoadAuthenticationSettings(BMessage *settings, BMessage *profile);
		
		virtual bool HasTemporaryProfile() const;
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
		bool fIsNew, fHasPassword;
		BString fDeviceName, fUsername, fPassword;
		DialUpAddon *fDeviceAddon;
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
			{ return fSavePassword->Value(); }
		
		const char *DeviceName() const;
		const char *AuthenticatorName() const;
		bool IsDeviceModified() const;
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		void ReloadDeviceView();
		
		void AddDevices();
		void AddAuthenticators();
		void AddAddonsToMenu(BMenu *menu, const char *type, uint32 what);
		int32 FindNextMenuInsertionIndex(BMenu *menu, const BString& name,
			int32 index = 0);

	private:
		GeneralAddon *fAddon;
		DialUpAddon *fDeviceAddon;
		BBox *fDeviceBox, *fAuthenticationBox;
		BMenuField *fDeviceField, *fAuthenticatorField;
		BMenuItem *fAuthenticatorNone;
		BTextControl *fUsername, *fPassword;
		BCheckBox *fSavePassword;
};


#endif
