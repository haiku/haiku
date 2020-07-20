/*
 * Copyright 2001-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		John Scipione <jscipione@gmail.com>
 */


/*!	BTextControl displays text that can act like a control. */


#include <TextControl.h>

#include <string.h>

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

	{ 0 }
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

			BRect				FrameInParent() const;

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

			BRect				FrameInParent() const;

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


static const int32 kFrameMargin = 2;
static const int32 kLabelInputSpacing = 3;


// #pragma mark - BTextControl


BTextControl::BTextControl(BRect frame, const char* name, const char* label,
	const char* text, BMessage* message, uint32 resizeMask, uint32 flags)
	:
	BControl(frame, name, label, message, resizeMask, flags | B_FRAME_EVENTS)
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


//	#pragma mark - Archiving


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
BTextControl::Archive(BMessage* data, bool deep) const
{
	BArchiver archiver(data);
	status_t result = BControl::Archive(data, deep);

	alignment labelAlignment;
	alignment textAlignment;
	if (result == B_OK)
		GetAlignment(&labelAlignment, &textAlignment);

	if (result == B_OK)
		result = data->AddInt32("_a_label", labelAlignment);

	if (result == B_OK)
		result = data->AddInt32("_a_text", textAlignment);

	if (result == B_OK)
		result = data->AddFloat("_divide", Divider());

	if (result == B_OK && ModificationMessage() != NULL)
		result = data->AddMessage("_mod_msg", ModificationMessage());

	return archiver.Finish(result);
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


//	#pragma mark - Hook methods


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
BTextControl::AttachedToWindow()
{
	BControl::AttachedToWindow();

	_UpdateTextViewColors(IsEnabled());
	fText->MakeEditable(IsEnabled());
}


void
BTextControl::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}


