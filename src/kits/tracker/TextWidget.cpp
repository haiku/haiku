/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <string.h>
#include <stdlib.h>

#include <Alert.h>
#include <Debug.h>
#include <Directory.h>
#include <MessageFilter.h>
#include <ScrollView.h>
#include <TextView.h>
#include <Volume.h>
#include <Window.h>

#include "Attributes.h"
#include "ContainerWindow.h"
#include "Commands.h"
#include "FSUtils.h"
#include "PoseView.h"
#include "TextWidget.h"
#include "Utilities.h"
#include "WidgetAttributeText.h"


const float kWidthMargin = 20;


BTextWidget::BTextWidget(Model *model, BColumn *column, BPoseView *view)
	:
	fText(WidgetAttributeText::NewWidgetText(model, column, view)),
	fAttrHash(column->AttrHash()),
	fAlignment(column->Alignment()),
	fEditable(column->Editable()),
	fVisible(true),
	fActive(false),
	fSymLink(model->IsSymLink())
{
}


BTextWidget::~BTextWidget()
{
	delete fText;
}


int
BTextWidget::Compare(const BTextWidget &with, BPoseView *view) const
{
	return fText->Compare(*with.fText, view);
}


void
BTextWidget::RecalculateText(const BPoseView *view)
{
	fText->SetDirty(true);
	fText->CheckViewChanged(view);	
}


const char *
BTextWidget::Text() const
{
	StringAttributeText *textAttribute = dynamic_cast<StringAttributeText *>(fText);
	
	ASSERT(textAttribute);
	if (!textAttribute)
		return "";

	return textAttribute->Value();
}


float
BTextWidget::TextWidth(const BPoseView *pose) const
{
	return fText->Width(pose);
}


float
BTextWidget::PreferredWidth(const BPoseView *pose) const
{
	return fText->PreferredWidth(pose) + 1;
}


BRect
BTextWidget::ColumnRect(BPoint poseLoc, const BColumn *column,
	const BPoseView *view)
{
	if (view->ViewMode() != kListMode) {
		// ColumnRect only makes sense in list view, return
		// CalcRect otherwise
		return CalcRect(poseLoc, column, view);
	}
	BRect result;
	result.left = column->Offset() + poseLoc.x;
	result.right = result.left + column->Width();
	result.bottom = poseLoc.y + view->ListElemHeight() - 1;
	result.top = result.bottom - view->FontHeight();
	return result;
}


BRect 
BTextWidget::CalcRectCommon(BPoint poseLoc, const BColumn *column,
	const BPoseView *view, float textWidth)
{
	BRect result;
	if (view->ViewMode() == kListMode) {
		poseLoc.x += column->Offset();

		switch (fAlignment) {
			case B_ALIGN_LEFT:
				result.left = poseLoc.x;
				result.right = result.left + textWidth + 1;
				break;

			case B_ALIGN_CENTER:
				result.left = poseLoc.x + (column->Width() / 2) - (textWidth / 2);
				if (result.left < 0)
					result.left = 0;
				result.right = result.left + textWidth + 1;
				break;

			case B_ALIGN_RIGHT:
				result.right = poseLoc.x + column->Width();
				result.left = result.right - textWidth - 1;
				if (result.left < 0)
					result.left = 0;
				break;
			default:
				TRESPASS();
		}

		result.bottom = poseLoc.y + (view->ListElemHeight() - 1);
	} else {
		if (view->ViewMode() == kIconMode
			|| view->ViewMode() == kScaleIconMode) {
			// large/scaled icon mode
			result.left = poseLoc.x + (view->IconSizeInt() - textWidth) / 2;
		} else {
			// mini icon mode
			result.left = poseLoc.x + B_MINI_ICON + kMiniIconSeparator;
		}

		result.right = result.left + textWidth;
		result.bottom = poseLoc.y + view->IconPoseHeight();

	}
	result.top = result.bottom - view->FontHeight();

	return result;
}


BRect
BTextWidget::CalcRect(BPoint poseLoc, const BColumn *column,
	const BPoseView *view)
{
	return CalcRectCommon(poseLoc, column, view, fText->Width(view));
}


BRect
BTextWidget::CalcOldRect(BPoint poseLoc, const BColumn *column,
	const BPoseView *view)
{
	return CalcRectCommon(poseLoc, column, view, fText->CurrentWidth());
}


BRect
BTextWidget::CalcClickRect(BPoint poseLoc, const BColumn *column,
	const BPoseView* view)
{
	BRect result = CalcRect(poseLoc, column, view);
	if (result.Width() < kWidthMargin) {
		// if resulting rect too narrow, make it a bit wider
		// for comfortable clicking
		if (column && column->Width() < kWidthMargin)
			result.right = result.left + column->Width();
		else
			result.right = result.left + kWidthMargin;
	}
	return result;
}


