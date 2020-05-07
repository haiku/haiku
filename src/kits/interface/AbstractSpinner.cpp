/*
 * Copyright 2004 DarkWyrm <darkwyrm@earthlink.net>
 * Copyright 2013 FeemanLou
 * Copyright 2014-2015 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT license.
 *
 * Originally written by DarkWyrm <darkwyrm@earthlink.net>
 * Updated by FreemanLou as part of Google GCI 2013
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		FeemanLou
 *		John Scipione, jscipione@gmail.com
 */


#include <AbstractSpinner.h>

#include <algorithm>

#include <AbstractLayoutItem.h>
#include <Alignment.h>
#include <ControlLook.h>
#include <Font.h>
#include <GradientLinear.h>
#include <LayoutItem.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <Point.h>
#include <PropertyInfo.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>


static const float kFrameMargin			= 2.0f;

const char* const kFrameField			= "BAbstractSpinner:layoutItem:frame";
const char* const kLabelItemField		= "BAbstractSpinner:labelItem";
const char* const kTextViewItemField	= "BAbstractSpinner:textViewItem";


static property_info sProperties[] = {
	{
		"Align",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the alignment of the spinner label.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"Align",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the alignment of the spinner label.",
		0,
		{ B_INT32_TYPE }
	},

	{
		"ButtonStyle",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the style of the spinner buttons.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"ButtonStyle",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the style of the spinner buttons.",
		0,
		{ B_INT32_TYPE }
	},

	{
		"Divider",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the divider position of the spinner.",
		0,
		{ B_FLOAT_TYPE }
	},
	{
		"Divider",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the divider position of the spinner.",
		0,
		{ B_FLOAT_TYPE }
	},

	{
		"Enabled",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns whether or not the spinner is enabled.",
		0,
		{ B_BOOL_TYPE }
	},
	{
		"Enabled",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets whether or not the spinner is enabled.",
		0,
		{ B_BOOL_TYPE }
	},

	{
		"Label",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the spinner label.",
		0,
		{ B_STRING_TYPE }
	},
	{
		"Label",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the spinner label.",
		0,
		{ B_STRING_TYPE }
	},

	{
		"Message",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the spinner invocation message.",
		0,
		{ B_MESSAGE_TYPE }
	},
	{
		"Message",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the spinner invocation message.",
		0,
		{ B_MESSAGE_TYPE }
	},

	{ 0 }
};


typedef enum {
	SPINNER_INCREMENT,
	SPINNER_DECREMENT
} spinner_direction;


class SpinnerButton : public BView {
public:
								SpinnerButton(BRect frame, const char* name,
									spinner_direction direction);
	virtual						~SpinnerButton();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);
	virtual void				MessageReceived(BMessage* message);

			bool				IsEnabled() const { return fIsEnabled; }
	virtual	void				SetEnabled(bool enable) { fIsEnabled = enable; };

private:
			spinner_direction	fSpinnerDirection;
			BAbstractSpinner*	fParent;
			bool				fIsEnabled;
			bool				fIsMouseDown;
			bool				fIsMouseOver;
			BMessageRunner*		fRepeater;
};


class SpinnerTextView : public BTextView {
public:
								SpinnerTextView(BRect rect, BRect textRect);
	virtual						~SpinnerTextView();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MakeFocus(bool focus);

private:
			BAbstractSpinner*	fParent;
};


class BAbstractSpinner::LabelLayoutItem : public BAbstractLayoutItem {
public:
								LabelLayoutItem(BAbstractSpinner* parent);
								LabelLayoutItem(BMessage* archive);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(BAbstractSpinner* parent);
	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

			BRect				FrameInParent() const;

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			BAbstractSpinner*	fParent;
			BRect				fFrame;
};


class BAbstractSpinner::TextViewLayoutItem : public BAbstractLayoutItem {
public:
								TextViewLayoutItem(BAbstractSpinner* parent);
								TextViewLayoutItem(BMessage* archive);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(BAbstractSpinner* parent);
	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

			BRect				FrameInParent() const;

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			BAbstractSpinner*	fParent;
			BRect				fFrame;
};


struct BAbstractSpinner::LayoutData {
	LayoutData(float width, float height)
	:
	label_layout_item(NULL),
	text_view_layout_item(NULL),
	label_width(0),
	label_height(0),
	text_view_width(0),
	text_view_height(0),
	previous_width(width),
	previous_height(height),
	valid(false)
	{
	}

	LabelLayoutItem* label_layout_item;
	TextViewLayoutItem* text_view_layout_item;

	font_height font_info;

