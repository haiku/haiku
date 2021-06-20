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


#include "TextWidget.h"

#include <string.h>
#include <stdlib.h>

#include <Alert.h>
#include <Catalog.h>
#include <Clipboard.h>
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
#include "Utilities.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TextWidget"


const float kWidthMargin = 20;


//	#pragma mark - BTextWidget


BTextWidget::BTextWidget(Model* model, BColumn* column, BPoseView* view)
	:
	fText(WidgetAttributeText::NewWidgetText(model, column, view)),
	fAttrHash(column->AttrHash()),
	fAlignment(column->Alignment()),
	fEditable(column->Editable()),
	fVisible(true),
	fActive(false),
	fSymLink(model->IsSymLink()),
	fMaxWidth(0),
	fLastClickedTime(0)
{
}


BTextWidget::~BTextWidget()
{
	if (fLastClickedTime != 0)
		fParams.poseView->SetTextWidgetToCheck(NULL, this);

	delete fText;
}


int
BTextWidget::Compare(const BTextWidget& with, BPoseView* view) const
{
	return fText->Compare(*with.fText, view);
}


const char*
BTextWidget::Text(const BPoseView* view) const
{
	StringAttributeText* textAttribute
		= dynamic_cast<StringAttributeText*>(fText);
	if (textAttribute == NULL)
		return NULL;

	return textAttribute->ValueAsText(view);
}


float
BTextWidget::TextWidth(const BPoseView* pose) const
{
	return fText->Width(pose);
}


float
BTextWidget::PreferredWidth(const BPoseView* pose) const
{
	return fText->PreferredWidth(pose) + 1;
}


BRect
BTextWidget::ColumnRect(BPoint poseLoc, const BColumn* column,
	const BPoseView* view)
{
	if (view->ViewMode() != kListMode) {
		// ColumnRect only makes sense in list view, return
		// CalcRect otherwise
		return CalcRect(poseLoc, column, view);
	}
	BRect result;
	result.left = column->Offset() + poseLoc.x;
	result.right = result.left + column->Width();
	result.bottom = poseLoc.y
		+ roundf((view->ListElemHeight() + view->FontHeight()) / 2);
	result.top = result.bottom - floorf(view->FontHeight());
	return result;
}


BRect
BTextWidget::CalcRectCommon(BPoint poseLoc, const BColumn* column,
	const BPoseView* view, float textWidth)
{
	BRect result;
	float viewWidth = textWidth;

	if (view->ViewMode() == kListMode) {
		viewWidth = std::min(column->Width(), textWidth);

		poseLoc.x += column->Offset();

		switch (fAlignment) {
			case B_ALIGN_LEFT:
				result.left = poseLoc.x;
				result.right = result.left + viewWidth;
				break;

			case B_ALIGN_CENTER:
				result.left = poseLoc.x
					+ roundf((column->Width() - viewWidth) / 2);
				if (result.left < 0)
					result.left = 0;

				result.right = result.left + viewWidth;
				break;

			case B_ALIGN_RIGHT:
				result.right = poseLoc.x + column->Width();
				result.left = result.right - viewWidth;
				if (result.left < 0)
					result.left = 0;
				break;

			default:
				TRESPASS();
				break;
		}

		result.bottom = poseLoc.y
			+ roundf((view->ListElemHeight() + view->FontHeight()) / 2);
	} else {
		viewWidth = std::min(view->StringWidth("M") * 30, textWidth);
		if (view->ViewMode() == kIconMode) {
			// icon mode
			result.left = poseLoc.x
				+ roundf((view->IconSizeInt() - viewWidth) / 2);
			result.bottom = poseLoc.y + view->IconPoseHeight();
		} else {
			// mini icon mode
			result.left = poseLoc.x + B_MINI_ICON + kMiniIconSeparator;
			result.bottom = poseLoc.y
				+ roundf((B_MINI_ICON + view->FontHeight()) / 2);
		}

		result.right = result.left + viewWidth;
	}

	result.top = result.bottom - floorf(view->FontHeight());

	return result;
}


