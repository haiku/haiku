/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef WINDOW_BEHAVIOUR_H
#define WINDOW_BEHAVIOUR_H


#include <Region.h>

#include "Decorator.h"


class BMessage;
class ClickTarget;


class WindowBehaviour {
public:
								WindowBehaviour();
	virtual						~WindowBehaviour();

	virtual	bool				MouseDown(BMessage* message, BPoint where,
									int32 lastHitRegion, int32& clickCount,
									int32& _hitRegion) = 0;
	virtual	void				MouseUp(BMessage* message, BPoint where) = 0;
	virtual	void				MouseMoved(BMessage *message, BPoint where,
									bool isFake) = 0;

	virtual	void				ModifiersChanged(int32 modifiers);

			bool				IsDragging() const { return fIsDragging; }
			bool				IsResizing() const { return fIsResizing; }

protected:
			bool				fIsResizing : 1;
			bool				fIsDragging : 1;
};


#endif
