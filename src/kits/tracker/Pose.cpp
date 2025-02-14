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


#include "Pose.h"

#include <stdlib.h>
#include <string.h>

#include <Debug.h>
#include <NodeMonitor.h>
#include <Volume.h>
#include <fs_info.h>

#include "Attributes.h"
#include "Commands.h"
#include "FSClipboard.h"
#include "IconCache.h"
#include "PoseView.h"


int32
CalcFreeSpace(BVolume* volume)
{
	off_t capacity = volume->Capacity();
	if (capacity == 0)
		return 100;

	int32 percent
		= static_cast<int32>(volume->FreeBytes() / (capacity / 100));

	// warn below 5% of free space
	if (percent < 5)
		return -2 - percent;
	return percent;
}


// SymLink handling:
// symlink pose uses the resolved model to retrieve the icon, if not broken
// everything else, like the attributes, etc. is retrieved directly from the
// symlink itself
BPose::BPose(Model* model, BPoseView* view, uint32 clipboardMode, bool selected)
	:
	fModel(model),
	fWidgetList(4),
	fClipboardMode(clipboardMode),
	fPercent(-1),
	fSelectionTime(0),
	fIsSelected(selected),
	fHasLocation(false),
	fNeedsSaveLocation(false),
	fListModeInited(false),
	fWasAutoPlaced(false),
	fBrokenSymLink(false),
	fBackgroundClean(false)
{
	CreateWidgets(view);

	if (model->IsVolume() && !model->IsRoot()) {
		fs_info info;
		dev_t device = model->NodeRef()->device;
		BVolume* volume = new BVolume(device);
		if (volume->InitCheck() == B_OK && fs_stat_dev(device, &info) == B_OK) {
			// Philosophy here:
			// Bars go on all read/write volumes
			// Exceptions: Not on CDDA or Disks
			if (strcmp(info.fsh_name, "cdda") != 0 && !volume->IsReadOnly()) {
				// The volume is ok and we want space bars on it
				gPeriodicUpdatePoses.AddPose(this, view,
					_PeriodicUpdateCallback, volume);
				if (TrackerSettings().ShowVolumeSpaceBar())
					fPercent = CalcFreeSpace(volume);
			} else {
				// no space bars on volume
				delete volume;
			}
		} else {
			// bad volume
			delete volume;
		}
	}
}


BPose::~BPose()
{
	if (fModel->IsVolume() && !fModel->IsRoot()) {
		// we might be registered for periodic updates
		BVolume* volume = NULL;
		if (gPeriodicUpdatePoses.RemovePose(this, (void**)&volume))
			delete volume;
	}
	int32 count = fWidgetList.CountItems();
	for (int32 i = 0; i < count; i++)
		delete fWidgetList.ItemAt(i);

	delete fModel;
}


void
BPose::CreateWidgets(BPoseView* poseView)
{
	for (int32 index = 0; ; index++) {
		BColumn* column = poseView->ColumnAt(index);
		if (column == NULL)
			break;

		fWidgetList.AddItem(new BTextWidget(fModel, column, poseView));
	}
}


BTextWidget*
BPose::AddWidget(BPoseView* poseView, BColumn* column)
{
	BModelOpener opener(fModel);
	if (fModel->InitCheck() != B_OK)
		return NULL;

	BTextWidget* widget = new BTextWidget(fModel, column, poseView);
	fWidgetList.AddItem(widget);

	return widget;
}


BTextWidget*
BPose::AddWidget(BPoseView* poseView, BColumn* column, ModelNodeLazyOpener& opener)
{
	opener.OpenNode();
	if (fModel->InitCheck() != B_OK)
		return NULL;

	BTextWidget* widget = new BTextWidget(fModel, column, poseView);
	fWidgetList.AddItem(widget);

	return widget;
}


void
BPose::RemoveWidget(BPoseView*, BColumn* column)
{
	int32 index;
	BTextWidget* widget = WidgetFor(column->AttrHash(), &index);
	if (widget != NULL)
		delete fWidgetList.RemoveItemAt(index);
}


