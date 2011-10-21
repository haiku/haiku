/*
 * Copyright 2001-2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

/*!	BTextControl displays text that can act like a control. */

#include <string.h>

#include <TextControl.h>

#include <AbstractLayoutItem.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>
#include <binary_compatibility/Support.h>

#include "TextInput.h"


//#define TRACE_TEXT_CONTROL
#ifdef TRACE_TEXT_CONTROL
#	include <stdio.h>
#	include <FunctionTracer.h>
	static int32 sFunctionDepth = -1;
#	define CALLED(x...)	FunctionTracer _ft("BTextControl", __FUNCTION__, \
							sFunctionDepth)
#	define TRACE(x...)	{ BString _to; \
							_to.Append(' ', (sFunctionDepth + 1) * 2); \
							printf("%s", _to.String()); printf(x); }
#else
#	define CALLED(x...)
#	define TRACE(x...)
#endif


namespace {
	const char* const kFrameField = "BTextControl:layoutitem:frame";
	const char* const kTextViewItemField = "BTextControl:textViewItem";
	const char* const kLabelItemField = "BMenuField:labelItem";
}


static property_info sPropertyList[] = {
	{
		"Value",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_STRING_TYPE }
	},
	{}
};


class BTextControl::LabelLayoutItem : public BAbstractLayoutItem {
public:
								LabelLayoutItem(BTextControl* parent);
								LabelLayoutItem(BMessage* from);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(BTextControl* parent);
	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);
private:
			BTextControl*		fParent;
			BRect				fFrame;
};


class BTextControl::TextViewLayoutItem : public BAbstractLayoutItem {
public:
								TextViewLayoutItem(BTextControl* parent);
								TextViewLayoutItem(BMessage* from);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(BTextControl* parent);
	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);
private:
			BTextControl*		fParent;
			BRect				fFrame;
};


struct BTextControl::LayoutData {
	LayoutData(float width, float height)
		:
		label_layout_item(NULL),
		text_view_layout_item(NULL),
		previous_width(width),
		previous_height(height),
		valid(false)
	{
	}

	LabelLayoutItem*	label_layout_item;
	TextViewLayoutItem*	text_view_layout_item;
	float				previous_width;		// used in FrameResized() for
	float				previous_height;	// invalidation
	font_height			font_info;
	float				label_width;
	float				label_height;
	BSize				min;
	BSize				text_view_min;
	bool				valid;
};


// #pragma mark -


static const int32 kFrameMargin = 2;
static const int32 kLabelInputSpacing = 3;


BTextControl::BTextControl(BRect frame, const char* name, const char* label,
		const char* text, BMessage* message, uint32 mask, uint32 flags)
	:
	BControl(frame, name, label, message, mask, flags | B_FRAME_EVENTS)
{
	_InitData(label);
	_InitText(text);
	_ValidateLayout();
}


BTextControl::BTextControl(const char* name, const char* label,
		const char* text, BMessage* message, uint32 flags)
	:
	BControl(name, label, message, flags | B_FRAME_EVENTS)
{
	_InitData(label);
	_InitText(text);
	_ValidateLayout();
}


BTextControl::BTextControl(const char* label, const char* text,
		BMessage* message)
	:
	BControl(NULL, label, message,
		B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
{
	_InitData(label);
	_InitText(text);
	_ValidateLayout();
}


BTextControl::~BTextControl()
{
	SetModificationMessage(NULL);
	delete fLayoutData;
}


BTextControl::BTextControl(BMessage* archive)
	:
	BControl(BUnarchiver::PrepareArchive(archive))
{
	BUnarchiver unarchiver(archive);

	_InitData(Label(), archive);

	if (!BUnarchiver::IsArchiveManaged(archive))
		_InitText(NULL, archive);

	status_t err = B_OK;
	if (archive->HasFloat("_divide"))
		err = archive->FindFloat("_divide", &fDivider);

	if (err == B_OK && archive->HasMessage("_mod_msg")) {
		BMessage* message = new BMessage;
		err = archive->FindMessage("_mod_msg", message);
		SetModificationMessage(message);
	}

	unarchiver.Finish(err);
}


BArchivable*
BTextControl::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BTextControl"))
		return new BTextControl(archive);

	return NULL;
}


