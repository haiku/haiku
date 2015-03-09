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


#include "Spinner.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
#include <Point.h>
#include <PropertyInfo.h>
#include <TextView.h>
#include <Window.h>

#include "Thread.h"


static const float kFrameMargin			= 2.0f;

const char* const kFrameField			= "BSpinner:layoutItem:frame";
const char* const kLabelItemField		= "BSpinner:labelItem";
const char* const kTextViewItemField	= "BSpinner:textViewItem";


static double
roundTo(double value, uint32 n)
{
	return floor(value * pow(10.0, n) + 0.5) / pow(10.0, n);
}


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

	{
		"MaxValue",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the maximum value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},
	{
		"MaxValue",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the maximum value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},

	{
		"MinValue",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the minimum value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},
	{
		"MinValue",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the minimum value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},

	{
		"Precision",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the number of decimal places of precision of the spinner.",
		0,
		{ B_UINT32_TYPE }
	},
	{
		"Precision",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the number of decimal places of precision of the spinner.",
		0,
		{ B_UINT32_TYPE }
	},

	{
		"Step",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the step size of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},
	{
		"Step",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the step size of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},

	{
		"Value",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},
	{
		"Value",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},

	{ 0 }
};


typedef enum {
	SPINNER_INCREMENT,
	SPINNER_DECREMENT
} spinner_direction;


class SpinnerArrow : public BView {
public:
								SpinnerArrow(BRect frame, const char* name,
									spinner_direction direction);
	virtual						~SpinnerArrow();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);

			bool				IsEnabled() const { return fIsEnabled; }
	virtual	void				SetEnabled(bool enable) { fIsEnabled = enable; };

private:
			void				_DoneTracking(BPoint where);
			void				_Track(BPoint where, uint32);

			spinner_direction	fSpinnerDirection;
			BSpinner*			fParent;
			bool				fIsEnabled;
			bool				fIsMouseDown;
			bool				fIsMouseOver;
			bigtime_t			fRepeatDelay;
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
			void				_SetValueToText();

			BSpinner*			fParent;
};


class BSpinner::LabelLayoutItem : public BAbstractLayoutItem {
public:
								LabelLayoutItem(BSpinner* parent);
								LabelLayoutItem(BMessage* archive);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(BSpinner* parent);
	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

			BRect				FrameInParent() const;

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			BSpinner*			fParent;
			BRect				fFrame;
};


class BSpinner::TextViewLayoutItem : public BAbstractLayoutItem {
public:
								TextViewLayoutItem(BSpinner* parent);
								TextViewLayoutItem(BMessage* archive);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(BSpinner* parent);
	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

			BRect				FrameInParent() const;

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			BSpinner*			fParent;
			BRect				fFrame;
};


struct BSpinner::LayoutData {
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


//	#pragma mark - SpinnerArrow


SpinnerArrow::SpinnerArrow(BRect frame, const char* name,
	spinner_direction direction)
	:
	BView(frame, name, B_FOLLOW_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW),
	fSpinnerDirection(direction),
	fParent(NULL),
	fIsEnabled(true),
	fIsMouseDown(false),
	fIsMouseOver(false),
	fRepeatDelay(100000)
{
	rgb_color bgColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(bgColor);
	SetLowColor(bgColor);
}


SpinnerArrow::~SpinnerArrow()
{
}


void
SpinnerArrow::AttachedToWindow()
{
	fParent = static_cast<BSpinner*>(Parent());

	BView::AttachedToWindow();
}


void
SpinnerArrow::DetachedFromWindow()
{
	fParent = NULL;

	BView::DetachedFromWindow();
}


void
SpinnerArrow::Draw(BRect updateRect)
{
	BRect rect(Bounds());
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	BView::Draw(updateRect);

	float fgTint;
	if (!fIsEnabled)
		fgTint = B_DARKEN_1_TINT;
	else if (fIsMouseDown)
		fgTint = B_DARKEN_MAX_TINT;
	else
		fgTint = B_DARKEN_3_TINT;

	float bgTint;
	if (fIsEnabled && fIsMouseOver)
		bgTint = B_DARKEN_1_TINT;
	else
		bgTint = B_NO_TINT;

	rgb_color bgColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	if (bgColor.red + bgColor.green + bgColor.blue <= 128 * 3) {
		// if dark background make the tint lighter
		fgTint = 2.0f - fgTint;
		bgTint = 2.0f - bgTint;
	}

	uint32 borders = be_control_look->B_TOP_BORDER
		| be_control_look->B_BOTTOM_BORDER;

	if (fSpinnerDirection == SPINNER_INCREMENT)
		borders |= be_control_look->B_RIGHT_BORDER;
	else
		borders |= be_control_look->B_LEFT_BORDER;

	// draw the button
	be_control_look->DrawButtonFrame(this, rect, updateRect,
		tint_color(bgColor, fgTint), bgColor, 0, borders);
	be_control_look->DrawButtonBackground(this, rect, updateRect,
		tint_color(bgColor, bgTint), 0, borders);

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
		StrokeLine(BPoint(rect.left + halfWidth, rect.top),
			BPoint(rect.left + halfWidth, rect.bottom));
	}
}


