/*
 * Copyright 2014-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */
#ifndef DNS_SETTINGS_VIEW_H
#define DNS_SETTINGS_VIEW_H


#include <NetworkSettingsAddOn.h>
#include <StringList.h>
#include <View.h>


using namespace BNetworkKit;

class BButton;
class BListView;
class BTextControl;


class DNSSettingsView : public BView {
public:
								DNSSettingsView(BNetworkSettingsItem* item);
								~DNSSettingsView();

			status_t			Revert();
			bool				IsRevertable() const;

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			status_t			_LoadDNSConfiguration();
			status_t			_SaveDNSConfiguration();

private:
			BNetworkSettingsItem*
								fItem;
			BListView*			fServerListView;
			BStringList			fRevertList;
			BTextControl*		fTextControl;
			BTextControl*		fDomain;

			BButton*			fAddButton;
			BButton*			fRemoveButton;
			BButton*			fUpButton;
			BButton*			fDownButton;
			BButton*			fApplyButton;
};


#endif // DNS_SETTINGS_VIEW_H
