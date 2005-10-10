/*
 * Copyright 2003-2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// GeneralAddon saves the loaded settings.
// GeneralView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _GENERAL_ADDON__H
#define _GENERAL_ADDON__H

#include <DialUpAddon.h>

#include <CheckBox.h>
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
		const char *SessionPassword() const;
		bool HasPassword() const
			{ return fHasPassword; }
		
		DialUpAddon *FindDevice(const BString& moduleName) const;
		DialUpAddon *DeviceAddon() const
			{ return fDeviceAddon; }
		
		int32 CountAuthenticators() const
			{ return fAuthenticatorsCount; }
		
		BMessage *Settings() const
			{ return fSettings; }
		
		virtual int32 Position() const
			{ return 0; }
		virtual bool LoadSettings(BMessage *settings, bool isNew);
		bool LoadDeviceSettings();
		bool LoadAuthenticationSettings();
		
		virtual void IsModified(bool *settings) const;
		void IsDeviceModified(bool *settings) const;
		void IsAuthenticationModified(bool *settings) const;
		
		virtual bool SaveSettings(BMessage *settings);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView(BPoint leftTop);

	private:
		bool GetAuthenticator(const BString& moduleName, BMessage *entry) const;
		bool MarkAuthenticatorAsValid(const BString& moduleName);

	private:
		bool fIsNew, fHasPassword, fDeleteView;
		BString fDeviceName, fUsername, fPassword;
		DialUpAddon *fDeviceAddon;
		int32 fAuthenticatorsCount;
		BMessage *fSettings;
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
		
		DialUpAddon *DeviceAddon() const
			{ return fDeviceAddon; }
		const char *DeviceName() const;
		const char *AuthenticatorName() const;
		void IsDeviceModified(bool *settings) const;
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		void ReloadDeviceView();
		void UpdateControls();
		
		void AddDevices();
		void AddAuthenticators();

	private:
		GeneralAddon *fAddon;
		DialUpAddon *fDeviceAddon;
		BBox *fDeviceBox, *fAuthenticationBox;
		BMenuField *fDeviceField, *fAuthenticatorField;
		BMenuItem *fAuthenticatorNone, *fAuthenticatorDefault;
		BTextControl *fUsername, *fPassword;
		BCheckBox *fSavePassword;
};


#endif
