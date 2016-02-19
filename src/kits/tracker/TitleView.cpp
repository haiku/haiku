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
#include <ControlLook.h>
#include <Debug.h>
#include <PopUpMenu.h>
#include <Window.h>

#include <stdio.h>
#include <string.h>

#include "Commands.h"
#include "ContainerWindow.h"
#include "PoseView.h"
#include "Utilities.h"


#define APP_SERVER_CLEARS_BACKGROUND 1

static void
_DrawLine(BPoseView* view, BPoint from, BPoint to)
{
	float tint = B_NO_TINT;
	color_which highColor = view->HighUIColor(&tint);
	view->SetHighUIColor(view->LowUIColor(), B_DARKEN_1_TINT);
	view->StrokeLine(from, to);
	view->SetHighUIColor(highColor, tint);
}


static void
_UndrawLine(BPoseView* view, BPoint from, BPoint to)
{
	view->StrokeLine(from, to, B_SOLID_LOW);
}


static void
_DrawOutline(BView* view, BRect where)
{
	where.right++;
	where.bottom--;
	float tint = B_NO_TINT;
	color_which highColor = view->HighUIColor(&tint);
	view->SetHighUIColor(B_CONTROL_HIGHLIGHT_COLOR);
	view->StrokeRect(where);
	view->SetHighUIColor(highColor, tint);
}


//	#pragma mark - BTitleView


BTitleView::BTitleView(BPoseView* view)
	:
	BView("BTitleView", B_WILL_DRAW),
	fPoseView(view),
	fTitleList(10, true),
	fHorizontalResizeCursor(B_CURSOR_ID_RESIZE_EAST_WEST),
	fPreviouslyClickedColumnTitle(0),
	fPreviousLeftClickTime(0),
	fTrackingState(NULL)
{
	SetHighUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
#if APP_SERVER_CLEARS_BACKGROUND
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
#else
	SetViewColor(B_TRANSPARENT_COLOR);
#endif

	BFont font(be_plain_font);
	font.SetSize(floorf(be_plain_font->Size() * 0.75f));
	SetFont(&font);

	font_height height;
	GetFontHeight(&height);
	fPreferredHeight = ceilf(height.ascent + height.descent) + 2;

	Reset();
}


BTitleView::~BTitleView()
{
	delete fTrackingState;
}


void
BTitleView::Reset()
{
	fTitleList.MakeEmpty();

	for (int32 index = 0; ; index++) {
		BColumn* column = fPoseView->ColumnAt(index);
		if (!column)
			break;
		fTitleList.AddItem(new BColumnTitle(this, column));
	}
	Invalidate();
}


void
BTitleView::AddTitle(BColumn* column, const BColumn* after)
{
	int32 count = fTitleList.CountItems();
	int32 index;
	if (after) {
		for (index = 0; index < count; index++) {
			BColumn* titleColumn = fTitleList.ItemAt(index)->Column();

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
BTitleView::RemoveTitle(BColumn* column)
{
	int32 count = fTitleList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BColumnTitle* title = fTitleList.ItemAt(index);
		if (title->Column() == column) {
			fTitleList.RemoveItem(title);
			break;
		}
	}

	Invalidate();
}


BSize
BTitleView::MinSize()
{
	return BSize(16, fPreferredHeight);
}


BSize
BTitleView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, fPreferredHeight);
}


void
BTitleView::Draw(BRect rect)
{
	Draw(rect, false);
}