status_t
BTextControl::Archive(BMessage *data, bool deep) const
{
	BArchiver archiver(data);
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

	return archiver.Finish(ret);
}


status_t
BTextControl::AllArchived(BMessage* into) const
{
	BArchiver archiver(into);
	status_t err = B_OK;

	if (archiver.IsArchived(fLayoutData->text_view_layout_item)) {
		err = archiver.AddArchivable(kTextViewItemField,
			fLayoutData->text_view_layout_item);
	}

	if (err == B_OK && archiver.IsArchived(fLayoutData->label_layout_item)) {
		err = archiver.AddArchivable(kLabelItemField,
			fLayoutData->label_layout_item);
	}

	return err;
}


status_t
BTextControl::AllUnarchived(const BMessage* from)
{
	status_t err;
	if ((err = BControl::AllUnarchived(from)) != B_OK)
		return err;

	_InitText(NULL, from);

	BUnarchiver unarchiver(from);
	if (unarchiver.IsInstantiated(kTextViewItemField)) {
		err = unarchiver.FindObject(kTextViewItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP,
			fLayoutData->text_view_layout_item);

		if (err == B_OK)
			fLayoutData->text_view_layout_item->SetParent(this);
		else
			return err;
	}

	if (unarchiver.IsInstantiated(kLabelItemField)) {
		err = unarchiver.FindObject(kLabelItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP,
			fLayoutData->label_layout_item);

		if (err == B_OK)
			fLayoutData->label_layout_item->SetParent(this);
	}
	return err;
}


void
BTextControl::SetText(const char *text)
{
	if (InvokeKind() != B_CONTROL_INVOKED)
		return;

	CALLED();

	fText->SetText(text);

	if (fText->IsFocus()) {
		fText->SetInitialText();
		fText->SelectAll();
	}

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
	fDivider = floorf(dividingLine + 0.5);

	_LayoutTextView();

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
	bool enabled = IsEnabled();
	bool active = fText->IsFocus() && Window()->IsActive();

	BRect rect = fText->Frame();
	rect.InsetBy(-2, -2);

	if (be_control_look != NULL) {
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		uint32 flags = 0;
		if (!enabled)
			flags |= BControlLook::B_DISABLED;
		if (active)
			flags |= BControlLook::B_FOCUSED;
		be_control_look->DrawTextControlBorder(this, rect, updateRect, base,
			flags);

		rect = Bounds();
		rect.right = fDivider - kLabelInputSpacing;
//		rect.right = fText->Frame().left - 2;
//		rect.right -= 3;//be_control_look->DefaultLabelSpacing();
		be_control_look->DrawLabel(this, Label(), rect, updateRect,
			base, flags, BAlignment(fLabelAlign, B_ALIGN_MIDDLE));

		return;
	}

	// outer bevel

	rgb_color noTint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lighten1 = tint_color(noTint, B_LIGHTEN_1_TINT);
	rgb_color lighten2 = tint_color(noTint, B_LIGHTEN_2_TINT);
	rgb_color lightenMax = tint_color(noTint, B_LIGHTEN_MAX_TINT);
	rgb_color darken1 = tint_color(noTint, B_DARKEN_1_TINT);
	rgb_color darken2 = tint_color(noTint, B_DARKEN_2_TINT);
	rgb_color darken4 = tint_color(noTint, B_DARKEN_4_TINT);
	rgb_color navigationColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

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
		_ValidateLayoutData();
		font_height& fontHeight = fLayoutData->font_info;

		float y = Bounds().top + (Bounds().Height() + 1 - fontHeight.ascent
			- fontHeight.descent) / 2 + fontHeight.ascent;
		float x;

		float labelWidth = StringWidth(Label());
		switch (fLabelAlign) {
			case B_ALIGN_RIGHT:
				x = fDivider - labelWidth - kLabelInputSpacing;
				break;

			case B_ALIGN_CENTER:
				x = fDivider - labelWidth / 2.0;
				break;

			default:
				x = 0.0;
				break;
		}

		BRect labelArea(x, Bounds().top, x + labelWidth, Bounds().bottom);
		if (x < fDivider && updateRect.Intersects(labelArea)) {
			labelArea.right = fText->Frame().left - kLabelInputSpacing;

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
		if (enabled)
			fText->SetFlags(fText->Flags() | B_NAVIGABLE);
		else
			fText->SetFlags(fText->Flags() & ~B_NAVIGABLE);

		_UpdateTextViewColors(enabled);

		fText->Invalidate();
		Window()->UpdateIfNeeded();
	}

	BControl::SetEnabled(enabled);
}


