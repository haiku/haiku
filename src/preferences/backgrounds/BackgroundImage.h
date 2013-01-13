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
//

#ifndef __BACKGROUND_IMAGE__
#define __BACKGROUND_IMAGE__


#include <GraphicsDefs.h>
#include <Node.h>
#include <Path.h>

#include "ObjectList.h"
#include "String.h"


class BView;
class BBitmap;

class BackgroundImage;
class Image;
class BackgroundsView;

extern const char* kBackgroundImageInfo;
extern const char* kBackgroundImageInfoOffset;
extern const char* kBackgroundImageInfoEraseText;
extern const char* kBackgroundImageInfoMode;
extern const char* kBackgroundImageInfoWorkspaces;
extern const char* kBackgroundImageInfoPath;
extern const char* kBackgroundImageInfoSet;
extern const char* kBackgroundImageInfoSetPeriod;

const uint32 kRestoreBackgroundImage = 'Tbgr';
const uint32 kChangeBackgroundImage = 'Cbgr';

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
		BackgroundImageInfo(uint32 workspace, int32 imageIndex, Mode mode,
			BPoint offset, bool textWidgetLabelOutline, uint32 imageSet,
			uint32 cacheMode);
		~BackgroundImageInfo();

		void LoadBitmap();
		void UnloadBitmap(uint32 globalCacheMode);

		uint32 fWorkspace;
		int32 fImageIndex;
		Mode fMode;
		BPoint fOffset;
		bool fTextWidgetLabelOutline;
		uint32 fImageSet;
		uint32 fCacheMode;		// image cache strategy (0 cache , 1 no cache)
	};

	static BackgroundImage* GetBackgroundImage(const BNode* node,
		bool isDesktop, BackgroundsView* view);
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
	/*static BackgroundImage* Refresh(BackgroundImage* oldBackgroundImage,
		const BNode* fromNode, bool desktop, BPoseView* poseView);
		// respond to a background image setting change
	void ChangeImageSet(BPoseView* poseView);
		// change to the next imageSet if any, no refresh*/
	BackgroundImageInfo* ImageInfoForWorkspace(int32) const;

	bool IsDesktop() { return fIsDesktop;}

	status_t SetBackgroundImage(BNode* node);

	void Show(BackgroundImageInfo*, BView* view);

	uint32 GetShowingImageSet() { return fShowingImageSet; }

	void Add(BackgroundImageInfo*);
	void Remove(BackgroundImageInfo*);
	void RemoveAll();

private:
	BackgroundImage(const BNode* node, bool isDesktop, BackgroundsView* view);
		// no public constructor, GetBackgroundImage factory function is
		// used instead

	float BRectRatio(BRect rect);
	float BRectHorizontalOverlap(BRect hostRect, BRect resizedRect);
	float BRectVerticalOverlap(BRect hostRect, BRect resizedRect);

	bool fIsDesktop;
	BNode fDefinedByNode;
	BView* fView;
	BackgroundsView* fBackgroundsView;
	BackgroundImageInfo* fShowingBitmap;

	BObjectList<BackgroundImageInfo> fBitmapForWorkspaceList;

	uint32 fImageSetPeriod;		// period between imagesets, 0 if none
	uint32 fShowingImageSet;	// current imageset
	uint32 fImageSetCount;		// imageset count
	uint32 fCacheMode;// image cache strategy (0 all, 1 none, 2 own strategy)
	bool fRandomChange; 		// random or sequential change
};

class Image {
	// element for each image
public:
	Image(BPath path);
	~Image();

	void UnloadBitmap();
	const char* GetName() { return fName.String(); }
	BBitmap* GetBitmap();
	BPath GetPath() { return fPath; }
private:
	BBitmap* fBitmap;
	BPath fPath;
	BString fName;
};

#endif

