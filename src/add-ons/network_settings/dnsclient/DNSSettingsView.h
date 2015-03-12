/*
 * Copyright 2014-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */
#ifndef DNS_SETTINGS_VIEW_H
#define DNS_SETTINGS_VIEW_H


#include <GroupView.h>
#include <StringList.h>


class BButton;
class BListView;
class BTextControl;


class DNSSettingsView : public BGroupView {
public:
								DNSSettingsView();
								~DNSSettingsView();

			status_t			Apply();
			status_t			Revert();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			status_t			_LoadDNSConfiguration();
			status_t			_SaveDNSConfiguration();

private:
			BListView*			fServerListView;
			BStringList			fRevertList;
			BTextControl*		fTextControl;
			BTextControl*		fDomain;

			BButton*			fAddButton;
			BButton*			fRemoveButton;
			BButton*			fUpButton;
			BButton*			fDownButton;
};


#endif // DNS_SETTINGS_VIEW_H
