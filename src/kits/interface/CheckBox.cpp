/*
 * Copyright 2001-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/*!	BCheckBox displays an on/off control. */


#include <CheckBox.h>

#include <new>

#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


BCheckBox::BCheckBox(BRect frame, const char *name, const char *label,
		BMessage *message, uint32 resizingMode, uint32 flags)
	:
	BControl(frame, name, label, message, resizingMode, flags),
	fPreferredSize(),
	fOutlined(false)
{
	// Resize to minimum height if needed
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float minHeight = (float)ceil(6.0f + fontHeight.ascent
		+ fontHeight.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


BCheckBox::BCheckBox(const char *name, const char *label, BMessage *message,
	uint32 flags)
	:
	BControl(name, label, message, flags | B_WILL_DRAW | B_NAVIGABLE),
	fPreferredSize(),
	fOutlined(false)
{
}


BCheckBox::BCheckBox(const char *label, BMessage *message)
	:
	BControl(NULL, label, message, B_WILL_DRAW | B_NAVIGABLE),
	fPreferredSize(),
	fOutlined(false)
{
}


BCheckBox::BCheckBox(BMessage *archive)
	:
	BControl(archive),
	fOutlined(false)
{
}


BCheckBox::~BCheckBox()
{
}


// #pragma mark -


BArchivable *
BCheckBox::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BCheckBox"))
		return new(std::nothrow) BCheckBox(archive);

	return NULL;
}


status_t
BCheckBox::Archive(BMessage *archive, bool deep) const
{
	return BControl::Archive(archive, deep);
}


// #pragma mark -


void
BCheckBox::Draw(BRect updateRect)
{
	if (be_control_look) {
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

		uint32 flags = be_control_look->Flags(this);
		if (fOutlined)
			flags |= BControlLook::B_CLICKED;

		BRect checkBoxRect(_CheckBoxFrame());
		BRect rect(checkBoxRect);
		be_control_look->DrawCheckBox(this, rect, updateRect,base, flags);

		BRect labelRect(Bounds());
		labelRect.left = checkBoxRect.right
			+ be_control_look->DefaultLabelSpacing();

		be_control_look->DrawLabel(this, Label(), labelRect, updateRect,
			base, flags);
		return;
	}

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	// If the focus is changing, just redraw the focus indicator
	if (IsFocusChanging()) {
		float x = (float)ceil(10.0f + fontHeight.ascent);
		float y = 5.0f + (float)ceil(fontHeight.ascent);

		if (IsFocus())
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		else
			SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		StrokeLine(BPoint(x, y), BPoint(x + StringWidth(Label()), y));
		return;
	}

	rgb_color noTint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lighten1 = tint_color(noTint, B_LIGHTEN_1_TINT);
	rgb_color lightenMax = tint_color(noTint, B_LIGHTEN_MAX_TINT);
	rgb_color darken1 = tint_color(noTint, B_DARKEN_1_TINT);
	rgb_color darken2 = tint_color(noTint, B_DARKEN_2_TINT);
	rgb_color darken3 = tint_color(noTint, B_DARKEN_3_TINT);
	rgb_color darken4 = tint_color(noTint, B_DARKEN_4_TINT);
	rgb_color darkenmax = tint_color(noTint, B_DARKEN_MAX_TINT);

	BRect rect = _CheckBoxFrame();

	if (IsEnabled()) {
		// Filling
		SetHighColor(lightenMax);
		FillRect(rect);

		// Box
		if (fOutlined) {
			SetHighColor(darken3);
			StrokeRect(rect);

			rect.InsetBy(1, 1);

			BeginLineArray(6);

			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.left, rect.top), darken2);
			AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.right, rect.top), darken2);
			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.right, rect.bottom), darken4);
			AddLine(BPoint(rect.right, rect.bottom),
					BPoint(rect.right, rect.top), darken4);

			EndLineArray();
		} else {
			BeginLineArray(6);

			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.left, rect.top), darken1);
			AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.right, rect.top), darken1);
			rect.InsetBy(1, 1);
			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.left, rect.top), darken4);
			AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.right, rect.top), darken4);
			AddLine(BPoint(rect.left + 1.0f, rect.bottom),
					BPoint(rect.right, rect.bottom), noTint);
			AddLine(BPoint(rect.right, rect.bottom),
					BPoint(rect.right, rect.top + 1.0f), noTint);

			EndLineArray();
		}

		// Checkmark
		if (Value() == B_CONTROL_ON) {
			rect.InsetBy(3, 3);

			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			SetPenSize(2);
			StrokeLine(BPoint(rect.left, rect.top),
					   BPoint(rect.right, rect.bottom));
			StrokeLine(BPoint(rect.left, rect.bottom),
					   BPoint(rect.right, rect.top));
			SetPenSize(1);
		}

		// Label
		SetHighColor(darkenmax);
		DrawString(Label(), BPoint((float)ceil(10.0f + fontHeight.ascent),
			3.0f + (float)ceil(fontHeight.ascent)));

		// Focus
		if (IsFocus()) {
			float x = (float)ceil(10.0f + fontHeight.ascent);
			float y = 5.0f + (float)ceil(fontHeight.ascent);

			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			StrokeLine(BPoint(x, y), BPoint(x + StringWidth(Label()), y));
		}
	} else {
		// Filling
		SetHighColor(lighten1);
		FillRect(rect);

		// Box
		BeginLineArray(6);

		AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), noTint);
		AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), noTint);
		rect.InsetBy(1, 1);
		AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), darken2);
		AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), darken2);
		AddLine(BPoint(rect.left + 1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), darken1);
		AddLine(BPoint(rect.right, rect.bottom),
				BPoint(rect.right, rect.top + 1.0f), darken1);

		EndLineArray();

		// Checkmark
		if (Value() == B_CONTROL_ON) {
			rect.InsetBy(3, 3);

			SetHighColor(tint_color(ui_color(B_KEYBOARD_NAVIGATION_COLOR),
				B_DISABLED_MARK_TINT));
			SetPenSize(2);
			StrokeLine(BPoint(rect.left, rect.top),
					   BPoint(rect.right, rect.bottom));
			StrokeLine(BPoint(rect.left, rect.bottom),
					   BPoint(rect.right, rect.top));
			SetPenSize(1);
		}

		// Label
		SetHighColor(tint_color(noTint, B_DISABLED_LABEL_TINT));
		DrawString(Label(), BPoint((float)ceil(10.0f + fontHeight.ascent),
			3.0f + (float)ceil(fontHeight.ascent)));
	}
}


