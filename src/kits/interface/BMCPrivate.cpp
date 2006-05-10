/*
 * Copyright 2001-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <stdio.h>

#include <BMCPrivate.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Region.h>
#include <Window.h>

/*
_BMCFilter_::_BMCFilter_(BMenuField *menuField, uint32)
{
}


_BMCFilter_::~_BMCFilter_()
{
}


filter_result
_BMCFilter_::Filter(BMessage *message, BHandler **handler)
{
}


_BMCFilter_ &
_BMCFilter_::operator=(const _BMCFilter_ &)
{
	return *this;
}*/


_BMCMenuBar_::_BMCMenuBar_(BRect frame, bool fixed_size, BMenuField *menuField)
	:	BMenuBar(frame, "_mc_mb_", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_ITEMS_IN_ROW,
			!fixed_size)
{
	SetFlags(Flags() | B_FRAME_EVENTS);
	SetBorder(B_BORDER_CONTENTS);

	fMenuField = menuField;
	fFixedSize = fixed_size;
	fRunner = NULL;

	float left, top, right, bottom;

	GetItemMargins(&left, &top, &right, &bottom);
	// give a bit more space to draw the small thumb
	left -= 1;
	right += 3;
	SetItemMargins(left, top, right, bottom);

	SetMaxContentWidth(frame.Width() - (left + right));
}


_BMCMenuBar_::_BMCMenuBar_(BMessage *data)
	:	BMenuBar(data)
{
	SetFlags(Flags() | B_FRAME_EVENTS);

	bool rsize_to_fit;

	if (data->FindBool("_rsize_to_fit", &rsize_to_fit) == B_OK)
		fFixedSize = !rsize_to_fit;
	else
		fFixedSize = true;
}


_BMCMenuBar_::~_BMCMenuBar_()
{
}


BArchivable *
_BMCMenuBar_::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "_BMCMenuBar_"))
		return new _BMCMenuBar_(data);

	return NULL;
}


void
_BMCMenuBar_::AttachedToWindow()
{
	fMenuField = static_cast<BMenuField *>(Parent());

	BMenuBar *menuBar = Window()->KeyMenuBar();
	BMenuBar::AttachedToWindow();
	Window()->SetKeyMenuBar(menuBar);
}


void
_BMCMenuBar_::Draw(BRect updateRect)
{
	// draw the right side with the popup marker

	// prevent the original BMenuBar's Draw from
	// drawing in those parts
	BRect bounds(Bounds());
	bounds.right -= 10.0;
	bounds.bottom -= 1.0;

	BRegion clipping(bounds);
	ConstrainClippingRegion(&clipping);

	BMenuBar::Draw(updateRect);

	// restore clipping
	ConstrainClippingRegion(NULL);
	bounds.right += 10.0;
	bounds.bottom += 1.0;

	// prepare some colors
	rgb_color normalNoTint = ui_color(B_MENU_BACKGROUND_COLOR);
	rgb_color noTint = tint_color(normalNoTint, 0.74);
	rgb_color darken4;
	rgb_color normalDarken4;
	rgb_color darken1;
	rgb_color lighten1;
	rgb_color lighten2;

	if (IsEnabled()) {
		darken4 = tint_color(noTint, B_DARKEN_4_TINT);
		normalDarken4 = tint_color(normalNoTint, B_DARKEN_4_TINT);
		darken1 = tint_color(noTint, B_DARKEN_1_TINT);
		lighten1 = tint_color(noTint, B_LIGHTEN_1_TINT);
		lighten2 = tint_color(noTint, B_LIGHTEN_2_TINT);
	} else {
		darken4 = tint_color(noTint, B_DARKEN_2_TINT);
		normalDarken4 = tint_color(normalNoTint, B_DARKEN_2_TINT);
		darken1 = tint_color(noTint, (B_NO_TINT + B_DARKEN_1_TINT) / 2.0);
		lighten1 = tint_color(noTint, (B_NO_TINT + B_LIGHTEN_1_TINT) / 2.0);
		lighten2 = tint_color(noTint, B_LIGHTEN_1_TINT);
	}

	BRect r(bounds);
	r.left = r.right - 10.0;	

	BeginLineArray(6);
		// bottom below item text, darker then BMenuBar
		// would normaly draw it
		AddLine(BPoint(bounds.left, r.bottom),
				BPoint(r.left - 1.0, r.bottom), normalDarken4);

		// bottom below popup marker
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.right, r.bottom), darken4);
		// right of popup marker
		AddLine(BPoint(r.right, r.bottom - 1),
				BPoint(r.right, r.top), darken4);
		// top above popup marker
		AddLine(BPoint(r.left, r.top),
				BPoint(r.right - 2, r.top), lighten2);

		r.top += 1;
		r.bottom -= 1;
		r.right -= 1;

		// bottom below popup marker
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.right, r.bottom), darken1);
		// right of popup marker
		AddLine(BPoint(r.right, r.bottom - 1),
				BPoint(r.right, r.top), darken1);
	EndLineArray();

	r.bottom -= 1;
	r.right -= 1;
	SetHighColor(noTint);
	FillRect(r);

	// popup marker
	BPoint center(roundf((r.left + r.right) / 2.0),
				  roundf((r.top + r.bottom) / 2.0));
	BPoint triangle[3];
	triangle[0] = center + BPoint(-2.5, -0.5);
	triangle[1] = center + BPoint(2.5, -0.5);
	triangle[2] = center + BPoint(0.0, 2.0);

	uint32 flags = Flags();
	SetFlags(flags | B_SUBPIXEL_PRECISE);

	SetHighColor(normalDarken4);
	FillTriangle(triangle[0], triangle[1], triangle[2]);

	SetFlags(flags);
}