BRect
BTextWidget::CalcRect(BPoint poseLoc, const BColumn* column,
	const BPoseView* view)
{
	return CalcRectCommon(poseLoc, column, view, fText->Width(view));
}


BRect
BTextWidget::CalcOldRect(BPoint poseLoc, const BColumn* column,
	const BPoseView* view)
{
	return CalcRectCommon(poseLoc, column, view, fText->CurrentWidth());
}


BRect
BTextWidget::CalcClickRect(BPoint poseLoc, const BColumn* column,
	const BPoseView* view)
{
	BRect result = CalcRect(poseLoc, column, view);
	if (result.Width() < kWidthMargin) {
		// if resulting rect too narrow, make it a bit wider
		// for comfortable clicking
		if (column != NULL && column->Width() < kWidthMargin)
			result.right = result.left + column->Width();
		else
			result.right = result.left + kWidthMargin;
	}

	return result;
}


void
BTextWidget::CheckExpiration()
{
	if (IsEditable() && fParams.pose->IsSelected() && fLastClickedTime) {
		bigtime_t doubleClickSpeed;
		get_click_speed(&doubleClickSpeed);

		bigtime_t delta = system_time() - fLastClickedTime;

		if (delta > doubleClickSpeed) {
			// at least 'doubleClickSpeed' microseconds ellapsed and no click
			// was registered since.
			fLastClickedTime = 0;
			StartEdit(fParams.bounds, fParams.poseView, fParams.pose);
		}
	} else {
		fLastClickedTime = 0;
		fParams.poseView->SetTextWidgetToCheck(NULL);
	}
}


void
BTextWidget::CancelWait()
{
	fLastClickedTime = 0;
	fParams.poseView->SetTextWidgetToCheck(NULL);
}


void
BTextWidget::MouseUp(BRect bounds, BPoseView* view, BPose* pose, BPoint)
{
	// Register the time of that click.  The PoseView, through its Pulse()
	// will allow us to StartEdit() if no other click have been registered since
	// then.

	// TODO: re-enable modifiers, one should be enough
	view->SetTextWidgetToCheck(NULL);
	if (IsEditable() && pose->IsSelected()) {
		bigtime_t doubleClickSpeed;
		get_click_speed(&doubleClickSpeed);

		if (fLastClickedTime == 0) {
			fLastClickedTime = system_time();
			if (fLastClickedTime - doubleClickSpeed < pose->SelectionTime())
				fLastClickedTime = 0;
		} else
			fLastClickedTime = 0;

		if (fLastClickedTime == 0)
			return;

		view->SetTextWidgetToCheck(this);

		fParams.pose = pose;
		fParams.bounds = bounds;
		fParams.poseView = view;
	} else
		fLastClickedTime = 0;
}


static filter_result
TextViewKeyDownFilter(BMessage* message, BHandler**, BMessageFilter* filter)
{
	uchar key;
	if (message->FindInt8("byte", (int8*)&key) != B_OK)
		return B_DISPATCH_MESSAGE;

	ThrowOnAssert(filter != NULL);

	BContainerWindow* window = dynamic_cast<BContainerWindow*>(
		filter->Looper());
	ThrowOnAssert(window != NULL);

	BPoseView* view = window->PoseView();
	ThrowOnAssert(view != NULL);

	if (key == B_RETURN || key == B_ESCAPE) {
		view->CommitActivePose(key == B_RETURN);
		return B_SKIP_MESSAGE;
	}

	if (key == B_TAB) {
		if (view->ActivePose()) {
			if (message->FindInt32("modifiers") & B_SHIFT_KEY)
				view->ActivePose()->EditPreviousWidget(view);
			else
				view->ActivePose()->EditNextWidget(view);
		}

		return B_SKIP_MESSAGE;
	}

	// the BTextView doesn't respect window borders when resizing itself;
	// we try to work-around this "bug" here.

	// find the text editing view
	BView* scrollView = view->FindView("BorderView");
	if (scrollView != NULL) {
		BTextView* textView = dynamic_cast<BTextView*>(
			scrollView->FindView("WidgetTextView"));
		if (textView != NULL) {
			ASSERT(view->ActiveTextWidget() != NULL);
			float maxWidth = view->ActiveTextWidget()->MaxWidth();
			bool tooWide = textView->TextRect().Width() > maxWidth;
			textView->MakeResizable(!tooWide, tooWide ? NULL : scrollView);
		}
	}

	return B_DISPATCH_MESSAGE;
}


