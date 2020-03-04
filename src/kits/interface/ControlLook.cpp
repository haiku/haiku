/*
 * Copyright 2012-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <ControlLook.h>

#include <binary_compatibility/Interface.h>


namespace BPrivate {


BControlLook::BControlLook()
	:
	fCachedWorkspace(-1)
{
}


BControlLook::~BControlLook()
{
}


float
BControlLook::ComposeSpacing(float spacing)
{
	switch ((int)spacing) {
		case B_USE_DEFAULT_SPACING:
		case B_USE_ITEM_SPACING:
			return be_control_look->DefaultItemSpacing();
		case B_USE_HALF_ITEM_SPACING:
			return ceilf(be_control_look->DefaultItemSpacing() * 0.5f);
		case B_USE_WINDOW_SPACING:
			return be_control_look->DefaultItemSpacing();
		case B_USE_SMALL_SPACING:
			return ceilf(be_control_look->DefaultItemSpacing() * 0.7f);
		case B_USE_BIG_SPACING:
			return ceilf(be_control_look->DefaultItemSpacing() * 1.3f);
	}

	return spacing;
}


void
BControlLook::DrawLabel(BView* view, const char* label, const BBitmap* icon,
	BRect rect, const BRect& updateRect, const rgb_color& base, uint32 flags,
	const rgb_color* textColor)
{
	DrawLabel(view, label, icon, rect, updateRect, base, flags,
		DefaultLabelAlignment(), textColor);
}


void
BControlLook::GetInsets(frame_type frameType, background_type backgroundType,
	uint32 flags, float& _left, float& _top, float& _right, float& _bottom)
{
	GetFrameInsets(frameType, flags, _left, _top, _right, _bottom);

	float left, top, right, bottom;
	GetBackgroundInsets(backgroundType, flags, left, top, right, bottom);

	_left += left;
	_top += top;
	_right += right;
	_bottom += bottom;
}


void
BControlLook::SetBackgroundInfo(const BMessage& backgroundInfo)
{
	fBackgroundInfo = backgroundInfo;
	fCachedWorkspace = -1;
}


extern "C" void
B_IF_GCC_2(_ReservedControlLook1__Q28BPrivate12BControlLook,
		_ZN8BPrivate12BControlLook21_ReservedControlLook1Ev)(
	BControlLook* controlLook, BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	controlLook->DrawTabFrame(view, rect, updateRect, base, flags, borders,
		borderStyle, side);
}


extern "C" void
B_IF_GCC_2(_ReservedControlLook2__Q28BPrivate12BControlLook,
		_ZN8BPrivate12BControlLook21_ReservedControlLook2Ev)(
	BControlLook* controlLook, BView* view, BRect rect,
		const BRect& updateRect, const rgb_color& base, uint32 flags,
		int32 direction, orientation orientation, bool down)
{
	controlLook->DrawScrollBarButton(view, rect, updateRect, base, flags,
		direction, orientation, down);
}


extern "C" void
B_IF_GCC_2(_ReservedControlLook3__Q28BPrivate12BControlLook,
		_ZN8BPrivate12BControlLook21_ReservedControlLook3Ev)(
	BControlLook* controlLook, BView* view, BRect rect,
		const BRect& updateRect, const rgb_color& base, uint32 flags,
		int32 direction, orientation orientation, uint32 knobStyle)
{
	controlLook->DrawScrollBarThumb(view, rect, updateRect, base, flags,
		orientation, knobStyle);
}


extern "C" void
B_IF_GCC_2(_ReservedControlLook4__Q28BPrivate12BControlLook,
		_ZN8BPrivate12BControlLook21_ReservedControlLook4Ev)(
	BControlLook* controlLook, BView* view, BRect rect,
		const BRect& updateRect, const rgb_color& base, uint32 flags,
		orientation orientation)
{
	controlLook->DrawScrollBarBorder(view, rect, updateRect, base, flags,
		orientation);
}


void BControlLook::_ReservedControlLook5() {}
void BControlLook::_ReservedControlLook6() {}
void BControlLook::_ReservedControlLook7() {}
void BControlLook::_ReservedControlLook8() {}
void BControlLook::_ReservedControlLook9() {}
void BControlLook::_ReservedControlLook10() {}


// Initialized in InterfaceDefs.cpp
BControlLook* be_control_look = NULL;

} // namespace BPrivate
