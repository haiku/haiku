//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// ExtrasAddon saves the loaded settings.
// ExtrasView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _EXTRAS_ADDON__H
#define _EXTRAS_ADDON__H

#include <DialUpAddon.h>

#include <CheckBox.h>
#include <RadioButton.h>

class ExtrasView;


class ExtrasAddon : public DialUpAddon {
	public:
		ExtrasAddon(BMessage *addons);
		virtual ~ExtrasAddon();
		
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
		virtual BView *CreateView(BPoint leftTop);

	private:
		bool fIsNew;
		bool fDoesDialOnDemand, fAskBeforeDialing, fDoesAutoRedial;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		ExtrasView *fExtrasView;
};


class ExtrasView : public BView {
	public:
		ExtrasView(ExtrasAddon *addon, BRect frame);
		
		ExtrasAddon *Addon() const
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
		ExtrasAddon *fAddon;
		BCheckBox *fAskBeforeDialing, *fAutoRedial;
		BRadioButton *fDialOnDemand, *fDialManually;
};


#endif
