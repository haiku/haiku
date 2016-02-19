/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 */


// BCheckBox displays an on/off control.


#include <CheckBox.h>

#include <algorithm>
#include <new>

#include <Bitmap.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


BCheckBox::BCheckBox(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	BControl(frame, name, label, message, resizingMode, flags),
	fPreferredSize(),
	fOutlined(false),
	fPartialToOff(false)
{
	// Resize to minimum height if needed
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float minHeight = (float)ceil(6.0f + fontHeight.ascent
		+ fontHeight.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


BCheckBox::BCheckBox(const char* name, const char* label, BMessage* message,
	uint32 flags)
	:
	BControl(name, label, message, flags | B_WILL_DRAW | B_NAVIGABLE),
	fPreferredSize(),
	fOutlined(false),
	fPartialToOff(false)
{
}


BCheckBox::BCheckBox(const char* label, BMessage* message)
	:
	BControl(NULL, label, message, B_WILL_DRAW | B_NAVIGABLE),
	fPreferredSize(),
	fOutlined(false),
	fPartialToOff(false)
{
}


BCheckBox::BCheckBox(BMessage* data)
	:
	BControl(data),
	fOutlined(false),
	fPartialToOff(false)
{
}


BCheckBox::~BCheckBox()
{
}


// #pragma mark - Archiving methods


BArchivable*
BCheckBox::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BCheckBox"))
		return new(std::nothrow) BCheckBox(data);

	return NULL;
}


status_t
BCheckBox::Archive(BMessage* data, bool deep) const
{
	return BControl::Archive(data, deep);
}


// #pragma mark - Hook methods


void
BCheckBox::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	uint32 flags = be_control_look->Flags(this);
	if (fOutlined)
		flags |= BControlLook::B_CLICKED;

	BRect checkBoxRect(_CheckBoxFrame());
	BRect rect(checkBoxRect);
	be_control_look->DrawCheckBox(this, rect, updateRect, base, flags);

	BRect labelRect(Bounds());
	labelRect.left = checkBoxRect.right + 1
		+ be_control_look->DefaultLabelSpacing();

	const BBitmap* icon = IconBitmap(
		B_INACTIVE_ICON_BITMAP | (IsEnabled() ? 0 : B_DISABLED_ICON_BITMAP));

	be_control_look->DrawLabel(this, Label(), icon, labelRect, updateRect,
		base, flags);
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
BCheckBox::FrameMoved(BPoint newPosition)
{
	BControl::FrameMoved(newPosition);
}


void
BCheckBox::FrameResized(float newWidth, float newHeight)
{
	BControl::FrameResized(newWidth, newHeight);
}


void
BCheckBox::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
}


void
BCheckBox::MessageReceived(BMessage* message)
{
	BControl::MessageReceived(message);
}


void
BCheckBox::KeyDown(const char* bytes, int32 numBytes)
{
	if (*bytes == B_ENTER || *bytes == B_SPACE) {
		if (!IsEnabled())
			return;

		SetValue(_NextState());
		Invoke();
	} else {
		// skip the BControl implementation
		BView::KeyDown(bytes, numBytes);
	}
}


void
BCheckBox::MouseDown(BPoint where)
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

			GetMouse(&where, &buttons, true);

			bool inside = bounds.Contains(where);
			if (fOutlined != inside) {
				fOutlined = inside;
				Invalidate();
				Window()->UpdateIfNeeded();
			}
		} while (buttons != 0);

		if (fOutlined) {
			fOutlined = false;
			SetValue(_NextState());
			Invoke();
		} else {
			Invalidate();
			Window()->UpdateIfNeeded();
		}
	}
}


void
BCheckBox::MouseUp(BPoint where)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(where);

	if (fOutlined != inside) {
		fOutlined = inside;
		Invalidate();
	}

	if (fOutlined) {
		fOutlined = false;
		SetValue(_NextState());
		Invoke();
	} else {
		Invalidate();
	}

	SetTracking(false);
}


void
BCheckBox::MouseMoved(BPoint where, uint32 code,
	const BMessage* dragMessage)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(where);

	if (fOutlined != inside) {
		fOutlined = inside;
		Invalidate();
	}
}


// #pragma mark -


