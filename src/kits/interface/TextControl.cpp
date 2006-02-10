/*
 * Copyright 2001-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/**	BTextControl displays text that can act like a control. */


#include <stdio.h>

#include <Message.h>
#include <Region.h>
#include <TextControl.h>
#include <Window.h>

#include "TextInput.h"


BTextControl::BTextControl(BRect frame, const char *name, const char *label,
						   const char *text, BMessage *message, uint32 mask,
						   uint32 flags)
	: BControl(frame, name, label, message, mask, flags | B_FRAME_EVENTS)
{
	_InitData(label, text);

	float height;
	BTextControl::GetPreferredSize(NULL, &height);

	ResizeTo(Bounds().Width(), height);

	float lineHeight = ceil(fText->LineHeight(0));
	fText->ResizeTo(fText->Bounds().Width(), lineHeight);
	fText->MoveTo(fText->Frame().left, (height - lineHeight) / 2);

	fPreviousHeight = Bounds().Height();
}


BTextControl::~BTextControl()
{
	SetModificationMessage(NULL);
}


BTextControl::BTextControl(BMessage* archive)
	: BControl(archive)
{
	_InitData(Label(), NULL, archive);

	int32 labelAlignment = B_ALIGN_LEFT;
	int32 textAlignment = B_ALIGN_LEFT;

	if (archive->HasInt32("_a_label"))
		archive->FindInt32("_a_label", &labelAlignment);

	if (archive->HasInt32("_a_text"))
		archive->FindInt32("_a_text", &textAlignment);
	
	SetAlignment((alignment)labelAlignment, (alignment)textAlignment);

	if (archive->HasFloat("_divide"))
		archive->FindFloat("_a_text", &fDivider);

	if (archive->HasMessage("_mod_msg")) {
		BMessage* message = new BMessage;
		archive->FindMessage("_mod_msg", message);
		SetModificationMessage(message);
	}
}


BArchivable *
BTextControl::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BTextControl"))
		return new BTextControl(archive);
	else
		return NULL;
}


status_t
BTextControl::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	alignment labelAlignment, textAlignment;

	GetAlignment(&labelAlignment, &textAlignment);

	data->AddInt32("_a_label", labelAlignment);
	data->AddInt32("_a_text", textAlignment);
	data->AddFloat("_divide", Divider());

	if (ModificationMessage())
		data->AddMessage("_mod_msg", ModificationMessage());

	return B_OK;
}


void
BTextControl::SetText(const char *text)
{
	if (InvokeKind() != B_CONTROL_INVOKED)
		return;

	fText->SetText(text);

	if (IsFocus())
		fText->SetInitialText();

	fText->Invalidate();
}


const char *
BTextControl::Text() const
{
	return fText->Text();
}


void
BTextControl::SetValue(int32 value)
{
	BControl::SetValue(value);
}


status_t
BTextControl::Invoke(BMessage *message)
{
	return BControl::Invoke(message);
}


BTextView *
BTextControl::TextView() const
{
	return fText;
}


void
BTextControl::SetModificationMessage(BMessage *message)
{
	delete fModificationMessage;
	fModificationMessage = message;
}


BMessage *
BTextControl::ModificationMessage() const
{
	return fModificationMessage;
}


void
BTextControl::SetAlignment(alignment labelAlignment, alignment textAlignment)
{
	fText->SetAlignment(textAlignment);
	fText->AlignTextRect();

	if (fLabelAlign != labelAlignment) {
		fLabelAlign = labelAlignment;
		Invalidate();
	}
}


void
BTextControl::GetAlignment(alignment *label, alignment *text) const
{
	if (label)
		*label = fLabelAlign;
	if (text)
		*text = fText->Alignment();
}


void
BTextControl::SetDivider(float dividingLine)
{
	dividingLine = floorf(dividingLine + 0.5);

	float dx = fDivider - dividingLine;

	fDivider = dividingLine;

	fText->MoveBy(-dx, 0.0f);
	fText->ResizeBy(dx, 0.0f);

	if (Window()) {
		fText->Invalidate();
		Invalidate();
	}
}


float
BTextControl::Divider() const
{
	return fDivider;
}