void
BPose::Commit(bool saveChanges, BPoint loc, BPoseView* poseView, int32 poseIndex)
{
	int32 count = fWidgetList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BTextWidget* widget = fWidgetList.ItemAt(index);
		if (widget != NULL && widget->IsActive()) {
			widget->StopEdit(saveChanges, loc, poseView, this, poseIndex);
			break;
		}
	}
}


inline bool
OneMouseUp(BTextWidget* widget, BPose* pose, BPoseView* poseView,
	BColumn* column, BPoint poseLoc, BPoint where)
{
	BRect rect;
	if (poseView->ViewMode() == kListMode)
		rect = widget->CalcClickRect(poseLoc, column, poseView);
	else
		rect = widget->CalcClickRect(pose->Location(poseView), NULL, poseView);

	if (rect.Contains(where)) {
		widget->MouseUp(rect, poseView, pose, where);
		return true;
	}

	return false;
}


void
BPose::MouseUp(BPoint poseLoc, BPoseView* poseView, BPoint where, int32)
{
	WhileEachTextWidget(this, poseView, OneMouseUp, poseLoc, where);
}


inline void
OneCheckAndUpdate(BTextWidget* widget, BPose*, BPoseView* poseView,
	BColumn* column, BPoint poseLoc)
{
	widget->CheckAndUpdate(poseLoc, column, poseView, true);
}


void
BPose::UpdateAllWidgets(int32, BPoint poseLoc, BPoseView* poseView)
{
	if (poseView->ViewMode() != kListMode)
		poseLoc = Location(poseView);

	ASSERT(fModel->IsNodeOpen());
	EachTextWidget(this, poseView, OneCheckAndUpdate, poseLoc);
}


void
BPose::UpdateWidgetAndModel(Model* resolvedModel, const char* attrName,
	uint32 attrType, int32, BPoint poseLoc, BPoseView* poseView, bool visible)
{
	if (poseView->ViewMode() != kListMode)
		poseLoc = Location(poseView);

	ASSERT(resolvedModel == NULL || resolvedModel->IsNodeOpen());

	if (attrName != NULL) {
		// pick up new attributes and find out if icon needs updating
		if (resolvedModel->AttrChanged(attrName) && visible)
			UpdateIcon(poseLoc, poseView);

		// ToDo: the following code is wrong, because this sort of hashing
		// may overlap and we get aliasing
		uint32 attrHash = AttrHashString(attrName, attrType);
		BTextWidget* widget = WidgetFor(attrHash);
		if (widget != NULL) {
			BColumn* column = poseView->ColumnFor(attrHash);
			if (column != NULL)
				widget->CheckAndUpdate(poseLoc, column, poseView, visible);
		} else if (attrType == 0) {
			// attribute got likely removed, so let's search the
			// column for the matching attribute name
			int32 count = fWidgetList.CountItems();
			for (int32 i = 0; i < count; i++) {
				BTextWidget* widget = fWidgetList.ItemAt(i);
				BColumn* column = poseView->ColumnFor(widget->AttrHash());
				if (column != NULL
					&& strcmp(column->AttrName(), attrName) == 0) {
					widget->CheckAndUpdate(poseLoc, column, poseView, visible);
					break;
				}
			}
		}
	} else {
		// no attr name means check all widgets for stat info changes

		// pick up stat changes
		if (resolvedModel && resolvedModel->StatChanged()) {
			if (resolvedModel->InitCheck() != B_OK)
				return;

			if (visible)
				UpdateIcon(poseLoc, poseView);
		}

		// distribute stat changes
		for (int32 index = 0; ; index++) {
			BColumn* column = poseView->ColumnAt(index);
			if (column == NULL)
				break;

			if (column->StatField()) {
				BTextWidget* widget = WidgetFor(column->AttrHash());
				if (widget != NULL)
					widget->CheckAndUpdate(poseLoc, column, poseView, visible);
			}
		}
	}
}


bool
BPose::_PeriodicUpdateCallback(BPose* pose, void* cookie)
{
	return pose->UpdateVolumeSpaceBar((BVolume*)cookie);
}