void
BTextControl::Draw(BRect updateRect)
{
	bool enabled = IsEnabled();
	bool active = fText->IsFocus() && Window()->IsActive();

	BRect rect = fText->Frame();
	rect.InsetBy(-2, -2);

	rgb_color base = ViewColor();
	uint32 flags = fLook;
	if (!enabled)
		flags |= BControlLook::B_DISABLED;

	if (active)
		flags |= BControlLook::B_FOCUSED;

	be_control_look->DrawTextControlBorder(this, rect, updateRect, base,
		flags);

	if (Label() != NULL) {
		if (fLayoutData->label_layout_item != NULL) {
			rect = fLayoutData->label_layout_item->FrameInParent();
		} else {
			rect = Bounds();
			rect.right = fDivider - kLabelInputSpacing;
		}

		// erase the is control flag before drawing the label so that the label
		// will get drawn using B_PANEL_TEXT_COLOR
		flags &= ~BControlLook::B_IS_CONTROL;

		be_control_look->DrawLabel(this, Label(), rect, updateRect,
			base, flags, BAlignment(fLabelAlign, B_ALIGN_MIDDLE));
	}
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


status_t
BTextControl::Invoke(BMessage* message)
{
	return BControl::Invoke(message);
}


void
BTextControl::LayoutInvalidated(bool descendants)
{
	CALLED();

	fLayoutData->valid = false;
}


void
BTextControl::MessageReceived(BMessage* message)
{
	if (message->what == B_COLORS_UPDATED) {

		if (message->HasColor(ui_color_name(B_PANEL_BACKGROUND_COLOR))
			|| message->HasColor(ui_color_name(B_PANEL_TEXT_COLOR))
			|| message->HasColor(ui_color_name(B_DOCUMENT_BACKGROUND_COLOR))
			|| message->HasColor(ui_color_name(B_DOCUMENT_TEXT_COLOR))) {
			_UpdateTextViewColors(IsEnabled());
		}
	}

	if (message->what == B_GET_PROPERTY || message->what == B_SET_PROPERTY) {
		BMessage reply(B_REPLY);
		bool handled = false;

		BMessage specifier;
		int32 index;
		int32 form;
		const char* property;
		if (message->GetCurrentSpecifier(&index, &specifier, &form, &property) == B_OK) {
			if (strcmp(property, "Value") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddString("result", fText->Text());
					handled = true;
				} else {
					const char* value = NULL;
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


void
BTextControl::MouseDown(BPoint where)
{
	if (!fText->IsFocus())
		fText->MakeFocus(true);
}


void
BTextControl::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	BControl::MouseMoved(where, transit, dragMessage);
}


void
BTextControl::MouseUp(BPoint where)
{
	BControl::MouseUp(where);
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


//	#pragma mark - Getters and Setters


void
BTextControl::SetText(const char* text)
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


const char*
BTextControl::Text() const
{
	return fText->Text();
}


int32
BTextControl::TextLength() const
{
	return fText->TextLength();
}


void
BTextControl::MarkAsInvalid(bool invalid)
{
	uint32 look = fLook;

	if (invalid)
		fLook |= BControlLook::B_INVALID;
	else
		fLook &= ~BControlLook::B_INVALID;

	if (look != fLook)
		Invalidate();
}


void
BTextControl::SetValue(int32 value)
{
	BControl::SetValue(value);
}


BTextView*
BTextControl::TextView() const
{
	return fText;
}


void
BTextControl::SetModificationMessage(BMessage* message)
{
	delete fModificationMessage;
	fModificationMessage = message;
}


BMessage*
BTextControl::ModificationMessage() const
{
	return fModificationMessage;
}


void
BTextControl::SetAlignment(alignment labelAlignment, alignment textAlignment)
{
	fText->SetAlignment(textAlignment);

	if (fLabelAlign != labelAlignment) {
		fLabelAlign = labelAlignment;
		Invalidate();
	}
}


void
BTextControl::GetAlignment(alignment* _label, alignment* _text) const
{
	if (_label != NULL)
		*_label = fLabelAlign;

	if (_text != NULL)
		*_text = fText->Alignment();
}


void
BTextControl::SetDivider(float position)
{
	fDivider = floorf(position + 0.5);

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
BTextControl::MakeFocus(bool state)
{
	if (state != fText->IsFocus()) {
		fText->MakeFocus(state);

		if (state)
			fText->SelectAll();
	}
}


void
BTextControl::SetEnabled(bool enable)
{
	if (IsEnabled() == enable)
		return;

	if (Window() != NULL) {
		fText->MakeEditable(enable);
		if (enable)
			fText->SetFlags(fText->Flags() | B_NAVIGABLE);
		else
			fText->SetFlags(fText->Flags() & ~B_NAVIGABLE);

		_UpdateTextViewColors(enable);

		fText->Invalidate();
		Window()->UpdateIfNeeded();
	}

	BControl::SetEnabled(enable);
}


void
BTextControl::GetPreferredSize(float* _width, float* _height)
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


//	#pragma mark - Scripting


BHandler*
BTextControl::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	BPropertyInfo propInfo(sPropertyList);

	if (propInfo.FindMatch(message, 0, specifier, what, property) >= B_OK)
		return this;

	return BControl::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
BTextControl::GetSupportedSuites(BMessage* data)
{
	return BControl::GetSupportedSuites(data);
}


//	#pragma mark - Layout


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


BAlignment
BTextControl::LayoutAlignment()
{
	CALLED();

	_ValidateLayoutData();
	return BLayoutUtils::ComposeAlignment(ExplicitAlignment(),
		BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
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

	BRect dirty(fText->Frame());
	BRect textFrame;

	// divider
	float divider = 0;
	if (fLayoutData->text_view_layout_item != NULL) {
		if (fLayoutData->label_layout_item != NULL) {
			// We have layout items. They define the divider location.
			divider = fabs(fLayoutData->text_view_layout_item->Frame().left
				- fLayoutData->label_layout_item->Frame().left);
		}
		textFrame = fLayoutData->text_view_layout_item->FrameInParent();
	} else {
		if (fLayoutData->label_width > 0) {
			divider = fLayoutData->label_width
				+ be_control_look->DefaultLabelSpacing();
		}
		textFrame.Set(divider, 0, size.width, size.height);
	}

	// place the text view and set the divider
	textFrame.InsetBy(kFrameMargin, kFrameMargin);
	BLayoutUtils::AlignInFrame(fText, textFrame);
	fText->SetTextRect(textFrame.OffsetToCopy(B_ORIGIN));

	fDivider = divider;

	// invalidate dirty region
	dirty = dirty | fText->Frame();
	dirty.InsetBy(-kFrameMargin, -kFrameMargin);

	Invalidate(dirty);
}


// #pragma mark - protected methods


status_t
BTextControl::SetIcon(const BBitmap* icon, uint32 flags)
{
	return BControl::SetIcon(icon, flags);
}


// #pragma mark - private methods


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

		case PERFORM_CODE_SET_ICON:
		{
			perform_data_set_icon* data = (perform_data_set_icon*)_data;
			return BTextControl::SetIcon(data->icon, data->flags);
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


//	#pragma mark - FBC padding


void BTextControl::_ReservedTextControl1() {}
void BTextControl::_ReservedTextControl2() {}
void BTextControl::_ReservedTextControl3() {}
void BTextControl::_ReservedTextControl4() {}


BTextControl&
BTextControl::operator=(const BTextControl&)
{
	return *this;
}


void
BTextControl::_UpdateTextViewColors(bool enable)
{
	rgb_color textColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	rgb_color viewColor = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	BFont font;

	fText->GetFontAndColor(0, &font);

	if (!enable) {
		textColor = disable_color(textColor, ViewColor());
		viewColor = disable_color(ViewColor(), viewColor);
	}

	fText->SetFontAndColor(&font, B_FONT_ALL, &textColor);
	fText->SetViewColor(viewColor);
	fText->SetLowColor(viewColor);
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

	if (label != NULL)
		fDivider = floorf(bounds.Width() / 2.0f);

	fLook = 0;
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

	BRect frame;
	if (fLayoutData->text_view_layout_item != NULL) {
		frame = fLayoutData->text_view_layout_item->FrameInParent();
	} else {
		frame = Bounds();
		frame.left = fDivider;
	}

	// we are stroking the frame around the text view, which
	// is 2 pixels wide
	frame.InsetBy(kFrameMargin, kFrameMargin);
	fText->MoveTo(frame.left, frame.top);
	fText->ResizeTo(frame.Width(), frame.Height());

	TRACE("width: %.2f, height: %.2f\n", Frame().Width(), Frame().Height());
	TRACE("fDivider: %.2f\n", fDivider);
	TRACE("fText frame: (%.2f, %.2f, %.2f, %.2f)\n",
		frame.left, frame.top, frame.right, frame.bottom);
}


void
BTextControl::_UpdateFrame()
{
	CALLED();

	if (fLayoutData->text_view_layout_item != NULL) {
		BRect textFrame = fLayoutData->text_view_layout_item->Frame();
		BRect labelFrame;
		if (fLayoutData->label_layout_item != NULL)
			labelFrame = fLayoutData->label_layout_item->Frame();

		BRect frame;
		if (labelFrame.IsValid()) {
			frame = textFrame | labelFrame;

			// update divider
			fDivider = fabs(textFrame.left - labelFrame.left);
		} else {
			frame = textFrame;
			fDivider = 0;
		}

		// update our frame
		MoveTo(frame.left, frame.top);
		BSize oldSize(Bounds().Size());
		ResizeTo(frame.Width(), frame.Height());
		BSize newSize(Bounds().Size());

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

	const char* label = Label();
	if (label != NULL) {
		fLayoutData->label_width = ceilf(StringWidth(label));
		fLayoutData->label_height = ceilf(fh.ascent) + ceilf(fh.descent);
	} else {
		fLayoutData->label_width = 0;
		fLayoutData->label_height = 0;
	}

	// compute the minimal divider
	float divider = 0;
	if (fLayoutData->label_width > 0) {
		divider = fLayoutData->label_width
			+ be_control_look->DefaultLabelSpacing();
	}

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


// #pragma mark - BTextControl::LabelLayoutItem


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

	return BSize(fParent->fLayoutData->label_width
			+ be_control_look->DefaultLabelSpacing(),
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


BRect
BTextControl::LabelLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
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


// #pragma mark - BTextControl::TextViewLayoutItem


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


BRect
BTextControl::TextViewLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
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


extern "C" void
B_IF_GCC_2(InvalidateLayout__12BTextControlb,
	_ZN12BTextControl16InvalidateLayoutEb)(BView* view, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}
