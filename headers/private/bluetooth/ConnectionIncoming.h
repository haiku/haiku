/*
* Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
* Copyright 2021, Haiku, Inc.
* Distributed under the terms of the MIT License.
*
* Authors:
* 		Tri-Edge AI <triedgeai@gmail.com>
*/

#ifndef _CONNECTION_INCOMING_H_
#define _CONNECTION_INCOMING_H_

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <AppKit.h>
#include <SupportKit.h>
#include <InterfaceKit.h>

#include <ConnectionView.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/bdaddrUtils.h>


namespace Bluetooth {

class RemoteDevice;
class ConnectionView;

class ConnectionIncoming : public BWindow {
public:
						ConnectionIncoming(bdaddr_t address);
						ConnectionIncoming(RemoteDevice* rDevice = NULL);
						~ConnectionIncoming();

	virtual void		MessageReceived(BMessage* message);
	virtual bool		QuitRequested();

private:
	ConnectionView*				fView;
};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::ConnectionIncoming;
#endif

#endif /* _CONNECTION_INCOMING_H_ */