bool
BPose::UpdateVolumeSpaceBar(BVolume* volume)
{
	bool enabled = TrackerSettings().ShowVolumeSpaceBar();
	if (!enabled) {
		if (fPercent == -1)
			return false;

		fPercent = -1;
		return true;
	}

	int32 percent = CalcFreeSpace(volume);
	if (fPercent != percent) {
		if (percent > 100)
			fPercent = 100;
		else
			fPercent = percent;

		return true;
	}

	return false;
}


void
BPose::UpdateIcon(BPoint poseLoc, BPoseView* poseView)
{
	IconCache::sIconCache->IconChanged(ResolvedModel());

	BRect rect;
	if (poseView->ViewMode() == kListMode)
		rect = _ListIconRect(poseView, poseLoc);
	else
		rect = _IconRect(poseView, poseLoc);

	poseView->Invalidate(rect);
}


void
BPose::UpdateBrokenSymLink(BPoint poseLoc, BPoseView* poseView)
{
	ASSERT(TargetModel()->IsSymLink());
	ASSERT(TargetModel()->LinkTo() == NULL);
	if (!TargetModel()->IsSymLink() || TargetModel()->LinkTo() != NULL)
		return;

	UpdateIcon(poseLoc, poseView);
}


void
BPose::UpdateWasBrokenSymlink(BPoint poseLoc, BPoseView* poseView)
{
	if (!fModel->IsSymLink())
		return;

	if (fModel->LinkTo() != NULL) {
		BEntry entry(fModel->EntryRef(), true);
		if (entry.InitCheck() != B_OK) {
			watch_node(fModel->LinkTo()->NodeRef(), B_STOP_WATCHING, poseView);
			fModel->SetLinkTo(NULL);
			UpdateIcon(poseLoc, poseView);
		}
		return;
	}

	poseView->CreateSymlinkPoseTarget(fModel);
	UpdateIcon(poseLoc, poseView);
	if (fModel->LinkTo() != NULL)
		fModel->LinkTo()->CloseNode();
}


void
BPose::EditFirstWidget(BPoint poseLoc, BPoseView* poseView)
{
	// find first editable widget
	BColumn* column;
	for (int32 i = 0; (column = poseView->ColumnAt(i)) != NULL; i++) {
		BTextWidget* widget = WidgetFor(column->AttrHash());

		if (widget != NULL && widget->IsEditable()) {
			BRect bounds;
			// ToDo:
			// fold the three StartEdit code sequences into a cover call
			if (poseView->ViewMode() == kListMode)
				bounds = widget->CalcRect(poseLoc, column, poseView);
			else
				bounds = widget->CalcRect(Location(poseView), NULL, poseView);

			widget->StartEdit(bounds, poseView, this);
			break;
		}
	}
}


void
BPose::EditPreviousNextWidgetCommon(BPoseView* poseView, bool next)
{
	bool found = false;
	int32 delta = next ? 1 : -1;
	for (int32 index = next ? 0 : poseView->CountColumns() - 1; ;
			index += delta) {
		BColumn* column = poseView->ColumnAt(index);
		if (column == NULL) {
			// out of columns
			break;
		}

		BTextWidget* widget = WidgetFor(column->AttrHash());
		if (widget == NULL) {
			// no widget for this column, next
			continue;
		}

		if (widget->IsActive()) {
			poseView->CommitActivePose();
			found = true;
			continue;
		}

		if (found && column->Editable()) {
			BRect bounds;
			if (poseView->ViewMode() == kListMode) {
				int32 poseIndex = poseView->IndexOfPose(this);
				BPoint poseLoc(0, poseIndex* poseView->ListElemHeight());
				bounds = widget->CalcRect(poseLoc, column, poseView);
			} else
				bounds = widget->CalcRect(Location(poseView), NULL, poseView);

			widget->StartEdit(bounds, poseView, this);
			break;
		}
	}
}


void
BPose::EditNextWidget(BPoseView* poseView)
{
	EditPreviousNextWidgetCommon(poseView, true);
}


void
BPose::EditPreviousWidget(BPoseView* poseView)
{
	EditPreviousNextWidgetCommon(poseView, false);
}


