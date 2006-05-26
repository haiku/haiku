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

//	ListView title drawing and mouse manipulation classes
#include "TitleView.h"

#include <Alert.h>
#include <Application.h>
#include <Debug.h>
#include <PopUpMenu.h>
#include <Window.h>

#include <stdio.h>
#include <string.h>

#include "Commands.h"
#include "ContainerWindow.h"
#include "PoseView.h"
#include "Utilities.h"

static rgb_color sTitleBackground;
static rgb_color sDarkTitleBackground;
static rgb_color sShineColor;
static rgb_color sLightShadowColor;
static rgb_color sShadowColor;
static rgb_color sDarkShadowColor;

const rgb_color kHighlightColor = {100, 100, 210, 255};

const unsigned char kHorizontalResizeCursor[] = {
	16, 1, 7, 7,
	0, 0, 1, 0, 1, 0, 1, 0, 9, 32, 25, 48, 57, 56, 121, 60,
	57, 56, 25, 48, 9, 32, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0,
	3, 128, 3, 128, 3, 128, 15, 224, 31, 240, 63, 248, 127, 252, 255, 254,
	127, 252, 63, 248, 31, 240, 15, 224, 3, 128, 3, 128, 3, 128, 0, 0
};


static void
_DrawLine(BPoseView *view, BPoint from, BPoint to)
{
	rgb_color highColor = view->HighColor();
	view->SetHighColor(kHighlightColor);
	view->StrokeLine(from, to);
	view->SetHighColor(highColor);
}


static void
_UndrawLine(BPoseView *view, BPoint from, BPoint to)
{
	view->StrokeLine(from, to, B_SOLID_LOW);
}


static void
_DrawOutline(BView *view, BRect where)
{
	where.InsetBy(1, 1);
	rgb_color highColor = view->HighColor();
	view->SetHighColor(kHighlightColor);
	view->StrokeRect(where);
	view->SetHighColor(highColor);
}


//	#pragma mark -


BTitleView::BTitleView(BRect frame, BPoseView *view)
	: BView(frame, "TitleView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
	fPoseView(view),
	fTitleList(10, true),
	fHorizontalResizeCursor(kHorizontalResizeCursor),
	fPreviouslyClickedColumnTitle(0)
{
	sTitleBackground = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 0.88f); // 216 -> 220
	sDarkTitleBackground = tint_color(sTitleBackground, B_DARKEN_1_TINT);
	sShineColor = tint_color(sTitleBackground, B_LIGHTEN_MAX_TINT);
	sLightShadowColor = tint_color(sTitleBackground, B_DARKEN_2_TINT);
	sShadowColor = tint_color(sTitleBackground, B_DARKEN_4_TINT);
	sDarkShadowColor = tint_color(sShadowColor, B_DARKEN_2_TINT);

	SetHighColor(sTitleBackground);
	SetLowColor(sTitleBackground);
	SetViewColor(sTitleBackground);
//SetViewColor(B_TRANSPARENT_COLOR);

	BFont font(be_plain_font);
	font.SetSize(9);
	SetFont(&font);

	Reset();
}


BTitleView::~BTitleView()
{
}


void
BTitleView::Reset()
{
	fTitleList.MakeEmpty();
	for (int32 index = 0; ; index++) {
		BColumn *column = fPoseView->ColumnAt(index);
		if (!column)
			break;
		fTitleList.AddItem(new BColumnTitle(this, column));
	}
}


void
BTitleView::AddTitle(BColumn *column, const BColumn *after)
{
	int32 count = fTitleList.CountItems();
	int32 index;
	if (after) {
		for (index = 0; index < count; index++) {
			BColumn *titleColumn = fTitleList.ItemAt(index)->Column();
			
			if (after == titleColumn) {
				index++;
				break;
			}
		}
	} else
		index = count;

	fTitleList.AddItem(new BColumnTitle(this, column), index);
	Invalidate();
}


void
BTitleView::RemoveTitle(BColumn *column)
{
	int32 count = fTitleList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BColumnTitle *title = fTitleList.ItemAt(index);
		if (title->Column() == column) {
			fTitleList.RemoveItem(title);
			break;
		}
	}

	Invalidate();
}


void
BTitleView::Draw(BRect rect)
{
	Draw(rect, false);
}