void
BTextControl::GetPreferredSize(float *_width, float *_height)
{
	CALLED();

	_ValidateLayoutData();

	if (_width) {
		float minWidth = fLayoutData->min.width;
		if (Label() == NULL && !(Flags() & B_SUPPORTS_LAYOUT)) {
			// Indeed, only if there is no label! BeOS backwards compatible
			// behavior:
			minWidth = max_c(minWidth, Bounds().Width());
		}
		*_width = minWidth;
	}

	if (_height)
		*_height = fLayoutData->min.height;
}


void
BTextControl::ResizeToPreferred()
{
	BView::ResizeToPreferred();

	fDivider = 0.0;
	const char* label = Label();
	if (label)
		fDivider = ceil(StringWidth(label)) + 2.0;

	_LayoutTextView();
}


void
BTextControl::SetFlags(uint32 flags)
{
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

	BView::SetFlags(flags);
}


void
BTextControl::MessageReceived(BMessage *message)
{
	if (message->what == B_GET_PROPERTY || message->what == B_SET_PROPERTY) {
		BMessage reply(B_REPLY);
		bool handled = false;

		BMessage specifier;
		int32 index;
		int32 form;
		const char *property;
		if (message->GetCurrentSpecifier(&index, &specifier, &form, &property) == B_OK) {
			if (strcmp(property, "Value") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddString("result", fText->Text());
					handled = true;
				} else {
					const char *value = NULL;
					// B_SET_PROPERTY
					if (message->FindString("data", &value) == B_OK) {
						fText->SetText(value);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			}
		}

		if (handled) {
			message->SendReply(&reply);
			return;
		}
	}

	BControl::MessageReceived(message);
}


BHandler *
BTextControl::ResolveSpecifier(BMessage *message, int32 index,
										 BMessage *specifier, int32 what,
										 const char *property)
{
	BPropertyInfo propInfo(sPropertyList);

	if (propInfo.FindMatch(message, 0, specifier, what, property) >= B_OK)
		return this;

	return BControl::ResolveSpecifier(message, index, specifier, what,
		property);
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
	CALLED();

	BControl::FrameResized(width, height);

	// TODO: this causes flickering still...

	// changes in width

	BRect bounds = Bounds();

	if (bounds.Width() > fLayoutData->previous_width) {
		// invalidate the region between the old and the new right border
		BRect rect = bounds;
		rect.left += fLayoutData->previous_width - kFrameMargin;
		rect.right--;
		Invalidate(rect);
	} else if (bounds.Width() < fLayoutData->previous_width) {
		// invalidate the region of the new right border
		BRect rect = bounds;
		rect.left = rect.right - kFrameMargin;
		Invalidate(rect);
	}

	// changes in height

	if (bounds.Height() > fLayoutData->previous_height) {
		// invalidate the region between the old and the new bottom border
		BRect rect = bounds;
		rect.top += fLayoutData->previous_height - kFrameMargin;
		rect.bottom--;
		Invalidate(rect);
		// invalidate label area
		rect = bounds;
		rect.right = fDivider;
		Invalidate(rect);
	} else if (bounds.Height() < fLayoutData->previous_height) {
		// invalidate the region of the new bottom border
		BRect rect = bounds;
		rect.top = rect.bottom - kFrameMargin;
		Invalidate(rect);
		// invalidate label area
		rect = bounds;
		rect.right = fDivider;
		Invalidate(rect);
	}

	fLayoutData->previous_width = bounds.Width();
	fLayoutData->previous_height = bounds.Height();

	TRACE("width: %.2f, height: %.2f\n", bounds.Width(), bounds.Height());
}


void
BTextControl::WindowActivated(bool active)
{
	if (fText->IsFocus()) {
		// invalidate to remove/show focus indication
		BRect rect = fText->Frame();
		rect.InsetBy(-1, -1);
		Invalidate(rect);

		// help out embedded text view which doesn't
		// get notified of this
		fText->Invalidate();
	}
}


BSize
BTextControl::MinSize()
{
	CALLED();

	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), fLayoutData->min);
}


