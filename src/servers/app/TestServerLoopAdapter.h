/*
 * Copyright 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Julian Harnath, <julian.harnath@rwth-aachen.de>
 */
#ifndef TEST_SERVER_LOOP_ADAPTER_H
#define TEST_SERVER_LOOP_ADAPTER_H

#include "MessageLooper.h"


class BMessage;
class Desktop;


class TestServerLoopAdapter : public MessageLooper {
public:
								TestServerLoopAdapter(const char* signature,
									const char* looperName, port_id port,
									bool initGui, status_t* outError);
	virtual						~TestServerLoopAdapter();

	// MessageLooper interface
	virtual	port_id				MessagePort() const { return fMessagePort; }
	virtual	bool				Run();

	// BApplication interface
	virtual	void				MessageReceived(BMessage* message) = 0;
	virtual	bool				QuitRequested() { return true; }

private:
	// MessageLooper interface
	virtual	void				_DispatchMessage(int32 code,
									BPrivate::LinkReceiver &link);

	virtual	Desktop*			_FindDesktop(uid_t userID,
									const char* targetScreen) = 0;
			port_id				_CreatePort();



private:
			port_id				fMessagePort;
};


#endif // TEST_SERVER_LOOP_ADAPTER_H
