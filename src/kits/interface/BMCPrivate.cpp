//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		BMCPrivate.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	The BMCPrivate classes are used by BMenuField.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BMCPrivate.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Region.h>
#include <Window.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
_BMCItem_::_BMCItem_(BMenu *menu)
	:	BMenuItem(menu),
		fShowPopUpMarker(true)
{
}
//------------------------------------------------------------------------------
_BMCItem_::_BMCItem_(BMessage *message)
	:	BMenuItem(message),
		fShowPopUpMarker(true)
{
}
//------------------------------------------------------------------------------
_BMCItem_::~_BMCItem_()
{
}
//------------------------------------------------------------------------------
BArchivable *_BMCItem_::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "_BMCItem_"))
		return new _BMCItem_(data);
	else
		return NULL;
}
//------------------------------------------------------------------------------
void _BMCItem_::Draw()
{
	BMenuItem::Draw();

	if (!fShowPopUpMarker)
		return;

	BMenu *menu = Menu();
	BRect rect(menu->Bounds());

	rect.right -= 4;
	rect.bottom -= 5;
	rect.left = rect.right - 4;
	rect.top = rect.bottom - 2;

	if (IsEnabled())
		menu->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_4_TINT));
	else
		menu->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_4_TINT));

	menu->StrokeLine(BPoint(rect.left, rect.top), BPoint(rect.right, rect.top));
	menu->StrokeLine(BPoint(rect.left + 1.0f, rect.top + 1.0f),
		BPoint(rect.right - 1.0f, rect.top + 1.0f));
	menu->StrokeLine(BPoint(rect.left + 2.0f, rect.bottom),
		BPoint(rect.left + 2.0f, rect.bottom));
}
//------------------------------------------------------------------------------
void _BMCItem_::GetContentSize(float *width, float *height)
{
	BMenuItem::GetContentSize(width, height);
}
//------------------------------------------------------------------------------
/*_BMCFilter_::_BMCFilter_(BMenuField *menuField, uint32)
{
}
//------------------------------------------------------------------------------
_BMCFilter_::~_BMCFilter_()
{
}
//------------------------------------------------------------------------------
filter_result _BMCFilter_::Filter(BMessage *message, BHandler **handler)
{
}
//------------------------------------------------------------------------------
_BMCFilter_ &_BMCFilter_::operator=(const _BMCFilter_ &)
{
	return *this;
}*/
//------------------------------------------------------------------------------
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
	SetItemMargins(left, top, right, bottom); // TODO:

	SetMaxContentWidth(frame.Width() - (left + right));
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
_BMCMenuBar_::~_BMCMenuBar_()
{
}
//------------------------------------------------------------------------------
BArchivable *_BMCMenuBar_::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "_BMCMenuBar_"))
		return new _BMCMenuBar_(data);
	else
		return NULL;
}
//------------------------------------------------------------------------------
void _BMCMenuBar_::AttachedToWindow()
{
	fMenuField = (BMenuField*)Parent();

	BMenuBar *menuBar = Window()->KeyMenuBar();
	BMenuBar::AttachedToWindow();
	Window()->SetKeyMenuBar(menuBar);
}
//------------------------------------------------------------------------------
void _BMCMenuBar_::Draw(BRect updateRect)
{
	// draw the right and bottom line here in a darker tint
	BRect bounds(Bounds());
	bounds.right -= 2.0;
	bounds.bottom -= 1.0;

	// prevent the original BMenuBar's Draw from
	// drawing in those parts
	BRegion clipping(bounds);
	ConstrainClippingRegion(&clipping);

	BMenuBar::Draw(updateRect);

	ConstrainClippingRegion(NULL);

	bounds.right += 2.0;
	bounds.bottom += 1.0;

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_4_TINT));
	StrokeLine(BPoint(bounds.left, bounds.bottom),
			   BPoint(bounds.right, bounds.bottom));
	StrokeLine(BPoint(bounds.right, bounds.bottom - 1),
			   BPoint(bounds.right, bounds.top));
	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	StrokeLine(BPoint(bounds.right - 1, bounds.bottom - 1),
			   BPoint(bounds.right - 1, bounds.top + 1));
}
//------------------------------------------------------------------------------
void _BMCMenuBar_::FrameResized(float width, float height)
{
	// TODO:
	BMenuBar::FrameResized(width, height);
}
//------------------------------------------------------------------------------
void _BMCMenuBar_::MessageReceived(BMessage *msg)
{
	if (msg->what == 'TICK')
	{
		BMenuItem *item = ItemAt(0);

		if (item && item->Submenu() &&  item->Submenu()->Window())
		{
			BMessage message(B_KEY_DOWN);

			message.AddInt8("byte", B_ESCAPE);
			message.AddInt8("key", B_ESCAPE);
			message.AddInt32("modifiers", 0);
			message.AddInt8("raw_char", B_ESCAPE);

			Window()->PostMessage(&message, this, NULL);
		}
	}

	BMenuBar::MessageReceived(msg);
}
//------------------------------------------------------------------------------
void _BMCMenuBar_::MakeFocus(bool focused)
{
	if (IsFocus() == focused)
		return;

	BMenuBar::MakeFocus(focused);

	if (focused)
	{
		BMessage message('TICK');
		//fRunner = new BMessageRunner(BMessenger(this, NULL, NULL), &message,
		//	50000, -1);
	}
	else if (fRunner)
	{
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
//------------------------------------------------------------------------------
_BMCMenuBar_ &_BMCMenuBar_::operator=(const _BMCMenuBar_ &)
{
	return *this;
}
//------------------------------------------------------------------------------
