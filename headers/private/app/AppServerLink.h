/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef APPSERVERLINK_H
#define APPSERVERLINK_H


#include <PortLink.h>


namespace BPrivate {

class AppServerLink : public ServerLink {
	public:
		AppServerLink();
		~AppServerLink();
};

}	// namespace BPrivate

#endif	/* APPSERVERLINK_H */
