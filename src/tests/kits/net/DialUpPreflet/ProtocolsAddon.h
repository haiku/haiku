//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// ProtocolsAddon saves the loaded settings.
// ProtocolsView saves the current settings.
//-----------------------------------------------------------------------

#ifndef _PROTOCOLS_ADDON__H
#define _PROTOCOLS_ADDON__H

#include <DialUpAddon.h>

#include <ListView.h>
#include <String.h>

class ProtocolsView;


class ProtocolsAddon : public DialUpAddon {
	public:
		ProtocolsAddon(BMessage *addons);
		virtual ~ProtocolsAddon();
		
		bool IsNew() const
			{ return fIsNew; }
		
		int32 CountProtocols() const
			{ return fProtocolsCount; }
		
		BMessage *Settings() const
			{ return fSettings; }
		BMessage *Profile() const
			{ return fProfile; }
		
		virtual int32 Position() const
			{ return 10; }
		virtual bool LoadSettings(BMessage *settings, BMessage *profile, bool isNew);
		bool LoadProtocolSettings(const BMessage& parameter);
		
		virtual bool HasTemporaryProfile() const;
		virtual void IsModified(bool& settings, bool& profile) const;
		
		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
			bool saveTemporary);
		virtual bool GetPreferredSize(float *width, float *height) const;
		virtual BView *CreateView(BPoint leftTop);

	private:
		bool GetProtocol(const BString& moduleName, DialUpAddon **protocol) const;

	private:
		bool fIsNew;
		int32 fProtocolsCount;
		BMessage *fSettings, *fProfile;
			// saves last settings state
		ProtocolsView *fProtocolsView;
};


class ProtocolsView : public BView {
	public:
		ProtocolsView(ProtocolsAddon *addon, BRect frame);
		virtual ~ProtocolsView();
		
		ProtocolsAddon *Addon() const
			{ return fAddon; }
		void Reload();
		
		bool HasTemporaryProfile() const;
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		
		int32 CountProtocols() const
			{ return fListView->CountItems(); }
		DialUpAddon *ProtocolAt(int32 index) const;
		bool HasProtocol(const BString& moduleName) const;

	private:
		void RegisterProtocol(const DialUpAddon *protocol);
		void RegisterProtocol(int32 index);
			// moves the protocol from the pop-up menu to the list view
		void UnregisterProtocol(int32 index);
			// moves the protocol from the list view to the pop-up menu
		
		void UpdateButtons();
			// enables/disables buttons depending on the current state

	private:
		ProtocolsAddon *fAddon;
		BButton *fAddButton, *fRemoveButton, *fPreferencesButton;
		BListView *fListView;
		BPopUpMenu *fProtocolsMenu;
};


#endif