void
SpinnerArrow::MouseDown(BPoint where)
{
	if (fIsEnabled) {
		fIsMouseDown = true;
		Invalidate();
		fRepeatDelay = 100000;
		MouseDownThread<SpinnerArrow>::TrackMouse(this,
			&SpinnerArrow::_DoneTracking, &SpinnerArrow::_Track);
	}

	BView::MouseDown(where);
}


void
SpinnerArrow::MouseMoved(BPoint where, uint32 transit,
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
			if (!fIsMouseDown)
				Invalidate();

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
SpinnerArrow::MouseUp(BPoint where)
{
	fIsMouseDown = false;
	Invalidate();

	BView::MouseUp(where);
}


//	#pragma mark  - SpinnerArrow private methods


void
SpinnerArrow::_DoneTracking(BPoint where)
{
	if (fIsMouseDown || !Bounds().Contains(where))
		fIsMouseDown = false;
}


void
SpinnerArrow::_Track(BPoint where, uint32)
{
	if (fParent == NULL || !Bounds().Contains(where)) {
		fIsMouseDown = false;
		return;
	}
	fIsMouseDown = true;

	double step = fSpinnerDirection == SPINNER_INCREMENT
		? fParent->Step()
		: -fParent->Step();
	double newValue = fParent->Value() + step;
	if (newValue < fParent->MinValue()) {
		// new value is below lower bound, clip to lower bound
		fParent->SetValue(fParent->MinValue());
	} else if (newValue>fParent->MaxValue()) {
		// new value is above upper bound, clip to upper bound
		fParent->SetValue(fParent->MaxValue());
	} else {
		// new value is in range
		fParent->SetValue(newValue);
	}
	fParent->Invoke();
	fParent->Invalidate();

	snooze(fRepeatDelay);
	fRepeatDelay = 10000;
}


//	#pragma mark - SpinnerTextView


SpinnerTextView::SpinnerTextView(BRect rect, BRect textRect)
	:
	BTextView(rect, "textview", textRect, B_FOLLOW_ALL,
		B_WILL_DRAW | B_NAVIGABLE),
	fParent(NULL)
{
	rgb_color bgColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(bgColor);
	SetLowColor(bgColor);

	SetAlignment(B_ALIGN_RIGHT);
	for (uint32 c = 0; c <= 42; c++)
		DisallowChar(c);

	DisallowChar('/');
	for (uint32 c = 58; c <= 127; c++)
		DisallowChar(c);
}


SpinnerTextView::~SpinnerTextView()
{
}


void
SpinnerTextView::AttachedToWindow()
{
	fParent = static_cast<BSpinner*>(Parent());

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
			_SetValueToText();
			break;

		case B_TAB:
			fParent->KeyDown(bytes, numBytes);
			break;

		case B_UP_ARROW:
		case B_PAGE_UP:
		case B_DOWN_ARROW:
		case B_PAGE_DOWN:
		{
			double step = fParent->Step();
			if (*bytes == B_DOWN_ARROW || *bytes == B_PAGE_DOWN)
				step *= -1;

			fParent->SetValue(fParent->Value() + step);
			fParent->Invoke();
			fParent->Invalidate();
			break;
		}

		default:
			BTextView::KeyDown(bytes, numBytes);
	}
}