static filter_result
TextViewPasteFilter(BMessage* message, BHandler**, BMessageFilter* filter)
{
	ThrowOnAssert(filter != NULL);

	BContainerWindow* window = dynamic_cast<BContainerWindow*>(
		filter->Looper());
	ThrowOnAssert(window != NULL);

	BPoseView* view = window->PoseView();
	ThrowOnAssert(view != NULL);

	// the BTextView doesn't respect window borders when resizing itself;
	// we try to work-around this "bug" here.

	// find the text editing view
	BView* scrollView = view->FindView("BorderView");
	if (scrollView != NULL) {
		BTextView* textView = dynamic_cast<BTextView*>(
			scrollView->FindView("WidgetTextView"));
		if (textView != NULL) {
			float textWidth = textView->TextRect().Width();

			// subtract out selected text region width
			int32 start, finish;
			textView->GetSelection(&start, &finish);
			if (start != finish) {
				BRegion selectedRegion;
				textView->GetTextRegion(start, finish, &selectedRegion);
				textWidth -= selectedRegion.Frame().Width();
			}

			// add pasted text width
			if (be_clipboard->Lock()) {
				BMessage* clip = be_clipboard->Data();
				if (clip != NULL) {
					const char* text = NULL;
					ssize_t length = 0;

					if (clip->FindData("text/plain", B_MIME_TYPE,
							(const void**)&text, &length) == B_OK) {
						textWidth += textView->StringWidth(text);
					}
				}	

				be_clipboard->Unlock();
			}

			// check if pasted text is too wide
			ASSERT(view->ActiveTextWidget() != NULL);
			float maxWidth = view->ActiveTextWidget()->MaxWidth();
			bool tooWide = textWidth > maxWidth;

			if (tooWide) {
				// resize text view to max width

				// move scroll view if not left aligned
				float oldWidth = textView->Bounds().Width();
				float newWidth = maxWidth;
				float right = oldWidth - newWidth;

				if (textView->Alignment() == B_ALIGN_CENTER)
					scrollView->MoveBy(roundf(right / 2), 0);
				else if (textView->Alignment() == B_ALIGN_RIGHT)
					scrollView->MoveBy(right, 0);

				// resize scroll view
				float grow = newWidth - oldWidth;
				scrollView->ResizeBy(grow, 0);
			}

			textView->MakeResizable(!tooWide, tooWide ? NULL : scrollView);
		}
	}

	return B_DISPATCH_MESSAGE;
}


