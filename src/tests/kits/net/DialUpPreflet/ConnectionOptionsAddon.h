/*
 * Copyright 2003-2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// ConnectionOptionsAddon saves the loaded settings.
// ConnectionOptionsView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _CONNECTION_OPTIONS_ADDON__H
#define _CONNECTION_OPTIONS_ADDON__H

#include <DialUpAddon.h>

#include <CheckBox.h>
#include <RadioButton.h>

class ConnectionOptionsView;


class ConnectionOptionsAddon : public DialUpAddon {
	public:
		ConnectionOptionsAddon(BMessage *addons);
		virtual ~ConnectionOptionsAddon();
		
		bool IsNew() const
			{ return fIsNew; }
		
		bool AskBeforeConnecting() const
			{ return fAskBeforeConnecting; }
		bool DoesAutoReconnect() const
			{ return fDoesAutoReconnect; }
		
		BMessage *Settings() const
			{ return fSettings; }
		BMessage *Profile() const
			{ return fProfile; }
		
		virtual int32 Position() const
			{ return 50; }
		virtual bool LoadSettings(BMessage *settings, BMessage *profile, bool isNew);
		virtual void IsModified(bool *settings, bool *profile) const;
		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
			bool saveTemporary);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView(BPoint leftTop);

	private:
		bool fIsNew, fDeleteView;
		bool fAskBeforeConnecting, fDoesAutoReconnect;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		ConnectionOptionsView *fConnectionOptionsView;
};


class ConnectionOptionsView : public BView {
	public:
		ConnectionOptionsView(ConnectionOptionsAddon *addon, BRect frame);
		
		ConnectionOptionsAddon *Addon() const
			{ return fAddon; }
		void Reload();
		
		bool AskBeforeConnecting() const
			{ return fAskBeforeConnecting->Value(); }
		bool DoesAutoReconnect() const
			{ return fAutoReconnect->Value(); }
		
		virtual void AttachedToWindow();

	private:
		ConnectionOptionsAddon *fAddon;
		BCheckBox *fAskBeforeConnecting, *fAutoReconnect;
};


#endif
