/* -----------------------------------------------------------------------
 * Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * ----------------------------------------------------------------------- */

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
		virtual BView *CreateView(BPoint leftTop);
		
		// used by ppp_up application
		BView *AuthenticationView() const;

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
		
		// used by ppp_up application
		BView *AuthenticationView() const
			{ return fAuthenticationView; }

	private:
		void ReloadDeviceView();
		void UpdateControls();
		
		void AddDevices();
		void AddAuthenticators();

	private:
		GeneralAddon *fAddon;
		DialUpAddon *fDeviceAddon;
		BBox *fDeviceBox, *fAuthenticationBox;
		BView *fAuthenticationView;
		BMenuField *fDeviceField, *fAuthenticatorField;
		BMenuItem *fAuthenticatorNone, *fAuthenticatorDefault;
		BTextControl *fUsername, *fPassword;
		BCheckBox *fSavePassword;
};


#endif