	float label_width;
	float label_height;
	float text_view_width;
	float text_view_height;

	float previous_width;
	float previous_height;

	BSize min;
	BAlignment alignment;

	bool valid;
};


//	#pragma mark - SpinnerButton


SpinnerButton::SpinnerButton(BRect frame, const char* name,
	spinner_direction direction)
	:
	BView(frame, name, B_FOLLOW_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW),
	fSpinnerDirection(direction),
	fParent(NULL),
	fIsEnabled(true),
	fIsMouseDown(false),
	fIsMouseOver(false),
	fRepeater(NULL)
{
}


SpinnerButton::~SpinnerButton()
{
	delete fRepeater;
}


void
SpinnerButton::AttachedToWindow()
{
	fParent = static_cast<BAbstractSpinner*>(Parent());

	AdoptParentColors();
	BView::AttachedToWindow();
}


void
SpinnerButton::DetachedFromWindow()
{
	fParent = NULL;

	BView::DetachedFromWindow();
}


void
SpinnerButton::Draw(BRect updateRect)
{
	BRect rect(Bounds());
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	BView::Draw(updateRect);

	float frameTint = fIsEnabled ? B_DARKEN_1_TINT : B_NO_TINT;

	float fgTint;
	if (!fIsEnabled)
		fgTint = B_DARKEN_1_TINT;
	else if (fIsMouseDown)
		fgTint = B_DARKEN_MAX_TINT;
	else
		fgTint = 1.777f;	// 216 --> 48.2 (48)

	float bgTint;
	if (fIsEnabled && fIsMouseOver)
		bgTint = B_DARKEN_1_TINT;
	else
		bgTint = B_NO_TINT;

	rgb_color bgColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	if (bgColor.red + bgColor.green + bgColor.blue <= 128 * 3) {
		// if dark background make the tint lighter
		frameTint = 2.0f - frameTint;
		fgTint = 2.0f - fgTint;
		bgTint = 2.0f - bgTint;
	}

	uint32 borders = be_control_look->B_TOP_BORDER
		| be_control_look->B_BOTTOM_BORDER;

	if (fSpinnerDirection == SPINNER_INCREMENT)
		borders |= be_control_look->B_RIGHT_BORDER;
	else
		borders |= be_control_look->B_LEFT_BORDER;

	uint32 flags = fIsMouseDown ? BControlLook::B_ACTIVATED : 0;
	flags |= !fIsEnabled ? BControlLook::B_DISABLED : 0;

	// draw the button
	be_control_look->DrawButtonFrame(this, rect, updateRect,
		tint_color(bgColor, frameTint), bgColor, flags, borders);
	be_control_look->DrawButtonBackground(this, rect, updateRect,
		tint_color(bgColor, bgTint), flags, borders);

	switch (fParent->ButtonStyle()) {
		case SPINNER_BUTTON_HORIZONTAL_ARROWS:
		{
			int32 arrowDirection = fSpinnerDirection == SPINNER_INCREMENT
				? be_control_look->B_RIGHT_ARROW
				: be_control_look->B_LEFT_ARROW;

			rect.InsetBy(0.0f, 1.0f);
			be_control_look->DrawArrowShape(this, rect, updateRect, bgColor,
				arrowDirection, 0, fgTint);
			break;
		}

		case SPINNER_BUTTON_VERTICAL_ARROWS:
		{
			int32 arrowDirection = fSpinnerDirection == SPINNER_INCREMENT
				? be_control_look->B_UP_ARROW
				: be_control_look->B_DOWN_ARROW;

			rect.InsetBy(0.0f, 1.0f);
			be_control_look->DrawArrowShape(this, rect, updateRect, bgColor,
				arrowDirection, 0, fgTint);
			break;
		}

		default:
		case SPINNER_BUTTON_PLUS_MINUS:
		{
			BFont font;
			fParent->GetFont(&font);
			float inset = floorf(font.Size() / 4);
			rect.InsetBy(inset, inset);

			if (rect.IntegerWidth() % 2 != 0)
				rect.right -= 1;

			if (rect.IntegerHeight() % 2 != 0)
				rect.bottom -= 1;

			SetHighColor(tint_color(bgColor, fgTint));

			// draw the +/-
			float halfHeight = floorf(rect.Height() / 2);
			StrokeLine(BPoint(rect.left, rect.top + halfHeight),
				BPoint(rect.right, rect.top + halfHeight));
			if (fSpinnerDirection == SPINNER_INCREMENT) {
				float halfWidth = floorf(rect.Width() / 2);
				StrokeLine(BPoint(rect.left + halfWidth, rect.top + 1),
					BPoint(rect.left + halfWidth, rect.bottom - 1));
			}
		}
	}
}


