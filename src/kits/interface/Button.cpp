/*
 *	Copyright 2001-2008, Haiku Inc. All rights reserved.
 *  Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Mike Wilber
 *		Stefano Ceccherini (burton666@libero.it)
 *		Ivan Tonizza
 *		Stephan Aßmus <superstippi@gmx.de>
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


static const float kLabelMargin = 3;


BButton::BButton(BRect frame, const char* name, const char* label,
		BMessage* message, uint32 resizingMode, uint32 flags)
	: BControl(frame, name, label, message, resizingMode,
			flags | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(-1, -1),
	fDrawAsDefault(false)
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
	: BControl(name, label, message,
			flags | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(-1, -1),
	fDrawAsDefault(false)
{
}


BButton::BButton(const char* label, BMessage* message)
	: BControl(NULL, label, message,
			B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(-1, -1),
	fDrawAsDefault(false)
{
}


BButton::~BButton()
{
}


BButton::BButton(BMessage* archive)
	: BControl(archive),
	fPreferredSize(-1, -1)
{
	if (archive->FindBool("_default", &fDrawAsDefault) != B_OK)
		fDrawAsDefault = false;
	// NOTE: Default button state will be synchronized with the window
	// in AttachedToWindow().
}


BArchivable*
BButton::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BButton"))
		return new(std::nothrow) BButton(archive);

	return NULL;
}


status_t
BButton::Archive(BMessage* archive, bool deep) const
{
	status_t err = BControl::Archive(archive, deep);

	if (err != B_OK)
		return err;

	if (IsDefault())
		err = archive->AddBool("_default", true);

	return err;
}


void
BButton::Draw(BRect updateRect)
{
	BRect rect(Bounds());
	rgb_color background = LowColor();
	rgb_color base = background;
	uint32 flags = be_control_look->Flags(this);
	if (IsDefault())
		flags |= BControlLook::B_DEFAULT_BUTTON;
	be_control_look->DrawButtonFrame(this, rect, updateRect,
		base, background, flags);
	be_control_look->DrawButtonBackground(this, rect, updateRect,
		base, flags);

	// always leave some room around the label
	rect.InsetBy(kLabelMargin, kLabelMargin);

	const BBitmap* icon = IconBitmap(
		(Value() == B_CONTROL_OFF ? B_OFF_BITMAP : B_ON_BITMAP)
			| (IsEnabled() ? 0 : B_DISABLED_BITMAP));
	be_control_look->DrawLabel(this, Label(), icon, rect, updateRect,
		base, flags, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));
}


void
BButton::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;

	SetValue(B_CONTROL_ON);

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
 		SetTracking(true);
 		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
 	} else {
		BRect bounds = Bounds();
		uint32 buttons;

		do {
			Window()->UpdateIfNeeded();
			snooze(40000);

			GetMouse(&point, &buttons, true);

 			bool inside = bounds.Contains(point);

			if ((Value() == B_CONTROL_ON) != inside)
				SetValue(inside ? B_CONTROL_ON : B_CONTROL_OFF);
		} while (buttons != 0);

		if (Value() == B_CONTROL_ON)
			Invoke();
	}
}


void
BButton::AttachedToWindow()
{
	BControl::AttachedToWindow();
		// low color will now be the parents view color

	if (IsDefault())
		Window()->SetDefaultButton(this);

	SetViewColor(B_TRANSPARENT_COLOR);
}


void
BButton::KeyDown(const char *bytes, int32 numBytes)
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
	BButton *oldDefault = NULL;
	BWindow *window = Window();

	if (window)
		oldDefault = window->DefaultButton();

	if (flag) {
		if (fDrawAsDefault && oldDefault == this)
			return;

		if (!fDrawAsDefault) {
			fDrawAsDefault = true;

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
		if (!fDrawAsDefault)
			return;

		fDrawAsDefault = false;

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
BButton::SetLabel(const char *string)
{
	BControl::SetLabel(string);
}


bool
BButton::IsDefault() const
{
	return fDrawAsDefault;
}


void
BButton::MessageReceived(BMessage *message)
{
	BControl::MessageReceived(message);
}


void
BButton::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
}


void
BButton::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(point);

	if ((Value() == B_CONTROL_ON) != inside)
		SetValue(inside ? B_CONTROL_ON : B_CONTROL_OFF);
}


void
BButton::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;

	if (Bounds().Contains(point))
		Invoke();

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
BButton::GetPreferredSize(float *_width, float *_height)
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
BButton::Invoke(BMessage *message)
{
	Sync();
	snooze(50000);

	status_t err = BControl::Invoke(message);

	SetValue(B_CONTROL_OFF);

	return err;
}


void
BButton::FrameMoved(BPoint newLocation)
{
	BControl::FrameMoved(newLocation);
}


void
BButton::FrameResized(float width, float height)
{
	BControl::FrameResized(width, height);
}


void
BButton::MakeFocus(bool focused)
{
	BControl::MakeFocus(focused);
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


BHandler *
BButton::ResolveSpecifier(BMessage *message, int32 index,
									BMessage *specifier, int32 what,
									const char *property)
{
	return BControl::ResolveSpecifier(message, index, specifier, what, property);
}


status_t
BButton::GetSupportedSuites(BMessage *message)
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
		flags | B_CREATE_ON_BITMAP | B_CREATE_DISABLED_BITMAPS);
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
		float left, top, right, bottom;
		be_control_look->GetInsets(BControlLook::B_BUTTON_FRAME,
			BControlLook::B_BUTTON_BACKGROUND,
			fDrawAsDefault ? BControlLook::B_DEFAULT_BUTTON : 0,
			left, top, right, bottom);

		// width
		float width = left + right + 2 * kLabelMargin - 1;

		const char* label = Label();
		if (label != NULL) {
			width = std::max(width, 20.0f);
			width += (float)ceil(StringWidth(label));
		}

		const BBitmap* icon = IconBitmap(B_OFF_BITMAP);
		if (icon != NULL)
			width += icon->Bounds().Width() + 1;

		if (label != NULL && icon != NULL)
			width += be_control_look->DefaultLabelSpacing();

		// height
		float minHorizontalMargins = top + bottom + 2 * kLabelMargin;
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
BButton::_DrawDefault(BRect bounds, bool enabled)
{
	rgb_color low = LowColor();
	rgb_color focusColor = tint_color(low, enabled ?
		(B_DARKEN_1_TINT + B_DARKEN_2_TINT) / 2
		: (B_NO_TINT + B_DARKEN_1_TINT) / 2);

	SetHighColor(focusColor);

	StrokeRect(bounds, B_SOLID_LOW);
	bounds.InsetBy(1.0, 1.0);
	StrokeRect(bounds);
	bounds.InsetBy(1.0, 1.0);
	StrokeRect(bounds);
	bounds.InsetBy(1.0, 1.0);

	return bounds;
}


void
BButton::_DrawFocusLine(float x, float y, float width, bool visible)
{
	if (visible)
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	else {
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_LIGHTEN_1_TINT));
	}

	// Blue Line
	StrokeLine(BPoint(x, y), BPoint(x + width, y));

	if (visible)
		SetHighColor(255, 255, 255);
	// White Line
	StrokeLine(BPoint(x, y + 1.0f), BPoint(x + width, y + 1.0f));
}


extern "C" void
B_IF_GCC_2(InvalidateLayout__7BButtonb, _ZN7BButton16InvalidateLayoutEb)(
	BView* view, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}