void
BCheckBox::AttachedToWindow()
{
	BControl::AttachedToWindow();
}


void
BCheckBox::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}


void
BCheckBox::AllAttached()
{
	BControl::AllAttached();
}


void
BCheckBox::AllDetached()
{
	BControl::AllDetached();
}


void
BCheckBox::FrameMoved(BPoint newLocation)
{
	BControl::FrameMoved(newLocation);
}


void
BCheckBox::FrameResized(float width, float height)
{
	BControl::FrameResized(width, height);
}


void
BCheckBox::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
}


// #pragma mark -


void
BCheckBox::MessageReceived(BMessage *message)
{
	BControl::MessageReceived(message);
}


void
BCheckBox::KeyDown(const char *bytes, int32 numBytes)
{
	BControl::KeyDown(bytes, numBytes);
}


void
BCheckBox::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;

	fOutlined = true;

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
		Invalidate();
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	} else {
		BRect bounds = Bounds();
		uint32 buttons;

		Invalidate();
		Window()->UpdateIfNeeded();

		do {
			snooze(40000);

			GetMouse(&point, &buttons, true);

			bool inside = bounds.Contains(point);
			if (fOutlined != inside) {
				fOutlined = inside;
				Invalidate();
				Window()->UpdateIfNeeded();
			}
		} while (buttons != 0);

		if (fOutlined) {
			fOutlined = false;
			SetValue(!Value());
			Invoke();
		} else {
			Invalidate();
			Window()->UpdateIfNeeded();
		}
	}	
}