void
SpinnerButton::MouseDown(BPoint where)
{
	if (fIsEnabled) {
		fIsMouseDown = true;
		fSpinnerDirection == SPINNER_INCREMENT
			? fParent->Increment()
			: fParent->Decrement();
		Invalidate();
		BMessage repeatMessage('rept');
		SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		fRepeater = new BMessageRunner(BMessenger(this), repeatMessage,
			200000);
	}

	BView::MouseDown(where);
}


void
SpinnerButton::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message)
{
	switch (transit) {
		case B_ENTERED_VIEW:
		case B_INSIDE_VIEW:
		{
			BPoint where;
			uint32 buttons;
			GetMouse(&where, &buttons);
			fIsMouseOver = Bounds().Contains(where) && buttons == 0;

			break;
		}

		case B_EXITED_VIEW:
		case B_OUTSIDE_VIEW:
			fIsMouseOver = false;
			MouseUp(Bounds().LeftTop());
			break;
	}

	BView::MouseMoved(where, transit, message);
}


void
SpinnerButton::MouseUp(BPoint where)
{
	fIsMouseDown = false;
	delete fRepeater;
	fRepeater = NULL;
	Invalidate();

	BView::MouseUp(where);
}


void
SpinnerButton::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case 'rept':
		{
			if (fIsMouseDown && fRepeater != NULL) {
				fSpinnerDirection == SPINNER_INCREMENT
					? fParent->Increment()
					: fParent->Decrement();
			}

			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


//	#pragma mark - SpinnerTextView


SpinnerTextView::SpinnerTextView(BRect rect, BRect textRect)
	:
	BTextView(rect, "textview", textRect, B_FOLLOW_ALL,
		B_WILL_DRAW | B_NAVIGABLE),
	fParent(NULL)
{
	MakeResizable(true);
}


SpinnerTextView::~SpinnerTextView()
{
}


void
SpinnerTextView::AttachedToWindow()
{
	fParent = static_cast<BAbstractSpinner*>(Parent());

	BTextView::AttachedToWindow();
}


void
SpinnerTextView::DetachedFromWindow()
{
	fParent = NULL;

	BTextView::DetachedFromWindow();
}


void
SpinnerTextView::KeyDown(const char* bytes, int32 numBytes)
{
	if (fParent == NULL) {
		BTextView::KeyDown(bytes, numBytes);
		return;
	}

	switch (bytes[0]) {
		case B_ENTER:
		case B_SPACE:
			fParent->SetValueFromText();
			break;

		case B_TAB:
			fParent->KeyDown(bytes, numBytes);
			break;

		case B_LEFT_ARROW:
			if (fParent->ButtonStyle() == SPINNER_BUTTON_HORIZONTAL_ARROWS
				&& (modifiers() & B_CONTROL_KEY) != 0) {
				// need to hold down control, otherwise can't move cursor
				fParent->Decrement();
			} else
				BTextView::KeyDown(bytes, numBytes);
			break;

		case B_UP_ARROW:
			if (fParent->ButtonStyle() != SPINNER_BUTTON_HORIZONTAL_ARROWS)
				fParent->Increment();
			else
				BTextView::KeyDown(bytes, numBytes);
			break;

		case B_RIGHT_ARROW:
			if (fParent->ButtonStyle() == SPINNER_BUTTON_HORIZONTAL_ARROWS
				&& (modifiers() & B_CONTROL_KEY) != 0) {
				// need to hold down control, otherwise can't move cursor
				fParent->Increment();
			} else
				BTextView::KeyDown(bytes, numBytes);
			break;

		case B_DOWN_ARROW:
			if (fParent->ButtonStyle() != SPINNER_BUTTON_HORIZONTAL_ARROWS)
				fParent->Decrement();
			else
				BTextView::KeyDown(bytes, numBytes);
			break;

		default:
			BTextView::KeyDown(bytes, numBytes);
			break;
	}
}


void
SpinnerTextView::MakeFocus(bool focus)
{
	BTextView::MakeFocus(focus);

	if (fParent == NULL)
		return;

	if (focus)
		SelectAll();
	else
		fParent->SetValueFromText();

	fParent->_DrawTextView(fParent->Bounds());
}


//	#pragma mark - BAbstractSpinner::LabelLayoutItem


BAbstractSpinner::LabelLayoutItem::LabelLayoutItem(BAbstractSpinner* parent)
	:
	fParent(parent),
	fFrame()
{
}


BAbstractSpinner::LabelLayoutItem::LabelLayoutItem(BMessage* from)
	:
	BAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
}


