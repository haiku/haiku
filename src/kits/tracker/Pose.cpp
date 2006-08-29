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

#include <stdlib.h>
#include <string.h>

#include <Debug.h>
#include <Volume.h>
#include <fs_info.h>

#include "Attributes.h"
#include "Commands.h"
#include "FSClipboard.h"
#include "IconCache.h"
#include "Pose.h"
#include "PoseView.h"
#include "Utilities.h"


int32
CalcFreeSpace(dev_t device)
{
	BVolume volume(device);
	fs_info info;
	if (volume.InitCheck() == B_OK && fs_stat_dev(device,&info) == B_OK) {
		// Philosophy here:
		// Bars go on all drives with read/write capabilities
		// Exceptions: Not on CDDA, but on NTFS/Ext2
		// Also note that some volumes may return 0 when
		// BVolume::Capacity() is called (believe-me... That *DOES*
		// happen) so we also check for that.
		off_t capacity = volume.Capacity();
		if (((!volume.IsReadOnly() && strcmp(info.fsh_name,"cdda"))
			|| !strcmp(info.fsh_name,"ntfs")
			|| !strcmp(info.fsh_name,"ext2"))
			&& (capacity > 0)) {
			int32 percent = static_cast<int32>(volume.FreeBytes() / (capacity / 100));

			// warn below 20 MB of free space (if this is less than 10% of free space)
			if (volume.FreeBytes() < 20 * 1024 * 1024 && percent < 10)
				return -2 - percent;

			return percent;
		}
	}
	return -1;
}


// SymLink handling:
// symlink pose uses the resolved model to retrieve the icon, if not broken
// everything else, like the attributes, etc. is retrieved directly from the
// symlink itself

BPose::BPose(Model *model, BPoseView *view, bool selected)
	:	fModel(model),
		fWidgetList(4, true),
		fPercent(-1),
		fIsSelected(selected),
		fDelayedEdit(true),
		fHasLocation(false),
		fNeedsSaveLocation(false),
		fListModeInited(false),
		fWasAutoPlaced(false),
		fBrokenSymLink(false),
		fBackgroundClean(false)
{
	CreateWidgets(view);

	if (model->IsVolume() && TrackerSettings().ShowVolumeSpaceBar()) {
		dev_t device = model->NodeRef()->device;
		fPercent = CalcFreeSpace(device);
	}

	if ((fClipboardMode = FSClipboardFindNodeMode(model,true)) != 0
		&& !view->HasPosesInClipboard()) {
		view->SetHasPosesInClipboard(true);
	}
}


BPose::~BPose()
{
	delete fModel;
}


void
BPose::CreateWidgets(BPoseView *poseView)
{
	for (int32 index = 0; ; index++) {
		BColumn *column = poseView->ColumnAt(index);
		if (!column)
			break;
		fWidgetList.AddItem(new BTextWidget(fModel, column, poseView));
	}
}


BTextWidget *
BPose::AddWidget(BPoseView *poseView, BColumn *column)
{
	BModelOpener opener(fModel);
	if (fModel->InitCheck() != B_OK)
		return NULL;
		
	BTextWidget *widget = new BTextWidget(fModel, column, poseView);
	fWidgetList.AddItem(widget);
	return widget;
}


BTextWidget *
BPose::AddWidget(BPoseView *poseView, BColumn *column, ModelNodeLazyOpener &opener)
{
	opener.OpenNode();
	if (fModel->InitCheck() != B_OK)
		return NULL;
	
	BTextWidget *widget = new BTextWidget(fModel, column, poseView);
	fWidgetList.AddItem(widget);
	return widget;
}


void
BPose::RemoveWidget(BPoseView *, BColumn *column)
{
	int32 index;
	BTextWidget *widget = WidgetFor(column->AttrHash(), &index);
	if (widget) 
		delete fWidgetList.RemoveItemAt(index);
}


void
BPose::Commit(bool saveChanges, BPoint loc, BPoseView *poseView, int32 poseIndex)
{
	int32 count = fWidgetList.CountItems();
	for (int32 index = 0; index < count; index++) {
		BTextWidget *widget = fWidgetList.ItemAt(index);
		if (widget->IsActive()) {
			widget->StopEdit(saveChanges, loc, poseView, this, poseIndex);
			break;
		}
	}
}


