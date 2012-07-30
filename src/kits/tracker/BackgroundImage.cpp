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

//  Classes used for setting up and managing background images


#include <Bitmap.h>
#include <ControlLook.h>
#include <Node.h>
#include <TranslationKit.h>
#include <View.h>
#include <Window.h>

#include <fs_attr.h>

#include "BackgroundImage.h"

#include "Background.h"
#include "Commands.h"
#include "PoseView.h"


namespace BPrivate {

const char* kBackgroundImageInfo 			= B_BACKGROUND_INFO;
const char* kBackgroundImageInfoOffset 		= B_BACKGROUND_ORIGIN;
const char* kBackgroundImageInfoTextOutline	= B_BACKGROUND_TEXT_OUTLINE;
const char* kBackgroundImageInfoMode 		= B_BACKGROUND_MODE;
const char* kBackgroundImageInfoWorkspaces 	= B_BACKGROUND_WORKSPACES;
const char* kBackgroundImageInfoPath 		= B_BACKGROUND_IMAGE;

} // namespace BPrivate


BackgroundImage*
BackgroundImage::GetBackgroundImage(const BNode* node, bool isDesktop)
{
	attr_info info;
	if (node->GetAttrInfo(kBackgroundImageInfo, &info) != B_OK)
		return NULL;

	BMessage container;
	char* buffer = new char[info.size];

	status_t error = node->ReadAttr(kBackgroundImageInfo, info.type, 0,
		buffer, (size_t)info.size);
	if (error == info.size)
		error = container.Unflatten(buffer);

	delete [] buffer;

	if (error != B_OK)
		return NULL;

	BackgroundImage* result = NULL;
	for (int32 index = 0; ; index++) {
		const char* path;
		uint32 workspaces = B_ALL_WORKSPACES;
		Mode mode = kTiled;
		bool textWidgetLabelOutline = false;
		BPoint offset;
		BBitmap* bitmap = NULL;

		if (container.FindString(kBackgroundImageInfoPath, index, &path)
				== B_OK) {
			bitmap = BTranslationUtils::GetBitmap(path);
			if (!bitmap) {
				PRINT(("failed to load background bitmap from path\n"));
				continue;
			}
		} else
			break;

		if (be_control_look != NULL && isDesktop) {
			be_control_look->SetBackgroundInfo(container);
		}

		container.FindInt32(kBackgroundImageInfoWorkspaces, index,
			(int32*)&workspaces);
		container.FindInt32(kBackgroundImageInfoMode, index, (int32*)&mode);
		container.FindBool(kBackgroundImageInfoTextOutline, index,
			&textWidgetLabelOutline);
		container.FindPoint(kBackgroundImageInfoOffset, index, &offset);

		BackgroundImage::BackgroundImageInfo* imageInfo = new
			BackgroundImage::BackgroundImageInfo(workspaces, bitmap, mode,
				offset, textWidgetLabelOutline);

		if (!result)
			result = new BackgroundImage(node, isDesktop);

		result->Add(imageInfo);
	}
	return result;
}


BackgroundImage::BackgroundImageInfo::BackgroundImageInfo(uint32 workspaces,
	BBitmap* bitmap, Mode mode, BPoint offset, bool textWidgetOutline)
	:	fWorkspace(workspaces),
		fBitmap(bitmap),
		fMode(mode),
		fOffset(offset),
		fTextWidgetOutline(textWidgetOutline)
{
}


BackgroundImage::BackgroundImageInfo::~BackgroundImageInfo()
{
	delete fBitmap;
}


BackgroundImage::BackgroundImage(const BNode* node, bool desktop)
	:	fIsDesktop(desktop),
		fDefinedByNode(*node),
		fView(NULL),
		fShowingBitmap(NULL),
		fBitmapForWorkspaceList(1, true)
{
}


BackgroundImage::~BackgroundImage()
{
}


void
BackgroundImage::Add(BackgroundImageInfo* info)
{
	fBitmapForWorkspaceList.AddItem(info);
}


void
BackgroundImage::Show(BView* view, int32 workspace)
{
	fView = view;

	BackgroundImageInfo* info = ImageInfoForWorkspace(workspace);
	if (info) {
		BPoseView* poseView = dynamic_cast<BPoseView*>(fView);
		if (poseView)
			poseView->SetWidgetTextOutline(info->fTextWidgetOutline);
		Show(info, fView);
	}
}


