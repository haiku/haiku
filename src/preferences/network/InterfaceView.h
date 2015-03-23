/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef INTERFACE_VIEW_H
#define INTERFACE_VIEW_H


#include <GroupView.h>
#include <NetworkInterface.h>


class BButton;
class BMenuField;
class BMessage;
class BStringView;


class InterfaceView : public BGroupView {
public:
								InterfaceView();
	virtual						~InterfaceView();

			void				SetTo(const char* name);

	virtual	void				MessageReceived(BMessage* message);
	virtual void				AttachedToWindow();
	virtual	void				Pulse();

private:
			status_t			_Update(bool updateWirelessNetworks = true);
			void				_EnableFields(bool enabled);

private:
			BNetworkInterface	fInterface;
			int					fPulseCount;

			BStringView*		fStatusField;
			BStringView*		fMacAddressField;
			BStringView*		fLinkSpeedField;
			BStringView*		fLinkTxField;
			BStringView*		fLinkRxField;

			BMenuField*			fNetworkMenuField;

			BButton*			fToggleButton;
			BButton*			fRenegotiateButton;
};


#endif // INTERFACE_HARDWARE_VIEW_H

