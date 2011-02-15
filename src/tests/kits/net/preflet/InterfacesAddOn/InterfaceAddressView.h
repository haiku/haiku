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
#include <View.h>


enum {
    AUTOSEL_MSG = 'iato',
    STATICSEL_MSG = 'istc',
    NONESEL_MSG = 'inon'
};


class InterfaceAddressView : public BView {
public:
								InterfaceAddressView(BRect frame,
									const char* name, int family,
									NetworkSettings* settings);
	virtual						~InterfaceAddressView();
	virtual	void				MessageReceived(BMessage* message);
	virtual void				AttachedToWindow();
			status_t			RevertFields();


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

