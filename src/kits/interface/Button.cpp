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

#include <new>

#include <ControlLook.h>
#include <Font.h>
#include <LayoutUtils.h>
#include <String.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


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
	if (be_control_look != NULL) {
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
		rect.InsetBy(3.0, 3.0);
		be_control_look->DrawLabel(this, Label(), rect, updateRect,
			base, flags, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));
		return;
	}

	font_height fh;
	GetFontHeight(&fh);

	const BRect bounds = Bounds();
	BRect rect = bounds;

	const bool enabled = IsEnabled();
	const bool pushed = Value() == B_CONTROL_ON;
	// Default indicator
	if (IsDefault())
		rect = _DrawDefault(rect, enabled);

	BRect fillArea = rect;
	fillArea.InsetBy(3.0, 3.0);

	BString text = Label();

#if 1
	// Label truncation
	BFont font;
	GetFont(&font);
	font.TruncateString(&text, B_TRUNCATE_END, fillArea.Width() - 4);
#endif

	// Label position
	const float stringWidth = StringWidth(text.String());
	const float x = (rect.right - stringWidth) / 2.0;
	const float labelY = bounds.top
		+ ((bounds.Height() - fh.ascent - fh.descent) / 2.0)
		+ fh.ascent + 1.0;
	const float focusLineY = labelY + fh.descent;

	/* speed trick:
	   if the focus changes but the button is not pressed then we can
	   redraw only the focus line,
	   if the focus changes and the button is pressed invert the internal rect
	   this block takes care of all the focus changes
	*/
	if (IsFocusChanging()) {
		if (pushed) {
			rect.InsetBy(2.0, 2.0);
			InvertRect(rect);
		} else {
			_DrawFocusLine(x, focusLineY, stringWidth, IsFocus()
				&& Window()->IsActive());
		}

		return;
	}

	// colors
	rgb_color panelBgColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color buttonBgColor = tint_color(panelBgColor, B_LIGHTEN_1_TINT);
	rgb_color lightColor;
	rgb_color maxLightColor;
	rgb_color maxShadowColor = tint_color(panelBgColor, B_DARKEN_MAX_TINT);

	rgb_color dark1BorderColor;
	rgb_color dark2BorderColor;

	rgb_color bevelColor1;
	rgb_color bevelColor2;
	rgb_color bevelColorRBCorner;

	rgb_color borderBevelShadow;
	rgb_color borderBevelLight;

	if (enabled) {
		lightColor = tint_color(panelBgColor, B_LIGHTEN_2_TINT);
		maxLightColor = tint_color(panelBgColor, B_LIGHTEN_MAX_TINT);

		dark1BorderColor = tint_color(panelBgColor, B_DARKEN_3_TINT);
		dark2BorderColor = tint_color(panelBgColor, B_DARKEN_4_TINT);

		bevelColor1 = tint_color(panelBgColor, B_DARKEN_2_TINT);
		bevelColor2 = panelBgColor;

		if (IsDefault()) {
			borderBevelShadow = tint_color(dark1BorderColor,
				(B_NO_TINT + B_DARKEN_1_TINT) / 2);
			borderBevelLight = tint_color(dark1BorderColor, B_LIGHTEN_1_TINT);

			borderBevelLight.red = (borderBevelLight.red + panelBgColor.red)
				/ 2;
			borderBevelLight.green = (borderBevelLight.green
				+ panelBgColor.green) / 2;
			borderBevelLight.blue = (borderBevelLight.blue
				+ panelBgColor.blue) / 2;

			dark1BorderColor = tint_color(dark1BorderColor, B_DARKEN_3_TINT);
			dark2BorderColor = tint_color(dark1BorderColor, B_DARKEN_4_TINT);

			bevelColorRBCorner = borderBevelShadow;
		} else {
			borderBevelShadow = tint_color(panelBgColor,
				(B_NO_TINT + B_DARKEN_1_TINT) / 2);
			borderBevelLight = buttonBgColor;

			bevelColorRBCorner = dark1BorderColor;
		}
	} else {
		lightColor = tint_color(panelBgColor, B_LIGHTEN_2_TINT);
		maxLightColor = tint_color(panelBgColor, B_LIGHTEN_1_TINT);

		dark1BorderColor = tint_color(panelBgColor, B_DARKEN_1_TINT);
		dark2BorderColor = tint_color(panelBgColor, B_DARKEN_2_TINT);

		bevelColor1 = panelBgColor;
		bevelColor2 = buttonBgColor;

		if (IsDefault()) {
			borderBevelShadow = dark1BorderColor;
			borderBevelLight = panelBgColor;
			dark1BorderColor = tint_color(dark1BorderColor, B_DARKEN_1_TINT);
			dark2BorderColor = tint_color(dark1BorderColor, 1.16);

		} else {
			borderBevelShadow = panelBgColor;
			borderBevelLight = panelBgColor;
		}

		bevelColorRBCorner = tint_color(panelBgColor, 1.08);;
	}

	// fill the button area
	SetHighColor(buttonBgColor);
	FillRect(fillArea);

	BeginLineArray(22);
	// bevel around external border
	AddLine(BPoint(rect.left, rect.bottom),
			BPoint(rect.left, rect.top), borderBevelShadow);
	AddLine(BPoint(rect.left + 1, rect.top),
			BPoint(rect.right, rect.top), borderBevelShadow);

	AddLine(BPoint(rect.right, rect.top + 1),
			BPoint(rect.right, rect.bottom), borderBevelLight);
	AddLine(BPoint(rect.left + 1, rect.bottom),
			BPoint(rect.right - 1, rect.bottom), borderBevelLight);

	rect.InsetBy(1.0, 1.0);

	// external border
	AddLine(BPoint(rect.left, rect.bottom),
			BPoint(rect.left, rect.top), dark1BorderColor);
	AddLine(BPoint(rect.left + 1, rect.top),
			BPoint(rect.right, rect.top), dark1BorderColor);
	AddLine(BPoint(rect.right, rect.top + 1),
			BPoint(rect.right, rect.bottom), dark2BorderColor);
	AddLine(BPoint(rect.right - 1, rect.bottom),
			BPoint(rect.left + 1, rect.bottom), dark2BorderColor);

	rect.InsetBy(1.0, 1.0);

	// Light
	AddLine(BPoint(rect.left, rect.top),
			BPoint(rect.left, rect.top), buttonBgColor);
	AddLine(BPoint(rect.left, rect.top + 1),
			BPoint(rect.left, rect.bottom - 1), lightColor);
	AddLine(BPoint(rect.left, rect.bottom),
			BPoint(rect.left, rect.bottom), bevelColor2);
	AddLine(BPoint(rect.left + 1, rect.top),
			BPoint(rect.right - 1, rect.top), lightColor);
	AddLine(BPoint(rect.right, rect.top),
			BPoint(rect.right, rect.top), bevelColor2);
	// Shadow
	AddLine(BPoint(rect.left + 1, rect.bottom),
			BPoint(rect.right - 1, rect.bottom), bevelColor1);
	AddLine(BPoint(rect.right, rect.bottom),
			BPoint(rect.right, rect.bottom), bevelColorRBCorner);
	AddLine(BPoint(rect.right, rect.bottom - 1),
			BPoint(rect.right, rect.top + 1), bevelColor1);

	rect.InsetBy(1.0, 1.0);

	// Light
	AddLine(BPoint(rect.left, rect.top),
			BPoint(rect.left, rect.bottom - 1), maxLightColor);
	AddLine(BPoint(rect.left, rect.bottom),
			BPoint(rect.left, rect.bottom), buttonBgColor);
	AddLine(BPoint(rect.left + 1, rect.top),
			BPoint(rect.right - 1, rect.top), maxLightColor);
	AddLine(BPoint(rect.right, rect.top),
			BPoint(rect.right, rect.top), buttonBgColor);
	// Shadow
	AddLine(BPoint(rect.left + 1, rect.bottom),
			BPoint(rect.right, rect.bottom), bevelColor2);
	AddLine(BPoint(rect.right, rect.bottom - 1),
			BPoint(rect.right, rect.top + 1), bevelColor2);

	rect.InsetBy(1.0,1.0);

	EndLineArray();

	// Invert if clicked
	if (enabled && pushed) {
		rect.InsetBy(-2.0, -2.0);
		InvertRect(rect);
	}

	// Label color
	if (enabled) {
		if (pushed) {
			SetHighColor(maxLightColor);
			SetLowColor(255 - buttonBgColor.red,
						255 - buttonBgColor.green,
						255 - buttonBgColor.blue);
		} else {
			SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
			SetLowColor(buttonBgColor);
		}
	} else {
		SetHighColor(tint_color(panelBgColor, B_DISABLED_LABEL_TINT));
		SetLowColor(buttonBgColor);
	}

	// Draw the label
	DrawString(text.String(), BPoint(x, labelY));

	// Focus line
	if (enabled && IsFocus() && Window()->IsActive() && !pushed)
		_DrawFocusLine(x, focusLineY, stringWidth, true);
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
		// width
		float width = 20.0f + (float)ceil(StringWidth(Label()));
		if (width < 75.0f)
			width = 75.0f;

		if (fDrawAsDefault)
			width += 6.0f;

		fPreferredSize.width = width;

		// height
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		fPreferredSize.height
			= ceilf((fontHeight.ascent + fontHeight.descent) * 1.8)
				+ (fDrawAsDefault ? 6.0f : 0);

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


#if __GNUC__ == 2


extern "C" void
InvalidateLayout__7BButtonb(BView* view, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}


#endif
