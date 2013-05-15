/*
 * Copyright 2001-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Marc Flerackers, mflerackers@androme.be
 *		John Scipione, jcipione@gmail.com
 */


#include <BMCPrivate.h>

#include <algorithm>

#include <ControlLook.h>
#include <LayoutUtils.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Window.h>


static const float kPopUpIndicatorWidth = 10.0f;
static const float kMarginWidth = 3.0f;


_BMCFilter_::_BMCFilter_(BMenuField* menuField, uint32 what)
	:
	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, what),
	fMenuField(menuField)
{
}


_BMCFilter_::~_BMCFilter_()
{
}


filter_result
_BMCFilter_::Filter(BMessage* message, BHandler** handler)
{
	if (message->what == B_MOUSE_DOWN) {
		if (BView* view = dynamic_cast<BView*>(*handler)) {
			BPoint point;
			message->FindPoint("be:view_where", &point);
			view->ConvertToParent(&point);
			message->ReplacePoint("be:view_where", point);
			*handler = fMenuField;
		}
	}

	return B_DISPATCH_MESSAGE;
}


// #pragma mark -


_BMCMenuBar_::_BMCMenuBar_(BRect frame, bool fixedSize, BMenuField* menuField)
	:
	BMenuBar(frame, "_mc_mb_", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_ITEMS_IN_ROW,
		!fixedSize),
	fMenuField(menuField),
	fFixedSize(fixedSize),
	fRunner(NULL),
	fShowPopUpMarker(true)
{
	_Init(true);
}


_BMCMenuBar_::_BMCMenuBar_(BMenuField* menuField)
	:
	BMenuBar("_mc_mb_", B_ITEMS_IN_ROW),
	fMenuField(menuField),
	fFixedSize(true),
	fRunner(NULL),
	fShowPopUpMarker(true)
{
	_Init(false);
}


_BMCMenuBar_::_BMCMenuBar_(BMessage* data)
	:
	BMenuBar(data),
	fMenuField(NULL),
	fFixedSize(true),
	fRunner(NULL),
	fShowPopUpMarker(true)
{
	SetFlags(Flags() | B_FRAME_EVENTS);

	bool resizeToFit;
	if (data->FindBool("_rsize_to_fit", &resizeToFit) == B_OK)
		fFixedSize = !resizeToFit;
}


_BMCMenuBar_::~_BMCMenuBar_()
{
	delete fRunner;
}


BArchivable*
_BMCMenuBar_::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "_BMCMenuBar_"))
		return new _BMCMenuBar_(data);

	return NULL;
}


void
_BMCMenuBar_::AttachedToWindow()
{
	fMenuField = static_cast<BMenuField*>(Parent());

	// Don't cause the KeyMenuBar to change by being attached
	BMenuBar* menuBar = Window()->KeyMenuBar();
	BMenuBar::AttachedToWindow();
	Window()->SetKeyMenuBar(menuBar);

	if (fFixedSize && (Flags() & B_SUPPORTS_LAYOUT) == 0)
		SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	if (Parent() != NULL)
		SetLowColor(Parent()->LowColor());
	else
		SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));
}


void
_BMCMenuBar_::Draw(BRect updateRect)
{
	// Set the width of the menu bar because the menu bar bounds may have
	// been expanded by the selected menu item.
	if (fFixedSize)
		ResizeTo(fMenuField->_MenuBarWidth(), Bounds().Height());
	else {
		// For compatability with BeOS R5 set the height to the preferred height
		// in auto-size mode ignoring the height of the menu field.
		float height;
		BMenuBar::GetPreferredSize(NULL, &height);
		ResizeTo(std::min(Bounds().Width(), fMenuField->_MenuBarWidth()), height);
	}
	BRect rect(Bounds());
	rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);
	uint32 flags = 0;
	if (!IsEnabled())
		flags |= BControlLook::B_DISABLED;
	if (IsFocus())
		flags |= BControlLook::B_FOCUSED;

	be_control_look->DrawMenuFieldBackground(this, rect,
		updateRect, base, fShowPopUpMarker, flags);

	_DrawItems(updateRect);
}


void
_BMCMenuBar_::FrameResized(float width, float height)
{
	// we need to take care of resizing and cleaning up
	// the parent menu field
	float diff = width - fPreviousWidth;
	fPreviousWidth = width;

	if (Window()) {
		if (diff > 0) {
			// clean up the dirty right border of
			// the menu field when enlarging
			BRect dirty(fMenuField->Bounds());
			dirty.right = Frame().right + 2;
			dirty.left = dirty.left - diff - 4;
			fMenuField->Invalidate(dirty);

			// clean up the arrow part
			dirty = Bounds();
			dirty.left = dirty.right - diff - 12;
			Invalidate(dirty);
		} else if (diff < 0) {
			// clean up the dirty right line of
			// the menu field when shrinking
			BRect dirty(fMenuField->Bounds());
			dirty.left = Frame().right - 2;
			dirty.right = dirty.left - diff + 4;
			fMenuField->Invalidate(dirty);

			// clean up the arrow part
			dirty = Bounds();
			dirty.left = dirty.right - 12;
			Invalidate(dirty);
		}
	}

	BMenuBar::FrameResized(width, height);
}


void
_BMCMenuBar_::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'TICK':
		{
			BMenuItem* item = ItemAt(0);

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


BSize
_BMCMenuBar_::MinSize()
{
	BSize size;
	BMenuBar::GetPreferredSize(&size.width, &size.height);

	if (fShowPopUpMarker) {
		// account for popup indicator + a few pixels margin
		size.width += kPopUpIndicatorWidth + kMarginWidth;
	}

	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
_BMCMenuBar_::MaxSize()
{
	// The maximum width of a normal BMenuBar is unlimited, but we want it
	// limited.
	BSize size;
	BMenuBar::GetPreferredSize(&size.width, &size.height);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


void
_BMCMenuBar_::_Init(bool setMaxContentWidth)
{
	SetFlags(Flags() | B_FRAME_EVENTS);
	SetBorder(B_BORDER_CONTENTS);

	float left, top, right, bottom;
	GetItemMargins(&left, &top, &right, &bottom);

#if 0
	// TODO: Better fix would be to make BMenuItem draw text properly
	// centered
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	top = ceilf((Bounds().Height() - ceilf(fontHeight.ascent)
		- ceilf(fontHeight.descent)) / 2) + 1;
	bottom = top - 1;
#else
	// TODO: Fix content location properly. This is just a quick fix to
	// make the BMenuField label and the super-item of the BMenuBar
	// align vertically.
	top++;
	bottom--;
#endif

	if (be_control_look != NULL)
		left = right = be_control_look->DefaultLabelSpacing();

	SetItemMargins(left, top,
		right + fShowPopUpMarker ? kPopUpIndicatorWidth + kMarginWidth : 0,
		bottom);

	fPreviousWidth = Bounds().Width();

	if (setMaxContentWidth)
		SetMaxContentWidth(fPreviousWidth - (left + right));
}
