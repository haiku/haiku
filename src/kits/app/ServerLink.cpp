/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	Class for low-overhead port-based messaging */


#include <ServerLink.h>

#include <stdlib.h>
#include <string.h>
#include <new>

#include <ServerProtocol.h>


namespace BPrivate {


ServerLink::ServerLink()
{
}


ServerLink::~ServerLink()
{
}


void
ServerLink::SetTo(port_id sender, port_id receiver)
{
	fSender->SetPort(sender);
	fReceiver->SetPort(receiver);
}


status_t
ServerLink::FlushWithReply(int32& code)
{
	status_t status = Flush(B_INFINITE_TIMEOUT, true);
	if (status < B_OK)
		return status;

	return GetNextMessage(code);
}


}	// namespace BPrivate
