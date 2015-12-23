/*
 * Copyright 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 * 		Christian Packmann
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 */
#include "TestServerLoopAdapter.h"

#include "Desktop.h"
#include "ServerConfig.h"
#include "ServerProtocol.h"

#include <PortLink.h>

#include <stdio.h>

//#define DEBUG_SERVER
#ifdef DEBUG_SERVER
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


TestServerLoopAdapter::TestServerLoopAdapter(const char* signature,
	const char*, port_id, bool, status_t* outError)
	:
	MessageLooper("test-app_server"),
	fMessagePort(_CreatePort())
{
	fLink.SetReceiverPort(fMessagePort);
	*outError = B_OK;
}


TestServerLoopAdapter::~TestServerLoopAdapter()
{
}


bool
TestServerLoopAdapter::Run()
{
 	rename_thread(find_thread(NULL), "picasso");
	_message_thread((void*)this);
	return true;
}


void
TestServerLoopAdapter::_DispatchMessage(int32 code,
	BPrivate::LinkReceiver& link)
{
	switch (code) {
		case AS_GET_DESKTOP:
		{
			port_id replyPort = 0;
			link.Read<port_id>(&replyPort);

			int32 userID = -1;
			link.Read<int32>(&userID);

			char* targetScreen = NULL;
			link.ReadString(&targetScreen);

			int32 version = -1;
			link.Read<int32>(&version);

 			BMessage message(AS_GET_DESKTOP);
			message.AddInt32("user", userID);
			message.AddInt32("version", version);
			message.AddString("target", targetScreen);
 			MessageReceived(&message);

 			// AppServer will try to send a reply, we just let that fail
 			// since we can find out the port by getting the desktop instance
 			// ourselves

			Desktop* desktop = _FindDesktop(userID, targetScreen);
			free(targetScreen);

			BPrivate::LinkSender reply(replyPort);
			if (desktop != NULL) {
				reply.StartMessage(B_OK);
				reply.Attach<port_id>(desktop->MessagePort());
			} else
			reply.StartMessage(B_ERROR);

			reply.Flush();

			break;
		}

		case B_QUIT_REQUESTED:
		{
			QuitRequested();
			break;
		}

		default:
			STRACE(("Server::MainLoop received unexpected code %" B_PRId32 " "
				"(offset %" B_PRId32 ")\n", code, code - SERVER_TRUE));
			break;
	}
}


port_id
TestServerLoopAdapter::_CreatePort()
{
	port_id port = create_port(DEFAULT_MONITOR_PORT_SIZE, SERVER_PORT_NAME);
	if (port < B_OK)
		debugger("test-app_server could not create message port");
	return port;
}
