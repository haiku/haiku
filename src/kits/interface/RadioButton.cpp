/*
 * Copyright 2001-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/*!
	BRadioButton represents a single on/off button. 
	All sibling BRadioButton objects comprise a single
	"multiple choice" control.
*/

#include <RadioButton.h>

#include <ControlLook.h>
#include <Debug.h>
#include <Box.h>
#include <LayoutUtils.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


BRadioButton::BRadioButton(BRect frame, const char* name, const char* label,
		BMessage* message, uint32 resizMask, uint32 flags)
	: BControl(frame, name, label, message, resizMask, flags | B_FRAME_EVENTS),
	  fOutlined(false)
{
	// Resize to minimum height if needed for BeOS compatibility
	float minHeight;
	GetPreferredSize(NULL, &minHeight);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


BRadioButton::BRadioButton(const char* name, const char* label,
		BMessage* message, uint32 flags)
	: BControl(name, label, message, flags | B_FRAME_EVENTS),
	  fOutlined(false)
{
}


BRadioButton::BRadioButton(const char* label, BMessage* message)
	: BControl(NULL, label, message,
		B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS),
	  fOutlined(false)
{
}


BRadioButton::BRadioButton(BMessage* archive)
	: BControl(archive),
	  fOutlined(false)
{
}


BRadioButton::~BRadioButton()
{
}


BArchivable*
BRadioButton::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BRadioButton"))
		return new BRadioButton(archive);

	return NULL;
}


status_t
BRadioButton::Archive(BMessage* archive, bool deep) const
{
	return BControl::Archive(archive, deep);
}


void
BRadioButton::Draw(BRect updateRect)
{
	// its size depends on the text height
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	uint32 flags = be_control_look->Flags(this);
	if (fOutlined)
		flags |= BControlLook::B_CLICKED;

	BRect knobRect(_KnobFrame(fontHeight));
	BRect rect(knobRect);
	be_control_look->DrawRadioButton(this, rect, updateRect, base, flags);

	BRect labelRect(Bounds());
	labelRect.left = knobRect.right
		+ be_control_look->DefaultLabelSpacing();

	be_control_look->DrawLabel(this, Label(), labelRect, updateRect,
		base, flags);
}


void
BRadioButton::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;

	fOutlined = true;

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
		Invalidate();
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	} else {
		_Redraw();

		BRect bounds = Bounds();
		uint32 buttons;

		do {
			snooze(40000);

			GetMouse(&point, &buttons, true);

			bool inside = bounds.Contains(point);

			if (fOutlined != inside) {
				fOutlined = inside;
				_Redraw();
			}
		} while (buttons != 0);

		if (fOutlined) {
			fOutlined = false;
			_Redraw();
			SetValue(B_CONTROL_ON);
			Invoke();
		} else {
			_Redraw();
		}
	}
}


void
BRadioButton::AttachedToWindow()
{
	BControl::AttachedToWindow();
}


void
BRadioButton::KeyDown(const char* bytes, int32 numBytes)
{
	// TODO: Add selecting the next button functionality (navigating radio
	// buttons with the cursor keys)!

	switch (bytes[0]) {
		case B_RETURN:
			// override B_RETURN, which BControl would use to toggle the value
			// but we don't allow setting the control to "off", only "on"
		case B_SPACE: {
			if (IsEnabled() && !Value()) {
				SetValue(B_CONTROL_ON);
				Invoke();
			}

			break;
		}
		default:
			BControl::KeyDown(bytes, numBytes);
	}
}


void
BRadioButton::SetValue(int32 value)
{
	if (value != Value()) {
		BControl::SetValueNoUpdate(value);
		Invalidate(_KnobFrame());
	}

	if (!value)
		return;

	BView* parent = Parent();
	BView* child = NULL;

	if (parent) {
		// If the parent is a BBox, the group parent is the parent of the BBox
		BBox* box = dynamic_cast<BBox*>(parent);

		if (box && box->LabelView() == this)
			parent = box->Parent();

		if (parent) {
			BBox* box = dynamic_cast<BBox*>(parent);

			// If the parent is a BBox, skip the label if there is one
			if (box && box->LabelView())
				child = parent->ChildAt(1);
			else
				child = parent->ChildAt(0);
		} else
			child = Window()->ChildAt(0);
	} else if (Window())
		child = Window()->ChildAt(0);

	while (child) {
		BRadioButton* radio = dynamic_cast<BRadioButton*>(child);

		if (radio && (radio != this))
			radio->SetValue(B_CONTROL_OFF);
		else {
			// If the child is a BBox, check if the label is a radiobutton
			BBox* box = dynamic_cast<BBox*>(child);

			if (box && box->LabelView()) {
				radio = dynamic_cast<BRadioButton*>(box->LabelView());

				if (radio && (radio != this))
					radio->SetValue(B_CONTROL_OFF);
			}
		}

		child = child->NextSibling();
	}

	ASSERT(Value() == B_CONTROL_ON);
}