BSize
BTextControl::MaxSize()
{
	CALLED();

	_ValidateLayoutData();

	BSize max = fLayoutData->min;
	max.width = B_SIZE_UNLIMITED;

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), max);
}


BSize
BTextControl::PreferredSize()
{
	CALLED();

	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), fLayoutData->min);
}


void
BTextControl::InvalidateLayout(bool descendants)
{
	CALLED();

	fLayoutData->valid = false;

	BView::InvalidateLayout(descendants);
}


BLayoutItem*
BTextControl::CreateLabelLayoutItem()
{
	if (!fLayoutData->label_layout_item)
		fLayoutData->label_layout_item = new LabelLayoutItem(this);
	return fLayoutData->label_layout_item;
}


BLayoutItem*
BTextControl::CreateTextViewLayoutItem()
{
	if (!fLayoutData->text_view_layout_item)
		fLayoutData->text_view_layout_item = new TextViewLayoutItem(this);
	return fLayoutData->text_view_layout_item;
}


void
BTextControl::DoLayout()
{
	// Bail out, if we shan't do layout.
	if (!(Flags() & B_SUPPORTS_LAYOUT))
		return;

	CALLED();

	// If the user set a layout, we let the base class version call its
	// hook.
	if (GetLayout()) {
		BView::DoLayout();
		return;
	}

	_ValidateLayoutData();

	// validate current size
	BSize size(Bounds().Size());
	if (size.width < fLayoutData->min.width)
		size.width = fLayoutData->min.width;
	if (size.height < fLayoutData->min.height)
		size.height = fLayoutData->min.height;

	// divider
	float divider = 0;
	if (fLayoutData->label_layout_item && fLayoutData->text_view_layout_item) {
		// We have layout items. They define the divider location.
		divider = fLayoutData->text_view_layout_item->Frame().left
			- fLayoutData->label_layout_item->Frame().left;
	} else {
		if (fLayoutData->label_width > 0)
			divider = fLayoutData->label_width + 5;
	}

	// text view
	BRect dirty(fText->Frame());
	BRect textFrame(divider + kFrameMargin, kFrameMargin,
		size.width - kFrameMargin, size.height - kFrameMargin);

	// place the text view and set the divider
	BLayoutUtils::AlignInFrame(fText, textFrame);

	fDivider = divider;

	// invalidate dirty region
	dirty = dirty | fText->Frame();
	dirty.InsetBy(-kFrameMargin, -kFrameMargin);

	Invalidate(dirty);
}


// #pragma mark -