void 
BTitleView::Draw(BRect, bool useOffscreen, bool updateOnly,
	const BColumnTitle *pressedColumn,
	void (*trackRectBlitter)(BView *, BRect), BRect passThru)
{
	BRect bounds(Bounds());

	BView *view;

	if (useOffscreen) {
		ASSERT(offscreen);
		BRect frame(bounds);
		frame.right += frame.left;
			// this is kind of messy way of avoiding being clipped by the ammount the
			// title is scrolled to the left
			// ToDo:
			//		fix this
		view = offscreen->BeginUsing(frame);
		view->SetOrigin(-bounds.left, 0);
		view->SetLowColor(LowColor());
		view->SetHighColor(HighColor());
		BFont font(be_plain_font);
		font.SetSize(9);
		view->SetFont(&font);
	} else
		view = this;

	// fill background with light gray background
	if (!updateOnly)
		view->FillRect(bounds, B_SOLID_LOW);

	view->BeginLineArray(4);
	view->AddLine(bounds.LeftTop(), bounds.RightTop(), sShadowColor);
	view->AddLine(bounds.LeftBottom(), bounds.RightBottom(), sShadowColor);
	// draw lighter gray and white inset lines
	bounds.InsetBy(0, 1);	
	view->AddLine(bounds.LeftBottom(), bounds.RightBottom(), sLightShadowColor);
	view->AddLine(bounds.LeftTop(), bounds.RightTop(), sShineColor);
	view->EndLineArray();

	int32 count = fTitleList.CountItems();
	float minx = bounds.right;
	float maxx = bounds.left;
	for (int32 index = 0; index < count; index++) {
		BColumnTitle *title = fTitleList.ItemAt(index);
		title->Draw(view, title == pressedColumn);
		BRect titleBounds(title->Bounds());
		if (titleBounds.left < minx)
			minx = titleBounds.left;
		if (titleBounds.right > maxx)
			maxx = titleBounds.right;
	}

	// first and last shades before and after first column
	maxx++;
	minx--;
	view->BeginLineArray(2);
	view->AddLine(BPoint(minx, bounds.top),
				  BPoint(minx, bounds.bottom), sShadowColor);
	view->AddLine(BPoint(maxx, bounds.top),
				  BPoint(maxx, bounds.bottom), sShineColor);
	view->EndLineArray();

	FillRect(BRect(bounds.left, bounds.top + 1, minx - 1, bounds.bottom - 1), B_SOLID_LOW);
	FillRect(BRect(maxx + 1, bounds.top + 1, bounds.right, bounds.bottom - 1), B_SOLID_LOW);

	if (useOffscreen) {
		if (trackRectBlitter)
			(trackRectBlitter)(view, passThru);
		view->Sync();
		DrawBitmap(offscreen->Bitmap());
		offscreen->DoneUsing();
	} else if (trackRectBlitter)
		(trackRectBlitter)(view, passThru);
}


void
BTitleView::MouseDown(BPoint where)
{
	if (!Window()->IsActive()) {
		// wasn't active, just activate and bail
		Window()->Activate();
		return;
	}
	
	// finish any pending edits
	fPoseView->CommitActivePose();

	BColumnTitle *title = FindColumnTitle(where);
	BColumnTitle *resizedTitle = InColumnResizeArea(where);
	
	uint32 buttons;
	GetMouse(&where, &buttons);

	// Check if the user clicked the secondary mouse button.
	// if so, display the attribute menu:

	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		BContainerWindow *window = dynamic_cast<BContainerWindow *>
			(Window());
		BPopUpMenu *menu = new BPopUpMenu("Attributes", false, false);
		menu->SetFont(be_plain_font);
		window->NewAttributeMenu(menu);
		window->AddMimeTypesToMenu(menu);
		window->MarkAttributeMenu(menu);
		menu->SetTargetForItems(window->PoseView());
		menu->Go(ConvertToScreen(where), true, false);
		return;
	}

	bigtime_t doubleClickSpeed;
	get_click_speed(&doubleClickSpeed);

	if (resizedTitle) {
		bool force = static_cast<bool>(buttons & B_TERTIARY_MOUSE_BUTTON);
		if (force || buttons & B_PRIMARY_MOUSE_BUTTON) {
			if (force || fPreviouslyClickedColumnTitle != 0) {
				if (force || system_time() - fPreviousLeftClickTime < doubleClickSpeed) {
					if (fPoseView->ResizeColumnToWidest(resizedTitle->Column())) {
						Invalidate();
						return;
					}
				}
			}
			fPreviousLeftClickTime = system_time();
			fPreviouslyClickedColumnTitle = resizedTitle;
		}
	} else if (!title)
		return;

	ColumnTrackState *trackState;
	if (resizedTitle)
		trackState = new ColumnResizeState(this, resizedTitle, where);
	else
		trackState = new ColumnDragState(this, title, where);

	// track the mouse
	// - if it is pressed shortly and not moved, it is a click 
	//	all else is a track
	bigtime_t pastClickTime = system_time() + doubleClickSpeed;
	bool pastClick = false;
	for (;;) {
		BPoint old(where);

		GetMouse(&where, &buttons);

		if (!buttons) {
			if (!pastClick)
				trackState->Clicked(where, buttons);
			else
				trackState->Done(where);
			break;
		}

		BRect oldMarging(old, old);
		oldMarging.InsetBy(-1, -1);

		if ((pastClick && where != old) || !oldMarging.Contains(where)) {
			// if not pressing yet, use a margin to start, else
			// call moved on any mouse movement
			pastClick = true;
			trackState->MouseMoved(where, buttons);
		}
		if (!pastClick && system_time() > pastClickTime) 
			pastClick = true;

		snooze(15000);
	}

	delete trackState;
}