void
BTitleView::Draw(BRect /*updateRect*/, bool useOffscreen, bool updateOnly,
	const BColumnTitle* pressedColumn,
	void (*trackRectBlitter)(BView*, BRect), BRect passThru)
{
	BRect bounds(Bounds());
	BView* view;

	if (useOffscreen) {
		ASSERT(sOffscreen);
		BRect frame(bounds);
		frame.right += frame.left;
			// ToDo: this is kind of messy way of avoiding being clipped
			// by the amount the title is scrolled to the left
		view = sOffscreen->BeginUsing(frame);
		view->SetOrigin(-bounds.left, 0);
		view->SetLowColor(LowColor());
		view->SetHighColor(HighColor());
		BFont font;
		GetFont(&font);
		view->SetFont(&font);
	} else
		view = this;

	view->SetHighUIColor(B_PANEL_BACKGROUND_COLOR, B_DARKEN_2_TINT);
	view->StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
	bounds.bottom--;

	rgb_color baseColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	be_control_look->DrawButtonBackground(view, bounds, bounds, baseColor, 0,
		BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER);

	int32 count = fTitleList.CountItems();
	float minx = bounds.right;
	float maxx = bounds.left;
	for (int32 index = 0; index < count; index++) {
		BColumnTitle* title = fTitleList.ItemAt(index);
		title->Draw(view, title == pressedColumn);
		BRect titleBounds(title->Bounds());
		if (titleBounds.left < minx)
			minx = titleBounds.left;
		if (titleBounds.right > maxx)
			maxx = titleBounds.right;
	}

	bounds = Bounds();
	minx--;
	view->SetHighUIColor(B_PANEL_BACKGROUND_COLOR, B_DARKEN_1_TINT);
	view->StrokeLine(BPoint(minx, bounds.top),
		BPoint(minx, bounds.bottom - 1));

#if !(APP_SERVER_CLEARS_BACKGROUND)
	FillRect(BRect(bounds.left, bounds.top + 1, minx - 1, bounds.bottom - 1),
		B_SOLID_LOW);
	FillRect(BRect(maxx + 1, bounds.top + 1, bounds.right, bounds.bottom - 1),
		B_SOLID_LOW);
#endif

	if (useOffscreen) {
		if (trackRectBlitter)
			(trackRectBlitter)(view, passThru);

		view->Sync();
		DrawBitmap(sOffscreen->Bitmap());
		sOffscreen->DoneUsing();
	} else if (trackRectBlitter)
		(trackRectBlitter)(view, passThru);
}


void
BTitleView::MouseDown(BPoint where)
{
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	if (window == NULL)
		return;

	if (!window->IsActive()) {
		// window wasn't active, activate it and bail
		window->Activate();
		return;
	}

	// finish any pending edits
	fPoseView->CommitActivePose();

	BColumnTitle* title = FindColumnTitle(where);
	BColumnTitle* resizedTitle = InColumnResizeArea(where);

	uint32 buttons;
	GetMouse(&where, &buttons);

	// Check if the user clicked the secondary mouse button.
	// if so, display the attribute menu:

	if (SecondaryMouseButtonDown(modifiers(), buttons)) {
		BPopUpMenu* menu = new BPopUpMenu("Attributes", false, false);
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
				if (force || system_time() - fPreviousLeftClickTime
						< doubleClickSpeed) {
					if (fPoseView->
							ResizeColumnToWidest(resizedTitle->Column())) {
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

	SetMouseEventMask(B_POINTER_EVENTS,
		B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);

	// track the mouse
	if (resizedTitle) {
		fTrackingState = new ColumnResizeState(this, resizedTitle, where,
			system_time() + doubleClickSpeed);
	} else {
		fTrackingState = new ColumnDragState(this, title, where,
			system_time() + doubleClickSpeed);
	}
}


void
BTitleView::MouseUp(BPoint where)
{
	if (fTrackingState == NULL)
		return;

	fTrackingState->MouseUp(where);

	delete fTrackingState;
	fTrackingState = NULL;
}


void
BTitleView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	if (window == NULL)
		return;

	if (fTrackingState != NULL) {
		int32 buttons = 0;
		if (Looper() != NULL && Looper()->CurrentMessage() != NULL)
			Looper()->CurrentMessage()->FindInt32("buttons", &buttons);
		fTrackingState->MouseMoved(where, buttons);
		return;
	}

	switch (code) {
		default:
			if (InColumnResizeArea(where) && window->IsActive())
				SetViewCursor(&fHorizontalResizeCursor);
			else
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			break;

		case B_EXITED_VIEW:
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			break;
	}

	_inherited::MouseMoved(where, code, dragMessage);
}


BColumnTitle*
BTitleView::InColumnResizeArea(BPoint where) const
{
	int32 count = fTitleList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BColumnTitle* title = fTitleList.ItemAt(index);
		if (title->InColumnResizeArea(where))
			return title;
	}

	return NULL;
}


BColumnTitle*
BTitleView::FindColumnTitle(BPoint where) const
{
	int32 count = fTitleList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BColumnTitle* title = fTitleList.ItemAt(index);
		if (title->Bounds().Contains(where))
			return title;
	}

	return NULL;
}


BColumnTitle*
BTitleView::FindColumnTitle(const BColumn* column) const
{
	int32 count = fTitleList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BColumnTitle* title = fTitleList.ItemAt(index);
		if (title->Column() == column)
			return title;
	}

	return NULL;
}


//	#pragma mark - BColumnTitle


