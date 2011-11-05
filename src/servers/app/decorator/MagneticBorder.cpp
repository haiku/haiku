/*
 * Copyright 2010-2011, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "MagneticBorder.h"

#include "Decorator.h"
#include "Window.h"
#include "Screen.h"


MagneticBorder::MagneticBorder()
	:
	fLastSnapTime(0)
{

}


bool
MagneticBorder::AlterDeltaForSnap(Window* window, BPoint& delta, bigtime_t now)
{
	BRect frame = window->Frame();
	Decorator* decorator = window->Decorator();
	if (decorator)
		frame = decorator->GetFootprint().Frame();

	return AlterDeltaForSnap(window->Screen(), frame, delta, now);
}


bool
MagneticBorder::AlterDeltaForSnap(const Screen* screen, BRect& frame,
	BPoint& delta, bigtime_t now)
{
	// Alter the delta (which is a proposed offset used while dragging a
	// window) so that the frame of the window 'snaps' to the edges of the
	// screen.

	const bigtime_t kSnappingDuration = 1500000LL;
	const bigtime_t kSnappingPause = 3000000LL;
	const float kSnapDistance = 8.0f;

	if (now - fLastSnapTime > kSnappingDuration
		&& now - fLastSnapTime < kSnappingPause) {
		// Maintain a pause between snapping.
		return false;
	}

	// TODO: Perhaps obtain the usable area (not covered by the Deskbar)?
	BRect screenFrame = screen->Frame();
	BRect originalFrame = frame;
	frame.OffsetBy(delta);

	float leftDist = fabs(frame.left - screenFrame.left);
	float topDist = fabs(frame.top - screenFrame.top);
	float rightDist = fabs(frame.right - screenFrame.right);
	float bottomDist = fabs(frame.bottom - screenFrame.bottom);

	bool snapped = false;
	if (leftDist < kSnapDistance || rightDist < kSnapDistance) {
		snapped = true;
		if (leftDist < rightDist)
			delta.x = screenFrame.left - originalFrame.left;
		else
			delta.x = screenFrame.right - originalFrame.right;
	}

	if (topDist < kSnapDistance || bottomDist < kSnapDistance) {
		snapped = true;
		if (topDist < bottomDist)
			delta.y = screenFrame.top - originalFrame.top;
		else
			delta.y = screenFrame.bottom - originalFrame.bottom;
	}
	if (snapped && now - fLastSnapTime > kSnappingPause)
		fLastSnapTime = now;

	return snapped;
}