void
BTextWidget::MouseUp(BRect bounds, BPoseView *view, BPose *pose, BPoint,
	bool delayedEdit)
{
	// wait until a double click time to see if we are double clicking
	// or selecting widget for editing
	// start editing early if mouse left widget or modifier down
	
	if (!IsEditable())
		return;

	if (delayedEdit) {
		bigtime_t doubleClickTime;
		get_click_speed(&doubleClickTime);
		doubleClickTime += system_time();

		while (system_time() < doubleClickTime) {
			// loop for double-click time and watch the mouse and keyboard

			BPoint point;
			uint32 buttons;
			view->GetMouse(&point, &buttons, false);
			if (buttons) 
				// if mouse button goes down then a double click, exit
				// without editing
				return;

			if (!bounds.Contains(point))
				// mouse has moved outside of text widget so go into edit mode
				break;

			if (modifiers() & (B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY | B_MENU_KEY))
				// watch the keyboard (ignoring standard locking keys)
				break;

			snooze(100000);
		}
	}

	StartEdit(bounds, view, pose);
}


static filter_result
TextViewFilter(BMessage *message, BHandler **, BMessageFilter *filter)
{
	uchar key;
	if (message->FindInt8("byte", (int8 *)&key) != B_OK)
		return B_DISPATCH_MESSAGE;

	BPoseView *poseView = dynamic_cast<BContainerWindow*>(filter->Looper())->
		PoseView();

	if (key == B_RETURN || key == B_ESCAPE) {
		poseView->CommitActivePose(key == B_RETURN);
		return B_SKIP_MESSAGE;
	}

	if (key == B_TAB) {
		if (poseView->ActivePose()) {
			if (message->FindInt32("modifiers") & B_SHIFT_KEY)
				poseView->ActivePose()->EditPreviousWidget(poseView);
			else
				poseView->ActivePose()->EditNextWidget(poseView);
		}

		return B_SKIP_MESSAGE;
	}

	// the BTextView doesn't respect window borders when resizing itself;
	// we try to work-around this "bug" here.

	// find the text editing view
	BView *scrollView = poseView->FindView("BorderView");
	if (scrollView != NULL) {
		BTextView *textView = dynamic_cast<BTextView *>(scrollView->FindView("WidgetTextView"));
		if (textView != NULL) {
			BRect rect = scrollView->Frame();

			if (rect.right + 3 > poseView->Bounds().right
				|| rect.left - 3 < 0)
				textView->MakeResizable(true, NULL);
		}
	}

	return B_DISPATCH_MESSAGE;
}


void
BTextWidget::StartEdit(BRect bounds, BPoseView *view, BPose *pose)
{
	if (!IsEditable())
		return;

	// don't allow editing of the trash directory name
	BEntry entry(pose->TargetModel()->EntryRef());
	if (entry.InitCheck() == B_OK && FSIsTrashDir(&entry))
		return;

	// don't allow editing of the "Disks" icon name
	if (pose->TargetModel()->IsRoot())
		return;

	if (!ConfirmChangeIfWellKnownDirectory(&entry, "rename"))
		return;

	// get bounds with full text length
	BRect rect(bounds);
	BRect textRect(bounds);
	rect.OffsetBy(-2, -1);
	rect.right += 1;

	BFont font;
	view->GetFont(&font);
	BTextView *textView = new BTextView(rect, "WidgetTextView", textRect, &font, 0,
		B_FOLLOW_ALL, B_WILL_DRAW);

	textView->SetWordWrap(false);
	DisallowMetaKeys(textView);
	fText->SetUpEditing(textView);

	textView->AddFilter(new BMessageFilter(B_KEY_DOWN, TextViewFilter));

	rect.right = rect.left + textView->LineWidth() + 3;
	// center new width, if necessary
	if (view->ViewMode() == kIconMode
		|| view->ViewMode() == kScaleIconMode
		|| view->ViewMode() == kListMode && fAlignment == B_ALIGN_CENTER) {
		rect.OffsetBy(bounds.Width() / 2 - rect.Width() / 2, 0);
	}

	rect.bottom = rect.top + textView->LineHeight() + 1;
	textRect = rect.OffsetToCopy(2, 1);
	textRect.right -= 3;
	textRect.bottom--;
	textView->SetTextRect(textRect);

	textRect = view->Bounds();
	bool hitBorder = false;
	if (rect.left < 1)
		rect.left = 1, hitBorder = true;
	if (rect.right > textRect.right)
		rect.right = textRect.right - 2, hitBorder = true;

	textView->MoveTo(rect.LeftTop());
	textView->ResizeTo(rect.Width(), rect.Height());

	BScrollView *scrollView = new BScrollView("BorderView", textView, 0, 0, false,
		false, B_PLAIN_BORDER);
	view->AddChild(scrollView);	 

	// configure text view
	switch (view->ViewMode()) {
		case kIconMode:
		case kScaleIconMode:
			textView->SetAlignment(B_ALIGN_CENTER);
			break;

		case kMiniIconMode:
			textView->SetAlignment(B_ALIGN_LEFT);
			break;

		case kListMode:
			textView->SetAlignment(fAlignment);
			break;
	}
	textView->MakeResizable(true, hitBorder ? NULL : scrollView);

	view->SetActivePose(pose);		// tell view about pose
	SetActive(true);				// for widget

	textView->SelectAll();
	textView->MakeFocus();	

	// make this text widget invisible while we edit it
	SetVisible(false);

	ASSERT(view->Window());	// how can I not have a Window here???

	if (view->Window())
		// force immediate redraw so TextView appears instantly
		view->Window()->UpdateIfNeeded();
}


