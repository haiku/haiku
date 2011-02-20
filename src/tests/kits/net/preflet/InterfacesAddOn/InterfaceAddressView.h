/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef INTERFACE_ADDRESS_VIEW_H
#define INTERFACE_ADDRESS_VIEW_H


#include "NetworkSettings.h"

#include <MenuField.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <TextControl.h>
#include <GroupView.h>


enum {
	M_MODE_AUTO = 'iato',
	M_MODE_STATIC = 'istc',
	M_MODE_NONE = 'inon'
};


class InterfaceAddressView : public BGroupView {
public:
								InterfaceAddressView(BRect frame,
									int family, NetworkSettings* settings);
	virtual						~InterfaceAddressView();
	virtual	void				MessageReceived(BMessage* message);
	virtual void				AttachedToWindow();
			status_t			RevertFields();
			status_t			SaveFields();


private:
			void				_EnableFields(bool enabled);

			NetworkSettings*	fSettings;
			int					fFamily;

			BPopUpMenu*			fModePopUpMenu;
			BMenuField*			fModeField;
			BTextControl*		fAddressField;
			BTextControl*		fNetmaskField;
			BTextControl*		fGatewayField;
};


#endif /* INTERFACE_ADDRESS_VIEW_H */

