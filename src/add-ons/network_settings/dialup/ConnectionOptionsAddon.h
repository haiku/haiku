/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// ConnectionOptionsAddon saves the loaded settings.
// ConnectionOptionsView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _CONNECTION_OPTIONS_ADDON__H
#define _CONNECTION_OPTIONS_ADDON__H

#include "DialUpAddon.h"

#include <CheckBox.h>
#include <RadioButton.h>

class ConnectionOptionsView;


class ConnectionOptionsAddon : public DialUpAddon {
	public:
		ConnectionOptionsAddon(BMessage *addons);
		virtual ~ConnectionOptionsAddon();

		bool IsNew() const
			{ return fIsNew; }

		bool DoesDialOnDemand() const
			{ return fDoesDialOnDemand; }
		bool AskBeforeDialing() const
			{ return fAskBeforeDialing; }
		bool DoesAutoRedial() const
			{ return fDoesAutoRedial; }

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
		virtual BView *CreateView();

	private:
		bool fIsNew;
		bool fDoesDialOnDemand, fAskBeforeDialing, fDoesAutoRedial;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		ConnectionOptionsView *fConnectionOptionsView;
};


class ConnectionOptionsView : public BView {
	public:
		ConnectionOptionsView(ConnectionOptionsAddon *addon);

		ConnectionOptionsAddon *Addon() const
			{ return fAddon; }
		void Reload();

		bool DoesDialOnDemand() const
			{ return fDialOnDemand->Value(); }
		bool AskBeforeDialing() const
			{ return fAskBeforeDialing->Value(); }
		bool DoesAutoRedial() const
			{ return fAutoRedial->Value(); }

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		void UpdateControls();

	private:
		ConnectionOptionsAddon *fAddon;
		BCheckBox *fDialOnDemand, *fAskBeforeDialing, *fAutoRedial;
};


#endif