bool
BPose::PointInPose(const BPoseView* poseView, BPoint where) const
{
	ASSERT(poseView != NULL);
	ASSERT(poseView->ViewMode() != kListMode);

	BPoint location = Location(poseView);
	BRect rect = _IconRect(poseView, location);

	if (rect.Contains(where)) {
		return IconCache::sIconCache->IconHitTest(where - location, ResolvedModel(), kNormalIcon,
			poseView->IconSize());
	}

	return CalcRect(poseView).Contains(where);
}


bool
BPose::PointInPose(BPoint poseLoc, const BPoseView* poseView, BPoint where,
	BTextWidget** hitWidget) const
{
	ASSERT(poseView != NULL);
	ASSERT(poseView->ViewMode() == kListMode);

	if (hitWidget != NULL)
		*hitWidget = NULL;

	// check intersection with icon
	BRect rect = _ListIconRect(poseView, poseLoc);
	if (rect.Contains(where))
		return true;

	for (int32 index = 0; ; index++) {
		BColumn* column = poseView->ColumnAt(index);
		if (column == NULL)
			break;

		BTextWidget* widget = WidgetFor(column->AttrHash());
		if (widget != NULL && widget->CalcClickRect(poseLoc, column, poseView).Contains(where)) {
			if (hitWidget != NULL)
				*hitWidget = widget;

			return true;
		}
	}

	return false;
}


void
BPose::Draw(BRect rect, const BRect& updateRect, BPoseView* poseView, BView* drawView,
	bool fullDraw, BPoint offset, bool selected)
{
	ASSERT(poseView);
	ASSERT(poseView->Window());

	// If the background wasn't cleared and Draw() is not called after
	// having edited a name or similar (with fullDraw)
	if (!fBackgroundClean && !fullDraw) {
		fBackgroundClean = true;
		poseView->Invalidate(rect);
		return;
	} else
		fBackgroundClean = false;

	bool direct = drawView == poseView;
	bool dragging = false;
	if (poseView->Window()->CurrentMessage() != NULL)
		dragging = poseView->Window()->CurrentMessage()->what == kMsgMouseDragged;
	bool windowActive = poseView->Window()->IsActive();
	bool showSelectionWhenInactive = poseView->ShowSelectionWhenInactive();
	bool drawIconUnselected = !windowActive && !showSelectionWhenInactive;

	if (direct)
		poseView->PushState();

	// This is so that the cut icon will be drawn semi-transparent.
	if (fClipboardMode == kMoveSelectionTo) {
		drawView->SetDrawingMode(B_OP_ALPHA);
		drawView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
		uint8 alpha = 64; // set the level of opacity by value
		if (poseView->HighColor().IsDark())
			drawView->SetHighColor(0, 0, 0, alpha);
		else
			drawView->SetHighColor(255, 255, 255, alpha);
	}

	// We are only concerned with setting the low color. BTextWidget::Draw()
	// will set the high color and drawing mode or they are set above or by
	// BPoseView::MakeDragBitmap() for dragged poses.

	// Set low color to high color on selected Desktop poses, don't worry,
	// high color will be inverted again.
	if (selected && poseView->IsDesktopView())
		drawView->SetLowColor(poseView->HighColor());
	else if (poseView->IsDesktopView())
		drawView->SetLowColor(poseView->LowColor());
	else
		drawView->SetLowUIColor(poseView->LowUIColor());

	ModelNodeLazyOpener modelOpener(fModel);

	if (poseView->ViewMode() == kListMode) {
		// draw in list mode
		BRect iconRect = _ListIconRect(poseView, rect.LeftTop());
		if (updateRect.Intersects(iconRect)) {
			iconRect.OffsetBy(offset);
			DrawIcon(iconRect.LeftTop(), drawView, poseView->IconSize(), drawIconUnselected);
		}

		int32 columnsToDraw = 1;
		if (fullDraw)
			columnsToDraw = poseView->CountColumns();

		for (int32 index = 0; index < columnsToDraw; index++) {
			BColumn* column = poseView->ColumnAt(index);
			if (column == NULL)
				break;

			BTextWidget* widget = WidgetFor(column, poseView, modelOpener);
			if (widget != NULL && widget->IsVisible()) {
				BRect columnRect(widget->ColumnRect(rect.LeftTop(), column, poseView));
				if (columnRect.Intersects(updateRect)) {
					BRect widgetRect(widget->CalcRect(rect.LeftTop(), column, poseView));

					// draw dragged text and all columns after the first one unselected
					if (dragging || index > 0)
						selected = false;

					// draw text
					DrawTextWidget(columnRect, widgetRect, widget, poseView, drawView, selected,
						fClipboardMode, offset);
				}
			}
		}
	} else {
		// draw in icon mode
		BPoint location(Location(poseView));
		BPoint iconOrigin(location);
		iconOrigin += offset;

		DrawIcon(iconOrigin, drawView, poseView->IconSize(), drawIconUnselected);

		BColumn* column = poseView->FirstColumn();
		BTextWidget* widget = WidgetFor(column, poseView, modelOpener);
		if (widget != NULL && widget->IsVisible()) {
			BRect widgetRect(widget->CalcRect(location, NULL, poseView));
			if (widgetRect.Intersects(updateRect)) {
				// draw dragged text unselected
				if (dragging)
					selected = false;

				// draw text
				DrawTextWidget(widgetRect, widgetRect, widget, poseView, drawView, selected,
					fClipboardMode, offset);
			}
		}
	}

	if (direct)
		poseView->PopState();
}