bool
BAbstractSpinner::LabelLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
BAbstractSpinner::LabelLayoutItem::SetVisible(bool visible)
{
}


BRect
BAbstractSpinner::LabelLayoutItem::Frame()
{
	return fFrame;
}


void
BAbstractSpinner::LabelLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


void
BAbstractSpinner::LabelLayoutItem::SetParent(BAbstractSpinner* parent)
{
	fParent = parent;
}


BView*
BAbstractSpinner::LabelLayoutItem::View()
{
	return fParent;
}


BSize
BAbstractSpinner::LabelLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	if (fParent->Label() == NULL)
		return BSize(-1.0f, -1.0f);

	return BSize(fParent->fLayoutData->label_width
			+ be_control_look->DefaultLabelSpacing(),
		fParent->fLayoutData->label_height);
}


BSize
BAbstractSpinner::LabelLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
BAbstractSpinner::LabelLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
BAbstractSpinner::LabelLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


BRect
BAbstractSpinner::LabelLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
}


status_t
BAbstractSpinner::LabelLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t result = BAbstractLayoutItem::Archive(into, deep);

	if (result == B_OK)
		result = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(result);
}


BArchivable*
BAbstractSpinner::LabelLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BAbstractSpinner::LabelLayoutItem"))
		return new LabelLayoutItem(from);

	return NULL;
}


//	#pragma mark - BAbstractSpinner::TextViewLayoutItem


BAbstractSpinner::TextViewLayoutItem::TextViewLayoutItem(BAbstractSpinner* parent)
	:
	fParent(parent),
	fFrame()
{
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


BAbstractSpinner::TextViewLayoutItem::TextViewLayoutItem(BMessage* from)
	:
	BAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
}


bool
BAbstractSpinner::TextViewLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
BAbstractSpinner::TextViewLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
BAbstractSpinner::TextViewLayoutItem::Frame()
{
	return fFrame;
}


void
BAbstractSpinner::TextViewLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


void
BAbstractSpinner::TextViewLayoutItem::SetParent(BAbstractSpinner* parent)
{
	fParent = parent;
}


BView*
BAbstractSpinner::TextViewLayoutItem::View()
{
	return fParent;
}


BSize
BAbstractSpinner::TextViewLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	BSize size(fParent->fLayoutData->text_view_width,
		fParent->fLayoutData->text_view_height);

	return size;
}


BSize
BAbstractSpinner::TextViewLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
BAbstractSpinner::TextViewLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
BAbstractSpinner::TextViewLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


BRect
BAbstractSpinner::TextViewLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
}


status_t
BAbstractSpinner::TextViewLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t result = BAbstractLayoutItem::Archive(into, deep);

	if (result == B_OK)
		result = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(result);
}


BArchivable*
BAbstractSpinner::TextViewLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BAbstractSpinner::TextViewLayoutItem"))
		return new LabelLayoutItem(from);

	return NULL;
}


//	#pragma mark - BAbstractSpinner


BAbstractSpinner::BAbstractSpinner(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	BControl(frame, name, label, message, resizingMode,
		flags | B_WILL_DRAW | B_FRAME_EVENTS)
{
	_InitObject();
}


BAbstractSpinner::BAbstractSpinner(const char* name, const char* label, BMessage* message,
	uint32 flags)
	:
	BControl(name, label, message, flags | B_WILL_DRAW | B_FRAME_EVENTS)
{
	_InitObject();
}


BAbstractSpinner::BAbstractSpinner(BMessage* data)
	:
	BControl(data),
	fButtonStyle(SPINNER_BUTTON_PLUS_MINUS)
{
	_InitObject();

	if (data->FindInt32("_align") != B_OK)
		fAlignment = B_ALIGN_LEFT;

	if (data->FindInt32("_button_style") != B_OK)
		fButtonStyle = SPINNER_BUTTON_PLUS_MINUS;

	if (data->FindInt32("_divider") != B_OK)
		fDivider = 0.0f;
}


BAbstractSpinner::~BAbstractSpinner()
{
	delete fLayoutData;
	fLayoutData = NULL;
}


BArchivable*
BAbstractSpinner::Instantiate(BMessage* data)
{
	// cannot instantiate an abstract spinner
	return NULL;
}


status_t
BAbstractSpinner::Archive(BMessage* data, bool deep) const
{
	status_t status = BControl::Archive(data, deep);
	data->AddString("class", "Spinner");

	if (status == B_OK)
		status = data->AddInt32("_align", fAlignment);

	if (status == B_OK)
		data->AddInt32("_button_style", fButtonStyle);

	if (status == B_OK)
		status = data->AddFloat("_divider", fDivider);

	return status;
}


status_t
BAbstractSpinner::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Haiku-spinner");

	BPropertyInfo prop_info(sProperties);
	message->AddFlat("messages", &prop_info);

	return BView::GetSupportedSuites(message);
}