BColumnTitle::BColumnTitle(BTitleView* view, BColumn* column)
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
	BRect bounds(fColumn->Offset() - kTitleColumnLeftExtraMargin, 0, 0,
		fParent->Bounds().Height());
	bounds.right = bounds.left + fColumn->Width() + kTitleColumnExtraMargin;

	return bounds;
}


void
BColumnTitle::Draw(BView* view, bool pressed)
{
	BRect bounds(Bounds());

	font_height height;
	view->GetFontHeight(&height);
	BPoint loc(0, bounds.top + ceilf(height.ascent) + 2);
	rgb_color baseColor = ui_color(B_PANEL_BACKGROUND_COLOR);

	if (pressed) {
		bounds.bottom--;
		BRect rect(bounds);
		rect.right--;
		baseColor = tint_color(baseColor, B_DARKEN_1_TINT);

		be_control_look->DrawButtonBackground(view, rect, rect, baseColor, 0,
			BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER);
	}

	BString titleString(fColumn->Title());
	view->TruncateString(&titleString, B_TRUNCATE_END,
		bounds.Width() - kTitleColumnExtraMargin);
	float resultingWidth = view->StringWidth(titleString.String());

	switch (fColumn->Alignment()) {
		case B_ALIGN_LEFT:
		default:
			loc.x = bounds.left + 1 + kTitleColumnLeftExtraMargin;
			break;

		case B_ALIGN_CENTER:
			loc.x = bounds.left + (bounds.Width() / 2) - (resultingWidth / 2);
			break;

		case B_ALIGN_RIGHT:
			loc.x = bounds.right - resultingWidth
				- kTitleColumnRightExtraMargin;
			break;
	}

	view->SetHighUIColor(B_PANEL_TEXT_COLOR, pressed ? B_DARKEN_1_TINT : 1.0f);
	view->SetLowColor(baseColor);
	view->DrawString(titleString.String(), loc);

	// show sort columns
	bool secondary
		= (fColumn->AttrHash() == fParent->PoseView()->SecondarySort());
	if (secondary
		|| (fColumn->AttrHash() == fParent->PoseView()->PrimarySort())) {

		BPoint center(loc.x - 6, roundf((bounds.top + bounds.bottom) / 2.0));
		BPoint triangle[3];
		if (fParent->PoseView()->ReverseSort()) {
			triangle[0] = center + BPoint(-3.5, 1.5);
			triangle[1] = center + BPoint(3.5, 1.5);
			triangle[2] = center + BPoint(0.0, -2.0);
		} else {
			triangle[0] = center + BPoint(-3.5, -1.5);
			triangle[1] = center + BPoint(3.5, -1.5);
			triangle[2] = center + BPoint(0.0, 2.0);
		}

		uint32 flags = view->Flags();
		view->SetFlags(flags | B_SUBPIXEL_PRECISE);

		if (secondary) {
			view->SetHighUIColor(B_PANEL_BACKGROUND_COLOR, 1.3);
			view->FillTriangle(triangle[0], triangle[1], triangle[2]);
		} else {
			view->SetHighUIColor(B_PANEL_BACKGROUND_COLOR, 1.6);
			view->FillTriangle(triangle[0], triangle[1], triangle[2]);
		}

		view->SetFlags(flags);
	}

	view->SetHighUIColor(B_PANEL_BACKGROUND_COLOR, B_DARKEN_1_TINT);
	view->StrokeLine(bounds.RightTop(), bounds.RightBottom());
}


//	#pragma mark - ColumnTrackState


ColumnTrackState::ColumnTrackState(BTitleView* view, BColumnTitle* title,
	BPoint where, bigtime_t pastClickTime)
	:
	fTitleView(view),
	fTitle(title),
	fFirstClickPoint(where),
	fPastClickTime(pastClickTime),
	fHasMoved(false)
{
}


void
ColumnTrackState::MouseUp(BPoint where)
{
	// if it is pressed shortly and not moved, it is a click
	// else it is a track
	if (system_time() <= fPastClickTime && !fHasMoved)
		Clicked(where);
	else
		Done(where);
}


void
ColumnTrackState::MouseMoved(BPoint where, uint32 buttons)
{
	if (!fHasMoved && system_time() < fPastClickTime) {
		BRect moveMargin(fFirstClickPoint, fFirstClickPoint);
		moveMargin.InsetBy(-1, -1);

		if (moveMargin.Contains(where))
			return;
	}

	Moved(where, buttons);
	fHasMoved = true;
}


//	#pragma mark - ColumnResizeState


