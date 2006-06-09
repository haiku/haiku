/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _DESKTOP_LINK_H
#define _DESKTOP_LINK_H


#include <PortLink.h>


namespace BPrivate {

class DesktopLink : public PortLink {
	public:
		DesktopLink();
		virtual ~DesktopLink();

		status_t InitCheck() const;

	private:
		port_id	fReplyPort;
};

}	// namespace BPrivate

#endif	/* _DESKTOP_LINK_H */