status_t
BTextControl::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BTextControl::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BTextControl::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BTextControl::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BTextControl::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BTextControl::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BTextControl::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BTextControl::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BTextControl::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BTextControl::DoLayout();
			return B_OK;
		}
		case PERFORM_CODE_ALL_UNARCHIVED:
		{
			perform_data_all_unarchived* data
				= (perform_data_all_unarchived*)_data;

			data->return_value = BTextControl::AllUnarchived(data->archive);
			return B_OK;
		}
		case PERFORM_CODE_ALL_ARCHIVED:
		{
			perform_data_all_archived* data
				= (perform_data_all_archived*)_data;

			data->return_value = BTextControl::AllArchived(data->archive);
			return B_OK;
		}
	}

	return BControl::Perform(code, _data);
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
BTextControl::_InitData(const char* label, const BMessage* archive)
{
	BRect bounds(Bounds());

	fText = NULL;
	fModificationMessage = NULL;
	fLabelAlign = B_ALIGN_LEFT;
	fDivider = 0.0f;
	fLayoutData = new LayoutData(bounds.Width(), bounds.Height());

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

}


void
BTextControl::_InitText(const char* initialText, const BMessage* archive)
{
	if (archive)
		fText = static_cast<BPrivate::_BTextInput_*>(FindView("_input_"));

	if (fText == NULL) {
		BRect bounds(Bounds());
		BRect frame(fDivider, bounds.top, bounds.right, bounds.bottom);
		// we are stroking the frame around the text view, which
		// is 2 pixels wide
		frame.InsetBy(kFrameMargin, kFrameMargin);
		BRect textRect(frame.OffsetToCopy(B_ORIGIN));

		fText = new BPrivate::_BTextInput_(frame, textRect,
			B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS
			| (Flags() & B_NAVIGABLE));
		AddChild(fText);

		SetText(initialText);
		fText->SetAlignment(B_ALIGN_LEFT);
		fText->AlignTextRect();
	}

	// Although this is not strictly initializing the text view,
	// it cannot be done while fText is NULL, so it resides here.
	if (archive) {
		int32 labelAlignment = B_ALIGN_LEFT;
		int32 textAlignment = B_ALIGN_LEFT;

		status_t err = B_OK;
		if (archive->HasInt32("_a_label"))
			err = archive->FindInt32("_a_label", &labelAlignment);

		if (err == B_OK && archive->HasInt32("_a_text"))
			err = archive->FindInt32("_a_text", &textAlignment);

		SetAlignment((alignment)labelAlignment, (alignment)textAlignment);
	}

	uint32 navigableFlags = Flags() & B_NAVIGABLE;
	if (navigableFlags != 0)
		BView::SetFlags(Flags() & ~B_NAVIGABLE);
}


void
BTextControl::_ValidateLayout()
{
	CALLED();

	_ValidateLayoutData();

	ResizeTo(Bounds().Width(), fLayoutData->min.height);

	_LayoutTextView();
}


void
BTextControl::_LayoutTextView()
{
	CALLED();

	BRect frame = Bounds();
	frame.left = fDivider;
	// we are stroking the frame around the text view, which
	// is 2 pixels wide
	frame.InsetBy(kFrameMargin, kFrameMargin);
	fText->MoveTo(frame.left, frame.top);
	fText->ResizeTo(frame.Width(), frame.Height());
	fText->AlignTextRect();

	TRACE("width: %.2f, height: %.2f\n", Frame().Width(), Frame().Height());
	TRACE("fDivider: %.2f\n", fDivider);
	TRACE("fText frame: (%.2f, %.2f, %.2f, %.2f)\n",
		frame.left, frame.top, frame.right, frame.bottom);
}


void
BTextControl::_UpdateFrame()
{
	CALLED();

	if (fLayoutData->label_layout_item && fLayoutData->text_view_layout_item) {
		BRect labelFrame = fLayoutData->label_layout_item->Frame();
		BRect textFrame = fLayoutData->text_view_layout_item->Frame();

		// update divider
		fDivider = textFrame.left - labelFrame.left;

		MoveTo(labelFrame.left, labelFrame.top);
		BSize oldSize = Bounds().Size();
		ResizeTo(textFrame.left + textFrame.Width() - labelFrame.left,
			textFrame.top + textFrame.Height() - labelFrame.top);
		BSize newSize = Bounds().Size();

		// If the size changes, ResizeTo() will trigger a relayout, otherwise
		// we need to do that explicitly.
		if (newSize != oldSize)
			Relayout();
	}
}