void
SpinnerTextView::MakeFocus(bool focus)
{
	BTextView::MakeFocus(focus);

	if (focus)
		SelectAll();
	else
		_SetValueToText();

	if (fParent != NULL)
		fParent->_DrawTextView(fParent->Bounds());
}


//	#pragma mark - SpinnerTextView private methods


void
SpinnerTextView::_SetValueToText()
{
	if (fParent == NULL)
		return;

	fParent->SetValue(roundTo(atof(Text()), fParent->Precision()));
	fParent->Invoke();
	fParent->Invalidate();
}


//	#pragma mark - BSpinner::LabelLayoutItem


BSpinner::LabelLayoutItem::LabelLayoutItem(BSpinner* parent)
	:
	fParent(parent),
	fFrame()
{
}


BSpinner::LabelLayoutItem::LabelLayoutItem(BMessage* from)
	:
	BAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
}


bool
BSpinner::LabelLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
BSpinner::LabelLayoutItem::SetVisible(bool visible)
{
}


BRect
BSpinner::LabelLayoutItem::Frame()
{
	return fFrame;
}


void
BSpinner::LabelLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


void
BSpinner::LabelLayoutItem::SetParent(BSpinner* parent)
{
	fParent = parent;
}


BView*
BSpinner::LabelLayoutItem::View()
{
	return fParent;
}


BSize
BSpinner::LabelLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	if (fParent->Label() == NULL)
		return BSize(-1.0f, -1.0f);

	return BSize(fParent->fLayoutData->label_width
			+ be_control_look->DefaultLabelSpacing(),
		fParent->fLayoutData->label_height);
}


BSize
BSpinner::LabelLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
BSpinner::LabelLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
BSpinner::LabelLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


BRect
BSpinner::LabelLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
}


status_t
BSpinner::LabelLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t result = BAbstractLayoutItem::Archive(into, deep);

	if (result == B_OK)
		result = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(result);
}


BArchivable*
BSpinner::LabelLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BSpinner::LabelLayoutItem"))
		return new LabelLayoutItem(from);

	return NULL;
}


//	#pragma mark - BSpinner::TextViewLayoutItem


BSpinner::TextViewLayoutItem::TextViewLayoutItem(BSpinner* parent)
	:
	fParent(parent),
	fFrame()
{
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


BSpinner::TextViewLayoutItem::TextViewLayoutItem(BMessage* from)
	:
	BAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
}


bool
BSpinner::TextViewLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
BSpinner::TextViewLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
BSpinner::TextViewLayoutItem::Frame()
{
	return fFrame;
}


void
BSpinner::TextViewLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


void
BSpinner::TextViewLayoutItem::SetParent(BSpinner* parent)
{
	fParent = parent;
}


BView*
BSpinner::TextViewLayoutItem::View()
{
	return fParent;
}


BSize
BSpinner::TextViewLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	BSize size(fParent->fLayoutData->text_view_width,
		fParent->fLayoutData->text_view_height);
	return size;
}


BSize
BSpinner::TextViewLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
BSpinner::TextViewLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
BSpinner::TextViewLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


BRect
BSpinner::TextViewLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
}


status_t
BSpinner::TextViewLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t result = BAbstractLayoutItem::Archive(into, deep);

	if (result == B_OK)
		result = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(result);
}


BArchivable*
BSpinner::TextViewLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BSpinner::TextViewLayoutItem"))
		return new LabelLayoutItem(from);

	return NULL;
}


//	#pragma mark - BSpinner


