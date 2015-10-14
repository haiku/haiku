/*
 * Copyright 2010-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NOTIFICATION_SERVER_H
#define _NOTIFICATION_SERVER_H


#include <Server.h>


class NotificationWindow;


class NotificationServer : public BServer {
public:
								NotificationServer(status_t& error);
	virtual						~NotificationServer();

	virtual	void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);

	virtual	status_t			GetSupportedSuites(BMessage* msg);
	virtual	BHandler*			ResolveSpecifier(BMessage* msg, int32 index,
									BMessage* spec, int32 form,
									const char* prop);

private:
			NotificationWindow*	fWindow;
};


#endif	// _NOTIFICATION_SERVER_H