void
BCheckBox::GetPreferredSize(float* _width, float* _height)
{
	_ValidatePreferredSize();

	if (_width)
		*_width = fPreferredSize.width;

	if (_height)
		*_height = fPreferredSize.height;
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
		_ValidatePreferredSize());
}


BSize
BCheckBox::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		_ValidatePreferredSize());
}


BAlignment
BCheckBox::LayoutAlignment()
{
	return BLayoutUtils::ComposeAlignment(ExplicitAlignment(),
		BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
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
	// We only accept three possible values.
	switch (value) {
		case B_CONTROL_OFF:
		case B_CONTROL_ON:
		case B_CONTROL_PARTIALLY_ON:
			break;
		default:
			value = B_CONTROL_ON;
			break;
	}

	if (value != Value()) {
		BControl::SetValueNoUpdate(value);
		Invalidate(_CheckBoxFrame());
	}
}


status_t
BCheckBox::Invoke(BMessage* message)
{
	return BControl::Invoke(message);
}


BHandler*
BCheckBox::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return BControl::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
BCheckBox::GetSupportedSuites(BMessage* message)
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
		case PERFORM_CODE_SET_ICON:
		{
			perform_data_set_icon* data = (perform_data_set_icon*)_data;
			return BCheckBox::SetIcon(data->icon, data->flags);
		}
	}

	return BControl::Perform(code, _data);
}


status_t
BCheckBox::SetIcon(const BBitmap* icon, uint32 flags)
{
	return BControl::SetIcon(icon, flags | B_CREATE_DISABLED_ICON_BITMAPS);
}


void
BCheckBox::LayoutInvalidated(bool descendants)
{
	// invalidate cached preferred size
	fPreferredSize.Set(B_SIZE_UNSET, B_SIZE_UNSET);
}


bool
BCheckBox::IsPartialStateToOff() const
{
	return fPartialToOff;
}


void
BCheckBox::SetPartialStateToOff(bool partialToOff)
{
	fPartialToOff = partialToOff;
}


// #pragma mark - FBC padding


void BCheckBox::_ReservedCheckBox1() {}
void BCheckBox::_ReservedCheckBox2() {}
void BCheckBox::_ReservedCheckBox3() {}


BRect
BCheckBox::_CheckBoxFrame(const font_height& fontHeight) const
{
	return BRect(0.0f, 2.0f, ceilf(3.0f + fontHeight.ascent),
		ceilf(5.0f + fontHeight.ascent));
}


BRect
BCheckBox::_CheckBoxFrame() const
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	return _CheckBoxFrame(fontHeight);
}


BSize
BCheckBox::_ValidatePreferredSize()
{
	if (!fPreferredSize.IsWidthSet()) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		BRect rect(_CheckBoxFrame(fontHeight));
		float width = rect.right + rect.left;
		float height = rect.bottom + rect.top;

		const BBitmap* icon = IconBitmap(B_INACTIVE_ICON_BITMAP);
		if (icon != NULL) {
			width += be_control_look->DefaultLabelSpacing()
				+ icon->Bounds().Width() + 1;
			height = std::max(height, icon->Bounds().Height());
		}

		if (const char* label = Label()) {
			width += be_control_look->DefaultLabelSpacing()
				+ ceilf(StringWidth(label));
			height = std::max(height,
				ceilf(6.0f + fontHeight.ascent + fontHeight.descent));
		}

		fPreferredSize.Set(width, height);

		ResetLayoutInvalidation();
	}

	return fPreferredSize;
}


int32
BCheckBox::_NextState() const
{
	switch (Value()) {
		case B_CONTROL_OFF:
			return B_CONTROL_ON;
		case B_CONTROL_PARTIALLY_ON:
			return fPartialToOff ? B_CONTROL_OFF : B_CONTROL_ON;
		case B_CONTROL_ON:
		default:
			return B_CONTROL_OFF;
	}
}


BCheckBox &
BCheckBox::operator=(const BCheckBox &)
{
	return *this;
}


extern "C" void
B_IF_GCC_2(InvalidateLayout__9BCheckBoxb, _ZN9BCheckBox16InvalidateLayoutEb)(
	BCheckBox* box, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	box->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}

