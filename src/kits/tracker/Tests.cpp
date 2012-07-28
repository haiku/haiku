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


#if DEBUG

#include "Tests.h"

#include <Debug.h>
#include <Locker.h>
#include <Path.h>
#include <String.h>
#include <Window.h>


#include "EntryIterator.h"
#include "IconCache.h"
#include "Model.h"
#include "NodeWalker.h"
#include "StopWatch.h"
#include "Thread.h"


const char* pathsToSearch[] = {
//	"/boot/home/config/settings/NetPositive/Bookmarks/",
#ifdef __HAIKU__
	"/boot/system",
#else
	"/boot/beos",
#endif
	"/boot/apps",
	"/boot/home",
	0
};


namespace BTrackerPrivate {

class IconSpewer : public SimpleThread {
public:
	IconSpewer(bool newCache = true);
	~IconSpewer();
	void SetTarget(BWindow* target)
		{ this->target = target; }

	void Quit();
	void Run();

protected:
	void DrawSomeNew();
	void DrawSomeOld();
	const entry_ref* NextRef();
private:
	BLocker locker;
	bool quitting;
	BWindow* target;
	TNodeWalker* walker;
	CachedEntryIterator* cachingIterator;
	int32 searchPathIndex;
	bigtime_t cycleTime;
	bigtime_t lastCycleLap;
	int32 numDrawn;
	BStopWatch watch;
	bool newCache;
	BPath currentPath;

	entry_ref ref;
};


class IconTestWindow : public BWindow {
public:
	IconTestWindow();
	bool QuitRequested();
private:
	IconSpewer iconSpewer;
};

}	// namespace BTrackerPrivate


IconSpewer::IconSpewer(bool newCache)
	:	quitting(false),
		cachingIterator(0),
		searchPathIndex(0),
		cycleTime(0),
		lastCycleLap(0),
		numDrawn(0),
		watch("", true),
		newCache(newCache)
{
	walker = new TNodeWalker(pathsToSearch[searchPathIndex++]);
	if (newCache)
		cachingIterator = new CachedEntryIterator(walker, 40);
}


IconSpewer::~IconSpewer()
{
	delete walker;
	delete cachingIterator;
}


void
IconSpewer::Run()
{
	BStopWatch watch("", true);
	for (;;) {
		AutoLock<BLocker> lock(locker);

		if (!lock || quitting)
			break;

		lock.Unlock();
		if (newCache)
			DrawSomeNew();
		else
			DrawSomeOld();
	}
}


void
IconSpewer::Quit()
{
	kill_thread(fScanThread);
	fScanThread = -1;
}


const icon_size kIconSize = B_LARGE_ICON;
const int32 kRowCount = 10;
const int32 kColumnCount = 10;


void
IconSpewer::DrawSomeNew()
{
	target->Lock();
	BView* view = target->FindView("iconView");
	ASSERT(view);

	BRect bounds(target->Bounds());
	view->SetHighColor(Color(255, 255, 255));
	view->FillRect(bounds);

	view->SetHighColor(Color(0, 0, 0));
	char buffer[256];
	if (cycleTime) {
		sprintf(buffer, "last cycle time %Ld ms", cycleTime/1000);
		view->DrawString(buffer, BPoint(20, bounds.bottom - 20));
	}

	if (numDrawn) {
		sprintf(buffer, "average draw time %Ld us per icon", watch.ElapsedTime() / numDrawn);
		view->DrawString(buffer, BPoint(20, bounds.bottom - 30));
	}

	sprintf(buffer, "directory: %s", currentPath.Path());
	view->DrawString(buffer, BPoint(20, bounds.bottom - 40));

	target->Unlock();

	for (int32 row = 0; row < kRowCount; row++) {
		for (int32 column = 0; column < kColumnCount; column++) {
			BEntry entry(NextRef());
			Model model(&entry, true);

			if (!target->Lock())
				return;

			if (model.IsDirectory())
				entry.GetPath(&currentPath);

			IconCache::sIconCache->Draw(&model, view, BPoint(column * (kIconSize + 2),
				row * (kIconSize + 2)), kNormalIcon, kIconSize, true);
			target->Unlock();
			numDrawn++;
		}
	}
}


bool oldIconCacheInited = false;


void
IconSpewer::DrawSomeOld()
{
#if 0
	if (!oldIconCacheInited)
		BIconCache::InitIconCaches();

	target->Lock();
	target->SetTitle("old cache");
	BView* view = target->FindView("iconView");
	ASSERT(view);

	BRect bounds(target->Bounds());
	view->SetHighColor(Color(255, 255, 255));
	view->FillRect(bounds);

	view->SetHighColor(Color(0, 0, 0));
	char buffer[256];
	if (cycleTime) {
		sprintf(buffer, "last cycle time %Ld ms", cycleTime/1000);
		view->DrawString(buffer, BPoint(20, bounds.bottom - 20));
	}
	if (numDrawn) {
		sprintf(buffer, "average draw time %Ld us per icon", watch.ElapsedTime() / numDrawn);
		view->DrawString(buffer, BPoint(20, bounds.bottom - 30));
	}
	sprintf(buffer, "directory: %s", currentPath.Path());
	view->DrawString(buffer, BPoint(20, bounds.bottom - 40));

	target->Unlock();

	for (int32 row = 0; row < kRowCount; row++) {
		for (int32 column = 0; column < kColumnCount; column++) {
			BEntry entry(NextRef());
			BModel model(&entry, true);

			if (!target->Lock())
				return;

			if (model.IsDirectory())
				entry.GetPath(&currentPath);

			BIconCache::LockIconCache();
			BIconCache* iconCache = BIconCache::GetIconCache(&model, kIconSize);
			iconCache->Draw(view, BPoint(column * (kIconSize + 2),
				row * (kIconSize + 2)), B_NORMAL_ICON, kIconSize, true);
			BIconCache::UnlockIconCache();

			target->Unlock();
			numDrawn++;
		}
	}
#endif
}


const entry_ref*
IconSpewer::NextRef()
{
	status_t result;
	if (newCache)
		result = cachingIterator->GetNextRef(&ref);
	else
		result = walker->GetNextRef(&ref);

	if (result == B_OK)
		return &ref;

	delete walker;
	if (!pathsToSearch[searchPathIndex]) {
		bigtime_t now = watch.ElapsedTime();
		cycleTime = now - lastCycleLap;
		lastCycleLap = now;
		PRINT(("**************************hit end of disk, starting over\n"));
		searchPathIndex = 0;
	}

	walker = new TNodeWalker(pathsToSearch[searchPathIndex++]);
	if (newCache) {
		cachingIterator->SetTo(walker);
		result = cachingIterator->GetNextRef(&ref);
	} else
		result = walker->GetNextRef(&ref);

	ASSERT(result == B_OK);
		// we don't expect and cannot deal with any problems here
	return &ref;
}


//	#pragma mark -


IconTestWindow::IconTestWindow()
	:	BWindow(BRect(100, 100, 500, 600), "icon cache test", B_TITLED_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, 0),
		iconSpewer(modifiers() == 0)
{
	iconSpewer.SetTarget(this);
	BView* view = new BView(Bounds(), "iconView", B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(view);
	iconSpewer.Go();
}


bool
IconTestWindow::QuitRequested()
{
	iconSpewer.Quit();
	return true;
}


void
RunIconCacheTests()
{
	(new IconTestWindow())->Show();
}

#endif
