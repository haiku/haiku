/*
 * Copyright 2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <GroupView.h>


class BButton;
class BListView;
class BTextControl;


class DNSSettingsView: public BGroupView
{
public:
							DNSSettingsView();
		void				AttachedToWindow();
		void				MessageReceived(BMessage* message);

private:
		status_t			_ParseResolvConf();

private:
		BListView*			fServerListView;
		BTextControl*		fTextControl;

		BButton*			fAddButton;
		BButton*			fRemoveButton;
		BButton*			fUpButton;
		BButton*			fDownButton;
};
