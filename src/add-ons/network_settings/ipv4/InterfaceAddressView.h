/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef INTERFACE_ADDRESS_VIEW_H
#define INTERFACE_ADDRESS_VIEW_H


#include <GroupView.h>
#include <NetworkInterface.h>


class BMenuField;
class BMessage;
class BPopUpMenu;
class BRect;
class BTextControl;


class InterfaceAddressView : public BGroupView {
public:
								InterfaceAddressView(int family,
									const char* interface);
	virtual						~InterfaceAddressView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			status_t			Revert();
			status_t			Save();

private:
			void				_EnableFields(bool enable);
			void				_ShowFields(bool show);

			int					fFamily;
			BNetworkInterface	fInterface;

			BPopUpMenu*			fModePopUpMenu;
			BMenuField*			fModeField;
			BTextControl*		fAddressField;
			BTextControl*		fNetmaskField;
			BTextControl*		fGatewayField;
};


#endif // INTERFACE_ADDRESS_VIEW_H