BSpinner::BSpinner(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	BView(frame, name, resizingMode, flags | B_WILL_DRAW | B_FRAME_EVENTS),
	fLabel(label)
{
	SetMessage(message);
	_InitObject();
}


BSpinner::BSpinner(const char* name, const char* label, BMessage* message,
	uint32 flags)
	:
	BView(name, flags | B_WILL_DRAW | B_FRAME_EVENTS),
	fLabel(label)
{
	SetMessage(message);
	_InitObject();
}


BSpinner::BSpinner(BMessage* data)
	:
	BView(data)
{
	_InitObject();

	if (data->FindInt32("_align") != B_OK)
		fAlignment = B_ALIGN_LEFT;

	if (data->FindInt32("_divider") != B_OK)
		fDivider = 0.0f;

	if (data->FindBool("_enabled") != B_OK)
		fIsEnabled = true;

	if (data->FindString("_label", &fLabel) != B_OK)
		fLabel = NULL;

	BMessage* message = NULL;
	if (data->FindMessage("_message", message) == B_OK)
		SetMessage(message);

	if (data->FindDouble("_max", &fMaxValue) != B_OK)
		fMinValue = 100.0;

	if (data->FindDouble("_min", &fMinValue) != B_OK)
		fMinValue = 0.0;

	if (data->FindUInt32("_precision", &fPrecision) != B_OK)
		fPrecision = 2;

	if (data->FindDouble("_step", &fStep) != B_OK)
		fStep = 1.0;

	if (data->FindDouble("_value", &fValue) != B_OK)
		fValue = 0.0;
}


BSpinner::~BSpinner()
{
	delete fLayoutData;
	fLayoutData = NULL;
}


BArchivable*
BSpinner::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "Spinner"))
		return new BSpinner(data);

	return NULL;
}


status_t
BSpinner::Archive(BMessage* data, bool deep) const
{
	status_t status = BView::Archive(data, deep);
	data->AddString("class", "Spinner");

	if (status == B_OK)
		status = data->AddInt32("_align", fAlignment);

	if (status == B_OK)
		status = data->AddFloat("_divider", fDivider);

	if (status == B_OK)
		status = data->AddBool("_enabled", fIsEnabled);

	if (status == B_OK && fLabel != NULL)
		status = data->AddString("_label", fLabel);

	if (status == B_OK && Message() != NULL)
		status = data->AddMessage("_message", Message());

	if (status == B_OK)
		status = data->AddDouble("_max", fMaxValue);

	if (status == B_OK)
		status = data->AddDouble("_min", fMinValue);

	if (status == B_OK)
		status = data->AddUInt32("_precision", fPrecision);

	if (status == B_OK)
		status = data->AddDouble("_step", fStep);

	if (status == B_OK)
		status = data->AddDouble("_value", fValue);

	return status;
}


status_t
BSpinner::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Haiku-spinner");

	BPropertyInfo prop_info(sProperties);
	message->AddFlat("messages", &prop_info);

	return BView::GetSupportedSuites(message);
}


BHandler*
BSpinner::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return BView::ResolveSpecifier(message, index, specifier, form,
		property);
}


void
BSpinner::AttachedToWindow()
{
	if (!Messenger().IsValid())
		SetTarget(Window());

	SetValue(fValue);
		// sets the text and enables or disables the arrows
	_UpdateTextViewColors(IsEnabled());
	fTextView->MakeEditable(IsEnabled());

	BView::AttachedToWindow();
}


void
BSpinner::Draw(BRect updateRect)
{
	_DrawLabel(updateRect);
	_DrawTextView(updateRect);
	fIncrement->Invalidate();
	fDecrement->Invalidate();
}


void
BSpinner::FrameResized(float width, float height)
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
BSpinner::MakeFocus(bool focus)
{
	fTextView->MakeFocus(focus);
}


void
BSpinner::ResizeToPreferred()
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
BSpinner::SetFlags(uint32 flags)
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
BSpinner::ValueChanged()
{
	// hook method - does nothing
}


void
BSpinner::WindowActivated(bool active)
{
	_DrawTextView(fTextView->Frame());
}


void
BSpinner::SetAlignment(alignment align)
{
	fAlignment = align;
}


void
BSpinner::SetDivider(float position)
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
BSpinner::SetEnabled(bool enable)
{
	if (IsEnabled() == enable)
		return;

	fIsEnabled = enable;
	fTextView->MakeEditable(enable);
	if (enable)
		fTextView->SetFlags(fTextView->Flags() | B_NAVIGABLE);
	else
		fTextView->SetFlags(fTextView->Flags() & ~B_NAVIGABLE);

	_UpdateTextViewColors(enable);
	fTextView->Invalidate();
	SetIncrementEnabled(enable && fValue < fMaxValue);
	SetDecrementEnabled(enable && fValue > fMinValue);

	_LayoutTextView();
	Invalidate();
	if (Window() != NULL)
		Window()->UpdateIfNeeded();
}


