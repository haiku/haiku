/*
 * Copyright 2005-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef _PORT_LINK_H
#define _PORT_LINK_H


#include <ServerLink.h>


namespace BPrivate {


/*! Provide a new LinkSender and LinkReceiver for the ServerLink. */
class PortLink : public ServerLink {
public:
								PortLink(port_id sender = -1,
									port_id receiver = -1);
	virtual						~PortLink();
};


/*! Use existing LinkSender and LinkReceiver in ServerLink. */
class PortLinkRef : public ServerLink {
public:
		PortLinkRef(LinkSender* send, LinkReceiver* receive);
};


}	// namespace BPrivate

#endif	/* _PORT_LINK_H */
