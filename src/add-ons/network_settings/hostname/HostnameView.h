/*
 * Copyright 2014-2019 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Rob Gill, rrobgill@protonmail.com
 */
#ifndef HOSTNAME_VIEW_H
#define HOSTNAME_VIEW_H


#include <NetworkSettingsAddOn.h>
#include <StringList.h>
#include <View.h>


using namespace BNetworkKit;

class BButton;
class BTextControl;


class HostnameView : public BView {
public:
								HostnameView(BNetworkSettingsItem* item);
								~HostnameView();

			status_t			Revert();
			bool				IsRevertable() const;

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			status_t			_LoadHostname();
			status_t			_SaveHostname();

private:
			BNetworkSettingsItem*
								fItem;
			BTextControl*		fHostname;

			BButton*			fApplyButton;
};


#endif // HOSTNAME_VIEW_H