void
BTextControl::Draw(BRect updateRect)
{
	rgb_color noTint = ui_color(B_PANEL_BACKGROUND_COLOR),
	lighten1 = tint_color(noTint, B_LIGHTEN_1_TINT),
	lighten2 = tint_color(noTint, B_LIGHTEN_2_TINT),
	lightenMax = tint_color(noTint, B_LIGHTEN_MAX_TINT),
	darken1 = tint_color(noTint, B_DARKEN_1_TINT),
	darken2 = tint_color(noTint, B_DARKEN_2_TINT),
	darken4 = tint_color(noTint, B_DARKEN_4_TINT),
	darkenMax = tint_color(noTint, B_DARKEN_MAX_TINT),
	navigationColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	bool enabled = IsEnabled();
	bool active = false;

	if (fText->IsFocus() && Window()->IsActive())
		active = true;

	// outer bevel

	BRect rect = Bounds();
	rect.left = fDivider;

	if (enabled)
		SetHighColor(darken1);
	else
		SetHighColor(noTint);

	StrokeLine(rect.LeftBottom(), rect.LeftTop());
	StrokeLine(rect.RightTop());

	if (enabled)
		SetHighColor(lighten2);
	else
		SetHighColor(lighten1);

	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
	StrokeLine(BPoint(rect.right, rect.top + 1.0f), rect.RightBottom());

	// inner bevel

	rect.InsetBy(1.0f, 1.0f);

	if (active) {
		SetHighColor(navigationColor);
		StrokeRect(rect);
	} else {
		if (enabled)
			SetHighColor(darken4);
		else
			SetHighColor(darken2);

		StrokeLine(rect.LeftTop(), rect.LeftBottom());
		StrokeLine(rect.LeftTop(), rect.RightTop());

		SetHighColor(noTint);
		StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
		StrokeLine(BPoint(rect.right, rect.top + 1.0f));
	}

	// area around text view

	SetHighColor(fText->ViewColor());
	rect.InsetBy(1, 1);
	BRegion region(rect);
	BRegion updateRegion(updateRect);
		// why is there no IntersectWith(BRect &) version?
	region.IntersectWith(&updateRegion);
	FillRegion(&region);

	if (Label()) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		float y = fontHeight.ascent + fText->Frame().top + 1;
		float x;

		switch (fLabelAlign) {
			case B_ALIGN_RIGHT:
				x = fDivider - StringWidth(Label()) - 3.0f;
				break;

			case B_ALIGN_CENTER:
				x = fDivider - StringWidth(Label()) / 2.0f;
				break;

			default:
				x = 3.0f;
				break;
		}

		SetHighColor(IsEnabled() ? darkenMax
			: tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT));
		DrawString(Label(), BPoint(x, y));
	}
}


void
BTextControl::MouseDown(BPoint where)
{
	if (!fText->IsFocus()) {
		fText->MakeFocus(true);
		fText->SelectAll();
	}
}


void
BTextControl::AttachedToWindow()
{
	BControl::AttachedToWindow();

	bool enabled = IsEnabled();
	rgb_color textColor;
	rgb_color color = HighColor();
	BFont font;

	fText->GetFontAndColor(0, &font, &color);

	if (enabled)
		textColor = color;
	else
		textColor = tint_color(color, B_LIGHTEN_2_TINT);

	fText->SetFontAndColor(&font, B_FONT_ALL, &textColor);

	if (enabled) {
		color.red = 255;
		color.green = 255;
		color.blue = 255;
	} else
		color = tint_color(color, B_LIGHTEN_2_TINT);

	fText->SetViewColor(color);
	fText->SetLowColor(color);

	fText->MakeEditable(enabled);
}


void
BTextControl::MakeFocus(bool state)
{
	if (state != fText->IsFocus()) {
		fText->MakeFocus(state);

		if (state)
			fText->SelectAll();
	}
}


void
BTextControl::SetEnabled(bool state)
{
	if (IsEnabled() == state)
		return;

	if (Window()) {
		fText->MakeEditable(state);

		rgb_color textColor;
		rgb_color color = {0, 0, 0, 255};
		BFont font;

		fText->GetFontAndColor(0, &font, &color);

		if (state)
			textColor = color;
		else
			textColor = tint_color(color, B_DISABLED_LABEL_TINT);

		fText->SetFontAndColor(&font, B_FONT_ALL, &textColor);

		if (state) {
			color.red = 255;
			color.green = 255;
			color.blue = 255;
		} else
			color = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
				B_LIGHTEN_2_TINT);

		fText->SetViewColor(color);
		fText->SetLowColor(color);

		fText->Invalidate();
		Window()->UpdateIfNeeded();
	}

	BControl::SetEnabled(state);
}


void
BTextControl::GetPreferredSize(float *_width, float *_height)
{
	if (_height) {
		// we need enough space for the label and the child text view
		font_height fontHeight;
		GetFontHeight(&fontHeight);
		float labelHeight = ceil(fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading);
		float textHeight = fText->LineHeight(0) + 4.0;

		*_height = max_c(labelHeight, textHeight);
	}

	if (_width) {
		// TODO: this one I need to find out
		float width = 20.0f + ceilf(StringWidth(Label()));
		if (width < Bounds().Width())
			width = Bounds().Width();
		*_width = width;
	}
}


void
BTextControl::ResizeToPreferred()
{
	// TODO: change divider?
	BView::ResizeToPreferred();
}


void
BTextControl::SetFlags(uint32 flags)
{
	if (!fSkipSetFlags) {
		// If the textview is navigable, set it to not navigable if needed
		// Else if it is not navigable, set it to navigable if needed
		if (fText->Flags() & B_NAVIGABLE) {
			if (!(flags & B_NAVIGABLE))
				fText->SetFlags(fText->Flags() & ~B_NAVIGABLE);

		} else {
			if (flags & B_NAVIGABLE)
				fText->SetFlags(fText->Flags() | B_NAVIGABLE);
		}

		// Don't make this one navigable
		flags &= ~B_NAVIGABLE;
	}

	BView::SetFlags(flags);
}