BHandler*
BAbstractSpinner::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return BView::ResolveSpecifier(message, index, specifier, form,
		property);
}


void
BAbstractSpinner::AttachedToWindow()
{
	if (!Messenger().IsValid())
		SetTarget(Window());

	BControl::SetValue(Value());
		// sets the text and enables or disables the arrows

	_UpdateTextViewColors(IsEnabled());
	fTextView->MakeEditable(IsEnabled());

	BView::AttachedToWindow();
}


void
BAbstractSpinner::Draw(BRect updateRect)
{
	_DrawLabel(updateRect);
	_DrawTextView(updateRect);
	fIncrement->Invalidate();
	fDecrement->Invalidate();
}


void
BAbstractSpinner::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);

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
}


void
BAbstractSpinner::ValueChanged()
{
	// hook method - does nothing
}


void
BAbstractSpinner::MessageReceived(BMessage* message)
{
	if (!IsEnabled() && message->what == B_COLORS_UPDATED)
		_UpdateTextViewColors(false);

	BControl::MessageReceived(message);
}


void
BAbstractSpinner::MakeFocus(bool focus)
{
	fTextView->MakeFocus(focus);
}


void
BAbstractSpinner::ResizeToPreferred()
{
	BView::ResizeToPreferred();

	const char* label = Label();
	if (label != NULL) {
		fDivider = ceilf(StringWidth(label))
			+ be_control_look->DefaultLabelSpacing();
	} else
		fDivider = 0.0f;

	_LayoutTextView();
}


void
BAbstractSpinner::SetFlags(uint32 flags)
{
	// If the textview is navigable, set it to not navigable if needed,
	// else if it is not navigable, set it to navigable if needed
	if (fTextView->Flags() & B_NAVIGABLE) {
		if (!(flags & B_NAVIGABLE))
			fTextView->SetFlags(fTextView->Flags() & ~B_NAVIGABLE);
	} else {
		if (flags & B_NAVIGABLE)
			fTextView->SetFlags(fTextView->Flags() | B_NAVIGABLE);
	}

	// Don't make this one navigable
	flags &= ~B_NAVIGABLE;

	BView::SetFlags(flags);
}


void
BAbstractSpinner::WindowActivated(bool active)
{
	_DrawTextView(fTextView->Frame());
}


void
BAbstractSpinner::SetAlignment(alignment align)
{
	fAlignment = align;
}


void
BAbstractSpinner::SetButtonStyle(spinner_button_style buttonStyle)
{
	fButtonStyle = buttonStyle;
}


void
BAbstractSpinner::SetDivider(float position)
{
	position = roundf(position);

	float delta = fDivider - position;
	if (delta == 0.0f)
		return;

	fDivider = position;

	if ((Flags() & B_SUPPORTS_LAYOUT) != 0) {
		// We should never get here, since layout support means, we also
		// layout the divider, and don't use this method at all.
		Relayout();
	} else {
		_LayoutTextView();
		Invalidate();
	}
}


void
BAbstractSpinner::SetEnabled(bool enable)
{
	if (IsEnabled() == enable)
		return;

	BControl::SetEnabled(enable);

	fTextView->MakeEditable(enable);
	if (enable)
		fTextView->SetFlags(fTextView->Flags() | B_NAVIGABLE);
	else
		fTextView->SetFlags(fTextView->Flags() & ~B_NAVIGABLE);

	_UpdateTextViewColors(enable);
	fTextView->Invalidate();

	_LayoutTextView();
	Invalidate();
	if (Window() != NULL)
		Window()->UpdateIfNeeded();
}


void
BAbstractSpinner::SetLabel(const char* label)
{
	BControl::SetLabel(label);

	if (Window() != NULL)
		Window()->UpdateIfNeeded();
}


bool
BAbstractSpinner::IsDecrementEnabled() const
{
	return fDecrement->IsEnabled();
}


void
BAbstractSpinner::SetDecrementEnabled(bool enable)
{
	if (IsDecrementEnabled() == enable)
		return;

	fDecrement->SetEnabled(enable);
	fDecrement->Invalidate();
}


bool
BAbstractSpinner::IsIncrementEnabled() const
{
	return fIncrement->IsEnabled();
}


