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
// IPCPAddon saves the loaded settings.
// IPCPView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _IPCP_ADDON__H
#define _IPCP_ADDON__H

#include <DialUpAddon.h>

#include <CheckBox.h>
#include <String.h>
#include <TextControl.h>

class IPCPView;


class IPCPAddon : public DialUpAddon {
	public:
		IPCPAddon(BMessage *addons);
		virtual ~IPCPAddon();
		
		bool IsNew() const
			{ return fIsNew; }
		
		bool IsEnabled() const
			{ return fIsEnabled; }
		const char *IPAddress() const
			{ return fIPAddress.String(); }
		const char *PrimaryDNS() const
			{ return fPrimaryDNS.String(); }
		const char *SecondaryDNS() const
			{ return fSecondaryDNS.String(); }
		
		BMessage *Settings() const
			{ return fSettings; }
		BMessage *Profile() const
			{ return fProfile; }
		
		virtual int32 Position() const
			{ return 10; }
		
		virtual bool LoadSettings(BMessage *settings, BMessage *profile, bool isNew);
		virtual void IsModified(bool *settings, bool *profile) const;
		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
			bool saveTemporary);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView(BPoint leftTop);

	private:
		int32 FindIPCPProtocol(const BMessage& message, BMessage *protocol) const;

	private:
		bool fIsNew, fIsEnabled;
		BString fIPAddress, fPrimaryDNS, fSecondaryDNS;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		IPCPView *fIPCPView;
};


class IPCPView : public BView {
	public:
		IPCPView(IPCPAddon *addon, BRect frame);
		
		IPCPAddon *Addon() const
			{ return fAddon; }
		void Reload();
		
		bool IsEnabled() const
			{ return fIsEnabled->Value(); }
		const char *IPAddress() const
			{ return fIPAddress->Text(); }
		const char *PrimaryDNS() const
			{ return fPrimaryDNS->Text(); }
		const char *SecondaryDNS() const
			{ return fSecondaryDNS->Text(); }
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		void UpdateControls();

	private:
		IPCPAddon *fAddon;
		BCheckBox *fIsEnabled;
		BButton *fCancelButton, *fOKButton;
		BTextControl *fIPAddress, *fPrimaryDNS, *fSecondaryDNS;
};


#endif