void
BSpinner::SetLabel(const char* label)
{
	fLabel = label;
	if (Window() != NULL) {
		Invalidate();
		Window()->UpdateIfNeeded();
	}

	InvalidateLayout();
}


void
BSpinner::SetMaxValue(double max)
{
	fMaxValue = max;
	if (fValue > fMaxValue)
		SetValue(fMaxValue);
}


void
BSpinner::SetMinValue(double min)
{
	fMinValue = min;
	if (fValue < fMinValue)
		SetValue(fMinValue);
}


void
BSpinner::Range(double* min, double* max)
{
	*min = fMinValue;
	*max = fMaxValue;
}


void
BSpinner::SetRange(double min, double max)
{
	SetMinValue(min);
	SetMaxValue(max);
}


void
BSpinner::SetValue(double value)
{
	// clip to range
	if (value < fMinValue)
		value = fMinValue;
	else if (value > fMaxValue)
		value = fMaxValue;

	// update the text view
	char* format;
	asprintf(&format, "%%.%" B_PRId32 "f", fPrecision);
	char* valueString;
	asprintf(&valueString, format, value);
	fTextView->SetText(valueString);
	free(format);
	free(valueString);

	// update the up and down arrows
	SetIncrementEnabled(IsEnabled() && value < fMaxValue);
	SetDecrementEnabled(IsEnabled() && value > fMinValue);

	if (value == fValue)
		return;

	fValue = value;
	ValueChanged();
}


bool
BSpinner::IsDecrementEnabled() const
{
	return fDecrement->IsEnabled();
}


void
BSpinner::SetDecrementEnabled(bool enable)
{
	if (IsDecrementEnabled() == enable)
		return;

	fDecrement->SetEnabled(enable);
	fDecrement->Invalidate();
}


bool
BSpinner::IsIncrementEnabled() const
{
	return fIncrement->IsEnabled();
}


void
BSpinner::SetIncrementEnabled(bool enable)
{
	if (IsIncrementEnabled() == enable)
		return;

	fIncrement->SetEnabled(enable);
	fIncrement->Invalidate();
}


BSize
BSpinner::MinSize()
{
	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), fLayoutData->min);
}


BSize
BSpinner::MaxSize()
{
	_ValidateLayoutData();

	BSize max = fLayoutData->min;
	max.width = B_SIZE_UNLIMITED;

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), max);
}


BSize
BSpinner::PreferredSize()
{
	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		fLayoutData->min);
}


