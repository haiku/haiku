/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _PINCODE_REQUEST_WINDOW_H
#define _PINCODE_REQUEST_WINDOW_H


#include <View.h>
#include <Window.h>

#include <bluetooth/bluetooth.h>

class BStringView;
class BButton;
class BTextControls;

namespace Bluetooth {

class RemoteDevice;

class PincodeView : public BView
{
	public:
		/* Constructors & Destructor*/
		PincodeView(BRect rect);
	
		void SetBDaddr(const char* address);
	
		BStringView* 	fMessage;
		BStringView* 	fRemoteInfo;
		BButton* 	fAcceptButton;
		BButton* 	fCancelButton;
		BTextControl* 	fPincodeText;

};

class PincodeWindow : public BWindow
{
	public:
		PincodeWindow(bdaddr_t address, hci_id hid);
		virtual void MessageReceived(BMessage *msg);
		virtual bool QuitRequested();

	private:
		PincodeView*	fView;
		bdaddr_t 		bdaddr;
		bdaddr_t		fBdaddr;
		hci_id			fHid;
};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::PincodeWindow;
#endif

#endif