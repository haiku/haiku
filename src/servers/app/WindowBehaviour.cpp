/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "WindowBehaviour.h"


WindowBehaviour::WindowBehaviour()
	:
	fIsResizing(false),
	fIsDragging(false)
{
}


WindowBehaviour::~WindowBehaviour()
{
}


void
WindowBehaviour::ModifiersChanged(int32 modifiers)
{
}


bool
WindowBehaviour::AlterDeltaForSnap(Window* window, BPoint& delta, bigtime_t now)
{
	return false;
}


/*!	\fn WindowBehaviour::MouseDown()
	\brief Handles a mouse-down message for the window.

	Note that values passed and returned for the hit regions are only meaningful
	to the WindowBehavior subclass, save for the value 0, which is refers to an
	invalid region.

	\param message The message.
	\param where The point where the mouse click happened.
	\param lastHitRegion The hit region of the previous click.
	\param clickCount The number of subsequent, no longer than double-click
		interval separated clicks that have happened so far. This number doesn't
		necessarily match the value in the message. It has already been
		pre-processed in order to avoid erroneous multi-clicks (e.g. when a
		different button has been used or a different window was targeted). This
		is an in-out variable. The method can reset the value to 1, if it
		doesn't want this event handled as a multi-click. Returning a different
		click hit region will also make the caller reset the click count.
	\param _hitRegion Set by the method to a value identifying the clicked
		decorator element. If not explicitly set, an invalid hit region (0) is
		assumed. Only needs to be set when returning \c true.
	\return \c true, if the event was a WindowBehaviour event and should be
		discarded.
*/