void
BTextWidget::StopEdit(bool saveChanges, BPoint poseLoc, BPoseView *view,
	BPose *pose, int32 poseIndex)
{
	// find the text editing view
	BView *scrollView = view->FindView("BorderView");
	ASSERT(scrollView);
	if (!scrollView)
		return;

	BTextView *textView = dynamic_cast<BTextView *>(scrollView->FindView("WidgetTextView"));
	ASSERT(textView);
	if (!textView)
		return;

	BColumn *column = view->ColumnFor(fAttrHash);
	ASSERT(column);
	if (!column)
		return;

	if (saveChanges && fText->CommitEditedText(textView)) {
		// we have an actual change, re-sort
		view->CheckPoseSortOrder(pose, poseIndex);
	}

	// make text widget visible again
	SetVisible(true);
	view->Invalidate(ColumnRect(poseLoc, column, view));

	// force immediate redraw so TEView disappears
	scrollView->RemoveSelf();
	delete scrollView;

	ASSERT(view->Window());
	view->Window()->UpdateIfNeeded();
	view->MakeFocus();

	SetActive(false);
}


void
BTextWidget::CheckAndUpdate(BPoint loc, const BColumn *column, BPoseView *view)
{
	BRect oldRect;
	if (view->ViewMode() != kListMode)
		oldRect = CalcOldRect(loc, column, view);

	if (fText->CheckAttributeChanged() && fText->CheckViewChanged(view)) {
		BRect invalRect(ColumnRect(loc, column, view));
		if (view->ViewMode() != kListMode)
			invalRect = invalRect | oldRect;
		view->Invalidate(invalRect);
	}
}


void
BTextWidget::SelectAll(BPoseView *view)
{
	BTextView *text = dynamic_cast<BTextView *>(view->FindView("WidgetTextView"));
	if (text)
		text->SelectAll();
}


void
BTextWidget::Draw(BRect eraseRect, BRect textRect, float, BPoseView *view,
	BView *drawView, bool selected, uint32 clipboardMode, BPoint offset, bool direct)
{
	if (direct) {
		// erase area we're going to draw in
		if (view->EraseWidgetTextBackground() || selected) {
			drawView->SetDrawingMode(B_OP_COPY);
			eraseRect.OffsetBy(offset);
			drawView->FillRect(eraseRect, B_SOLID_LOW);
		} else
			drawView->SetDrawingMode(B_OP_OVER);	

		// set high color
		rgb_color highColor;
		if (view->IsDesktopWindow()) {
			if (selected)
				highColor = kWhite;
			else
				highColor = view->DeskTextColor();
		} else if (selected && view->Window()->IsActive() && !view->EraseWidgetTextBackground()) {
			highColor = kWhite;
		} else
			highColor = kBlack;

		if (clipboardMode == kMoveSelectionTo && !selected) {
			view->SetDrawingMode(B_OP_ALPHA);
			view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			highColor.alpha = 64;
		}
		drawView->SetHighColor(highColor);
	}

	BPoint loc;
	textRect.OffsetBy(offset);

	loc.y = textRect.bottom - view->FontInfo().descent;
	loc.x = textRect.left + 1;

	drawView->MovePenTo(loc);
	drawView->DrawString(fText->FittingText(view));

	if (fSymLink && (fAttrHash == view->FirstColumn()->AttrHash())) {
		// ToDo:
		// this should be exported to the WidgetAttribute class, probably
		// by having a per widget kind style
		if (direct) 
			drawView->SetHighColor(125, 125, 125);

		textRect.right = textRect.left + fText->Width(view);
			// only underline text part
		drawView->StrokeLine(textRect.LeftBottom(), textRect.RightBottom(),
			B_MIXED_COLORS);
	}
}