void
BTextWidget::StartEdit(BRect bounds, BPoseView* view, BPose* pose)
{
	view->SetTextWidgetToCheck(NULL, this);
	if (!IsEditable() || IsActive())
		return;

	BEntry entry(pose->TargetModel()->EntryRef());
	if (entry.InitCheck() == B_OK
		&& !ConfirmChangeIfWellKnownDirectory(&entry, kRename)) {
		return;
	}

	view->SetActiveTextWidget(this);

	// TODO fix text rect being off by a pixel on some files

	BRect rect(bounds);
	rect.OffsetBy(view->ViewMode() == kListMode ? -1 : 1, -4);
	BTextView* textView = new BTextView(rect, "WidgetTextView", rect,
		be_plain_font, 0, B_FOLLOW_ALL, B_WILL_DRAW);

	textView->SetWordWrap(false);
	textView->SetInsets(2, 2, 2, 2);
	DisallowMetaKeys(textView);
	fText->SetUpEditing(textView);

	textView->AddFilter(new BMessageFilter(B_KEY_DOWN, TextViewKeyDownFilter));
	textView->AddFilter(new BMessageFilter(B_PASTE, TextViewPasteFilter));

	// get full text length
	rect.right = rect.left + textView->LineWidth();
	rect.bottom = rect.top + textView->LineHeight() - 1 + 4;

	if (view->ViewMode() == kListMode) {
		// limit max width to column width in list mode
		BColumn* column = view->ColumnFor(fAttrHash);
		ASSERT(column != NULL);
		fMaxWidth = column->Width();
	} else {
		// limit max width to 30em in icon and mini icon mode
		fMaxWidth = textView->StringWidth("M") * 30;

		if (textView->LineWidth() > fMaxWidth
			|| view->ViewMode() == kMiniIconMode) {
			// compensate for text going over right inset
			rect.OffsetBy(-2, 0);
		}
	}

	// resize textView
	textView->MoveTo(rect.LeftTop());
	textView->ResizeTo(std::min(fMaxWidth, rect.Width()), rect.Height());
	textView->SetTextRect(rect);

	// set alignment before adding textView so it doesn't redraw
	switch (view->ViewMode()) {
		case kIconMode:
			textView->SetAlignment(B_ALIGN_CENTER);
			break;

		case kMiniIconMode:
			textView->SetAlignment(B_ALIGN_LEFT);
			break;

		case kListMode:
			textView->SetAlignment(fAlignment);
			break;
	}

	BScrollView* scrollView = new BScrollView("BorderView", textView, 0, 0,
		false, false, B_PLAIN_BORDER);
	view->AddChild(scrollView);

	bool tooWide = textView->TextRect().Width() > fMaxWidth;
	textView->MakeResizable(!tooWide, tooWide ? NULL : scrollView);

	view->SetActivePose(pose);
		// tell view about pose
	SetActive(true);
		// for widget

	textView->SelectAll();
	textView->ScrollToSelection();
		// scroll to beginning so that text is visible
	textView->MakeFocus();

	// make this text widget invisible while we edit it
	SetVisible(false);

	ASSERT(view->Window() != NULL);
		// how can I not have a Window here???

	if (view->Window()) {
		// force immediate redraw so TextView appears instantly
		view->Window()->UpdateIfNeeded();
	}
}


void
BTextWidget::StopEdit(bool saveChanges, BPoint poseLoc, BPoseView* view,
	BPose* pose, int32 poseIndex)
{
	view->SetActiveTextWidget(NULL);

	// find the text editing view
	BView* scrollView = view->FindView("BorderView");
	ASSERT(scrollView != NULL);
	if (scrollView == NULL)
		return;

	BTextView* textView = dynamic_cast<BTextView*>(
		scrollView->FindView("WidgetTextView"));
	ASSERT(textView != NULL);
	if (textView == NULL)
		return;

	BColumn* column = view->ColumnFor(fAttrHash);
	ASSERT(column != NULL);
	if (column == NULL)
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

	ASSERT(view->Window() != NULL);
	view->Window()->UpdateIfNeeded();
	view->MakeFocus();

	SetActive(false);
}


void
BTextWidget::CheckAndUpdate(BPoint loc, const BColumn* column,
	BPoseView* view, bool visible)
{
	BRect oldRect;
	if (view->ViewMode() != kListMode)
		oldRect = CalcOldRect(loc, column, view);

	if (fText->CheckAttributeChanged() && fText->CheckViewChanged(view)
		&& visible) {
		BRect invalRect(ColumnRect(loc, column, view));
		if (view->ViewMode() != kListMode)
			invalRect = invalRect | oldRect;
		view->Invalidate(invalRect);
	}
}


void
BTextWidget::SelectAll(BPoseView* view)
{
	BTextView* text = dynamic_cast<BTextView*>(
		view->FindView("WidgetTextView"));
	if (text != NULL)
		text->SelectAll();
}


