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
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>

class RemoteDevice;

class ConnectionView
		: public BView
{
public:
						ConnectionView(BRect frame, const char *name, uint32 resizeMask, uint32 flags);
						~ConnectionView();
	virtual void		MessageReceived(BMessage *message);
	void				Draw(BRect update);
	void				Pulse();

private:


};


class ConnectionIncoming
		 : public BWindow
{
public:
						ConnectionIncoming(RemoteDevice* rDevice);
						~ConnectionIncoming();
	virtual void		MessageReceived(BMessage *message);
	virtual bool		QuitRequested();

private:
	ConnectionView*				_ConnectionView;
};

#endif