ColumnResizeState::ColumnResizeState(BTitleView* view, BColumnTitle* title,
		BPoint where, bigtime_t pastClickTime)
	:
	ColumnTrackState(view, title, where, pastClickTime),
	fLastLineDrawPos(-1),
	fInitialTrackOffset((title->fColumn->Offset() + title->fColumn->Width())
		- where.x)
{
	DrawLine();
}


bool
ColumnResizeState::ValueChanged(BPoint where)
{
	float newWidth = where.x + fInitialTrackOffset
		- fTitle->fColumn->Offset();
	if (newWidth < kMinColumnWidth)
		newWidth = kMinColumnWidth;

	return newWidth != fTitle->fColumn->Width();
}


void
ColumnResizeState::Moved(BPoint where, uint32)
{
	float newWidth = where.x + fInitialTrackOffset
		- fTitle->fColumn->Offset();
	if (newWidth < kMinColumnWidth)
		newWidth = kMinColumnWidth;

	BPoseView* poseView = fTitleView->PoseView();

	//bool shrink = (newWidth < fTitle->fColumn->Width());

	// resize the column
	poseView->ResizeColumn(fTitle->fColumn, newWidth, &fLastLineDrawPos,
		_DrawLine, _UndrawLine);

	BRect bounds(fTitleView->Bounds());
	bounds.left = fTitle->fColumn->Offset();

	// force title redraw
	fTitleView->Draw(bounds, true, false);
}


void
ColumnResizeState::Done(BPoint /*where*/)
{
	UndrawLine();
}


void
ColumnResizeState::Clicked(BPoint /*where*/)
{
	UndrawLine();
}


void
ColumnResizeState::DrawLine()
{
	BPoseView* poseView = fTitleView->PoseView();
	ASSERT(!poseView->IsDesktopWindow());

	BRect poseViewBounds(poseView->Bounds());
	// remember the line location
	poseViewBounds.left = fTitle->Bounds().right;
	fLastLineDrawPos = poseViewBounds.left;

	// draw the line in the new location
	_DrawLine(poseView, poseViewBounds.LeftTop(),
		poseViewBounds.LeftBottom());
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


//	#pragma mark - ColumnDragState


ColumnDragState::ColumnDragState(BTitleView* view, BColumnTitle* columnTitle,
	BPoint where, bigtime_t pastClickTime)
	:
	ColumnTrackState(view, columnTitle, where, pastClickTime),
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
	BColumnTitle* overTitle = overTitleView
		? fTitleView->FindColumnTitle(where) : 0;
	BRect titleBoundsWithMargin(titleBounds);
	titleBoundsWithMargin.InsetBy(0, -kRemoveTitleMargin);
	bool inMarginRect = overTitleView
		|| titleBoundsWithMargin.Contains(where);

	bool drawOutline = false;
	bool undrawOutline = false;

	if (fTrackingRemovedColumn) {
		if (overTitleView) {
			// tracked back with a removed title into the title bar, add it
			// back
			fTitleView->EndRectTracking();
			fColumnArchive.Seek(0, SEEK_SET);
			BColumn* column = BColumn::InstantiateFromStream(&fColumnArchive);
			ASSERT(column);
			const BColumn* after = NULL;
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
			if (fTitleView->PoseView()->RemoveColumn(fTitle->Column(),
					false)) {
				fTitle = 0;
				fTitleView->BeginRectTracking(rect);
				fTrackingRemovedColumn = true;
				undrawOutline = true;
			}
		} else if (overTitle && overTitle != fTitle
					// over a different column
				&& (overTitle->Bounds().left >= fTitle->Bounds().right
						// over the one to the right
					|| where.x < overTitle->Bounds().left
							+ fTitle->Bounds().Width())) {
						// over the one to the left, far enough to not snap
						// right back

			BColumn* column = fTitle->Column();
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
ColumnDragState::Done(BPoint /*where*/)
{
	if (fTrackingRemovedColumn)
		fTitleView->EndRectTracking();
	UndrawOutline();
}


void
ColumnDragState::Clicked(BPoint /*where*/)
{
	BPoseView* poseView = fTitleView->PoseView();
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
	fTitleView->Draw(fTitleView->Bounds(), true, false, fTitle, _DrawOutline,
		outline);
}


void
ColumnDragState::UndrawOutline()
{
	fTitleView->Draw(fTitleView->Bounds(), true, false);
}


OffscreenBitmap* BTitleView::sOffscreen = new OffscreenBitmap;