BAlignment
BSpinner::LayoutAlignment()
{
	_ValidateLayoutData();
	return BLayoutUtils::ComposeAlignment(ExplicitAlignment(),
		BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
}


BLayoutItem*
BSpinner::CreateLabelLayoutItem()
{
	if (fLayoutData->label_layout_item == NULL)
		fLayoutData->label_layout_item = new LabelLayoutItem(this);

	return fLayoutData->label_layout_item;
}


BLayoutItem*
BSpinner::CreateTextViewLayoutItem()
{
	if (fLayoutData->text_view_layout_item == NULL)
		fLayoutData->text_view_layout_item = new TextViewLayoutItem(this);

	return fLayoutData->text_view_layout_item;
}


BTextView*
BSpinner::TextView() const
{
	return dynamic_cast<BTextView*>(fTextView);
}


//	#pragma mark - BSpinner protected methods


status_t
BSpinner::AllArchived(BMessage* into) const
{
	status_t result;
	if ((result = BView::AllArchived(into)) != B_OK)
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
BSpinner::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);

	status_t result = B_OK;
	if ((result = BView::AllUnarchived(from)) != B_OK)
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
BSpinner::DoLayout()
{
	if ((Flags() & B_SUPPORTS_LAYOUT) == 0)
		return;

	if (GetLayout()) {
		BView::DoLayout();
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
BSpinner::LayoutInvalidated(bool descendants)
{
	if (fLayoutData != NULL)
		fLayoutData->valid = false;
}


//	#pragma mark - BSpinner private methods


void
BSpinner::_DrawLabel(BRect updateRect)
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
		+ fontHeight.ascent + kFrameMargin * 2;

	uint32 flags = 0;
	if (!IsEnabled())
		flags |= BControlLook::B_DISABLED;

	be_control_look->DrawLabel(this, label, LowColor(), flags, BPoint(x, y));
}


void
BSpinner::_DrawTextView(BRect updateRect)
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
BSpinner::_InitObject()
{
	rgb_color bgColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(bgColor);
	SetLowColor(bgColor);

	fAlignment = B_ALIGN_LEFT;
	if (Label() != NULL) {
		fDivider = StringWidth(Label())
			+ be_control_look->DefaultLabelSpacing();
	} else
		fDivider = 0.0f;

	fIsEnabled = true;

	fMaxValue = 100.0;
	fMinValue = 0.0;
	fPrecision = 2;
	fStep = 1.0;
	fValue = 0.0;

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

	fDecrement = new SpinnerArrow(rect, "decrement", SPINNER_DECREMENT);
	AddChild(fDecrement);

	rect.left = rect.right + 1.0f;
	rect.right = rect.left + rect.Height() - kFrameMargin * 2;

	fIncrement = new SpinnerArrow(rect, "increment", SPINNER_INCREMENT);
	AddChild(fIncrement);

	uint32 navigableFlags = Flags() & B_NAVIGABLE;
	if (navigableFlags != 0)
		BView::SetFlags(Flags() & ~B_NAVIGABLE);
}


void
BSpinner::_LayoutTextView()
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
BSpinner::_UpdateFrame()
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
BSpinner::_UpdateTextViewColors(bool enable)
{
	rgb_color textColor;
	rgb_color bgColor;
	BFont font;

	fTextView->GetFontAndColor(0, &font);

	if (enable)
		textColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	else {
		textColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DISABLED_LABEL_TINT);
	}

	fTextView->SetFontAndColor(&font, B_FONT_ALL, &textColor);

	if (enable)
		bgColor = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	else {
		bgColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_LIGHTEN_2_TINT);
	}

	fTextView->SetViewColor(bgColor);
	fTextView->SetLowColor(bgColor);
}


void
BSpinner::_ValidateLayoutData()
{
	if (fLayoutData->valid)
		return;

	font_height fontHeight = fLayoutData->font_info;
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

	char* format;
	asprintf(&format, "%%.%" B_PRId32 "f", fPrecision);
	char* maxValue;
	asprintf(&maxValue, format, fMaxValue);
	char* minValue;
	asprintf(&minValue, format, fMinValue);
	float longestValue = ceilf(std::max(fTextView->StringWidth(maxValue),
		fTextView->StringWidth(minValue)));
	free(format);
	free(maxValue);
	free(minValue);

	float textWidth = ceilf(std::max(longestValue,
		fTextView->StringWidth("99999")));

	float textViewHeight = fTextView->LineHeight(0) + kFrameMargin * 2;
	float textViewWidth = textWidth + textViewHeight * 2;

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

void BSpinner::_ReservedSpinner20() {}
void BSpinner::_ReservedSpinner19() {}
void BSpinner::_ReservedSpinner18() {}
void BSpinner::_ReservedSpinner17() {}
void BSpinner::_ReservedSpinner16() {}
void BSpinner::_ReservedSpinner15() {}
void BSpinner::_ReservedSpinner14() {}
void BSpinner::_ReservedSpinner13() {}
void BSpinner::_ReservedSpinner12() {}
void BSpinner::_ReservedSpinner11() {}
void BSpinner::_ReservedSpinner10() {}
void BSpinner::_ReservedSpinner9() {}
void BSpinner::_ReservedSpinner8() {}
void BSpinner::_ReservedSpinner7() {}
void BSpinner::_ReservedSpinner6() {}
void BSpinner::_ReservedSpinner5() {}
void BSpinner::_ReservedSpinner4() {}
void BSpinner::_ReservedSpinner3() {}
void BSpinner::_ReservedSpinner2() {}
void BSpinner::_ReservedSpinner1() {}
