/*
 * Copyright 2001-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

/**	BTextControl displays text that can act like a control. */


#include <stdio.h>

#include <AbstractLayoutItem.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <Region.h>
#include <TextControl.h>
#include <Window.h>

#include "TextInput.h"


class BTextControl::LabelLayoutItem : public BAbstractLayoutItem {
public:
								LabelLayoutItem(BTextControl* parent);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

private:
			BTextControl*		fParent;
			BRect				fFrame;
};


class BTextControl::TextViewLayoutItem : public BAbstractLayoutItem {
public:
								TextViewLayoutItem(BTextControl* parent);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

private:
			BTextControl*		fParent;
			BRect				fFrame;
};


// #pragma mark -


BTextControl::BTextControl(BRect frame, const char *name, const char *label,
						   const char *text, BMessage *message, uint32 mask,
						   uint32 flags)
	: BControl(frame, name, label, message, mask, flags | B_FRAME_EVENTS)
{
	_InitData(label, text);
	_ValidateLayout();
}


BTextControl::BTextControl(const char *name, const char *label,
						   const char *text, BMessage *message,
						   uint32 flags)
	: BControl(BRect(0, 0, -1, -1), name, label, message, B_FOLLOW_NONE,
			flags | B_FRAME_EVENTS | B_SUPPORTS_LAYOUT)
{
	_InitData(label, text);
	_ValidateLayout();
}