void
BAbstractSpinner::SetIncrementEnabled(bool enable)
{
	if (IsIncrementEnabled() == enable)
		return;

	fIncrement->SetEnabled(enable);
	fIncrement->Invalidate();
}


BSize
BAbstractSpinner::MinSize()
{
	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), fLayoutData->min);
}


BSize
BAbstractSpinner::MaxSize()
{
	_ValidateLayoutData();

	BSize max = fLayoutData->min;
	max.width = B_SIZE_UNLIMITED;

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), max);
}


BSize
BAbstractSpinner::PreferredSize()
{
	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		fLayoutData->min);
}


BAlignment
BAbstractSpinner::LayoutAlignment()
{
	_ValidateLayoutData();
	return BLayoutUtils::ComposeAlignment(ExplicitAlignment(),
		BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
}


BLayoutItem*
BAbstractSpinner::CreateLabelLayoutItem()
{
	if (fLayoutData->label_layout_item == NULL)
		fLayoutData->label_layout_item = new LabelLayoutItem(this);

	return fLayoutData->label_layout_item;
}


BLayoutItem*
BAbstractSpinner::CreateTextViewLayoutItem()
{
	if (fLayoutData->text_view_layout_item == NULL)
		fLayoutData->text_view_layout_item = new TextViewLayoutItem(this);

	return fLayoutData->text_view_layout_item;
}


BTextView*
BAbstractSpinner::TextView() const
{
	return dynamic_cast<BTextView*>(fTextView);
}


//	#pragma mark - BAbstractSpinner protected methods


status_t
BAbstractSpinner::AllArchived(BMessage* into) const
{
	status_t result;
	if ((result = BControl::AllArchived(into)) != B_OK)
		return result;

	BArchiver archiver(into);

	BArchivable* textViewItem = fLayoutData->text_view_layout_item;
	if (archiver.IsArchived(textViewItem))
		result = archiver.AddArchivable(kTextViewItemField, textViewItem);

	if (result != B_OK)
		return result;

	BArchivable* labelBarItem = fLayoutData->label_layout_item;
	if (archiver.IsArchived(labelBarItem))
		result = archiver.AddArchivable(kLabelItemField, labelBarItem);

	return result;
}


status_t
BAbstractSpinner::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);

	status_t result = B_OK;
	if ((result = BControl::AllUnarchived(from)) != B_OK)
		return result;

	if (unarchiver.IsInstantiated(kTextViewItemField)) {
		TextViewLayoutItem*& textViewItem
			= fLayoutData->text_view_layout_item;
		result = unarchiver.FindObject(kTextViewItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, textViewItem);

		if (result == B_OK)
			textViewItem->SetParent(this);
		else
			return result;
	}

	if (unarchiver.IsInstantiated(kLabelItemField)) {
		LabelLayoutItem*& labelItem = fLayoutData->label_layout_item;
		result = unarchiver.FindObject(kLabelItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, labelItem);

		if (result == B_OK)
			labelItem->SetParent(this);
	}

	return result;
}


void
BAbstractSpinner::DoLayout()
{
	if ((Flags() & B_SUPPORTS_LAYOUT) == 0)
		return;

	if (GetLayout()) {
		BControl::DoLayout();
		return;
	}

	_ValidateLayoutData();

	BSize size(Bounds().Size());
	if (size.width < fLayoutData->min.width)
		size.width = fLayoutData->min.width;

	if (size.height < fLayoutData->min.height)
		size.height = fLayoutData->min.height;

	float divider = 0;
	if (fLayoutData->label_layout_item != NULL
		&& fLayoutData->text_view_layout_item != NULL
		&& fLayoutData->label_layout_item->Frame().IsValid()
		&& fLayoutData->text_view_layout_item->Frame().IsValid()) {
		divider = fLayoutData->text_view_layout_item->Frame().left
			- fLayoutData->label_layout_item->Frame().left;
	} else if (fLayoutData->label_width > 0) {
		divider = fLayoutData->label_width
			+ be_control_look->DefaultLabelSpacing();
	}
	fDivider = divider;

	BRect dirty(fTextView->Frame());
	_LayoutTextView();

	// invalidate dirty region
	dirty = dirty | fTextView->Frame();
	dirty = dirty | fIncrement->Frame();
	dirty = dirty | fDecrement->Frame();

	Invalidate(dirty);
}


void
BAbstractSpinner::LayoutInvalidated(bool descendants)
{
	if (fLayoutData != NULL)
		fLayoutData->valid = false;
}


//	#pragma mark - BAbstractSpinner private methods


