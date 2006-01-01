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

#ifndef BARVIEW_H
#define BARVIEW_H

#include <Deskbar.h>
#include <View.h>

#include "NavMenu.h"
#include "ObjectList.h"

enum DeskbarShelf {
	B_DESKBAR_ANY_SHELF = 0,
	B_DESKBAR_TRAY = 1
};

enum {
	kMiniState = 0,
	kExpandoState = 1,
	kFullState = 2
};

const float kMiniHeight = 46.0f;
const float kHModeHeight = 21.0f;
const float kMenuBarHeight = 21.0f;
const float kStatusHeight = 22.0f;

class BShelf;
class TBarMenuBar;
class TExpandoMenuBar;
class TReplicantTray;
class TDragRegion;
class TTeamMenuItem;


class TBarView : public BView {
	public:
		TBarView(BRect frame, bool vertical, bool left, bool top,
			bool ampmMode, uint32 state, float width, bool showClock);
		~TBarView();

		virtual void AttachedToWindow();
		virtual void Draw(BRect);
		virtual void MessageReceived(BMessage *);

		void SaveSettings();
		void UpdatePlacement();
		void ChangeState(int32 state, bool vertical, bool left, bool top);

		bool Vertical() const;
		bool Left() const;
		bool Top() const;
		bool AcrossTop() const;
		bool AcrossBottom() const;
		bool Expando() const;
		int32 State() const;

		bool MilTime() const;
		void ShowClock(bool);
		bool ShowingClock() const;

		void CacheDragData(BMessage *incoming);
		status_t DragStart();
		static bool MenuTrackingHook(BMenu *menu, void *castToThis);
		void DragStop(bool full=false);
		TrackingHookData *GetTrackingHookData();
		bool Dragging() const;
		const BMessage *DragMessage() const;
		BObjectList<BString> *CachedTypesList() const;
		bool AppCanHandleTypes(const char *signature);
		void SetDragOverride(bool);
		bool DragOverride();
		bool InvokeItem(const char *signature);

		void HandleBeMenu(BMessage *targetmessage);

		status_t ItemInfo(int32 id, const char **name, DeskbarShelf *shelf);
		status_t ItemInfo(const char *name, int32 *id, DeskbarShelf *shelf);

		bool ItemExists(int32 id, DeskbarShelf shelf);
		bool ItemExists(const char *name, DeskbarShelf shelf);

		int32 CountItems(DeskbarShelf shelf);

		status_t AddItem(BMessage *, DeskbarShelf shelf, int32 *id);

		void RemoveItem(int32 id);
		void RemoveItem(const char *name, DeskbarShelf shelf);

		BRect OffsetIconFrame(BRect rect) const;
		BRect IconFrame(int32 id) const;
		BRect IconFrame(const char *name) const;

		void GetPreferredWindowSize(BRect screenFrame, float *width, float *height);
		void SizeWindow(BRect screenFrame);
		void PositionWindow(BRect screenFrame);

		TExpandoMenuBar *ExpandoMenuBar() const;
		TBarMenuBar *BarMenuBar() const;
		friend class TBeMenu;

	private:
		status_t SendDragMessage(const char *signature, entry_ref *ref = NULL);

		void PlaceBeMenu();
		void PlaceTray(bool vertSwap, bool leftSwap, BRect screenFrame);
		void PlaceApplicationBar(BRect screenFrame);

		TBarMenuBar *fBarMenuBar;
		TExpandoMenuBar *fExpando;

		int32 fTrayLocation;
		TDragRegion *fDragRegion;
		TReplicantTray *fReplicantTray;

		bool fShowInterval;
		bool fShowClock;
		bool fVertical;
		bool fTop;
		bool fLeft;

		int32 fState;

		bigtime_t fPulseRate;
		bool fRefsRcvdOnly;
		BMessage *fDragMessage;
		BObjectList<BString> *fCachedTypesList;
		TrackingHookData fTrackingHookData;

		uint32 fMaxRecentDocs;
		uint32 fMaxRecentApps;

		TTeamMenuItem *fLastDragItem;
		bool fClickToOpen;
};


inline TExpandoMenuBar *
TBarView::ExpandoMenuBar() const
{
	return fExpando;
}


inline TBarMenuBar *
TBarView::BarMenuBar() const
{
	return fBarMenuBar;
}


inline bool
TBarView::Dragging() const
{
	return (fCachedTypesList && fDragMessage);
}


inline const BMessage *
TBarView::DragMessage() const
{
	return fDragMessage;
}


inline BObjectList<BString> *
TBarView::CachedTypesList() const
{
	return fCachedTypesList;
}

#endif /* BARVIEW_H */