void
BPose::DrawTextWidget(BRect rect, BRect textRect, BTextWidget* widget,
	BPoseView* poseView, BView* drawView, bool selected, uint32 clipboardMode, BPoint offset)
{
	bool windowActive = poseView->Window()->IsActive();
	bool showSelectionWhenInactive = poseView->ShowSelectionWhenInactive();
	bool isDrawingSelectionRect = poseView->IsDrawingSelectionRect();

	widget->Draw(rect, textRect, poseView, drawView, selected, fClipboardMode, offset);

	if (selected) {
		if (windowActive || isDrawingSelectionRect) {
			// invert colors to select label using "reverse video"
			drawView->InvertRect(textRect.OffsetByCopy(offset));
		} else if (!windowActive && showSelectionWhenInactive) {
			// the selection rect is alpha-blended on top for inactive windows
			drawView->InvertRect(textRect.OffsetByCopy(offset));
			drawView->SetDrawingMode(B_OP_BLEND);
			drawView->SetHighColor(128, 128, 128, 255);
			drawView->FillRect(textRect.OffsetByCopy(offset));
			drawView->SetDrawingMode(B_OP_OVER);
		}
	}
}


void
BPose::DeselectWithoutErasingBackground(BRect, BPoseView* poseView)
{
	ASSERT(poseView->ViewMode() != kListMode);
	ASSERT(!IsSelected());

	BPoint location(Location(poseView));

	if (fPercent == -1) {
		// draw icon directly
		poseView->SetDrawingMode(B_OP_OVER);
		DrawIcon(location, poseView, poseView->IconSize());
	} else {
		// invalidate icon
		UpdateIcon(location, poseView);
	}

	BColumn* column = poseView->FirstColumn();
	if (column == NULL)
		return;

	BTextWidget* widget = WidgetFor(column->AttrHash());
	if (widget == NULL || !widget->IsVisible())
		return;

	// just invalidate the background, don't draw anything
	poseView->Invalidate(widget->CalcRect(location, NULL, poseView));
}


void
BPose::MoveTo(BPoint point, BPoseView* poseView, bool invalidate)
{
	point.x = floorf(point.x);
	point.y = floorf(point.y);

	BRect oldBounds;

	BPoint oldLocation = Location(poseView);

	ASSERT(poseView->ViewMode() != kListMode);
	if (point == oldLocation || poseView->ViewMode() == kListMode)
		return;

	if (invalidate)
		oldBounds = CalcRect(poseView);

	// might need to move a text view if we're active
	if (poseView->ActivePose() == this) {
		BView* border_view = poseView->FindView("BorderView");
		if (border_view != NULL)
			border_view->MoveBy(point.x - oldLocation.x, point.y - oldLocation.y);
	}

	float scale = 1.0;
	if (poseView->ViewMode() == kIconMode)
		scale = (float)poseView->IconSizeInt() / 32.0;

	fLocation.x = point.x / scale;
	fLocation.y = point.y / scale;

	fHasLocation = true;
	fNeedsSaveLocation = true;

	if (invalidate) {
		poseView->Invalidate(oldBounds);
		poseView->Invalidate(CalcRect(poseView));
	}
}


