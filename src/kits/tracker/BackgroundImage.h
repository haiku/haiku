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
#ifndef __BACKGROUND_IMAGE__
#define __BACKGROUND_IMAGE__


//  Classes used for setting up and managing background images


#include <String.h>
#include "ObjectList.h"


class BNode;
class BView;
class BBitmap;

namespace BPrivate {

class BackgroundImage;
class BPoseView;

extern const char* kBackgroundImageInfo;
extern const char* kBackgroundImageInfoOffset;
extern const char* kBackgroundImageInfoTextOutline;
extern const char* kBackgroundImageInfoMode;
extern const char* kBackgroundImageInfoWorkspaces;
extern const char* kBackgroundImageInfoPath;

const uint32 kRestoreBackgroundImage = 'Tbgr';

class BackgroundImage {
	// This class knows everything about which bitmap to use for a given
	// view and how.
	// Unlike other windows, the Desktop window can have different backgrounds
	// for each workspace
public:

	enum Mode {
		kAtOffset,
		kCentered,			// only works on Desktop
		kScaledToFit,		// only works on Desktop
		kTiled
	};

	class BackgroundImageInfo {
		// element of the per-workspace list
	public:
		BackgroundImageInfo(uint32 workspace, BBitmap* bitmap, Mode mode,
			BPoint offset, bool textWidgetOutline);
		~BackgroundImageInfo();

		uint32 fWorkspace;
		BBitmap* fBitmap;
		Mode fMode;
		BPoint fOffset;
		bool fTextWidgetOutline;
	};

	static BackgroundImage* GetBackgroundImage(const BNode* node,
		bool isDesktop);
		// create a BackgroundImage object by reading it from a node
	virtual ~BackgroundImage();

	void Show(BView* view, int32 workspace);
		// display the right background for a given workspace
	void Remove();
		// remove the background from it's current view

	void WorkspaceActivated(BView* view, int32 workspace, bool state);
		// respond to a workspace change
	void ScreenChanged(BRect rect, color_space space);
		// respond to a screen size change
	static BackgroundImage* Refresh(BackgroundImage* oldBackgroundImage,
		const BNode* fromNode, bool desktop, BPoseView* poseView);
		// respond to a background image setting change

private:
	BackgroundImageInfo* ImageInfoForWorkspace(int32) const;
	void Show(BackgroundImageInfo*, BView* view);

	BackgroundImage(const BNode*, bool);
		// no public constructor, GetBackgroundImage factory function is
		// used instead

	void Add(BackgroundImageInfo*);

	float BRectRatio(BRect rect);
	float BRectHorizontalOverlap(BRect hostRect, BRect resizedRect);
	float BRectVerticalOverlap(BRect hostRect, BRect resizedRect);

	bool fIsDesktop;
	BNode fDefinedByNode;
	BView* fView;
	BackgroundImageInfo* fShowingBitmap;

	BObjectList<BackgroundImageInfo> fBitmapForWorkspaceList;
};

} // namespace BPrivate

using namespace BPrivate;

#endif
