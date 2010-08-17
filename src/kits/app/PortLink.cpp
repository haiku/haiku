/*
 * Copyright 2005-2010, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include <PortLink.h>


namespace BPrivate {


PortLink::PortLink(port_id send, port_id receive)
{
	fSender = new LinkSender(send);
	fReceiver = new LinkReceiver(receive);
}


PortLink::~PortLink()
{
	delete fReceiver;
	delete fSender;
}


PortLinkRef::PortLinkRef(LinkSender* send, LinkReceiver* receive)
{
	fSender = send;
	fReceiver = receive;
}


}	// namespace BPrivate