void 
BTitleView::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	switch (code) {
		default:
			if (InColumnResizeArea(where) && Window()->IsActive())
				SetViewCursor(&fHorizontalResizeCursor);
			else
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			break;
			
		case B_EXITED_VIEW:
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			break;
	}
	_inherited::MouseMoved(where, code, message);
}


BColumnTitle *
BTitleView::InColumnResizeArea(BPoint where) const
{
	int32 count = fTitleList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BColumnTitle *title = fTitleList.ItemAt(index);
		if (title->InColumnResizeArea(where))
			return title;
	}

	return NULL;
}


BColumnTitle *
BTitleView::FindColumnTitle(BPoint where) const
{
	int32 count = fTitleList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BColumnTitle *title = fTitleList.ItemAt(index);
		if (title->Bounds().Contains(where))
			return title;
	}

	return NULL;
}


BColumnTitle *
BTitleView::FindColumnTitle(const BColumn *column) const
{
	int32 count = fTitleList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BColumnTitle *title = fTitleList.ItemAt(index);
		if (title->Column() == column)
			return title;
	}

	return NULL;
}


//	#pragma mark -


BColumnTitle::BColumnTitle(BTitleView *view, BColumn *column)
	:
	fColumn(column),
	fParent(view)
{
}


bool 
BColumnTitle::InColumnResizeArea(BPoint where) const
{
	BRect edge(Bounds());
	edge.left = edge.right - kEdgeSize;
	edge.right += kEdgeSize;

	return edge.Contains(where);
}


BRect
BColumnTitle::Bounds() const
{
	BRect bounds(fColumn->Offset() - kTitleColumnLeftExtraMargin, 0, 0, kTitleViewHeight);
	bounds.right = bounds.left + fColumn->Width() + kTitleColumnExtraMargin;

	return bounds;
}


void
BColumnTitle::Draw(BView *view, bool pressed)
{
	BRect bounds(Bounds());
	BPoint loc(0, bounds.bottom - 4);

	view->SetLowColor(pressed ? sDarkTitleBackground : sTitleBackground);
	view->FillRect(bounds, B_SOLID_LOW);

	BString titleString(fColumn->Title());
	view->TruncateString(&titleString, B_TRUNCATE_END,
		bounds.Width() - kTitleColumnExtraMargin);
	float resultingWidth = view->StringWidth(titleString.String());

	switch (fColumn->Alignment()) {
		case B_ALIGN_LEFT:
			loc.x = bounds.left + 1 + kTitleColumnLeftExtraMargin;
			break;

		case B_ALIGN_CENTER:
			loc.x = bounds.left + (bounds.Width() / 2) - (resultingWidth / 2);
			break;

		case B_ALIGN_RIGHT:
			loc.x = bounds.right - resultingWidth - kTitleColumnRightExtraMargin;
			break;
	}
		
	view->SetHighColor(0, 0, 0);
	view->DrawString(titleString.String(), loc);

	// show sort columns
	bool secondary = (fColumn->AttrHash() == fParent->PoseView()->SecondarySort());
	if (secondary || (fColumn->AttrHash() == fParent->PoseView()->PrimarySort())) {
		BPoint pt1(loc);
		BPoint pt2(view->PenLocation());
		pt1.x--;
		pt2.x--;
		pt1.y++;
		pt2.y++;
		if (secondary)
			view->StrokeLine(pt1, pt2, B_MIXED_COLORS);
		else
			view->StrokeLine(pt1, pt2);
	}

	BRect rect(bounds);

	view->SetHighColor(sShadowColor);
	view->StrokeRect(rect);

	view->BeginLineArray(4);
	// draw lighter gray and white inset lines
	rect.InsetBy(1, 1);	
	view->AddLine(rect.LeftBottom(), rect.RightBottom(),
		pressed ? sLightShadowColor : sLightShadowColor);
	view->AddLine(rect.LeftTop(), rect.RightTop(),
		pressed ? sDarkShadowColor : sShineColor);

	view->AddLine(rect.LeftTop(), rect.LeftBottom(),
		pressed ? sDarkShadowColor : sShineColor);
	view->AddLine(rect.RightTop(), rect.RightBottom(),
		pressed ? sLightShadowColor : sLightShadowColor);

	view->EndLineArray();
}


