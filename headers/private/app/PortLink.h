/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _PORT_LINK_H
#define _PORT_LINK_H


#include <ServerLink.h>


namespace BPrivate {

class PortLink : public ServerLink {
	public:
		PortLink(port_id sender = -1, port_id receiver = -1);
		virtual ~PortLink();

		void SetTo(port_id sender, port_id receiver);
};

}	// namespace BPrivate

#endif	/* _PORT_LINK_H */