void
BRadioButton::GetPreferredSize(float* _width, float* _height)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	if (_width) {
		BRect rect = _KnobFrame(fontHeight);
		float width = rect.right + floorf(ceilf(fontHeight.ascent
			+ fontHeight.descent) / 2.0);

		if (Label())
			width += StringWidth(Label());
	
		*_width = ceilf(width);
	}

	if (_height)
		*_height = ceilf(fontHeight.ascent + fontHeight.descent) + 6.0;
}


void
BRadioButton::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}


status_t
BRadioButton::Invoke(BMessage* message)
{
	return BControl::Invoke(message);
}


void
BRadioButton::MessageReceived(BMessage* message)
{
	BControl::MessageReceived(message);
}


void
BRadioButton::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
}


void
BRadioButton::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;

	fOutlined = Bounds().Contains(point);
	if (fOutlined) {
		fOutlined = false;
		if (Value() != B_CONTROL_ON) {
			SetValue(B_CONTROL_ON);
			Invoke();
		}
	}
	Invalidate();

	SetTracking(false);
}


void
BRadioButton::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(point);

	if (fOutlined != inside) {
		fOutlined = inside;
		Invalidate();
	}
}


void
BRadioButton::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}


void
BRadioButton::FrameMoved(BPoint newLocation)
{
	BControl::FrameMoved(newLocation);
}


void
BRadioButton::FrameResized(float width, float height)
{
	Invalidate();
	BControl::FrameResized(width, height);
}


BHandler*
BRadioButton::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return BControl::ResolveSpecifier(message, index, specifier, what,
		property);
}


void
BRadioButton::MakeFocus(bool focused)
{
	BControl::MakeFocus(focused);
}


void
BRadioButton::AllAttached()
{
	BControl::AllAttached();
}


void
BRadioButton::AllDetached()
{
	BControl::AllDetached();
}


status_t
BRadioButton::GetSupportedSuites(BMessage* message)
{
	return BControl::GetSupportedSuites(message);
}


status_t
BRadioButton::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BRadioButton::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BRadioButton::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BRadioButton::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BRadioButton::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BRadioButton::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BRadioButton::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BRadioButton::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BRadioButton::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BRadioButton::DoLayout();
			return B_OK;
		}
		case PERFORM_CODE_SET_ICON:
		{
			perform_data_set_icon* data = (perform_data_set_icon*)_data;
			return BRadioButton::SetIcon(data->icon, data->flags);
		}
	}

	return BControl::Perform(code, _data);
}


BSize
BRadioButton::MaxSize()
{
	float width, height;
	GetPreferredSize(&width, &height);

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(width, height));
}


BAlignment
BRadioButton::LayoutAlignment()
{
	return BLayoutUtils::ComposeAlignment(ExplicitAlignment(),
		BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
}


status_t
BRadioButton::SetIcon(const BBitmap* icon, uint32 flags)
{
	return BControl::SetIcon(icon, flags);
}


void BRadioButton::_ReservedRadioButton1() {}
void BRadioButton::_ReservedRadioButton2() {}


BRadioButton&
BRadioButton::operator=(const BRadioButton &)
{
	return *this;
}


BRect
BRadioButton::_KnobFrame() const
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	return _KnobFrame(fontHeight);
}


BRect
BRadioButton::_KnobFrame(const font_height& fontHeight) const
{
	// Same as BCheckBox...
	return BRect(0.0f, 2.0f, ceilf(3.0f + fontHeight.ascent),
		ceilf(5.0f + fontHeight.ascent));
}


void
BRadioButton::_Redraw()
{
	BRect b(Bounds());
	// fill background with ViewColor()
	rgb_color highColor = HighColor();
	SetHighColor(ViewColor());
	FillRect(b);
	// restore previous HighColor()
	SetHighColor(highColor);
	Draw(b);
	Flush();
}
