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
#include <NetworkSettings.h>
#include <NetworkSettingsAddOn.h>


class BButton;
class BMenuField;
class BMessage;
class BPopUpMenu;
class BRect;
class BTextControl;


using namespace BNetworkKit;


class InterfaceAddressView : public BGroupView {
public:
								InterfaceAddressView(int family,
									const char* interface,
									BNetworkSettings& settings);
	virtual						~InterfaceAddressView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			status_t			Revert();
			status_t			Save();

			void				ConfigurationUpdated(const BMessage& message);

private:
			void				_EnableFields(bool enable);
			void				_UpdateFields();
			void				_SetModeField(uint32 mode);
			void				_UpdateSettings();
			uint32				_Mode() const;

			void				_ConfigureAddress(
									BNetworkInterfaceAddressSettings& address);
			void				_SetAddress(BNetworkAddress& address,
									const char* text);

private:
			int					fFamily;
			BNetworkInterface	fInterface;
			BNetworkSettings&	fSettings;
			uint32				fLastMode;

			BMessage			fOriginalInterface;
			BMessage			fInterfaceSettings;

			BPopUpMenu*			fModePopUpMenu;
			BMenuField*			fModeField;
			BTextControl*		fAddressField;
			BTextControl*		fNetmaskField;
			BTextControl*		fGatewayField;
			BButton*			fApplyButton;
};


#endif // INTERFACE_ADDRESS_VIEW_H
