/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CanvasView.h"

#include "CommandStack.h"

CanvasView::CanvasView(BRect frame)
	: StateView(frame, "canvas view", B_FOLLOW_ALL, B_WILL_DRAW)
{
#if __HAIKU__
	SetFlags(Flags() | B_SUBPIXEL_PRECISE);
#endif // __HAIKU__
}


CanvasView::~CanvasView()
{
}


bool
CanvasView::_HandleKeyDown(uint32 key, uint32 modifiers)
{
	switch (key) {
		case 'z':
		case 'y':
			if (modifiers & B_SHIFT_KEY)
				CommandStack()->Redo();
			else
				CommandStack()->Undo();
			break;

		default:
			return StateView::HandleKeyDown(key, modifiers);
	}

	return true;
}



