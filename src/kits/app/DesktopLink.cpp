/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <DesktopLink.h>
#include <ServerProtocol.h>


namespace BPrivate {

DesktopLink::DesktopLink()
	:
	fReplyPort(B_ERROR)
{
	// get the app server port
	port_id port = find_port(SERVER_PORT_NAME);
	if (port < B_OK)
		return;

	// create a reply port
	fReplyPort = create_port(1, "desktop reply");
	if (fReplyPort < B_OK)
		return;

	SetTo(port, fReplyPort);

	// We can't use AppServerLink because be_app may be NULL
	StartMessage(AS_GET_DESKTOP);
	Attach<port_id>(fReplyPort);
	Attach<int32>(getuid());

	int32 code;
	if (FlushWithReply(code) != B_OK || code != B_OK)
		return;

	// we now talk to the desktop
	Read<port_id>(&port);
	SetSenderPort(port);
}


DesktopLink::~DesktopLink()
{
	delete_port(fReplyPort);
}


status_t
DesktopLink::InitCheck() const
{
	return fReplyPort < B_OK ? fReplyPort : B_OK;
}

}	// namespace BPrivate