void
BTextControl::_ValidateLayoutData()
{
	CALLED();

	if (fLayoutData->valid)
		return;

	// cache font height
	font_height& fh = fLayoutData->font_info;
	GetFontHeight(&fh);

	if (Label() != NULL) {
		fLayoutData->label_width = ceilf(StringWidth(Label()));
		fLayoutData->label_height = ceilf(fh.ascent) + ceilf(fh.descent);
	} else {
		fLayoutData->label_width = 0;
		fLayoutData->label_height = 0;
	}

	// compute the minimal divider
	float divider = 0;
	if (fLayoutData->label_width > 0)
		divider = fLayoutData->label_width + 5;

	// If we shan't do real layout, we let the current divider take influence.
	if (!(Flags() & B_SUPPORTS_LAYOUT))
		divider = max_c(divider, fDivider);

	// get the minimal (== preferred) text view size
	fLayoutData->text_view_min = fText->MinSize();

	TRACE("text view min width: %.2f\n", fLayoutData->text_view_min.width);

	// compute our minimal (== preferred) size
	BSize min(fLayoutData->text_view_min);
	min.width += 2 * kFrameMargin;
	min.height += 2 * kFrameMargin;

	if (divider > 0)
		min.width += divider;
	if (fLayoutData->label_height > min.height)
		min.height = fLayoutData->label_height;

	fLayoutData->min = min;

	fLayoutData->valid = true;
	ResetLayoutInvalidation();

	TRACE("width: %.2f, height: %.2f\n", min.width, min.height);
}


// #pragma mark -


BTextControl::LabelLayoutItem::LabelLayoutItem(BTextControl* parent)
	:
	fParent(parent),
	fFrame()
{
}


BTextControl::LabelLayoutItem::LabelLayoutItem(BMessage* from)
	:
	BAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
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


void
BTextControl::LabelLayoutItem::SetParent(BTextControl* parent)
{
	fParent = parent;
}


BView*
BTextControl::LabelLayoutItem::View()
{
	return fParent;
}


BSize
BTextControl::LabelLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	if (!fParent->Label())
		return BSize(-1, -1);

	return BSize(fParent->fLayoutData->label_width + 5,
		fParent->fLayoutData->label_height);
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


status_t
BTextControl::LabelLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BAbstractLayoutItem::Archive(into, deep);
	if (err == B_OK)
		err = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(err);
}


BArchivable*
BTextControl::LabelLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BTextControl::LabelLayoutItem"))
		return new LabelLayoutItem(from);
	return NULL;
}


// #pragma mark -


BTextControl::TextViewLayoutItem::TextViewLayoutItem(BTextControl* parent)
	:
	fParent(parent),
	fFrame()
{
	// by default the part right of the divider shall have an unlimited maximum
	// width
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


BTextControl::TextViewLayoutItem::TextViewLayoutItem(BMessage* from)
	:
	BAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
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


void
BTextControl::TextViewLayoutItem::SetParent(BTextControl* parent)
{
	fParent = parent;
}


BView*
BTextControl::TextViewLayoutItem::View()
{
	return fParent;
}


BSize
BTextControl::TextViewLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	BSize size = fParent->fLayoutData->text_view_min;
	size.width += 2 * kFrameMargin;
	size.height += 2 * kFrameMargin;

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


status_t
BTextControl::TextViewLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BAbstractLayoutItem::Archive(into, deep);
	if (err == B_OK)
		err = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(err);
}


BArchivable*
BTextControl::TextViewLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BTextControl::TextViewLayoutItem"))
		return new TextViewLayoutItem(from);
	return NULL;
}


