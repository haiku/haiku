/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
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


static const float kPopUpIndicatorWidth = 13.0f;


#if __GNUC__ == 2


// This is kept only for binary compatibility with BeOS R5. This class was
// used in their BMenuField implementation and we may come across some archived
// BMenuField that needs it.
class _BMCItem_: public BMenuItem {
public:
	_BMCItem_(BMessage* data);
	static BArchivable* Instantiate(BMessage *data);
};


_BMCItem_::_BMCItem_(BMessage* data)
	: BMenuItem(data)
{

}


/*static*/ BArchivable*
_BMCItem_::Instantiate(BMessage *data) {
	if (validate_instantiation(data, "_BMCItem_"))
		return new _BMCItem_(data);

	return NULL;
}


#endif


//	#pragma mark - _BMCFilter_


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


//	#pragma mark - _BMCMenuBar_


_BMCMenuBar_::_BMCMenuBar_(BRect frame, bool fixedSize, BMenuField* menuField)
	:
	BMenuBar(frame, "_mc_mb_", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_ITEMS_IN_ROW,
		!fixedSize),
	fMenuField(menuField),
	fFixedSize(fixedSize),
	fShowPopUpMarker(true)
{
	_Init();
}


_BMCMenuBar_::_BMCMenuBar_(BMenuField* menuField)
	:
	BMenuBar("_mc_mb_", B_ITEMS_IN_ROW),
	fMenuField(menuField),
	fFixedSize(true),
	fShowPopUpMarker(true)
{
	_Init();
}


_BMCMenuBar_::_BMCMenuBar_(BMessage* data)
	:
	BMenuBar(data),
	fMenuField(NULL),
	fFixedSize(true),
	fShowPopUpMarker(true)
{
	SetFlags(Flags() | B_FRAME_EVENTS);

	bool resizeToFit;
	if (data->FindBool("_rsize_to_fit", &resizeToFit) == B_OK)
		fFixedSize = !resizeToFit;
}


_BMCMenuBar_::~_BMCMenuBar_()
{
}


//	#pragma mark - _BMCMenuBar_ public methods


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

	if (Parent() != NULL) {
		color_which which = Parent()->LowUIColor();
		if (which == B_NO_COLOR)
			SetLowColor(Parent()->LowColor());
		else
			SetLowUIColor(which);

	} else
		SetLowUIColor(B_MENU_BACKGROUND_COLOR);

	fPreviousWidth = Bounds().Width();
}


void
_BMCMenuBar_::Draw(BRect updateRect)
{
	if (fFixedSize) {
		// Set the width of the menu bar because the menu bar bounds may have
		// been expanded by the selected menu item.
		ResizeTo(fMenuField->_MenuBarWidth(), Bounds().Height());
	} else {
		// For compatability with BeOS R5:
		//  - Set to the minimum of the menu bar width set by the menu frame
		//    and the selected menu item width.
		//  - Set the height to the preferred height ignoring the height of the
		//    menu field.
		float height;
		BMenuBar::GetPreferredSize(NULL, &height);
		ResizeTo(std::min(Bounds().Width(), fMenuField->_MenuBarWidth()),
			height);
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
	// we need to take care of cleaning up the parent menu field
	float diff = width - fPreviousWidth;
	fPreviousWidth = width;

	if (Window() != NULL && diff != 0) {
		BRect dirty(fMenuField->Bounds());
		if (diff > 0) {
			// clean up the dirty right border of
			// the menu field when enlarging
			dirty.right = Frame().right + kVMargin;
			dirty.left = dirty.right - diff - kVMargin * 2;
			fMenuField->Invalidate(dirty);
		} else if (diff < 0) {
			// clean up the dirty right line of
			// the menu field when shrinking
			dirty.left = Frame().right - kVMargin;
			fMenuField->Invalidate(dirty);
		}
	}

	BMenuBar::FrameResized(width, height);
}


void
_BMCMenuBar_::MakeFocus(bool focused)
{
	if (IsFocus() == focused)
		return;

	BMenuBar::MakeFocus(focused);
}


void
_BMCMenuBar_::MessageReceived(BMessage* message)
{
	switch (message->what) {
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
			BMenuBar::MessageReceived(message);
			break;
	}
}


void
_BMCMenuBar_::SetMaxContentWidth(float width)
{
	float left;
	float right;
	GetItemMargins(&left, NULL, &right, NULL);

	BMenuBar::SetMaxContentWidth(width - (left + right));
}


void
_BMCMenuBar_::SetEnabled(bool enabled)
{
	fMenuField->SetEnabled(enabled);

	BMenuBar::SetEnabled(enabled);
}


BSize
_BMCMenuBar_::MinSize()
{
	BSize size;
	BMenuBar::GetPreferredSize(&size.width, &size.height);
	if (fShowPopUpMarker) {
		// account for popup indicator + a few pixels margin
		size.width += kPopUpIndicatorWidth;
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


//	#pragma mark - _BMCMenuBar_ private methods


void
_BMCMenuBar_::_Init()
{
	SetFlags(Flags() | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
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
		right + fShowPopUpMarker ? kPopUpIndicatorWidth : 0, bottom);
}
