/*
 * Copyright 2001-2006, Haiku, Inc.
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

#include <Debug.h>
#include <Box.h>
#include <LayoutUtils.h>
#include <Window.h>



BRadioButton::BRadioButton(BRect frame, const char *name, const char *label,
						   BMessage *message, uint32 resizMask, uint32 flags)
	: BControl(frame, name, label, message, resizMask, flags),
	fOutlined(false)
{
	// Resize to minimum height if needed
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float minHeight = ceilf(6.0f + fontHeight.ascent + fontHeight.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


BRadioButton::BRadioButton(const char *name, const char *label,
						   BMessage *message, uint32 flags)
	: BControl(BRect(0, 0, -1, -1), name, label, message, B_FOLLOW_NONE,
		flags | B_SUPPORTS_LAYOUT),
	fOutlined(false)
{
	// Resize to minimum height if needed
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float minHeight = ceilf(6.0f + fontHeight.ascent + fontHeight.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


BRadioButton::BRadioButton(const char *label, BMessage *message)
	: BControl(BRect(0, 0, -1, -1), NULL, label, message, B_FOLLOW_NONE,
		B_WILL_DRAW | B_NAVIGABLE | B_SUPPORTS_LAYOUT),
	fOutlined(false)
{
	// Resize to minimum height if needed
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float minHeight = ceilf(6.0f + fontHeight.ascent + fontHeight.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


BRadioButton::BRadioButton(BMessage *archive)
	: BControl(archive),
	fOutlined(false)
{
}


BRadioButton::~BRadioButton()
{
}


BArchivable*
BRadioButton::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BRadioButton"))
		return new BRadioButton(archive);

	return NULL;
}


status_t
BRadioButton::Archive(BMessage *archive, bool deep) const
{
	return BControl::Archive(archive, deep);
}


void
BRadioButton::Draw(BRect updateRect)
{
	// layout the rect for the dot
	BRect rect = _KnobFrame();

	// its size depends on the text height
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);

	BPoint labelPos(rect.right + floorf(textHeight / 2.0),
		floorf((rect.top + rect.bottom + textHeight) / 2.0 - fontHeight.descent + 0.5) + 1.0);

	// if the focus is changing, just redraw the focus indicator
	if (IsFocusChanging()) {
		if (IsFocus())
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		else
			SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		BPoint underLine = labelPos;
		underLine.y += fontHeight.descent;
		StrokeLine(underLine, underLine + BPoint(StringWidth(Label()), 0.0));

		return;
	}

	// colors
	rgb_color bg = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenmax;
	rgb_color lighten1;
	rgb_color darken1;
	rgb_color darken2;
	rgb_color darken3;
	rgb_color darkenmax;

	rgb_color naviColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	rgb_color knob;
	rgb_color knobDark;
	rgb_color knobLight;

	if (IsEnabled()) {
		lightenmax	= tint_color(bg, B_LIGHTEN_MAX_TINT);
		lighten1	= tint_color(bg, B_LIGHTEN_1_TINT);
		darken1		= tint_color(bg, B_DARKEN_1_TINT);
		darken2		= tint_color(bg, B_DARKEN_2_TINT);
		darken3		= tint_color(bg, B_DARKEN_3_TINT);
		darkenmax	= tint_color(bg, B_DARKEN_MAX_TINT);

		knob		= naviColor;
		knobDark	= tint_color(naviColor, B_DARKEN_3_TINT);
		knobLight	= tint_color(naviColor, 0.15);
	} else {
		lightenmax	= tint_color(bg, B_LIGHTEN_2_TINT);
		lighten1	= bg;
		darken1		= bg;
		darken2		= tint_color(bg, B_DARKEN_1_TINT);
		darken3		= tint_color(bg, B_DARKEN_2_TINT);
		darkenmax	= tint_color(bg, B_DISABLED_LABEL_TINT);

		knob		= tint_color(naviColor, B_LIGHTEN_2_TINT);
		knobDark	= tint_color(naviColor, B_LIGHTEN_1_TINT);
		knobLight	= tint_color(naviColor, (B_LIGHTEN_2_TINT + B_LIGHTEN_MAX_TINT) / 2.0);
	}

	// dot
	if (Value() == B_CONTROL_ON) {
		// full
		SetHighColor(knobDark);
		FillEllipse(rect);

		SetHighColor(knob);
		FillEllipse(BRect(rect.left + 2, rect.top + 2, rect.right - 3, rect.bottom - 3));

		SetHighColor(knobLight);
		FillEllipse(BRect(rect.left + 3, rect.top + 3, rect.right - 5, rect.bottom - 5));
	} else {
		// empty
		SetHighColor(lightenmax);
		FillEllipse(rect);
	}

	rect.InsetBy(-1.0, -1.0);

	// outer circle
	if (fOutlined) {
		// indicating "about to change value"
		SetHighColor(darken3);
		StrokeEllipse(rect);
	} else {
		SetHighColor(darken1);
		StrokeArc(rect, 45.0f, 180.0f);
		SetHighColor(lightenmax);
		StrokeArc(rect, 45.0f, -180.0f);
	}

	rect.InsetBy(1, 1);

	// inner circle
	SetHighColor(darken3);
	StrokeArc(rect, 45.0f, 180.0f);
	SetHighColor(bg);
	StrokeArc(rect, 45.0f, -180.0f);

	// for faster font rendering, we can restore B_OP_COPY
	SetDrawingMode(B_OP_COPY);

	// label
	SetHighColor(darkenmax);
	DrawString(Label(), labelPos);

	// underline label if focused
	if (IsFocus()) {
		SetHighColor(naviColor);
		BPoint underLine = labelPos;
		underLine.y += fontHeight.descent;
		StrokeLine(underLine, underLine + BPoint(StringWidth(Label()), 0.0));
	}
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
BRadioButton::KeyDown(const char *bytes, int32 numBytes)
{
	// TODO add select_next_button functionality

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

	BView *parent = Parent();
	BView *child = NULL;

	if (parent) {
		// If the parent is a BBox, the group parent is the parent of the BBox
		BBox *box = dynamic_cast<BBox*>(parent);

		if (box && box->LabelView() == this)
			parent = box->Parent();

		if (parent) {
			BBox *box = dynamic_cast<BBox*>(parent);

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
		BRadioButton *radio = dynamic_cast<BRadioButton*>(child);

		if (radio && (radio != this))
			radio->SetValue(B_CONTROL_OFF);
		else {
			// If the child is a BBox, check if the label is a radiobutton
			BBox *box = dynamic_cast<BBox*>(child);

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
		BRect rect = _KnobFrame();
		float width = rect.right + floorf(ceilf(fontHeight.ascent
			+ fontHeight.descent) / 2.0);

		if (Label())
			width += StringWidth(Label());
	
		*_width = ceilf(width);
	}

	if (_height)
		*_height = ceilf(fontHeight.ascent + fontHeight.descent) + 6.0f;
}


void
BRadioButton::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}


status_t
BRadioButton::Invoke(BMessage *message)
{
	return BControl::Invoke(message);
}


void
BRadioButton::MessageReceived(BMessage *message)
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
		SetValue(B_CONTROL_ON);
		Invoke();
	}
	Invalidate();

	SetTracking(false);
}


void
BRadioButton::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
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
	BControl::FrameResized(width, height);
}


BHandler*
BRadioButton::ResolveSpecifier(BMessage *message, int32 index,
							   BMessage *specifier, int32 what,
							   const char *property)
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
BRadioButton::GetSupportedSuites(BMessage *message)
{
	return BControl::GetSupportedSuites(message);
}


status_t
BRadioButton::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}


BSize
BRadioButton::MaxSize()
{
	float width, height;
	GetPreferredSize(&width, &height);

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(B_SIZE_UNLIMITED, height));
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

	// layout the rect for the dot
	BRect rect(Bounds());

	// its size depends on the text height
	float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);
	float inset = -floorf(textHeight / 2 - 2);

	rect.left -= (inset - 1);
	rect.top = floorf((rect.top + rect.bottom) / 2.0);
	rect.bottom = rect.top;
	rect.right = rect.left;
	rect.InsetBy(inset, inset);

	return rect;
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
