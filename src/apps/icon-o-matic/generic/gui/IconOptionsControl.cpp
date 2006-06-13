/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconOptionsControl.h"

#include <stdio.h>

#include <Window.h>

#include "IconButton.h"

#define LABEL_DIST 8.0

// constructor
IconOptionsControl::IconOptionsControl(const char* name,
									   const char* label,
									   BMessage* message,
									   BHandler* target)
	: MView(),
	  BControl(BRect(0.0, 0.0, 10.0, 10.0), name, label, message,
	  		   B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
	  fTargetCache(target)
{
	if (Label()) {
		labelwidth = StringWidth(Label()) + LABEL_DIST;
	} else {
		labelwidth = 0.0;
	}
}

// destructor
IconOptionsControl::~IconOptionsControl()
{
}

// layoutprefs
minimax
IconOptionsControl::layoutprefs()
{
	// sanity checks
	if (rolemodel)
		labelwidth = rolemodel->LabelWidth();
	if (labelwidth < LabelWidth())
		labelwidth = LabelWidth();

	mpm.mini.x = 0.0 + labelwidth + 5.0;
	mpm.mini.y = 0.0;

	for (int32 i = 0; IconButton* button = _FindIcon(i); i++) {
		minimax childPrefs = button->layoutprefs();
		mpm.mini.x += childPrefs.mini.x + 1;
		if (mpm.mini.y < childPrefs.mini.y + 1)
			mpm.mini.y = childPrefs.mini.y + 1;
	}

	mpm.maxi.x = mpm.mini.x;
	mpm.maxi.y = mpm.mini.y;

	mpm.weight = 1.0;

	return mpm;
}

// layout
BRect
IconOptionsControl::layout(BRect frame)
{
	if (frame.Width() < mpm.mini.x + 1)
		frame.right = frame.left + mpm.mini.x + 1;

	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());

	return Frame();
}

// LabelWidth
float
IconOptionsControl::LabelWidth()
{
	float width = ceilf(StringWidth(Label()));
	if (width > 0.0)
		width += LABEL_DIST;
	return width;
}

// SetLabel
void
IconOptionsControl::SetLabel(const char* label)
{
	BControl::SetLabel(label);
	float width = LabelWidth();
	if (rolemodel)
		labelwidth = rolemodel->LabelWidth() > labelwidth ?
						rolemodel->LabelWidth() : labelwidth;

	labelwidth = width > labelwidth ? width : labelwidth;

	_TriggerRelayout();
}

// AttachedToWindow
void
IconOptionsControl::AttachedToWindow()
{
	BControl::AttachedToWindow();

	SetViewColor(B_TRANSPARENT_32_BIT);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

// AllAttached
void
IconOptionsControl::AllAttached()
{
	for (int32 i = 0; IconButton* button = _FindIcon(i); i++)
		button->SetTarget(this);
	if (fTargetCache)
		SetTarget(fTargetCache);
}

// Draw
void
IconOptionsControl::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);

	if (Label()) {
		if (!IsEnabled())
			SetHighColor(tint_color(LowColor(), B_DISABLED_LABEL_TINT));
		else
			SetHighColor(tint_color(LowColor(), B_DARKEN_MAX_TINT));

		font_height fh;
		GetFontHeight(&fh);
		BPoint p(Bounds().LeftTop());
		p.y += floorf(Bounds().Height() / 2.0 + (fh.ascent + fh.descent) / 2.0) - 2.0;
		DrawString(Label(), p);
	}
}

// FrameResized
void
IconOptionsControl::FrameResized(float width, float height)
{
	_LayoutIcons(Bounds());
}

// SetValue
void
IconOptionsControl::SetValue(int32 value)
{
	if (IconButton* valueButton = _FindIcon(value)) {
		for (int32 i = 0; IconButton* button = _FindIcon(i); i++) {
			button->SetPressed(button == valueButton);
		}
	}
}

// Value
int32
IconOptionsControl::Value() const
{
	for (int32 i = 0; IconButton* button = _FindIcon(i); i++) {
		if (button->IsPressed())
			return i;
	}
	return -1;
}

// SetEnabled
void
IconOptionsControl::SetEnabled(bool enable)
{
	for (int32 i = 0; IconButton* button = _FindIcon(i); i++) {
		button->SetEnabled(enable);
	}
	BControl::SetEnabled(enable);
	Invalidate();
}

// MessageReceived
void
IconOptionsControl::MessageReceived(BMessage* message)
{
	// catch a message from the attached IconButtons to
	// handle switching the pressed icon
	BView* source;
	if (message->FindPointer("be:source", (void**)&source) >= B_OK) {
		if (IconButton* sourceIcon = dynamic_cast<IconButton*>(source)) {
			for (int32 i = 0; IconButton* button = _FindIcon(i); i++) {
				if (button == sourceIcon) {
					SetValue(i);
					break;
				}
			}
			// forward the message
			Invoke(message);
			return;
		}
	}
	BControl::MessageReceived(message);
}

// Invoke
status_t
IconOptionsControl::Invoke(BMessage* message)
{
	return BInvoker::Invoke(message);
}

// AddOption
void
IconOptionsControl::AddOption(IconButton* icon)
{
	if (icon) {
		if (!_FindIcon(0)) {
			// first icon added, mark it
			icon->SetPressed(true);
		}
		AddChild(icon);
		icon->SetTarget(this);
		layoutprefs();
		_TriggerRelayout();
	}
}

// _FindIcon
IconButton*
IconOptionsControl::_FindIcon(int32 index) const
{
	if (BView* view = ChildAt(index))
		return dynamic_cast<IconButton*>(view);
	return NULL;
}

// _TriggerRelayout
void
IconOptionsControl::_TriggerRelayout()
{
	if (!Parent())
		return;

	MView* mParent = dynamic_cast<MView*>(Parent());
	if (mParent) {
		if (BWindow* window = Window()) {
			window->PostMessage(M_RECALCULATE_SIZE);
		}
	} else {
		_LayoutIcons(Bounds());
	}
}

// _LayoutIcons
void
IconOptionsControl::_LayoutIcons(BRect frame)
{
	BPoint lt = frame.LeftTop();

	// sanity checks
	if (rolemodel)
		labelwidth = rolemodel->LabelWidth();
	if (labelwidth < LabelWidth())
		labelwidth = LabelWidth();

	lt.x += labelwidth;

	for (int32 i = 0; IconButton* button = _FindIcon(i); i++) {
		if (i == 0) {
			lt.y = ceilf((frame.top + frame.bottom - button->mpm.mini.y) / 2.0);
		}
		button->MoveTo(lt);
		button->ResizeTo(button->mpm.mini.x, button->mpm.mini.y);
		lt = button->Frame().RightTop() + BPoint(1.0, 0.0);
	}
}