BTextWidget*
BPose::ActiveWidget() const
{
	for (int32 i = fWidgetList.CountItems(); i-- > 0;) {
		BTextWidget* widget = fWidgetList.ItemAt(i);
		if (widget->IsActive())
			return widget;
	}

	return NULL;
}


BTextWidget*
BPose::WidgetFor(uint32 attr, int32* index) const
{
	int32 count = fWidgetList.CountItems();
	for (int32 i = 0; i < count; i++) {
		BTextWidget* widget = fWidgetList.ItemAt(i);
		if (widget->AttrHash() == attr) {
			if (index != NULL)
				*index = i;

			return widget;
		}
	}

	return NULL;
}


BTextWidget*
BPose::WidgetFor(BColumn* column, BPoseView* poseView, ModelNodeLazyOpener& opener, int32* index)
{
	if (column == NULL)
		return NULL;

	BTextWidget* widget = WidgetFor(column->AttrHash(), index);
	if (widget == NULL)
		widget = AddWidget(poseView, column, opener);

	return widget;
}


void
BPose::DrawIcon(BPoint where, BView* view, BSize size, bool drawUnselected)
{
	IconCache::sIconCache->Draw(ResolvedModel(), view, where,
		fIsSelected && !drawUnselected ? kSelectedIcon : kNormalIcon, size, true);

	if (fPercent != -1)
		DrawBar(where, view, size);
}


void
BPose::DrawBar(BPoint where, BView* view, BSize size)
{
	view->PushState();

	int32 iconSize = size.IntegerWidth();
	int32 yOffset;
	int32 barWidth = (int32)(7.0f / 32.0f * (float)(iconSize + 1));
	if (barWidth < 4) {
		barWidth = 4;
		yOffset = 0;
	} else
		yOffset = 2;

	int32 barHeight = iconSize - 4 - 2 * yOffset;

	// the black shadowed line
	view->SetHighColor(32, 32, 32, 92);
	view->MovePenTo(BPoint(where.x + iconSize, where.y + 1 + yOffset));
	view->StrokeLine(BPoint(where.x + iconSize, where.y + iconSize - yOffset));
	view->StrokeLine(BPoint(where.x + iconSize - barWidth + 1, where.y + iconSize - yOffset));

	view->SetDrawingMode(B_OP_ALPHA);

	// the gray frame
	view->SetHighColor(76, 76, 76, 192);
	float left = where.x + iconSize - barWidth;
	float top = where.y + yOffset;
	float right = where.x + iconSize - 1;
	float bottom = where.y + iconSize - 1 - yOffset;
	BRect rect(left, top, right, bottom);
	view->StrokeRect(rect);

	// calculate bar height
	int32 percent = fPercent > -1 ? fPercent : -2 - fPercent;
	int32 barPos = int32(barHeight * percent / 100.0);
	if (barPos < 0)
		barPos = 0;
	else if (barPos > barHeight)
		barPos = barHeight;

	// the free space bar
	view->SetHighColor(TrackerSettings().FreeSpaceColor());

	rect.InsetBy(1,1);
	BRect bar(rect);
	bar.bottom = bar.top + barPos - 1;
	if (barPos > 0)
		view->FillRect(bar);

	// the used space bar
	bar.top = bar.bottom + 1;
	bar.bottom = rect.bottom;
	view->SetHighColor(fPercent < -1
		? TrackerSettings().WarningSpaceColor()
		: TrackerSettings().UsedSpaceColor());
	view->FillRect(bar);

	view->PopState();
}


void
BPose::DrawToggleSwitch(BRect, BPoseView*)
{
}


