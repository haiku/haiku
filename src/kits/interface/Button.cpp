/*
 *	Copyright 2001-2015 Haiku Inc. All rights reserved.
 *  Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Mike Wilber
 *		Stefano Ceccherini (burton666@libero.it)
 *		Ivan Tonizza
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <Button.h>

#include <algorithm>
#include <new>

#include <Bitmap.h>
#include <ControlLook.h>
#include <Font.h>
#include <LayoutUtils.h>
#include <String.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


enum {
	FLAG_DEFAULT 		= 0x01,
	FLAG_FLAT			= 0x02,
	FLAG_INSIDE			= 0x04,
	FLAG_WAS_PRESSED	= 0x08,
};


BButton::BButton(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	BControl(frame, name, label, message, resizingMode,
		flags | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(-1, -1),
	fFlags(0),
	fBehavior(B_BUTTON_BEHAVIOR),
	fPopUpMessage(NULL)
{
	// Resize to minimum height if needed
	font_height fh;
	GetFontHeight(&fh);
	float minHeight = 12.0f + (float)ceil(fh.ascent + fh.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


BButton::BButton(const char* name, const char* label, BMessage* message,
	uint32 flags)
	:
	BControl(name, label, message,
		flags | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(-1, -1),
	fFlags(0),
	fBehavior(B_BUTTON_BEHAVIOR),
	fPopUpMessage(NULL)
{
}


BButton::BButton(const char* label, BMessage* message)
	:
	BControl(NULL, label, message,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(-1, -1),
	fFlags(0),
	fBehavior(B_BUTTON_BEHAVIOR),
	fPopUpMessage(NULL)
{
}


BButton::~BButton()
{
	SetPopUpMessage(NULL);
}


BButton::BButton(BMessage* data)
	:
	BControl(data),
	fPreferredSize(-1, -1),
	fFlags(0),
	fBehavior(B_BUTTON_BEHAVIOR),
	fPopUpMessage(NULL)
{
	bool isDefault = false;
	if (data->FindBool("_default", &isDefault) == B_OK && isDefault)
		_SetFlag(FLAG_DEFAULT, true);
	// NOTE: Default button state will be synchronized with the window
	// in AttachedToWindow().
}


BArchivable*
BButton::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BButton"))
		return new(std::nothrow) BButton(data);

	return NULL;
}


status_t
BButton::Archive(BMessage* data, bool deep) const
{
	status_t err = BControl::Archive(data, deep);

	if (err != B_OK)
		return err;

	if (IsDefault())
		err = data->AddBool("_default", true);

	return err;
}


void
BButton::Draw(BRect updateRect)
{
	BRect rect(Bounds());
	rgb_color background = ViewColor();
	rgb_color base = LowColor();
	rgb_color textColor = ui_color(B_CONTROL_TEXT_COLOR);

	uint32 flags = be_control_look->Flags(this);
	if (_Flag(FLAG_DEFAULT))
		flags |= BControlLook::B_DEFAULT_BUTTON;
	if (_Flag(FLAG_FLAT) && !IsTracking())
		flags |= BControlLook::B_FLAT;
	if (_Flag(FLAG_INSIDE))
		flags |= BControlLook::B_HOVER;

	be_control_look->DrawButtonFrame(this, rect, updateRect,
		base, background, flags);

	if (fBehavior == B_POP_UP_BEHAVIOR) {
		be_control_look->DrawButtonWithPopUpBackground(this, rect, updateRect,
			base, flags);
	} else {
		be_control_look->DrawButtonBackground(this, rect, updateRect,
			base, flags);
	}

	// always leave some room around the label
	float labelMargin = be_control_look->DefaultLabelSpacing() / 2;
	rect.InsetBy(labelMargin, labelMargin);

	const BBitmap* icon = IconBitmap(
		(Value() == B_CONTROL_OFF
				? B_INACTIVE_ICON_BITMAP : B_ACTIVE_ICON_BITMAP)
			| (IsEnabled() ? 0 : B_DISABLED_ICON_BITMAP));

	be_control_look->DrawLabel(this, Label(), icon, rect, updateRect, base,
		flags, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE), &textColor);
}


void
BButton::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	if (fBehavior == B_POP_UP_BEHAVIOR && _PopUpRect().Contains(where)) {
		InvokeNotify(fPopUpMessage, B_CONTROL_MODIFIED);
		return;
	}

	bool toggleBehavior = fBehavior == B_TOGGLE_BEHAVIOR;

	if (toggleBehavior) {
		bool wasPressed = Value() == B_CONTROL_ON;
		_SetFlag(FLAG_WAS_PRESSED, wasPressed);
		SetValue(wasPressed ? B_CONTROL_OFF : B_CONTROL_ON);
		Invalidate();
	} else
		SetValue(B_CONTROL_ON);

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	} else {
		BRect bounds = Bounds();
		uint32 buttons;
		bool inside = false;

		do {
			Window()->UpdateIfNeeded();
			snooze(40000);

			GetMouse(&where, &buttons, true);
			inside = bounds.Contains(where);

			if (toggleBehavior) {
				bool pressed = inside ^ _Flag(FLAG_WAS_PRESSED);
				SetValue(pressed ? B_CONTROL_ON : B_CONTROL_OFF);
			} else {
				if ((Value() == B_CONTROL_ON) != inside)
					SetValue(inside ? B_CONTROL_ON : B_CONTROL_OFF);
			}
		} while (buttons != 0);

		if (inside) {
			if (toggleBehavior) {
				SetValue(
					_Flag(FLAG_WAS_PRESSED) ? B_CONTROL_OFF : B_CONTROL_ON);
			}

			Invoke();
		} else if (_Flag(FLAG_FLAT))
			Invalidate();
	}
}

void
BButton::AttachedToWindow()
{
	BControl::AttachedToWindow();

	// Tint default control background color to match default panel background.
	SetLowUIColor(B_CONTROL_BACKGROUND_COLOR, 1.115);
	SetHighUIColor(B_CONTROL_TEXT_COLOR);

	if (IsDefault())
		Window()->SetDefaultButton(this);
}


void
BButton::KeyDown(const char* bytes, int32 numBytes)
{
	if (*bytes == B_ENTER || *bytes == B_SPACE) {
		if (!IsEnabled())
			return;

		SetValue(B_CONTROL_ON);

		// make sure the user saw that
		Window()->UpdateIfNeeded();
		snooze(25000);

		Invoke();
	} else
		BControl::KeyDown(bytes, numBytes);
}


void
BButton::MakeDefault(bool flag)
{
	BButton* oldDefault = NULL;
	BWindow* window = Window();

	if (window != NULL)
		oldDefault = window->DefaultButton();

	if (flag) {
		if (_Flag(FLAG_DEFAULT) && oldDefault == this)
			return;

		if (_SetFlag(FLAG_DEFAULT, true)) {
			if ((Flags() & B_SUPPORTS_LAYOUT) != 0)
				InvalidateLayout();
			else {
				ResizeBy(6.0f, 6.0f);
				MoveBy(-3.0f, -3.0f);
			}
		}

		if (window && oldDefault != this)
			window->SetDefaultButton(this);
	} else {
		if (!_SetFlag(FLAG_DEFAULT, false))
			return;

		if ((Flags() & B_SUPPORTS_LAYOUT) != 0)
			InvalidateLayout();
		else {
			ResizeBy(-6.0f, -6.0f);
			MoveBy(3.0f, 3.0f);
		}

		if (window && oldDefault == this)
			window->SetDefaultButton(NULL);
	}
}


void
BButton::SetLabel(const char* label)
{
	BControl::SetLabel(label);
}


bool
BButton::IsDefault() const
{
	return _Flag(FLAG_DEFAULT);
}


bool
BButton::IsFlat() const
{
	return _Flag(FLAG_FLAT);
}


void
BButton::SetFlat(bool flat)
{
	if (_SetFlag(FLAG_FLAT, flat))
		Invalidate();
}


BButton::BBehavior
BButton::Behavior() const
{
	return fBehavior;
}


void
BButton::SetBehavior(BBehavior behavior)
{
	if (behavior != fBehavior) {
		fBehavior = behavior;
		InvalidateLayout();
		Invalidate();
	}
}


BMessage*
BButton::PopUpMessage() const
{
	return fPopUpMessage;
}


void
BButton::SetPopUpMessage(BMessage* message)
{
	delete fPopUpMessage;
	fPopUpMessage = message;
}


void
BButton::MessageReceived(BMessage* message)
{
	BControl::MessageReceived(message);
}


void
BButton::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
}


void
BButton::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	bool inside = (code != B_EXITED_VIEW) && Bounds().Contains(where);
	if (_SetFlag(FLAG_INSIDE, inside))
		Invalidate();

	if (!IsTracking())
		return;

	if (fBehavior == B_TOGGLE_BEHAVIOR) {
		bool pressed = inside ^ _Flag(FLAG_WAS_PRESSED);
		SetValue(pressed ? B_CONTROL_ON : B_CONTROL_OFF);
	} else {
		if ((Value() == B_CONTROL_ON) != inside)
			SetValue(inside ? B_CONTROL_ON : B_CONTROL_OFF);
	}
}


void
BButton::MouseUp(BPoint where)
{
	if (!IsTracking())
		return;

	if (Bounds().Contains(where)) {
		if (fBehavior == B_TOGGLE_BEHAVIOR)
			SetValue(_Flag(FLAG_WAS_PRESSED) ? B_CONTROL_OFF : B_CONTROL_ON);

		Invoke();
	} else if (_Flag(FLAG_FLAT))
		Invalidate();

	SetTracking(false);
}


void
BButton::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}


void
BButton::SetValue(int32 value)
{
	if (value != Value())
		BControl::SetValue(value);
}


void
BButton::GetPreferredSize(float* _width, float* _height)
{
	_ValidatePreferredSize();

	if (_width)
		*_width = fPreferredSize.width;

	if (_height)
		*_height = fPreferredSize.height;
}


void
BButton::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}


status_t
BButton::Invoke(BMessage* message)
{
	Sync();
	snooze(50000);

	status_t err = BControl::Invoke(message);

	if (fBehavior != B_TOGGLE_BEHAVIOR)
		SetValue(B_CONTROL_OFF);

	return err;
}


void
BButton::FrameMoved(BPoint newPosition)
{
	BControl::FrameMoved(newPosition);
}


void
BButton::FrameResized(float newWidth, float newHeight)
{
	BControl::FrameResized(newWidth, newHeight);
}


void
BButton::MakeFocus(bool focus)
{
	BControl::MakeFocus(focus);
}


void
BButton::AllAttached()
{
	BControl::AllAttached();
}


void
BButton::AllDetached()
{
	BControl::AllDetached();
}


BHandler*
BButton::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return BControl::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
BButton::GetSupportedSuites(BMessage* message)
{
	return BControl::GetSupportedSuites(message);
}


status_t
BButton::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BButton::MinSize();
			return B_OK;

		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BButton::MaxSize();
			return B_OK;

		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BButton::PreferredSize();
			return B_OK;

		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BButton::LayoutAlignment();
			return B_OK;

		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BButton::HasHeightForWidth();
			return B_OK;

		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BButton::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}

		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BButton::SetLayout(data->layout);
			return B_OK;
		}

		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BButton::LayoutInvalidated(data->descendants);
			return B_OK;
		}

		case PERFORM_CODE_DO_LAYOUT:
		{
			BButton::DoLayout();
			return B_OK;
		}

		case PERFORM_CODE_SET_ICON:
		{
			perform_data_set_icon* data = (perform_data_set_icon*)_data;
			return BButton::SetIcon(data->icon, data->flags);
		}
	}

	return BControl::Perform(code, _data);
}


BSize
BButton::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		_ValidatePreferredSize());
}


BSize
BButton::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		_ValidatePreferredSize());
}


BSize
BButton::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		_ValidatePreferredSize());
}


status_t
BButton::SetIcon(const BBitmap* icon, uint32 flags)
{
	return BControl::SetIcon(icon,
		flags | B_CREATE_ACTIVE_ICON_BITMAP | B_CREATE_DISABLED_ICON_BITMAPS);
}


void
BButton::LayoutInvalidated(bool descendants)
{
	// invalidate cached preferred size
	fPreferredSize.Set(-1, -1);
}


void BButton::_ReservedButton1() {}
void BButton::_ReservedButton2() {}
void BButton::_ReservedButton3() {}


BButton &
BButton::operator=(const BButton &)
{
	return *this;
}


BSize
BButton::_ValidatePreferredSize()
{
	if (fPreferredSize.width < 0) {
		BControlLook::background_type backgroundType
			= fBehavior == B_POP_UP_BEHAVIOR
				? BControlLook::B_BUTTON_WITH_POP_UP_BACKGROUND
				: BControlLook::B_BUTTON_BACKGROUND;
		float left, top, right, bottom;
		be_control_look->GetInsets(BControlLook::B_BUTTON_FRAME, backgroundType,
			IsDefault() ? BControlLook::B_DEFAULT_BUTTON : 0,
			left, top, right, bottom);

		// width
		float width = left + right + be_control_look->DefaultLabelSpacing() - 1;

		const char* label = Label();
		if (label != NULL) {
			width = std::max(width, 20.0f);
			width += (float)ceil(StringWidth(label));
		}

		const BBitmap* icon = IconBitmap(B_INACTIVE_ICON_BITMAP);
		if (icon != NULL)
			width += icon->Bounds().Width() + 1;

		if (label != NULL && icon != NULL)
			width += be_control_look->DefaultLabelSpacing();

		// height
		float minHorizontalMargins = top + bottom + be_control_look->DefaultLabelSpacing();
		float height = -1;

		if (label != NULL) {
			font_height fontHeight;
			GetFontHeight(&fontHeight);
			float textHeight = fontHeight.ascent + fontHeight.descent;
			height = ceilf(textHeight * 1.8);
			float margins = height - ceilf(textHeight);
			if (margins < minHorizontalMargins)
				height += minHorizontalMargins - margins;
		}

		if (icon != NULL) {
			height = std::max(height,
				icon->Bounds().Height() + minHorizontalMargins);
		}

		// force some minimum width/height values
		width = std::max(width, label != NULL ? 75.0f : 5.0f);
		height = std::max(height, 5.0f);

		fPreferredSize.Set(width, height);

		ResetLayoutInvalidation();
	}

	return fPreferredSize;
}


BRect
BButton::_PopUpRect() const
{
	if (fBehavior != B_POP_UP_BEHAVIOR)
		return BRect();

	float left, top, right, bottom;
	be_control_look->GetInsets(BControlLook::B_BUTTON_FRAME,
		BControlLook::B_BUTTON_WITH_POP_UP_BACKGROUND,
		IsDefault() ? BControlLook::B_DEFAULT_BUTTON : 0,
		left, top, right, bottom);

	BRect rect(Bounds());
	rect.left = rect.right - right + 1;
	return rect;
}


inline bool
BButton::_Flag(uint32 flag) const
{
	return (fFlags & flag) != 0;
}


inline bool
BButton::_SetFlag(uint32 flag, bool set)
{
	if (((fFlags & flag) != 0) == set)
		return false;

	if (set)
		fFlags |= flag;
	else
		fFlags &= ~flag;

	return true;
}


extern "C" void
B_IF_GCC_2(InvalidateLayout__7BButtonb, _ZN7BButton16InvalidateLayoutEb)(
	BView* view, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}
