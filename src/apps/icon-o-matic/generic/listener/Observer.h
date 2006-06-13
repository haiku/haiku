/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef OBSERVER_H
#define OBSERVER_H

#include <SupportDefs.h>

class Observable;

class Observer {
 public:
								Observer();
	virtual						~Observer();

	virtual	void				ObjectChanged(const Observable* object) = 0;
};

#endif // OBSERVER_H
