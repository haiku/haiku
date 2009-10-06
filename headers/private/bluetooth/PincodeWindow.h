/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved.	Distributed	under the terms	of the MIT License.
 */
#ifndef	_PINCODE_REQUEST_WINDOW_H
#define	_PINCODE_REQUEST_WINDOW_H


#include <View.h>
#include <Window.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI.h>

class BStringView;
class BButton;
class BTextControl;

namespace Bluetooth	{

class RemoteDevice;

class PincodeWindow : public BWindow
{
public:
							PincodeWindow(bdaddr_t address, hci_id hid);
							PincodeWindow(RemoteDevice* rDevice);
	virtual void			MessageReceived(BMessage *msg);
	virtual bool			QuitRequested();
			void			SetBDaddr(const char* address);

private:
			void			InitUI();
			bdaddr_t		fBdaddr;
			hci_id			fHid;

			BStringView*	fMessage;
			BStringView*	fRemoteInfo;
			BButton*		fAcceptButton;
			BButton*		fCancelButton;
			BTextControl*	fPincodeText;
};

}

#ifndef	_BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::PincodeWindow;
#endif

#endif
