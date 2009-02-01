/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _CONNECTION_INCOMING_H_
#define _CONNECTION_INCOMING_H_


//----------------------- Global includes  ----------------------
#include <AppKit.h>
#include <SupportKit.h>
#include <InterfaceKit.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

namespace Bluetooth {

class RemoteDevice;

class ConnectionView
		: public BView
{
public:
						ConnectionView(BRect frame, const char *name);
						~ConnectionView();
	virtual void		MessageReceived(BMessage *message);
	void				Draw(BRect update);
	void				Pulse();

private:


};


class ConnectionIncoming : public BWindow
{
public:
						ConnectionIncoming(RemoteDevice* rDevice);
						~ConnectionIncoming();
	virtual void		MessageReceived(BMessage *message);
	virtual bool		QuitRequested();

private:
	ConnectionView*				_ConnectionView;
};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::ConnectionIncoming;
#endif


#endif
