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
	StringAttributeText* textAttribute = dynamic_cast<StringAttributeText*>(fText);
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
	return fText->PreferredWidth(pose);
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

	BRect rect;
	rect.left = column->Offset() + poseLoc.x;
	rect.right = rect.left + column->Width();
	rect.bottom = poseLoc.y + roundf((view->ListElemHeight() + ActualFontHeight(view)) / 2.f);
	rect.top = rect.bottom - floorf(ActualFontHeight(view));

	return rect;
}


BRect
BTextWidget::CalcRectCommon(BPoint poseLoc, const BColumn* column,
	const BPoseView* view, float textWidth)
{
	BRect rect;
	float viewWidth;

	if (view->ViewMode() == kListMode) {
		viewWidth = ceilf(std::min(column->Width(), textWidth));

		poseLoc.x += column->Offset();

		switch (fAlignment) {
			case B_ALIGN_LEFT:
				rect.left = poseLoc.x;
				rect.right = rect.left + viewWidth;
				break;

			case B_ALIGN_CENTER:
				rect.left = poseLoc.x + roundf((column->Width() - viewWidth) / 2.f);
				if (rect.left < 0)
					rect.left = 0;

				rect.right = rect.left + viewWidth;
				break;

			case B_ALIGN_RIGHT:
				rect.right = poseLoc.x + column->Width();
				rect.left = rect.right - viewWidth;
				if (rect.left < 0)
					rect.left = 0;
				break;

			default:
				TRESPASS();
				break;
		}

		rect.bottom = poseLoc.y + roundf((view->ListElemHeight() + ActualFontHeight(view)) / 2.f);
		rect.top = rect.bottom - floorf(ActualFontHeight(view));
	} else {
		float iconSize = (float)view->IconSizeInt();

		if (view->ViewMode() == kIconMode) {
			// icon mode
			viewWidth = ceilf(std::min(view->StringWidth("M") * 30, textWidth));

			rect.left = poseLoc.x + roundf((iconSize - viewWidth) / 2.f);
			rect.bottom = poseLoc.y + ceilf(view->IconPoseHeight());
			rect.top = rect.bottom - floorf(ActualFontHeight(view));
		} else {
			// mini icon mode
			viewWidth = ceilf(textWidth);

			rect.left = poseLoc.x + iconSize + kMiniIconSeparator;
			rect.bottom = poseLoc.y + roundf((iconSize + ActualFontHeight(view)) / 2.f);
			rect.top = poseLoc.y;
		}

		rect.right = rect.left + viewWidth;
	}

	return rect;
}


BRect
BTextWidget::CalcRect(BPoint poseLoc, const BColumn* column, const BPoseView* view)
{
	return CalcRectCommon(poseLoc, column, view, fText->Width(view));
}


BRect
BTextWidget::CalcOldRect(BPoint poseLoc, const BColumn* column, const BPoseView* view)
{
	return CalcRectCommon(poseLoc, column, view, fText->CurrentWidth());
}


