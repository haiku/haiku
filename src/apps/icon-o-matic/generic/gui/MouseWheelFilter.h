/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef MOUSE_WHEEL_FILTER_H
#define MOUSE_WHEEL_FILTER_H

#include <Message.h>
#include <MessageFilter.h>

class MouseWheelTarget {
 public:
		MouseWheelTarget()
		{
		}
	virtual ~MouseWheelTarget()
		{
		}

	virtual bool MouseWheelChanged(float x, float y) = 0;
};

class MouseWheelFilter : public BMessageFilter {
 public:
	MouseWheelFilter(MouseWheelTarget* target)
		: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
		  fTarget(target),
		  fTargetHandler(dynamic_cast<BHandler*>(fTarget))
		{
		}
	virtual	~MouseWheelFilter()
		{
		}
	virtual	filter_result	Filter(BMessage* message, BHandler** target)
		{
			filter_result result = B_DISPATCH_MESSAGE;
			switch (message->what) {
				case B_MOUSE_WHEEL_CHANGED: {
					float x;
					float y;
					if (message->FindFloat("be:wheel_delta_x", &x) >= B_OK
						&& message->FindFloat("be:wheel_delta_y", &y) >= B_OK) {
						if (fTarget->MouseWheelChanged(x, y))
							//result = B_SKIP_MESSAGE;
							*target = fTargetHandler;
					}
					break;
				}
				default:
					break;
			}
			return result;
		}
 private:
 	MouseWheelTarget*		fTarget;
 	BHandler*				fTargetHandler;
};

#endif // MOUSE_WHEEL_FILTER_H