inline bool
OneMouseUp(BTextWidget *widget, BPose *pose, BPoseView *poseView, BColumn *column,
	BPoint poseLoc, BPoint where)
{
	BRect rect;
	if (poseView->ViewMode() == kListMode)
		rect = widget->CalcClickRect(poseLoc, column, poseView);
	else
		rect = widget->CalcClickRect(pose->Location(), 0, poseView);

	if (rect.Contains(where)) {
		widget->MouseUp(rect, poseView, pose, where, pose->DelayedEdit());
		return true;
	}
	return false;
}


void
BPose::MouseUp(BPoint poseLoc, BPoseView *poseView, BPoint where, int32)
{
	WhileEachTextWidget(this, poseView, OneMouseUp, poseLoc, where);
}


inline void
OneCheckAndUpdate(BTextWidget *widget, BPose *, BPoseView *poseView,
	BColumn *column, BPoint poseLoc)
{
	widget->CheckAndUpdate(poseLoc, column, poseView);
}


void
BPose::UpdateAllWidgets(int32, BPoint poseLoc, BPoseView *poseView)
{
	if (poseView->ViewMode() != kListMode)
		poseLoc = fLocation;

	ASSERT(fModel->IsNodeOpen());
	EachTextWidget(this, poseView, OneCheckAndUpdate, poseLoc);
}


