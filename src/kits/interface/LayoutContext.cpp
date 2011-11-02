/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <LayoutContext.h>


// constructor
BLayoutContextListener::BLayoutContextListener()
{
}

// destructor
BLayoutContextListener::~BLayoutContextListener()
{
}


void BLayoutContextListener::_ReservedLayoutContextListener1() {}
void BLayoutContextListener::_ReservedLayoutContextListener2() {}
void BLayoutContextListener::_ReservedLayoutContextListener3() {}
void BLayoutContextListener::_ReservedLayoutContextListener4() {}
void BLayoutContextListener::_ReservedLayoutContextListener5() {}


// #pragma mark -


// constructor
BLayoutContext::BLayoutContext()
{
}

// destructor
BLayoutContext::~BLayoutContext()
{
	// notify the listeners
	for (int32 i = 0;
		 BLayoutContextListener* listener
		 	= (BLayoutContextListener*)fListeners.ItemAt(i);
		 i++) {
		listener->LayoutContextLeft(this);
	}
}

// AddListener
void
BLayoutContext::AddListener(BLayoutContextListener* listener)
{
	if (listener)
		fListeners.AddItem(listener);
}

// RemoveListener
void
BLayoutContext::RemoveListener(BLayoutContextListener* listener)
{
	if (listener)
		fListeners.RemoveItem(listener);
}
