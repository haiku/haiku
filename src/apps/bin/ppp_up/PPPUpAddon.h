/*
 * Copyright 2003-2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef PPP_UP_ADDON__H
#define PPP_UP_ADDON__H

#include <DialUpAddon.h>

#include <CheckBox.h>
#include <String.h>
#include <TextControl.h>

class ConnectionView;

// string constants for information saved in the settings message
#define PPP_UP_AUTHENTICATION	"Authentication"
#define PPP_UP_AUTHENTICATORS	"Authenticators"


class PPPUpAddon : public DialUpAddon {
	public:
		PPPUpAddon(BMessage *addons, ConnectionView *connectionView);
		virtual ~PPPUpAddon();
		
		bool IsNew() const
			{ return fIsNew; }
		bool AskBeforeConnecting() const
			{ return fAskBeforeConnecting; }
		
		const char *DeviceName() const
			{ return fDeviceName.String(); }
		const char *AuthenticatorName() const
			{ return fAuthenticatorName.String(); }
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

	private:
		bool GetAuthenticator(const BString& moduleName, BMessage *entry) const;
		bool MarkAuthenticatorAsValid(const BString& moduleName);

	private:
		bool fIsNew, fAskBeforeConnecting, fHasPassword;
		BString fDeviceName, fAuthenticatorName, fUsername, fPassword;
		DialUpAddon *fDeviceAddon;
		int32 fAuthenticatorsCount;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		ConnectionView *fConnectionView;
};


#endif
