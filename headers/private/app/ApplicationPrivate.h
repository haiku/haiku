/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _APPLICATION_PRIVATE_H
#define _APPLICATION_PRIVATE_H

#include <Application.h>

class BApplication::Private {
public:
	static inline status_t InitGUIContext()
		{ return be_app->_InitGUIContext(); }

	static inline BPrivate::PortLink *ServerLink()
		{ return be_app->fServerLink; }
};

#endif	// _APPLICATION_PRIVATE_H
