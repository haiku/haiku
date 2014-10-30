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
		ConnectionOptionsView *fConnectionOptionsView;
};


class ConnectionOptionsView : public BView {
	public:
		ConnectionOptionsView(ConnectionOptionsAddon *addon, BRect frame);
		
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