//	#pragma mark -


ColumnTrackState::ColumnTrackState(BTitleView *view, BColumnTitle *title,
	BPoint where)
	:
	fTitleView(view),
	fTitle(title),
	fLastPos(where)
{
}


void 
ColumnTrackState::MouseMoved(BPoint where, uint32 buttons)
{
	if (!ValueChanged(where))
		return;
	
	Moved(where, buttons);
	fLastPos = where;
}


//	#pragma mark -


ColumnResizeState::ColumnResizeState(BTitleView *view, BColumnTitle *title,
	BPoint where)
	: ColumnTrackState(view, title, where),
	fLastLineDrawPos(-1),
	fInitialTrackOffset((title->fColumn->Offset() + title->fColumn->Width()) - where.x)
{
	DrawLine();
}


bool 
ColumnResizeState::ValueChanged(BPoint where)
{
	float newWidth = where.x + fInitialTrackOffset - fTitle->fColumn->Offset();
	if (newWidth < kMinColumnWidth)
		newWidth = kMinColumnWidth;

	return newWidth != fTitle->fColumn->Width();
}


void 
ColumnResizeState::Moved(BPoint where, uint32)
{
	float newWidth = where.x + fInitialTrackOffset - fTitle->fColumn->Offset();
	if (newWidth < kMinColumnWidth)
		newWidth = kMinColumnWidth;
	
	BPoseView *poseView = fTitleView->PoseView();

//	bool shrink = (newWidth < fTitle->fColumn->Width());
	
	// resize the column 
	poseView->ResizeColumn(fTitle->fColumn, newWidth, &fLastLineDrawPos,
		_DrawLine, _UndrawLine);

	BRect bounds(fTitleView->Bounds());
	bounds.left = fTitle->fColumn->Offset();
	
	// force title redraw
	fTitleView->Draw(bounds, true, false); 
}


void 
ColumnResizeState::Done(BPoint)
{
	UndrawLine();
}


void 
ColumnResizeState::Clicked(BPoint, uint32)
{
	UndrawLine();
}


void 
ColumnResizeState::DrawLine()
{
	BPoseView *poseView = fTitleView->PoseView();
	ASSERT(!poseView->IsDesktopWindow());

	BRect poseViewBounds(poseView->Bounds());
	// remember the line location
	poseViewBounds.left = fTitle->Bounds().right;
	fLastLineDrawPos = poseViewBounds.left;

	// draw the line in the new location
	_DrawLine(poseView, poseViewBounds.LeftTop(), poseViewBounds.LeftBottom());
}


void 
ColumnResizeState::UndrawLine()
{
	if (fLastLineDrawPos < 0)
		return;

	BRect poseViewBounds(fTitleView->PoseView()->Bounds());
	poseViewBounds.left = fLastLineDrawPos;
	
	_UndrawLine(fTitleView->PoseView(), poseViewBounds.LeftTop(),
		poseViewBounds.LeftBottom());
}


//	#pragma mark -


ColumnDragState::ColumnDragState(BTitleView *view, BColumnTitle *columnTitle,
	BPoint where)
	: ColumnTrackState(view, columnTitle, where),
	fInitialMouseTrackOffset(where.x),
	fTrackingRemovedColumn(false)
{
	ASSERT(columnTitle);
	ASSERT(fTitle);
	ASSERT(fTitle->Column());
	DrawPressNoOutline();
}


