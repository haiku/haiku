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
// PPPoEAddon saves the loaded settings.
// PPPoEView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _PPPoE_ADDON__H
#define _PPPoE_ADDON__H

#include <DialUpAddon.h>

#include <String.h>
#include <TextControl.h>

class PPPoEView;


class PPPoEAddon : public DialUpAddon {
	public:
		PPPoEAddon(BMessage *addons);
		virtual ~PPPoEAddon();
		
		bool IsNew() const
			{ return fIsNew; }
		
		const char *InterfaceName() const
			{ return fInterfaceName.String(); }
		const char *ServiceName() const
			{ return fServiceName.String(); }
		
		BMessage *Settings() const
			{ return fSettings; }
		BMessage *Profile() const
			{ return fProfile; }
		
		virtual const char *FriendlyName() const;
		virtual const char *TechnicalName() const;
		virtual const char *KernelModuleName() const;
		
		virtual bool LoadSettings(BMessage *settings, BMessage *profile, bool isNew);
		
		virtual void IsModified(bool *settings, bool *profile) const;
		
		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
			bool saveTemporary);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView(BPoint leftTop);
		
		void UnregisterView()
			{ fPPPoEView = NULL; }

	private:
		bool fIsNew;
		BString fInterfaceName, fServiceName;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		PPPoEView *fPPPoEView;
		float fHeight;
			// height of PPPoEView
};


class PPPoEView : public BView {
	public:
		PPPoEView(PPPoEAddon *addon, BRect frame);
		virtual ~PPPoEView();
		
		PPPoEAddon *Addon() const
			{ return fAddon; }
		void Reload();
		
		const char *InterfaceName() const
			{ return fInterfaceName.String(); }
		const char *ServiceName() const
			{ return fServiceName->Text(); }
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		void ReloadInterfaces();

	private:
		PPPoEAddon *fAddon;
		BMenuField *fInterface;
		BMenuItem *fOtherInterface;
		BString fInterfaceName;
		BTextControl *fServiceName;
};


#endif
