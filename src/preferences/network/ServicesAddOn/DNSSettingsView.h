/*
 * Copyright 2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <GroupView.h>
#include <StringList.h>


class BButton;
class BListView;
class BTextControl;


class DNSSettingsView: public BGroupView
{
public:
							DNSSettingsView();
		void				AttachedToWindow();
		void				MessageReceived(BMessage* message);

		status_t			Apply();
		status_t			Revert();

private:
		status_t			_LoadDNSConfiguration();
		status_t			_SaveDNSConfiguration();

private:
		BListView*			fServerListView;
		BStringList			fRevertList;
		BTextControl*		fTextControl;

		BButton*			fAddButton;
		BButton*			fRemoveButton;
		BButton*			fUpButton;
		BButton*			fDownButton;
};