void
BPose::UpdateWidgetAndModel(Model *resolvedModel, const char *attrName,
	uint32 attrType, int32, BPoint poseLoc, BPoseView *poseView)
{
	if (poseView->ViewMode() != kListMode)
		poseLoc = fLocation;

	ASSERT(!resolvedModel || resolvedModel->IsNodeOpen());

	if (attrName) {
		// pick up new attributes and find out if icon needs updating
		if (resolvedModel->AttrChanged(attrName))
			UpdateIcon(poseLoc, poseView);

		// ToDo: the following code is wrong, because this sort of hashing
		// may overlap and we get aliasing
		uint32 attrHash = AttrHashString(attrName, attrType);
		BTextWidget *widget = WidgetFor(attrHash);
		if (widget) {
			BColumn *column = poseView->ColumnFor(attrHash);
			if (column) 
				widget->CheckAndUpdate(poseLoc, column, poseView);
		} else if (attrType == 0) {
			// attribute got likely removed, so let's search the
			// column for the matching attribute name
			int32 count = fWidgetList.CountItems();
			for (int32 i = 0; i < count; i++) {
				BTextWidget *widget = fWidgetList.ItemAt(i);
				BColumn *column = poseView->ColumnFor(widget->AttrHash());
				if (column != NULL && !strcmp(column->AttrName(), attrName)) {
					widget->CheckAndUpdate(poseLoc, column, poseView);
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

			UpdateIcon(poseLoc, poseView);
		}

		// distribute stat changes
		for (int32 index = 0; ; index++) {
			BColumn *column = poseView->ColumnAt(index);
			if (!column)
				break;

			if (column->StatField()) {
				BTextWidget *widget = WidgetFor(column->AttrHash());
				if (widget) 
					widget->CheckAndUpdate(poseLoc, column, poseView);
			}
		}
	}
}


bool
BPose::UpdateVolumeSpaceBar(bool enabled)
{
	if (!enabled) {
		if (fPercent == -1)
			return false;

		fPercent = -1;
		return true;
	}

	dev_t device = TargetModel()->NodeRef()->device;
	int32 percent = CalcFreeSpace(device);

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
BPose::UpdateIcon(BPoint poseLoc, BPoseView *poseView)
{
	IconCache::sIconCache->IconChanged(ResolvedModel());

	BRect rect;
	if (poseView->ViewMode() == kListMode) {
		rect = CalcRect(poseLoc, poseView);
		rect.left += kListOffset;
		// TODO: make this depend on IconSizeInt() as well?
		rect.right = rect.left + B_MINI_ICON;
		rect.top = rect.bottom - B_MINI_ICON;
	} else {
		rect.left = fLocation.x;
		rect.top = fLocation.y;
		rect.right = rect.left + poseView->IconSizeInt();
		rect.bottom = rect.top + poseView->IconSizeInt();
	}

	poseView->Invalidate(rect);
}


void 
BPose::UpdateBrokenSymLink(BPoint poseLoc, BPoseView *poseView)
{
	ASSERT(TargetModel()->IsSymLink());
	ASSERT(!TargetModel()->LinkTo());
	UpdateIcon(poseLoc, poseView);
}


void 
BPose::UpdateWasBrokenSymlink(BPoint poseLoc, BPoseView *poseView)
{
	if (!fModel->IsSymLink())
		return;

	if (fModel->LinkTo())
		return;

	poseView->CreateSymlinkPoseTarget(fModel);
	if (!fModel->LinkTo())
		return;
	
	UpdateIcon(poseLoc, poseView);
	fModel->LinkTo()->CloseNode();
}


void
BPose::EditFirstWidget(BPoint poseLoc, BPoseView *poseView)
{
	// find first editable widget
	BColumn *column;
	for (int32 i = 0;(column = poseView->ColumnAt(i)) != NULL;i++) {
		BTextWidget *widget = WidgetFor(column->AttrHash());

		if (widget && widget->IsEditable()) {
			BRect bounds;
			// ToDo:
			// fold the three StartEdit code sequences into a cover call
			if (poseView->ViewMode() == kListMode)
				bounds = widget->CalcRect(poseLoc, column, poseView);
			else
				bounds = widget->CalcRect(fLocation, NULL, poseView);
			widget->StartEdit(bounds, poseView, this);
			break;
		}
	}
}


void
BPose::EditPreviousNextWidgetCommon(BPoseView *poseView, bool next)
{
	bool found = false;
	int32 delta = next ? 1 : -1;
	for (int32 index = next ? 0 : poseView->CountColumns() - 1; ; index += delta) {
		BColumn *column = poseView->ColumnAt(index);
		if (!column)
			break;

		BTextWidget *widget = WidgetFor(column->AttrHash());
		if (widget && widget->IsActive()) {
			poseView->CommitActivePose();
			found = true;
			continue;
		}

		if (found && column->Editable()) {
			BRect bounds;
			if (poseView->ViewMode() == kListMode) {
				int32 poseIndex = poseView->IndexOfPose(this);
				BPoint poseLoc(0, poseIndex * poseView->ListElemHeight());
				bounds = widget->CalcRect(poseLoc, column, poseView);
			} else
				bounds = widget->CalcRect(fLocation, 0, poseView);

			widget->StartEdit(bounds, poseView, this);
			break;
		}
	}
}


void
BPose::EditNextWidget(BPoseView *poseView)
{
	EditPreviousNextWidgetCommon(poseView, true);
}


void
BPose::EditPreviousWidget(BPoseView *poseView)
{
	EditPreviousNextWidgetCommon(poseView, false);
}


bool
BPose::PointInPose(const BPoseView *poseView, BPoint where) const 
{
	ASSERT(poseView->ViewMode() != kListMode);

	if (poseView->ViewMode() == kIconMode
		|| poseView->ViewMode() == kScaleIconMode) {
		// check icon rect, then actual icon pixel
		BRect rect(fLocation, fLocation);
		rect.right += poseView->IconSizeInt() - 1;
		rect.bottom += poseView->IconSizeInt() - 1;

		if (rect.Contains(where))
			return IconCache::sIconCache->IconHitTest(where - fLocation,
													  ResolvedModel(),
													  kNormalIcon,
													  poseView->IconSize());

		BTextWidget *widget = WidgetFor(poseView->FirstColumn()->AttrHash());
		if (widget) {
			float textWidth = ceilf(widget->TextWidth(poseView) + 1);
			rect.left += (poseView->IconSizeInt() - textWidth) / 2;
			rect.right = rect.left + textWidth; 
		}

		rect.top = fLocation.y + poseView->IconSizeInt();
		rect.bottom = rect.top + poseView->FontHeight();

		return rect.Contains(where);
	}
	
	// MINI_ICON_MODE rect calc
	BRect rect(fLocation, fLocation);
	rect.right += B_MINI_ICON + kMiniIconSeparator;
	rect.bottom += poseView->IconPoseHeight();
	BTextWidget *widget = WidgetFor(poseView->FirstColumn()->AttrHash());
	if (widget)
		rect.right += ceil(widget->TextWidth(poseView) + 1);
	
	return rect.Contains(where);
}


bool
BPose::PointInPose(BPoint loc, const BPoseView *poseView, BPoint where,
	BTextWidget **hitWidget) const
{
	if (hitWidget)
		*hitWidget = NULL;

	// check intersection with icon
	BRect rect;
	rect.left = loc.x + kListOffset;
	rect.right = rect.left + B_MINI_ICON;
	rect.bottom = loc.y + poseView->ListElemHeight();
	rect.top = rect.bottom - B_MINI_ICON;
	if (rect.Contains(where))
		return true;

	for (int32 index = 0; ; index++) {
		BColumn *column = poseView->ColumnAt(index);
		if (!column)
			break;
		BTextWidget *widget = WidgetFor(column->AttrHash());
		if (widget && widget->CalcClickRect(loc, column, poseView).Contains(where)) {
			if (hitWidget)
				*hitWidget = widget;
			return true;
		}
	}

	return false;
}


void
BPose::Draw(BRect rect, BPoseView *poseView, BView *drawView, bool fullDraw,
	const BRegion *updateRgn, BPoint offset, bool selected, bool recalculateText)
{
	// If the background wasn't cleared and Draw() is not called after
	// having edited a name or similar (with fullDraw)
	if (!fBackgroundClean && !fullDraw) {
		fBackgroundClean = true;
		poseView->Invalidate(rect);
		return;
	} else
		fBackgroundClean = false;
	
	bool directDraw = (drawView == poseView);
	bool windowActive = poseView->Window()->IsActive();
	bool showSelectionWhenInactive = poseView->fShowSelectionWhenInactive;
	bool isDrawingSelectionRect = poseView->fIsDrawingSelectionRect;
	
	ModelNodeLazyOpener modelOpener(fModel);
	
	if (poseView->ViewMode() == kListMode) {
		uint32 size = poseView->IconSizeInt();
		BRect iconRect(rect);
		iconRect.OffsetBy(offset);
		iconRect.left += kListOffset;
		iconRect.right = iconRect.left + size;
		iconRect.top = iconRect.bottom - size;
		if (!updateRgn || updateRgn->Intersects(iconRect)) {
			DrawIcon(iconRect.LeftTop(), drawView, poseView->IconSize(), directDraw,
				!windowActive && !showSelectionWhenInactive);
		}
		
		// draw text
		int32 columnsToDraw = 1;
		if (fullDraw)
			columnsToDraw = poseView->CountColumns();
		
		for (int32 index = 0; index < columnsToDraw; index++) {
			BColumn *column = poseView->ColumnAt(index);
			if (!column)
				break;
			
			// if widget doesn't exist, create it
			BTextWidget *widget = WidgetFor(column, poseView, modelOpener);
			
			if (widget && widget->IsVisible()) {
				BRect widgetRect(widget->ColumnRect(rect.LeftTop(), column,
					poseView));
				
				if (!updateRgn || updateRgn->Intersects(widgetRect)) {
					BRect widgetTextRect(widget->CalcRect(rect.LeftTop(), column,
						poseView));
					
					if (recalculateText)
						widget->RecalculateText(poseView);
					
					bool selectDuringDraw = directDraw && selected
						&& (windowActive && !poseView->EraseWidgetTextBackground());
					
					if (index == 0 && selectDuringDraw) {
						//draw with dark background to select text
						drawView->PushState();
						drawView->SetLowColor(0, 0, 0);
					}
					
					if (index == 0)
						widget->Draw(widgetRect, widgetTextRect, column->Width(),
							poseView, drawView, selected, fClipboardMode, offset, directDraw);
					else
						widget->Draw(widgetTextRect, widgetTextRect, column->Width(),
							poseView, drawView, false, fClipboardMode, offset, directDraw);
					
					if (index == 0 && selectDuringDraw)
						drawView->PopState();
					else if (index == 0 && selected) {
						if (windowActive || isDrawingSelectionRect) {
							widgetTextRect.OffsetBy(offset);
							drawView->InvertRect(widgetTextRect);
						} else if (!windowActive && showSelectionWhenInactive) {
							widgetTextRect.OffsetBy(offset);
							drawView->PushState();
							drawView->SetDrawingMode(B_OP_BLEND);
							drawView->SetHighColor(128, 128, 128, 255);
							drawView->FillRect(widgetTextRect);
							drawView->PopState();
						}
					}
				}
			}
		}
	} else {

		// draw in icon mode
		if (updateRgn && !updateRgn->Intersects(rect))
			return;

		BPoint iconOrigin(fLocation);
		iconOrigin += offset;

		DrawIcon(iconOrigin, drawView, poseView->IconSize(), directDraw,
			!windowActive && !showSelectionWhenInactive);
		
		BColumn *column = poseView->FirstColumn();
		if (!column)
			return;
		
		BTextWidget *widget = WidgetFor(column, poseView, modelOpener);
		if (!widget || !widget->IsVisible())
			return;

		rect = widget->CalcRect(fLocation, 0, poseView);

		bool selectDuringDraw = directDraw && selected
			&& (poseView->IsDesktopWindow()
			|| (windowActive && !poseView->EraseWidgetTextBackground()));
			
		if (selectDuringDraw) {
			// draw with dark background to select text
			drawView->PushState();
			drawView->SetLowColor(0, 0, 0);
		}

		widget->Draw(rect, rect, rect.Width(), poseView, drawView,
			selected, fClipboardMode, offset, directDraw);

		if (selectDuringDraw)
			drawView->PopState();
		else if (selected && directDraw) {
			if (windowActive || isDrawingSelectionRect) {
				rect.OffsetBy(offset);
				drawView->InvertRect(rect);
			} else if (!windowActive && showSelectionWhenInactive) {
				drawView->PushState();
				drawView->SetDrawingMode(B_OP_BLEND);
				drawView->SetHighColor(128, 128, 128, 255);
				drawView->FillRect(rect);
				drawView->PopState();
			}
		}
	}
}


void
BPose::DeselectWithoutErasingBackground(BRect, BPoseView *poseView)
{
	ASSERT(poseView->ViewMode() != kListMode);
	ASSERT(!poseView->EraseWidgetTextBackground());
	ASSERT(!IsSelected());

	// draw icon directly
	if (fPercent == -1)
		DrawIcon(fLocation, poseView, poseView->IconSize(), true);
	else
		UpdateIcon(fLocation, poseView);

	BColumn *column = poseView->FirstColumn();
	if (!column)
		return;
	
	BTextWidget *widget = WidgetFor(column->AttrHash());
	if (!widget || !widget->IsVisible())
		return;

	// just invalidate the background, don't draw anything
	poseView->Invalidate(widget->CalcRect(fLocation, 0, poseView));
}


void
BPose::MoveTo(BPoint point, BPoseView *poseView, bool inval)
{
	point.x = floorf(point.x);
	point.y = floorf(point.y);

	BRect oldBounds;

	ASSERT(poseView->ViewMode() != kListMode);
	if (point == fLocation || poseView->ViewMode() == kListMode)
		return;

	if (inval)
		oldBounds = CalcRect(poseView);

	// might need to move a text view if we're active
	if (poseView->ActivePose() == this) {
		BView *border_view = poseView->FindView("BorderView");
		if (border_view)
			border_view->MoveBy(point.x - fLocation.x, point.y - fLocation.y);
	}

	fLocation = point;
	fHasLocation = true;
	fNeedsSaveLocation = true;

	if (inval) {
		poseView->Invalidate(oldBounds);
		poseView->Invalidate(CalcRect(poseView));
	}
}


BTextWidget *
BPose::ActiveWidget() const
{
	for (int32 i = fWidgetList.CountItems(); i-- > 0;) {
		BTextWidget *widget = fWidgetList.ItemAt(i);
		if (widget->IsActive())
			return widget;
	}
	return NULL;
}


BTextWidget *
BPose::WidgetFor(uint32 attr, int32 *index) const 
{
	int32 count = fWidgetList.CountItems();
	for (int32 i = 0; i < count; i++) {
		BTextWidget *widget = fWidgetList.ItemAt(i);
		if (widget->AttrHash() == attr) {
			if (index)
				*index = i;
			return widget;
		}
	}

	return 0;
}


BTextWidget *
BPose::WidgetFor(BColumn *column, BPoseView *poseView, ModelNodeLazyOpener &opener,
	int32 *index)
{
	BTextWidget *widget = WidgetFor(column->AttrHash(), index);
	if (!widget)
		widget = AddWidget(poseView, column, opener);

	return widget;
}


/* deprecated */
bool
BPose::TestLargeIconPixel(BPoint point) const
{
	return IconCache::sIconCache->IconHitTest(point, ResolvedModel(),
		kNormalIcon, B_LARGE_ICON);
}
/* deprecated */


void
BPose::DrawIcon(BPoint where, BView *view, icon_size kind, bool direct, bool drawUnselected)
{
	if (fClipboardMode == kMoveSelectionTo) {
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetHighColor(0, 0, 0, 64);	// set the level of transparency
		view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
	} else if (direct)
		view->SetDrawingMode(B_OP_OVER);

	IconCache::sIconCache->Draw(ResolvedModel(), view, where,
		fIsSelected && !drawUnselected ? kSelectedIcon : kNormalIcon, kind, true);

	if (fPercent != -1)
		DrawBar(where, view, kind);
}


void 
BPose::DrawBar(BPoint where,BView *view,icon_size kind)
{
	view->PushState();

	int32 size, barWidth, barHeight, yOffset;
	if (kind >= B_LARGE_ICON) {
		size = kind - 1;
		barWidth = (int32)((float)7 / (float)32 * (float)kind);
		yOffset = 2;
		barHeight = size - 4 - 2 * yOffset;
	} else {
		size = B_MINI_ICON;
		barWidth = 4;
		yOffset = 0;
		barHeight = size - 4 - 2 * yOffset;
	}

	// the black shadowed line 
	view->SetHighColor(32, 32, 32, 92);
	view->MovePenTo(BPoint(where.x + size, where.y + 1 + yOffset));
	view->StrokeLine(BPoint(where.x + size, where.y + size - yOffset));
	view->StrokeLine(BPoint(where.x + size - barWidth + 1, where.y + size - yOffset));

	view->SetDrawingMode(B_OP_ALPHA);

	// the gray frame
	view->SetHighColor(76, 76, 76, 192);
	BRect rect(	where.x + size - barWidth,where.y + yOffset,
				where.x + size - 1,where.y + size - 1 - yOffset);
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
	view->SetHighColor(fPercent < -1 ? TrackerSettings().WarningSpaceColor() : TrackerSettings().UsedSpaceColor());
	view->FillRect(bar);

	view->PopState();
}


void
BPose::DrawToggleSwitch(BRect, BPoseView *)
{
	return;
}


BRect
BPose::CalcRect(BPoint loc, const BPoseView *poseView, bool minimalRect)
{
	ASSERT(poseView->ViewMode() == kListMode);

	BColumn *column = poseView->LastColumn();
	BRect rect;
	rect.left = loc.x;
	rect.top = loc.y;
	rect.right = loc.x + column->Offset() + column->Width();
	rect.bottom = rect.top + poseView->ListElemHeight();

	if (minimalRect) {
		BTextWidget *widget = WidgetFor(poseView->FirstColumn()->AttrHash());
		if (widget) 
			rect.right = widget->CalcRect(loc, poseView->FirstColumn(), poseView).right;
	}

	return rect;
}


BRect
BPose::CalcRect(const BPoseView *poseView)
{
	ASSERT(poseView->ViewMode() != kListMode);

	BRect rect;
	if (poseView->ViewMode() == kIconMode
		|| poseView->ViewMode() == kScaleIconMode) {
		rect.left = fLocation.x;
		rect.right = rect.left + poseView->IconSizeInt();

		BTextWidget *widget = WidgetFor(poseView->FirstColumn()->AttrHash());
		if (widget) {
			float textWidth = ceilf(widget->TextWidth(poseView) + 1);
			if (textWidth > poseView->IconSizeInt()) {
				rect.left += (poseView->IconSizeInt() - textWidth) / 2;
				rect.right = rect.left + textWidth; 
			}
		}

		rect.top = fLocation.y;
		rect.bottom = rect.top + poseView->IconPoseHeight();
	} else {
		// MINI_ICON_MODE rect calc
		rect.left = fLocation.x;
		rect.top = fLocation.y;
		rect.right = rect.left + B_MINI_ICON + kMiniIconSeparator;
		rect.bottom = rect.top + poseView->IconPoseHeight();
		BTextWidget *widget = WidgetFor(poseView->FirstColumn()->AttrHash());
		if (widget)
			rect.right += ceil(widget->TextWidth(poseView) + 1);
	}

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
	}
	PRINT(("%sselected\n", IsSelected() ? "" : "not "));
	PRINT(("location %s x:%f y:%f\n", HasLocation() ? "" : "unknown ",
		HasLocation() ? Location().x : 0,
		HasLocation() ? Location().y : 0));
	PRINT(("%s autoplaced \n", WasAutoPlaced() ? "was" : "not"));
}

#endif