void
BAbstractSpinner::_DrawLabel(BRect updateRect)
{
	BRect rect(Bounds());
	rect.right = fDivider;
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	_ValidateLayoutData();

	const char* label = Label();
	if (label == NULL)
		return;

	// horizontal position
	float x;
	switch (fAlignment) {
		case B_ALIGN_RIGHT:
			x = fDivider - fLayoutData->label_width - 3.0f;
			break;

		case B_ALIGN_CENTER:
			x = fDivider - roundf(fLayoutData->label_width / 2.0f);
			break;

		default:
			x = 0.0f;
			break;
	}

	// vertical position
	font_height& fontHeight = fLayoutData->font_info;
	float y = rect.top
		+ roundf((rect.Height() + 1.0f - fontHeight.ascent
			- fontHeight.descent) / 2.0f)
		+ fontHeight.ascent;

	uint32 flags = be_control_look->Flags(this);

	// erase the is control flag before drawing the label so that the label
	// will get drawn using B_PANEL_TEXT_COLOR.
	flags &= ~BControlLook::B_IS_CONTROL;

	be_control_look->DrawLabel(this, label, LowColor(), flags, BPoint(x, y));
}


void
BAbstractSpinner::_DrawTextView(BRect updateRect)
{
	BRect rect = fTextView->Frame();
	rect.InsetBy(-kFrameMargin, -kFrameMargin);
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = 0;
	if (!IsEnabled())
		flags |= BControlLook::B_DISABLED;

	if (fTextView->IsFocus() && Window()->IsActive())
		flags |= BControlLook::B_FOCUSED;

	be_control_look->DrawTextControlBorder(this, rect, updateRect, base,
		flags);
}


void
BAbstractSpinner::_InitObject()
{
	fAlignment = B_ALIGN_LEFT;
	fButtonStyle = SPINNER_BUTTON_PLUS_MINUS;

	if (Label() != NULL) {
		fDivider = StringWidth(Label())
			+ be_control_look->DefaultLabelSpacing();
	} else
		fDivider = 0.0f;

	BControl::SetEnabled(true);
	BControl::SetValue(0);

	BRect rect(Bounds());
	fLayoutData = new LayoutData(rect.Width(), rect.Height());

	rect.left = fDivider;
	rect.InsetBy(kFrameMargin, kFrameMargin);
	rect.right -= rect.Height() * 2 + kFrameMargin * 2 + 1.0f;
	BRect textRect(rect.OffsetToCopy(B_ORIGIN));

	fTextView = new SpinnerTextView(rect, textRect);
	AddChild(fTextView);

	rect.InsetBy(0.0f, -kFrameMargin);

	rect.left = rect.right + kFrameMargin * 2;
	rect.right = rect.left + rect.Height() - kFrameMargin * 2;

	fDecrement = new SpinnerButton(rect, "decrement", SPINNER_DECREMENT);
	AddChild(fDecrement);

	rect.left = rect.right + 1.0f;
	rect.right = rect.left + rect.Height() - kFrameMargin * 2;

	fIncrement = new SpinnerButton(rect, "increment", SPINNER_INCREMENT);
	AddChild(fIncrement);

	uint32 navigableFlags = Flags() & B_NAVIGABLE;
	if (navigableFlags != 0)
		BControl::SetFlags(Flags() & ~B_NAVIGABLE);
}


void
BAbstractSpinner::_LayoutTextView()
{
	BRect rect;
	if (fLayoutData->text_view_layout_item != NULL) {
		rect = fLayoutData->text_view_layout_item->FrameInParent();
	} else {
		rect = Bounds();
		rect.left = fDivider;
	}
	rect.InsetBy(kFrameMargin, kFrameMargin);
	rect.right -= rect.Height() * 2 + kFrameMargin * 2 + 1.0f;

	fTextView->MoveTo(rect.left, rect.top);
	fTextView->ResizeTo(rect.Width(), rect.Height());
	fTextView->SetTextRect(rect.OffsetToCopy(B_ORIGIN));

	rect.InsetBy(0.0f, -kFrameMargin);

	rect.left = rect.right + kFrameMargin * 2;
	rect.right = rect.left + rect.Height() - kFrameMargin * 2;

	fDecrement->ResizeTo(rect.Width(), rect.Height());
	fDecrement->MoveTo(rect.LeftTop());

	rect.left = rect.right + 1.0f;
	rect.right = rect.left + rect.Height() - kFrameMargin * 2;

	fIncrement->ResizeTo(rect.Width(), rect.Height());
	fIncrement->MoveTo(rect.LeftTop());
}