void
BTextWidget::Draw(BRect eraseRect, BRect textRect, float, BPoseView* view,
	BView* drawView, bool selected, uint32 clipboardMode, BPoint offset,
	bool direct)
{
	textRect.OffsetBy(offset);

	if (direct) {
		// draw selection box if selected
		if (selected) {
			drawView->SetDrawingMode(B_OP_COPY);
//			eraseRect.OffsetBy(offset);
//			drawView->FillRect(eraseRect, B_SOLID_LOW);
			drawView->FillRect(textRect, B_SOLID_LOW);
		} else
			drawView->SetDrawingMode(B_OP_OVER);

		// set high color
		rgb_color highColor;
		// for active views, the selection is drawn as inverse text (background color for the text,
		// solid black for the background).
		// For inactive windows, the text is drawn normally, then the selection rect is
		// alpha-blended on top of it.
		// This all happens in BPose::Draw before and after calling this function, here we are
		// only concerned with setting the correct color for the text.
		if (selected && view->Window()->IsActive())
			highColor = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
		else
			highColor = view->DeskTextColor();

		if (clipboardMode == kMoveSelectionTo && !selected) {
			drawView->SetDrawingMode(B_OP_ALPHA);
			drawView->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			highColor.alpha = 64;
		}
		drawView->SetHighColor(highColor);
	}

	BPoint loc;
	loc.y = textRect.bottom - view->FontInfo().descent;
	loc.x = textRect.left + 1;

	const char* fittingText = fText->FittingText(view);

	// TODO: Comparing view and drawView here to avoid rendering
	// the text outline when producing a drag bitmap. The check is
	// not fully correct, since an offscreen view is also used in some
	// other rare cases (something to do with columns). But for now, this
	// fixes the broken drag bitmaps when dragging icons from the Desktop.
	if (!selected && view == drawView && view->WidgetTextOutline()) {
		// draw a halo around the text by using the "false bold"
		// feature for text rendering. Either black or white is used for
		// the glow (whatever acts as contrast) with a some alpha value,
		drawView->SetDrawingMode(B_OP_ALPHA);
		drawView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);

		BFont font;
		drawView->GetFont(&font);

		rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
		if (view->IsDesktopWindow())
			textColor = view->DeskTextColor();

		if (textColor.Brightness() < 100) {
			// dark text on light outline
			rgb_color glowColor = ui_color(B_SHINE_COLOR);

			font.SetFalseBoldWidth(2.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);
			glowColor.alpha = 30;
			drawView->SetHighColor(glowColor);

			drawView->DrawString(fittingText, loc);

			font.SetFalseBoldWidth(1.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);
			glowColor.alpha = 65;
			drawView->SetHighColor(glowColor);

			drawView->DrawString(fittingText, loc);

			font.SetFalseBoldWidth(0.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);
		} else {
			// light text on dark outline
			rgb_color outlineColor = kBlack;

			font.SetFalseBoldWidth(1.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);
			outlineColor.alpha = 30;
			drawView->SetHighColor(outlineColor);

			drawView->DrawString(fittingText, loc);

			font.SetFalseBoldWidth(0.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);

			outlineColor.alpha = 200;
			drawView->SetHighColor(outlineColor);

			drawView->DrawString(fittingText, loc + BPoint(1, 1));
		}

		drawView->SetDrawingMode(B_OP_OVER);
		drawView->SetHighColor(textColor);
	}

	drawView->DrawString(fittingText, loc);

	if (fSymLink && (fAttrHash == view->FirstColumn()->AttrHash())) {
		// TODO:
		// this should be exported to the WidgetAttribute class, probably
		// by having a per widget kind style
		if (direct) {
			rgb_color underlineColor = drawView->HighColor();
			underlineColor.alpha = 180;
			drawView->SetHighColor(underlineColor);
			drawView->SetDrawingMode(B_OP_ALPHA);
			drawView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		}

		textRect.right = textRect.left + fText->Width(view);
			// only underline text part
		drawView->StrokeLine(textRect.LeftBottom(), textRect.RightBottom(),
			B_MIXED_COLORS);
	}
}
