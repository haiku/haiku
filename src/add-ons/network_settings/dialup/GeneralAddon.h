/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// GeneralAddon saves the loaded settings.
// GeneralView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _GENERAL_ADDON__H
#define _GENERAL_ADDON__H

#include "DialUpAddon.h"

#include <CheckBox.h>
#include <String.h>
#include <TextControl.h>

class BBox;
class BMenuField;
class BMenuItem;
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

		bool NeedsAuthenticationRequest() const;

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
		bool LoadDeviceSettings();
		bool LoadAuthenticationSettings();

		virtual bool HasTemporaryProfile() const;
		virtual void IsModified(bool *settings, bool *profile) const;
		void IsDeviceModified(bool *settings, bool *profile) const;
		void IsAuthenticationModified(bool *settings, bool *profile) const;

		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
			bool saveTemporary);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView();

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
		GeneralView(GeneralAddon *addon);
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

		bool HasTemporaryProfile() const
			{ return !DoesSavePassword() || (fDeviceAddon &&
				fDeviceAddon->HasTemporaryProfile()); }

		DialUpAddon *DeviceAddon() const
			{ return fDeviceAddon; }
		const char *DeviceName() const;
		const char *AuthenticatorName() const;
		void IsDeviceModified(bool *settings, bool *profile) const;

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
		BBox *fDeviceBox;
		BMenuField *fDeviceField, *fAuthenticatorField;
		BMenuItem *fAuthenticatorNone, *fAuthenticatorDefault;
		BTextControl *fUsername, *fPassword;
		BCheckBox *fSavePassword;
};


#endif
