// HWInterface.cpp

#include "ServerCursor.h"

#include "HWInterface.h"

// constructor
HWInterface::HWInterface()
	: BLocker("hw interface lock"),
	  fCursor(NULL),
	  fCursorVisible(true),
	  fCursorLocation(0.5, 0.5)
{
}

// destructor
HWInterface::~HWInterface()
{
	delete fCursor;
}

// SetCursor
void
HWInterface::SetCursor(ServerCursor* cursor)
{
	if (Lock()) {
		if (fCursor != cursor) {
			BRect oldFrame = _CursorFrame();
			delete fCursor;
			fCursor = cursor;
			Invalidate(oldFrame);
			Invalidate(_CursorFrame());
		}
		Unlock();
	}
}

// SetCursorVisible
void
HWInterface::SetCursorVisible(bool visible)
{
	if (Lock()) {
		if (fCursorVisible != visible) {
			fCursorVisible = visible;
			Invalidate(_CursorFrame());
		}
		Unlock();
	}
}

// IsCursorVisible
bool
HWInterface::IsCursorVisible()
{
	bool visible = true;
	if (Lock()) {
		visible = fCursorVisible;
		Unlock();
	}
	return visible;
}

// MoveCursorTo
void
HWInterface::MoveCursorTo(const float& x, const float& y)
{
	if (Lock()) {
		BPoint p(x, y);
		if (p != fCursorLocation) {
			BRect oldFrame = _CursorFrame();
			fCursorLocation = p;
			Invalidate(oldFrame);
			Invalidate(_CursorFrame());
		}
		Unlock();
	}
}

// GetCursorPosition
BPoint
HWInterface::GetCursorPosition()
{
	BPoint location;
	if (Lock()) {
		location = fCursorLocation;
		Unlock();
	}
	return location;
}

// _CursorFrame
//
// PRE: the object must be locked
BRect
HWInterface::_CursorFrame() const
{
	BRect frame(0.0, 0.0, -1.0, -1.0);
	if (fCursor && fCursorVisible) {
		frame = fCursor->Bounds();
		frame.OffsetTo(fCursorLocation - fCursor->GetHotSpot());
	}
	return frame;
}