void
BCheckBox::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(point);

	if (fOutlined != inside) {
		fOutlined = inside;
		Invalidate();
	}

	if (fOutlined) {
		fOutlined = false;
		SetValue(!Value());
		Invoke();
	} else {
		Invalidate();
	}

	SetTracking(false);
}


void
BCheckBox::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(point);

	if (fOutlined != inside) {
		fOutlined = inside;
		Invalidate();
	}
}


// #pragma mark -


void
BCheckBox::GetPreferredSize(float* _width, float* _height)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	if (_width) {
		float width = 12.0f + fontHeight.ascent;

		if (Label())
			width += StringWidth(Label());

		*_width = (float)ceil(width);
	}

	if (_height)
		*_height = (float)ceil(6.0f + fontHeight.ascent + fontHeight.descent);
}


void
BCheckBox::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}


BSize
BCheckBox::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		_ValidatePreferredSize());
}


BSize
BCheckBox::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(B_SIZE_UNLIMITED, _ValidatePreferredSize().height));
}


BSize
BCheckBox::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		_ValidatePreferredSize());
}


// #pragma mark -


void
BCheckBox::MakeFocus(bool focused)
{
	BControl::MakeFocus(focused);
}


void
BCheckBox::SetValue(int32 value)
{
	value = value ? B_CONTROL_ON : B_CONTROL_OFF;
		// we only accept boolean values

	if (value != Value()) {
		BControl::SetValueNoUpdate(value);
		Invalidate(_CheckBoxFrame());
	}
}


status_t
BCheckBox::Invoke(BMessage *message)
{
	return BControl::Invoke(message);
}


BHandler *
BCheckBox::ResolveSpecifier(BMessage *message, int32 index,
	BMessage *specifier, int32 what, const char *property)
{
	return BControl::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
BCheckBox::GetSupportedSuites(BMessage *message)
{
	return BControl::GetSupportedSuites(message);
}


status_t
BCheckBox::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BCheckBox::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BCheckBox::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BCheckBox::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BCheckBox::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BCheckBox::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BCheckBox::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BCheckBox::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BCheckBox::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BCheckBox::DoLayout();
			return B_OK;
		}
	}

	return BControl::Perform(code, _data);
}


void
BCheckBox::LayoutInvalidated(bool descendants)
{
	// invalidate cached preferred size
	fPreferredSize.Set(B_SIZE_UNSET, B_SIZE_UNSET);
}


// #pragma mark - FBC padding


void BCheckBox::_ReservedCheckBox1() {}
void BCheckBox::_ReservedCheckBox2() {}
void BCheckBox::_ReservedCheckBox3() {}


BRect
BCheckBox::_CheckBoxFrame() const
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	return BRect(0.0f, 2.0f, ceilf(3.0f + fontHeight.ascent),
		ceilf(5.0f + fontHeight.ascent));
}


BSize
BCheckBox::_ValidatePreferredSize()
{
	if (!fPreferredSize.IsWidthSet()) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		float width = 12.0f + fontHeight.ascent;

		if (Label())
			width += StringWidth(Label());

		fPreferredSize.width = (float)ceil(width);

		fPreferredSize.height = (float)ceil(6.0f + fontHeight.ascent
			+ fontHeight.descent);

		ResetLayoutInvalidation();
	}

	return fPreferredSize;
}


BCheckBox &
BCheckBox::operator=(const BCheckBox &)
{
	return *this;
}


#if __GNUC__ == 2


extern "C" void
InvalidateLayout__9BCheckBoxb(BCheckBox* box)
{
	box->Perform(PERFORM_CODE_LAYOUT_CHANGED, NULL);
}


#endif
