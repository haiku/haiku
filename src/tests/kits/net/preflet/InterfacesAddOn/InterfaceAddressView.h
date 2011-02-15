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

#include <Screen.h>
#include <View.h>
#include <TextControl.h>


class InterfaceAddressView : public BView {
public:
								InterfaceAddressView(BRect frame,
									const char* name, int family,
									NetworkSettings* settings);
	virtual						~InterfaceAddressView();
			status_t			RevertFields();

private:
			NetworkSettings*	fSettings;
			int					fFamily;

			BTextControl*		fAddressField;
			BTextControl*		fNetmaskField;
			BTextControl*		fGatewayField;
};


#endif /* INTERFACE_ADDRESS_VIEW_H */