void
BTextControl::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_SET_PROPERTY:
		case B_GET_PROPERTY:
			// TODO
			break;
		default:
			BControl::MessageReceived(msg);
			break;
	}
}


BHandler *
BTextControl::ResolveSpecifier(BMessage *msg, int32 index,
										 BMessage *specifier, int32 form,
										 const char *property)
{
	/*
	BPropertyInfo propInfo(prop_list);
	BHandler *target = NULL;

	if (propInfo.FindMatch(message, 0, specifier, what, property) < B_OK)
		return BControl::ResolveSpecifier(message, index, specifier, what,
			property);
	else
		return this;
	*/
	return BControl::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BTextControl::GetSupportedSuites(BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}


void
BTextControl::MouseUp(BPoint pt)
{
	BControl::MouseUp(pt);
}


void
BTextControl::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BControl::MouseMoved(pt, code, msg);
}


void
BTextControl::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}


void
BTextControl::AllAttached()
{
	BControl::AllAttached();
}


void
BTextControl::AllDetached()
{
	BControl::AllDetached();
}


void
BTextControl::FrameMoved(BPoint newPosition)
{
	BControl::FrameMoved(newPosition);
}


void
BTextControl::FrameResized(float width, float height)
{
	BControl::FrameResized(width, height);

	// changes in width

	BRect bounds = Bounds();
	const float border = 2.0f;

	if (bounds.Width() > fPreviousWidth) {
		// invalidate the region between the old and the new right border
		BRect rect = bounds;
		rect.left += fPreviousWidth - border;
		rect.right--;
		Invalidate(rect);
	} else if (bounds.Width() < fPreviousWidth) {
		// invalidate the region of the new right border
		BRect rect = bounds;
		rect.left = rect.right - border;
		Invalidate(rect);
	}

	// changes in height

	if (bounds.Height() > fPreviousHeight) {
		// invalidate the region between the old and the new bottom border
		BRect rect = bounds;
		rect.top += fPreviousHeight - border;
		rect.bottom--;
		Invalidate(rect);
	} else if (bounds.Height() < fPreviousHeight) {
		// invalidate the region of the new bottom border
		BRect rect = bounds;
		rect.top = rect.bottom - border;
		Invalidate(rect);
	}

	fPreviousWidth = uint16(bounds.Width());
	fPreviousHeight = uint16(bounds.Height());
}


void
BTextControl::WindowActivated(bool active)
{
	if (fText->IsFocus()) {
		// invalidate to remove/show focus indication
		BRect rect = Bounds();
		rect.left = fDivider;
		Invalidate(rect);

		// help out embedded text view which doesn't
		// get notified of this
		fText->Invalidate();
	}
}


status_t
BTextControl::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}


void BTextControl::_ReservedTextControl1() {}
void BTextControl::_ReservedTextControl2() {}
void BTextControl::_ReservedTextControl3() {}
void BTextControl::_ReservedTextControl4() {}


BTextControl &
BTextControl::operator=(const BTextControl&)
{
	return *this;
}


void
BTextControl::_CommitValue()
{
}


void
BTextControl::_InitData(const char* label, const char* initialText,
	BMessage* archive)
{
	BRect bounds(Bounds());

	fText = NULL;
	fModificationMessage = NULL;
	fLabelAlign = B_ALIGN_LEFT;
	fDivider = 0.0f;
	fPreviousWidth = bounds.Width();
	fPreviousHeight = bounds.Height();
	fSkipSetFlags = false;

	int32 flags = 0;

	BFont font(be_plain_font);

	if (!archive || !archive->HasString("_fname"))
		flags |= B_FONT_FAMILY_AND_STYLE;

	if (!archive || !archive->HasFloat("_fflt"))
		flags |= B_FONT_SIZE;

	if (flags != 0)
		SetFont(&font, flags);

	if (label)
		fDivider = floorf(bounds.Width() / 2.0f);

	uint32 navigableFlags = Flags() & B_NAVIGABLE;
	if (navigableFlags != 0) {
		fSkipSetFlags = true;
		SetFlags(Flags() & ~B_NAVIGABLE);
		fSkipSetFlags = false;
	}

	if (archive)
		fText = static_cast<_BTextInput_ *>(FindView("_input_"));
	else {
		BRect frame(fDivider, bounds.top,
					bounds.right, bounds.bottom);
		// we are stroking the frame around the text view, which
		// is 2 pixels wide
		frame.InsetBy(4.0, 3.0);
		BRect textRect(frame.OffsetToCopy(0.0f, 0.0f));

		fText = new _BTextInput_(frame, textRect,
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_FRAME_EVENTS | navigableFlags);
		AddChild(fText);

		SetText(initialText);
		fText->SetAlignment(B_ALIGN_LEFT);
		fText->AlignTextRect();
	}
}