void
BackgroundImage::Show(BackgroundImageInfo* info, BView* view)
{
	BPoseView* poseView = dynamic_cast<BPoseView*>(view);
	if (poseView)
		poseView->SetWidgetTextOutline(info->fTextWidgetOutline);

	if (info->fBitmap == NULL) {
		view->ClearViewBitmap();
		view->Invalidate();
		fShowingBitmap = info;
		return;
	}
	BRect viewBounds(view->Bounds());
	BRect bitmapBounds(info->fBitmap->Bounds());
	BRect destinationBitmapBounds(bitmapBounds);

	uint32 tile = 0;
	uint32 followFlags = B_FOLLOW_TOP | B_FOLLOW_LEFT;

	// figure out the display mode and the destination bounds for the bitmap
	switch (info->fMode) {
		case kCentered:
			if (fIsDesktop) {
				destinationBitmapBounds.OffsetBy(
					(viewBounds.Width() - bitmapBounds.Width()) / 2,
					(viewBounds.Height() - bitmapBounds.Height()) / 2);
				break;
			}
			// else fall thru
		case kScaledToFit:
			if (fIsDesktop) {
				if (BRectRatio(destinationBitmapBounds)
						>= BRectRatio(viewBounds)) {
					float overlap = BRectHorizontalOverlap(viewBounds,
						destinationBitmapBounds);
					destinationBitmapBounds.Set(-overlap, 0,
						viewBounds.Width() + overlap, viewBounds.Height());
				} else {
					float overlap = BRectVerticalOverlap(viewBounds,
						destinationBitmapBounds);
					destinationBitmapBounds.Set(0, -overlap,
						viewBounds.Width(), viewBounds.Height() + overlap);
				}
				followFlags = B_FOLLOW_ALL;
				break;
			}
			// else fall thru
		case kAtOffset:
			destinationBitmapBounds.OffsetTo(info->fOffset);
			break;
		case kTiled:
			if (fIsDesktop) {
				destinationBitmapBounds.OffsetBy(
					(viewBounds.Width() - bitmapBounds.Width()) / 2,
					(viewBounds.Height() - bitmapBounds.Height()) / 2);
			}
			tile = B_TILE_BITMAP;
			break;
	}

	// switch to the bitmap and force a redraw
	view->SetViewBitmap(info->fBitmap, bitmapBounds, destinationBitmapBounds,
		followFlags, tile);
	view->Invalidate();
	fShowingBitmap = info;
}


float
BackgroundImage::BRectRatio(BRect rect)
{
	return rect.Width() / rect.Height();
}


float
BackgroundImage::BRectHorizontalOverlap(BRect hostRect, BRect resizedRect)
{
	return ((hostRect.Height() / resizedRect.Height() * resizedRect.Width())
		- hostRect.Width()) / 2;
}


float
BackgroundImage::BRectVerticalOverlap(BRect hostRect, BRect resizedRect)
{
	return ((hostRect.Width() / resizedRect.Width() * resizedRect.Height())
		- hostRect.Height()) / 2;
}


void
BackgroundImage::Remove()
{
	if (fShowingBitmap) {
		fView->ClearViewBitmap();
		fView->Invalidate();
		BPoseView* poseView = dynamic_cast<BPoseView*>(fView);
		// make sure text widgets draw the default way, erasing
		// their background
		if (poseView)
			poseView->SetWidgetTextOutline(true);
	}
	fShowingBitmap = NULL;
}


BackgroundImage::BackgroundImageInfo*
BackgroundImage::ImageInfoForWorkspace(int32 workspace) const
{
	uint32 workspaceMask = 1;

	for ( ; workspace; workspace--)
		workspaceMask *= 2;

	int32 count = fBitmapForWorkspaceList.CountItems();

	// do a simple lookup for the most likely candidate bitmap -
	// pick the imageInfo that is only defined for this workspace over one
	// that supports multiple workspaces
	BackgroundImageInfo* result = NULL;
	for (int32 index = 0; index < count; index++) {
		BackgroundImageInfo* info = fBitmapForWorkspaceList.ItemAt(index);
		if (info->fWorkspace == workspaceMask)
			return info;
		if (info->fWorkspace & workspaceMask)
			result = info;
	}

	return result;
}


void
BackgroundImage::WorkspaceActivated(BView* view, int32 workspace, bool state)
{
	if (!fIsDesktop)
		// we only care for desktop bitmaps
		return;

	if (!state)
		// we only care comming into a new workspace, not leaving one
		return;

	BackgroundImageInfo* info = ImageInfoForWorkspace(workspace);

	if (info != fShowingBitmap) {
		if (info)
			Show(info, view);
		else {
			if (BPoseView* poseView = dynamic_cast<BPoseView*>(view))
				poseView->SetWidgetTextOutline(true);
			view->ClearViewBitmap();
			view->Invalidate();
		}
		fShowingBitmap = info;
	}
}


void
BackgroundImage::ScreenChanged(BRect, color_space)
{
	if (!fIsDesktop || !fShowingBitmap)
		return;

	if (fShowingBitmap->fMode == kCentered) {
		BRect viewBounds(fView->Bounds());
		BRect bitmapBounds(fShowingBitmap->fBitmap->Bounds());
		BRect destinationBitmapBounds(bitmapBounds);
		destinationBitmapBounds.OffsetBy(
			(viewBounds.Width() - bitmapBounds.Width()) / 2,
			(viewBounds.Height() - bitmapBounds.Height()) / 2);

		fView->SetViewBitmap(fShowingBitmap->fBitmap, bitmapBounds,
			destinationBitmapBounds, B_FOLLOW_NONE, 0);
		fView->Invalidate();
	}
}


BackgroundImage*
BackgroundImage::Refresh(BackgroundImage* oldBackgroundImage,
	const BNode* fromNode, bool desktop, BPoseView* poseView)
{
	if (oldBackgroundImage) {
		oldBackgroundImage->Remove();
		delete oldBackgroundImage;
	}

	BackgroundImage* result = GetBackgroundImage(fromNode, desktop);
	if (result && poseView->ViewMode() != kListMode)
		result->Show(poseView, current_workspace());

	return result;
}
