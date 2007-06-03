/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef LISTENER_H
#define LISTENER_H

#include <SupportDefs.h>

class Notifier;

class Listener {
 public:
								Listener();
	virtual						~Listener();

	virtual	void				ObjectChanged(const Notifier* object) = 0;
};

#endif // LISTENER_H