BTextControl::BTextControl(const char *label,
						   const char *text, BMessage *message)
	: BControl(BRect(0, 0, -1, -1), NULL, label, message, B_FOLLOW_NONE,
			B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS | B_SUPPORTS_LAYOUT)
{
	_InitData(label, text);
	_ValidateLayout();
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
	status_t ret = BControl::Archive(data, deep);
	alignment labelAlignment, textAlignment;

	GetAlignment(&labelAlignment, &textAlignment);

	if (ret == B_OK)
		ret = data->AddInt32("_a_label", labelAlignment);
	if (ret == B_OK)
		ret = data->AddInt32("_a_text", textAlignment);
	if (ret == B_OK)
		ret = data->AddFloat("_divide", Divider());

	if (ModificationMessage() && (ret == B_OK))
		ret = data->AddMessage("_mod_msg", ModificationMessage());

	return ret;
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
BTextControl::GetAlignment(alignment* _label, alignment* _text) const
{
	if (_label)
		*_label = fLabelAlign;
	if (_text)
		*_text = fText->Alignment();
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
	rgb_color noTint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lighten1 = tint_color(noTint, B_LIGHTEN_1_TINT);
	rgb_color lighten2 = tint_color(noTint, B_LIGHTEN_2_TINT);
	rgb_color lightenMax = tint_color(noTint, B_LIGHTEN_MAX_TINT);
	rgb_color darken1 = tint_color(noTint, B_DARKEN_1_TINT);
	rgb_color darken2 = tint_color(noTint, B_DARKEN_2_TINT);
	rgb_color darken4 = tint_color(noTint, B_DARKEN_4_TINT);
	rgb_color navigationColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

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

	// label

	if (Label()) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		float y = fontHeight.ascent + fText->Frame().top + 1;
		float x;

		float labelWidth = StringWidth(Label());
		switch (fLabelAlign) {
			case B_ALIGN_RIGHT:
				x = fDivider - labelWidth - 3.0f;
				break;

			case B_ALIGN_CENTER:
				x = fDivider - labelWidth / 2.0f;
				break;

			default:
				x = 3.0f;
				break;
		}

		BRect labelArea(x, fText->Frame().top, x + labelWidth,
				ceilf(fontHeight.ascent + fontHeight.descent) + 1);
		if (x < fDivider && updateRect.Intersects(labelArea)) {
			labelArea.right = fDivider;
			
			BRegion clipRegion(labelArea);
			ConstrainClippingRegion(&clipRegion);
			SetHighColor(IsEnabled() ? ui_color(B_CONTROL_TEXT_COLOR)
				: tint_color(noTint, B_DISABLED_LABEL_TINT));
			DrawString(Label(), BPoint(x, y));
		}
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

	_UpdateTextViewColors(IsEnabled());
	fText->MakeEditable(IsEnabled());
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
BTextControl::SetEnabled(bool enabled)
{
	if (IsEnabled() == enabled)
		return;

	if (Window()) {
		fText->MakeEditable(enabled);

		_UpdateTextViewColors(enabled);

		fText->Invalidate();
		Window()->UpdateIfNeeded();
	}

	BControl::SetEnabled(enabled);
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


BLayoutItem*
BTextControl::CreateLabelLayoutItem()
{
	if (!fLabelLayoutItem)
		fLabelLayoutItem = new LabelLayoutItem(this);
	return fLabelLayoutItem;
}


BLayoutItem*
BTextControl::CreateTextViewLayoutItem()
{
	if (!fTextViewLayoutItem)
		fTextViewLayoutItem = new TextViewLayoutItem(this);
	return fTextViewLayoutItem;
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
BTextControl::_UpdateTextViewColors(bool enabled)
{
	rgb_color textColor;
	rgb_color color;
	BFont font;

	fText->GetFontAndColor(0, &font);

	if (enabled)
		textColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	else {
		textColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DISABLED_LABEL_TINT);
	}

	fText->SetFontAndColor(&font, B_FONT_ALL, &textColor);

	if (enabled) {
		color = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	} else {
		color = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_LIGHTEN_2_TINT);
	}

	fText->SetViewColor(color);
	fText->SetLowColor(color);
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
	fLabelLayoutItem = NULL;
	fTextViewLayoutItem = NULL;
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
		frame.InsetBy(2.0, 3.0);
		BRect textRect(frame.OffsetToCopy(B_ORIGIN));

		fText = new _BTextInput_(frame, textRect,
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_FRAME_EVENTS | navigableFlags);
		AddChild(fText);

		SetText(initialText);
		fText->SetAlignment(B_ALIGN_LEFT);
		fText->AlignTextRect();
	}
}


void
BTextControl::_ValidateLayout()
{
	float height;
	BTextControl::GetPreferredSize(NULL, &height);

	ResizeTo(Bounds().Width(), height);

	float lineHeight = ceil(fText->LineHeight(0));
	fText->ResizeTo(fText->Bounds().Width(), lineHeight);
	fText->MoveTo(fText->Frame().left, (height - lineHeight) / 2);

	fPreviousHeight = Bounds().Height();
}


void
BTextControl::_UpdateFrame()
{
	if (fLabelLayoutItem && fTextViewLayoutItem) {
		BRect labelFrame = fLabelLayoutItem->Frame();
		BRect textFrame = fTextViewLayoutItem->Frame();
		MoveTo(labelFrame.left, labelFrame.top);
		ResizeTo(textFrame.left + textFrame.Width() - labelFrame.left,
			textFrame.top + textFrame.Height() - labelFrame.top);
		SetDivider(textFrame.left - labelFrame.left);
	}
}


// #pragma mark -


BTextControl::LabelLayoutItem::LabelLayoutItem(BTextControl* parent)
	: fParent(parent),
	  fFrame()
{
}


bool
BTextControl::LabelLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
BTextControl::LabelLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
BTextControl::LabelLayoutItem::Frame()
{
	return fFrame;
}


void
BTextControl::LabelLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


BView*
BTextControl::LabelLayoutItem::View()
{
	return fParent;
}


BSize
BTextControl::LabelLayoutItem::BaseMinSize()
{
// TODO: Cache the info. Might be too expensive for this call.
	const char* label = fParent->Label();
	if (!label)
		return BSize(-1, -1);

	BSize size;
	fParent->GetPreferredSize(NULL, &size.height);

	size.width = fParent->StringWidth(label) + 2 * 3 - 1;

	return size;
}


BSize
BTextControl::LabelLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
BTextControl::LabelLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
BTextControl::LabelLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


// #pragma mark -


BTextControl::TextViewLayoutItem::TextViewLayoutItem(BTextControl* parent)
	: fParent(parent),
	  fFrame()
{
}


bool
BTextControl::TextViewLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
BTextControl::TextViewLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
BTextControl::TextViewLayoutItem::Frame()
{
	return fFrame;
}


void
BTextControl::TextViewLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


BView*
BTextControl::TextViewLayoutItem::View()
{
	return fParent;
}


BSize
BTextControl::TextViewLayoutItem::BaseMinSize()
{
// TODO: Cache the info. Might be too expensive for this call.
	BSize size;
	fParent->GetPreferredSize(NULL, &size.height);

	// mmh, some arbitrary minimal width
	size.width = 30;

	return size;
}


BSize
BTextControl::TextViewLayoutItem::BaseMaxSize()
{
	BSize size(BaseMinSize());
	size.width = B_SIZE_UNLIMITED;
	return size;
}


BSize
BTextControl::TextViewLayoutItem::BasePreferredSize()
{
	BSize size(BaseMinSize());
	// puh, no idea...
	size.width = 100;
	return size;
}


BAlignment
BTextControl::TextViewLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}