BRect
BTextWidget::CalcClickRect(BPoint poseLoc, const BColumn* column, const BPoseView* view)
{
	BRect rect = CalcRect(poseLoc, column, view);
	if (rect.Width() < kWidthMargin) {
		// if recting rect too narrow, make it a bit wider
		// for comfortable clicking
		if (column != NULL && column->Width() < kWidthMargin)
			rect.right = rect.left + column->Width();
		else
			rect.right = rect.left + kWidthMargin;
	}

	return rect;
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
	ASSERT(view != NULL);
	ASSERT(view->Window() != NULL);

	view->SetTextWidgetToCheck(NULL, this);
	if (!IsEditable() || IsActive())
		return;

	// do not start edit while dragging
	if (view->IsDragging())
		return;

	view->SetActiveTextWidget(this);

	// The initial text color has to be set differently on Desktop
	rgb_color initialTextColor;
	if (view->IsDesktopView())
		initialTextColor = InvertColor(view->HighColor());
	else
		initialTextColor = view->HighColor();

	BRect rect(bounds);
	rect.OffsetBy(view->ViewMode() == kListMode ? -2 : 0, -2);
	BTextView* textView = new BTextView(rect, "WidgetTextView", rect, be_plain_font,
		&initialTextColor, B_FOLLOW_ALL, B_WILL_DRAW);

	textView->SetWordWrap(false);
	textView->SetInsets(2, 2, 2, 2);
	DisallowMetaKeys(textView);
	fText->SetupEditing(textView);

	if (view->IsDesktopView()) {
		// force text view colors to be inverse of Desktop text color, white or black
		rgb_color backColor = view->HighColor();
		rgb_color textColor = InvertColor(backColor);
		backColor = tint_color(backColor,
			view->SelectedVolumeIsReadOnly() ? ReadOnlyTint(backColor) : B_NO_TINT);

		textView->SetViewColor(backColor);
		textView->SetLowColor(backColor);
		textView->SetHighColor(textColor);
	} else {
		// document colors or tooltip colors on Open with... window
		textView->SetViewUIColor(view->ViewUIColor());
		textView->SetLowUIColor(view->LowUIColor());
		textView->SetHighUIColor(view->HighUIColor());
	}

	if (view->SelectedVolumeIsReadOnly()) {
		textView->MakeEditable(false);
		textView->MakeSelectable(true);
	}

	textView->AddFilter(new BMessageFilter(B_KEY_DOWN, TextViewKeyDownFilter));
	if (!view->SelectedVolumeIsReadOnly())
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

	BScrollView* scrollView
		= new BScrollView("BorderView", textView, 0, 0, false, false, B_PLAIN_BORDER);
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

	// force immediate redraw so TextView appears instantly
	view->Window()->UpdateIfNeeded();
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

	BTextView* textView = dynamic_cast<BTextView*>(scrollView->FindView("WidgetTextView"));
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
BTextWidget::CheckAndUpdate(BPoint loc, const BColumn* column, BPoseView* view, bool visible)
{
	BRect oldRect;
	if (view->ViewMode() != kListMode)
		oldRect = CalcOldRect(loc, column, view);

	if (fText->CheckAttributeChanged() && fText->CheckViewChanged(view) && visible) {
		BRect invalidRect(ColumnRect(loc, column, view));
		if (view->ViewMode() != kListMode)
			invalidRect = invalidRect | oldRect;

		view->Invalidate(invalidRect);
	}
}


void
BTextWidget::SelectAll(BPoseView* view)
{
	BTextView* text = dynamic_cast<BTextView*>(view->FindView("WidgetTextView"));
	if (text != NULL)
		text->SelectAll();
}


void
BTextWidget::Draw(BRect eraseRect, BRect textRect, BPoseView* view, BView* drawView,
	bool selected, uint32 clipboardMode, BPoint offset)
{
	ASSERT(view != NULL);
	ASSERT(view->Window() != NULL);
	ASSERT(drawView != NULL);

	textRect.OffsetBy(offset);

	// We are only concerned with setting the correct text color.

	// For active views the selection is drawn as inverse text
	// (background color for the text, solid black for the background).
	// For inactive windows the text is drawn normally, then the
	// selection rect is alpha-blended on top. This all happens in
	// BPose::Draw before and after calling this function.

	bool direct = drawView == view;
	bool dragging = false;
	if (view->Window()->CurrentMessage() != NULL)
		dragging = view->Window()->CurrentMessage()->what == kMsgMouseDragged;

	if (!dragging) {
		if (selected) {
			if (direct) {
				// erase selection rect background
				drawView->SetDrawingMode(B_OP_COPY);
				BRect invertRect(textRect);
				invertRect.left = ceilf(invertRect.left);
				invertRect.top = ceilf(invertRect.top);
				invertRect.right = floorf(invertRect.right);
				invertRect.bottom = floorf(invertRect.bottom);
				drawView->FillRect(invertRect, B_SOLID_LOW);
			}
			drawView->SetDrawingMode(B_OP_OVER);

			// High color is set to inverted low, then the whole thing is
			// inverted again so that the background color "shines through".
			drawView->SetHighColor(InvertColorSmart(drawView->LowColor()));
		} else if (clipboardMode == kMoveSelectionTo) {
			drawView->SetDrawingMode(B_OP_ALPHA);
			drawView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			uint8 alpha = 128; // set the level of opacity by value
			if (drawView->LowColor().IsLight())
				drawView->SetHighColor(0, 0, 0, alpha);
			else
				drawView->SetHighColor(255, 255, 255, alpha);
		} else {
			drawView->SetDrawingMode(B_OP_OVER);
			if (view->IsDesktopView())
				drawView->SetHighColor(view->HighColor());
			else
				drawView->SetHighUIColor(view->HighUIColor());
		}
	} else {
		drawView->SetDrawingMode(B_OP_ALPHA);
		drawView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
		uint8 alpha = 192; // set the level of opacity by value
		if (drawView->LowColor().IsLight())
			drawView->SetHighColor(0, 0, 0, alpha);
		else
			drawView->SetHighColor(255, 255, 255, alpha);
	}

	float decenderHeight = roundf(view->FontInfo().descent);
	BPoint location(textRect.left, textRect.bottom - decenderHeight);

	const char* fittingText = fText->FittingText(view);

	// Draw text outline unless selected or column resizing.
	// The direct parameter is false when dragging or column resizing.
	if (!selected && (direct || dragging) && view->WidgetTextOutline()) {
		// draw a halo around the text by using the "false bold"
		// feature for text rendering. Either black or white is used for
		// the glow (whatever acts as contrast) with a some alpha value,
		if (direct && clipboardMode != kMoveSelectionTo) {
			drawView->SetDrawingMode(B_OP_ALPHA);
			drawView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		}

		BFont font;
		drawView->GetFont(&font);

		rgb_color textColor = drawView->HighColor();
		if (textColor.IsDark()) {
			// dark text on light outline
			rgb_color glowColor = ui_color(B_SHINE_COLOR);

			font.SetFalseBoldWidth(2.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);
			glowColor.alpha = 30;
			drawView->SetHighColor(glowColor);

			drawView->DrawString(fittingText, location);

			font.SetFalseBoldWidth(1.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);
			glowColor.alpha = 65;
			drawView->SetHighColor(glowColor);

			drawView->DrawString(fittingText, location);

			font.SetFalseBoldWidth(0.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);
		} else {
			// light text on dark outline
			rgb_color outlineColor = kBlack;

			font.SetFalseBoldWidth(1.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);
			outlineColor.alpha = 30;
			drawView->SetHighColor(outlineColor);

			drawView->DrawString(fittingText, location);

			font.SetFalseBoldWidth(0.0);
			drawView->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);

			outlineColor.alpha = 200;
			drawView->SetHighColor(outlineColor);

			drawView->DrawString(fittingText, location + BPoint(1, 1));
		}

		if (direct && clipboardMode != kMoveSelectionTo)
			drawView->SetDrawingMode(B_OP_OVER);

		drawView->SetHighColor(textColor);
	}

	drawView->DrawString(fittingText, location);

	if (fSymLink && (fAttrHash == view->FirstColumn()->AttrHash())) {
		// TODO:
		// this should be exported to the WidgetAttribute class, probably
		// by having a per widget kind style
		if (direct && clipboardMode != kMoveSelectionTo) {
			rgb_color underlineColor = drawView->HighColor();
			underlineColor.alpha = 180;

			drawView->SetDrawingMode(B_OP_ALPHA);
			drawView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
			drawView->SetHighColor(underlineColor);
		}

		BRect lineRect(textRect.OffsetByCopy(0, decenderHeight > 2 ? -(decenderHeight - 2) : 0));
			// move underline 2px under text
		lineRect.InsetBy(roundf(textRect.Width() - fText->Width(view)), 0);
			// only underline text part
		drawView->StrokeLine(lineRect.LeftBottom(), lineRect.RightBottom(), B_MIXED_COLORS);

		if (direct && clipboardMode != kMoveSelectionTo)
			drawView->SetDrawingMode(B_OP_OVER);
	}
}