// ToDo:
// Autoscroll when dragging column left/right
// fix dragging back a column before the first column (now adds as last)
// make column swaps/adds not invalidate/redraw columns to the left
void 
ColumnDragState::Moved(BPoint where, uint32)
{
	// figure out where we are with the mouse
	BRect titleBounds(fTitleView->Bounds());
	bool overTitleView = titleBounds.Contains(where);
	BColumnTitle *overTitle = overTitleView
		? fTitleView->FindColumnTitle(where) : 0;
	BRect titleBoundsWithMargin(titleBounds);
	titleBoundsWithMargin.InsetBy(0, -kRemoveTitleMargin);
	bool inMarginRect = overTitleView || titleBoundsWithMargin.Contains(where);

	bool drawOutline = false;
	bool undrawOutline = false;

	if (fTrackingRemovedColumn) {
		if (overTitleView) {
			// tracked back with a removed title into the title bar, add it
			// back
			fTitleView->EndRectTracking();
			fColumnArchive.Seek(0, SEEK_SET);
			BColumn *column = BColumn::InstantiateFromStream(&fColumnArchive);
			ASSERT(column);
			const BColumn *after = NULL;
			if (overTitle) 
				after = overTitle->Column();
			fTitleView->PoseView()->AddColumn(column, after);
			fTrackingRemovedColumn = false;
			fTitle = fTitleView->FindColumnTitle(column);
			fInitialMouseTrackOffset += fTitle->Bounds().left;
			drawOutline = true;
		}
	} else {
		if (!inMarginRect) {
			// dragged a title out of the hysteresis margin around the
			// title bar - remove it and start dragging it as a dotted outline
			
			BRect rect(fTitle->Bounds());
			rect.OffsetBy(where.x - fInitialMouseTrackOffset, where.y - 5);
			fColumnArchive.Seek(0, SEEK_SET);
			fTitle->Column()->ArchiveToStream(&fColumnArchive);
			fInitialMouseTrackOffset -= fTitle->Bounds().left;
			if (fTitleView->PoseView()->RemoveColumn(fTitle->Column(), false)) {
				fTitle = 0;
				fTitleView->BeginRectTracking(rect);
				fTrackingRemovedColumn = true;
				undrawOutline = true;
			}
		} else if (overTitle && overTitle != fTitle
					// over a different column
				&& (overTitle->Bounds().left >= fTitle->Bounds().right
						// over the one to the right
					|| where.x < overTitle->Bounds().left + fTitle->Bounds().Width())){
						// over the one to the left, far enough to not snap right back
						
			BColumn *column = fTitle->Column();
			fInitialMouseTrackOffset -= fTitle->Bounds().left;
			// swap the columns
			fTitleView->PoseView()->MoveColumnTo(column, overTitle->Column());
			// re-grab the title object looking it up by the column
			fTitle = fTitleView->FindColumnTitle(column);
			// recalc initialMouseTrackOffset
			fInitialMouseTrackOffset += fTitle->Bounds().left;
			drawOutline = true;
		} else
			drawOutline = true;
	}
	
	if (drawOutline)
		DrawOutline(where.x - fInitialMouseTrackOffset);
	else if (undrawOutline)
		UndrawOutline();
}


void 
ColumnDragState::Done(BPoint)
{
	if (fTrackingRemovedColumn)
		fTitleView->EndRectTracking();
	UndrawOutline();
}


void 
ColumnDragState::Clicked(BPoint, uint32)
{
	BPoseView *poseView = fTitleView->PoseView();
	uint32 hash = fTitle->Column()->AttrHash();
	uint32 primarySort = poseView->PrimarySort();
	uint32 secondarySort = poseView->SecondarySort();
	bool shift = (modifiers() & B_SHIFT_KEY) != 0;
	
	// For now:
	// if we hit the primary sort field again
	// then if shift key was down, switch primary and secondary
	if (hash == primarySort) {
		if (shift && secondarySort) {
			poseView->SetPrimarySort(secondarySort);
			poseView->SetSecondarySort(primarySort);
		} else
			poseView->SetReverseSort(!poseView->ReverseSort());
	} else if (shift) {
		// hit secondary sort column with shift key, disable
		if (hash == secondarySort)
			poseView->SetSecondarySort(0);
		else
			poseView->SetSecondarySort(hash);
	} else {
		poseView->SetPrimarySort(hash);
		poseView->SetReverseSort(false);
	}

	if (poseView->PrimarySort() == poseView->SecondarySort())
		poseView->SetSecondarySort(0);

	UndrawOutline();

	poseView->SortPoses();
	poseView->Invalidate();
}


void 
ColumnDragState::Pressing(BPoint, uint32)
{
}


bool 
ColumnDragState::ValueChanged(BPoint)
{
	return true;
}


void 
ColumnDragState::DrawPressNoOutline()
{
	fTitleView->Draw(fTitleView->Bounds(), true, false, fTitle);
}


void 
ColumnDragState::DrawOutline(float pos)
{
	BRect outline(fTitle->Bounds());
	outline.OffsetBy(pos, 0);
	fTitleView->Draw(fTitleView->Bounds(), true, false, fTitle, _DrawOutline, outline);
}


void 
ColumnDragState::UndrawOutline()
{
	fTitleView->Draw(fTitleView->Bounds(), true, false);
}


OffscreenBitmap *BTitleView::offscreen = new OffscreenBitmap;