void
BAbstractSpinner::_UpdateFrame()
{
	if (fLayoutData->label_layout_item == NULL
		|| fLayoutData->text_view_layout_item == NULL) {
		return;
	}

	BRect labelFrame = fLayoutData->label_layout_item->Frame();
	BRect textViewFrame = fLayoutData->text_view_layout_item->Frame();

	if (!labelFrame.IsValid() || !textViewFrame.IsValid())
		return;

	// update divider
	fDivider = textViewFrame.left - labelFrame.left;

	BRect frame = textViewFrame | labelFrame;
	MoveTo(frame.left, frame.top);
	BSize oldSize = Bounds().Size();
	ResizeTo(frame.Width(), frame.Height());
	BSize newSize = Bounds().Size();

	// If the size changes, ResizeTo() will trigger a relayout, otherwise
	// we need to do that explicitly.
	if (newSize != oldSize)
		Relayout();
}


void
BAbstractSpinner::_UpdateTextViewColors(bool enable)
{
	// Mimick BTextControl's appearance.
	rgb_color textColor = ui_color(B_DOCUMENT_TEXT_COLOR);

	if (enable) {
		fTextView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
		fTextView->SetLowUIColor(ViewUIColor());
	} else {
		rgb_color color = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
		color = disable_color(ViewColor(), color);
		textColor = disable_color(textColor, ViewColor());

		fTextView->SetViewColor(color);
		fTextView->SetLowColor(color);
	}

	BFont font;
	fTextView->GetFontAndColor(0, &font);
	fTextView->SetFontAndColor(&font, B_FONT_ALL, &textColor);
}


void
BAbstractSpinner::_ValidateLayoutData()
{
	if (fLayoutData->valid)
		return;

	font_height& fontHeight = fLayoutData->font_info;
	GetFontHeight(&fontHeight);

	if (Label() != NULL) {
		fLayoutData->label_width = StringWidth(Label());
		fLayoutData->label_height = ceilf(fontHeight.ascent
			+ fontHeight.descent + fontHeight.leading);
	} else {
		fLayoutData->label_width = 0;
		fLayoutData->label_height = 0;
	}

	float divider = 0;
	if (fLayoutData->label_width > 0) {
		divider = ceilf(fLayoutData->label_width
			+ be_control_look->DefaultLabelSpacing());
	}

	if ((Flags() & B_SUPPORTS_LAYOUT) == 0)
		divider = std::max(divider, fDivider);

	float minTextWidth = fTextView->StringWidth("99999");

	float textViewHeight = fTextView->LineHeight(0) + kFrameMargin * 2;
	float textViewWidth = minTextWidth + textViewHeight * 2;

	fLayoutData->text_view_width = textViewWidth;
	fLayoutData->text_view_height = textViewHeight;

	BSize min(textViewWidth, textViewHeight);
	if (divider > 0.0f)
		min.width += divider;

	if (fLayoutData->label_height > min.height)
		min.height = fLayoutData->label_height;

	fLayoutData->min = min;
	fLayoutData->valid = true;

	ResetLayoutInvalidation();
}


// FBC padding

void BAbstractSpinner::_ReservedAbstractSpinner20() {}
void BAbstractSpinner::_ReservedAbstractSpinner19() {}
void BAbstractSpinner::_ReservedAbstractSpinner18() {}
void BAbstractSpinner::_ReservedAbstractSpinner17() {}
void BAbstractSpinner::_ReservedAbstractSpinner16() {}
void BAbstractSpinner::_ReservedAbstractSpinner15() {}
void BAbstractSpinner::_ReservedAbstractSpinner14() {}
void BAbstractSpinner::_ReservedAbstractSpinner13() {}
void BAbstractSpinner::_ReservedAbstractSpinner12() {}
void BAbstractSpinner::_ReservedAbstractSpinner11() {}
void BAbstractSpinner::_ReservedAbstractSpinner10() {}
void BAbstractSpinner::_ReservedAbstractSpinner9() {}
void BAbstractSpinner::_ReservedAbstractSpinner8() {}
void BAbstractSpinner::_ReservedAbstractSpinner7() {}
void BAbstractSpinner::_ReservedAbstractSpinner6() {}
void BAbstractSpinner::_ReservedAbstractSpinner5() {}
void BAbstractSpinner::_ReservedAbstractSpinner4() {}
void BAbstractSpinner::_ReservedAbstractSpinner3() {}
void BAbstractSpinner::_ReservedAbstractSpinner2() {}
void BAbstractSpinner::_ReservedAbstractSpinner1() {}