void
_BMCMenuBar_::FrameResized(float width, float height)
{
	// we need to take care of resizing and cleaning up
	// the parent menu field
	float diff = Frame().right - fMenuField->Bounds().right;
	if (Window()) {
		if (diff > 0) {
			// clean up the dirty right border of
			// the menu field when enlarging
			BRect dirty(fMenuField->Bounds());
			dirty.left = dirty.right - 2;
			fMenuField->Invalidate(dirty);
			
			// clean up the arrow part
			dirty = Bounds();
			dirty.right -= diff;
			dirty.left = dirty.right - 12;
			Invalidate(dirty);

		} else if (diff < 0) {
			// clean up the dirty right line of
			// the menu field when shrinking
			BRect dirty(fMenuField->Bounds());
			dirty.left = dirty.right + diff + 1;
			dirty.right = dirty.left + 1;
			fMenuField->Invalidate(dirty);
			
			// clean up the arrow part
			dirty = Bounds();
			dirty.left = dirty.right - 12;
			Invalidate(dirty);
		}
	}

	// we have been shrinked or enlarged and need to take
	// of the size of the parent menu field as well
	// NOTE: no worries about follow mode, we follow left and top
	fMenuField->ResizeBy(diff + 2, 0.0);
	BMenuBar::FrameResized(width, height);
}


void
_BMCMenuBar_::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'TICK':
		{
			BMenuItem *item = ItemAt(0);

			if (item && item->Submenu() &&  item->Submenu()->Window()) {
				BMessage message(B_KEY_DOWN);
	
				message.AddInt8("byte", B_ESCAPE);
				message.AddInt8("key", B_ESCAPE);
				message.AddInt32("modifiers", 0);
				message.AddInt8("raw_char", B_ESCAPE);
	
				Window()->PostMessage(&message, this, NULL);
			}
		}
		// fall through	
		default:
			BMenuBar::MessageReceived(msg);
			break;
	}
}


void
_BMCMenuBar_::MakeFocus(bool focused)
{
	if (IsFocus() == focused)
		return;

	BMenuBar::MakeFocus(focused);

	if (focused) {
		BMessage message('TICK');
		//fRunner = new BMessageRunner(BMessenger(this, NULL, NULL), &message,
		//	50000, -1);
	} else if (fRunner) {
		//delete fRunner;
		fRunner = NULL;
	}

	if (focused)
		return;

	fMenuField->fSelected = false;
	fMenuField->fTransition = true;

	BRect bounds(fMenuField->Bounds());

	fMenuField->Invalidate(BRect(bounds.left, bounds.top, fMenuField->fDivider,
		bounds.bottom));
}


void
_BMCMenuBar_::MouseDown(BPoint where)
{
	// Don't show the associated menu if it's disabled
	if (fMenuField->IsEnabled() && SubmenuAt(0)->IsEnabled())
		BMenuBar::MouseDown(where);
}


_BMCMenuBar_ 
&_BMCMenuBar_::operator=(const _BMCMenuBar_ &)
{
	return *this;
}