BPoint
BPose::Location(const BPoseView* poseView) const
{
	float scale = 1.0;
	if (poseView->ViewMode() == kIconMode)
		scale = (float)poseView->IconSizeInt() / 32.0;

	return BPoint(fLocation.x * scale, fLocation.y * scale);
}


void
BPose::SetLocation(BPoint point, const BPoseView* poseView)
{
	float scale = 1.0;
	if (poseView->ViewMode() == kIconMode)
		scale = (float)poseView->IconSizeInt() / 32.0;

	fLocation = BPoint(floorf(point.x / scale), floorf(point.y / scale));
	if (isinff(fLocation.x) || isinff(fLocation.y))
		debugger("BPose::SetLocation() - infinite location");

	fHasLocation = true;
}


BRect
BPose::CalcRect(BPoint loc, const BPoseView* poseView, bool minimalRect) const
{
	ASSERT(poseView->ViewMode() == kListMode);

	BColumn* column = poseView->LastColumn();
	BRect rect;
	rect.left = loc.x;
	rect.top = loc.y;
	rect.right = loc.x + column->Offset() + column->Width();
	rect.bottom = rect.top + poseView->ListElemHeight();

	if (minimalRect) {
		BTextWidget* widget = WidgetFor(poseView->FirstColumn()->AttrHash());
		if (widget != NULL)
			rect.right = widget->CalcRect(loc, poseView->FirstColumn(), poseView).right;
	}

	return rect;
}


BRect
BPose::CalcRect(const BPoseView* poseView) const
{
	ASSERT(poseView != NULL);
	ASSERT(poseView->ViewMode() != kListMode);

	BTextWidget* widget = WidgetFor(poseView->FirstColumn()->AttrHash());
	BPoint location = Location(poseView);
	BRect rect(_IconRect(poseView, location));
	float textWidth = (widget != NULL ? widget->TextWidth(poseView) : 0);

	if (poseView->ViewMode() == kIconMode) {
		// icon mode
		if (widget != NULL && textWidth > rect.Width()) {
			rect.left += roundf((rect.Width() - textWidth) / 2.f);
			rect.right = rect.left + ceilf(textWidth);
		}
	} else {
		// mini icon mode
		if (widget != NULL)
			rect.right += kMiniIconSeparator + ceilf(textWidth);
	}

	rect.bottom = rect.top + ceilf(poseView->IconPoseHeight());

	return rect;
}


BRect
BPose::_ListIconRect(const BPoseView* poseView, BPoint location) const
{
	ASSERT(poseView != NULL);

	BRect rect;
	uint32 iconSize = poseView->IconSizeInt();

	rect.left = location.x + poseView->ListOffset();
	rect.right = rect.left + iconSize;
	rect.top = location.y + roundf((poseView->ListElemHeight() - iconSize) / 2);
	rect.bottom = rect.top + iconSize;

	return rect;
}


BRect
BPose::_IconRect(const BPoseView* poseView, BPoint location) const
{
	ASSERT(poseView != NULL);

	BRect rect;
	uint32 iconSize = poseView->IconSizeInt();

	rect.left = location.x;
	rect.right = rect.left + iconSize;
	rect.top = location.y;
	rect.bottom = rect.top + iconSize;

	return rect;
}


#if DEBUG
void
BPose::PrintToStream()
{
	TargetModel()->PrintToStream();
	switch (fClipboardMode) {
		case kMoveSelectionTo:
			PRINT(("clipboardMode: Cut\n"));
			break;

		case kCopySelectionTo:
			PRINT(("clipboardMode: Copy\n"));
			break;

		default:
			PRINT(("clipboardMode: 0 - not in clipboard\n"));
			break;
	}
	PRINT(("%sselected\n", IsSelected() ? "" : "not "));
	PRINT(("location %s x:%f y:%f\n", HasLocation() ? "" : "unknown ",
		HasLocation() ? fLocation.x : 0,
		HasLocation() ? fLocation.y : 0));
	PRINT(("%s autoplaced \n", WasAutoPlaced() ? "was" : "not"));
}
#endif
