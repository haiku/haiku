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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/
#ifndef BARVIEW_H
#define BARVIEW_H


#include <Deskbar.h>
#include <View.h>

#include "NavMenu.h"
#include "ObjectList.h"


const uint32 kStateChanged = 'stch';


enum DeskbarShelf {
	B_DESKBAR_ANY_SHELF = 0,
	B_DESKBAR_TRAY = 1
};

enum {
	kMiniState = 0,
	kExpandoState = 1,
	kFullState = 2
};

const float kHModeHeight = 21.0f;
const float kMenuBarHeight = 21.0f;
const float kStatusHeight = 22.0f;
const float kHiddenDimension = 1.0f;
const float kMaxPreventHidingDist = 80.0f;


class BShelf;
class TBarApp;
class TBarMenuBar;
class TBarWindow;
class TExpandoMenuBar;
class TReplicantTray;
class TDragRegion;
class TResizeControl;
class TInlineScrollView;
class TTeamMenuItem;

class TBarView : public BView {
public:
							TBarView(BRect frame, bool vertical, bool left,
								bool top, int32 state, float width);
							~TBarView();

	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();

	virtual	void			Draw(BRect updateRect);

	virtual	void			MessageReceived(BMessage* message);

	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseMoved(BPoint where, uint32 transit,
								const BMessage* dragMessage);
	virtual	void			MouseUp(BPoint where);

			void			SaveSettings();

			void			UpdatePlacement();
			void			ChangeState(int32 state, bool vertical, bool left,
								bool top, bool aSync = false);

			void			RaiseDeskbar(bool raise);
			void			HideDeskbar(bool hide);

	// window placement methods
			bool			Vertical() const { return fVertical; };
			bool			Left() const { return fLeft; };
			bool			Top() const { return fTop; };
			bool			AcrossTop() const { return fTop && !fVertical
								&& fState != kMiniState; };
			bool			AcrossBottom() const { return !fTop && !fVertical
								&& fState != kMiniState; };

	// window state methods
			bool			ExpandoState() const
								{ return fState == kExpandoState; };
			bool			FullState() const { return fState == kFullState; };
			bool			MiniState() const { return fState == kMiniState; };
			int32			State() const { return fState; };

	// drag and drop methods
			void			CacheDragData(const BMessage* incoming);
			status_t		DragStart();
	static	bool			MenuTrackingHook(BMenu* menu, void* castToThis);
			void			DragStop(bool full = false);
			TrackingHookData*	GetTrackingHookData();
			bool			Dragging() const;
			const			BMessage* DragMessage() const;
			BObjectList<BString>*	CachedTypesList() const;
			bool			AppCanHandleTypes(const char* signature);
			void			SetDragOverride(bool);
			bool			DragOverride();
			bool			InvokeItem(const char* signature);

			void			HandleDeskbarMenu(BMessage* targetmessage);

			status_t		ItemInfo(int32 id, const char** name,
								DeskbarShelf* shelf);
			status_t		ItemInfo(const char* name, int32* id,
								DeskbarShelf* shelf);

			bool			ItemExists(int32 id, DeskbarShelf shelf);
			bool			ItemExists(const char* name, DeskbarShelf shelf);

			int32			CountItems(DeskbarShelf shelf);

			BSize			MaxItemSize(DeskbarShelf shelf);

			status_t		AddItem(BMessage* archive, DeskbarShelf shelf,
								int32* id);
			status_t		AddItem(BEntry* entry, DeskbarShelf shelf,
								int32* id);

			void			RemoveItem(int32 id);
			void			RemoveItem(const char* name, DeskbarShelf shelf);

			BRect			OffsetIconFrame(BRect rect) const;
			BRect			IconFrame(int32 id) const;
			BRect			IconFrame(const char* name) const;

			void			GetPreferredWindowSize(BRect screenFrame,
								float* width, float* height);
			void			SizeWindow(BRect screenFrame);
			void			PositionWindow(BRect screenFrame);

			void			CheckForScrolling();

			TExpandoMenuBar*	ExpandoMenuBar() const;
			TBarMenuBar*		BarMenuBar() const;
			TDragRegion*		DragRegion() const { return fDragRegion; }
			TReplicantTray*		ReplicantTray() const { return fReplicantTray; }

			float			TeamMenuItemHeight() const;
			float			TabHeight() const { return fTabHeight; };

private:
	friend class TBarApp;
	friend class TDeskbarMenu;
	friend class PreferencesWindow;

			status_t		SendDragMessage(const char* signature,
								entry_ref* ref = NULL);

			void			PlaceDeskbarMenu();
			void			PlaceTray(bool vertSwap, bool leftSwap);
			void			PlaceApplicationBar();

			void			_ChangeState(BMessage* message);

			TBarApp*			fBarApp;
			TBarWindow*			fBarWindow;
			TInlineScrollView*	fInlineScrollView;
			TBarMenuBar*		fBarMenuBar;
			TExpandoMenuBar*	fExpandoMenuBar;

			int32			fTrayLocation;
			TDragRegion*	fDragRegion;
			TResizeControl*	fResizeControl;
			TReplicantTray*	fReplicantTray;

			bool			fIsRaised : 1;
			bool			fMouseDownOutside : 1;

			bool			fVertical : 1;
			bool			fTop : 1;
			bool			fLeft : 1;
			int32			fState;

			bigtime_t		fPulseRate;
			bool			fRefsRcvdOnly;
			BMessage*		fDragMessage;
			BObjectList<BString>*	fCachedTypesList;
			TrackingHookData		fTrackingHookData;

			uint32			fMaxRecentDocs;
			uint32			fMaxRecentApps;

			TTeamMenuItem*	fLastDragItem;
			BMessageFilter*	fMouseFilter;

			float			fTabHeight;
};


inline TExpandoMenuBar*
TBarView::ExpandoMenuBar() const
{
	return fExpandoMenuBar;
}


inline TBarMenuBar*
TBarView::BarMenuBar() const
{
	return fBarMenuBar;
}


inline bool
TBarView::Dragging() const
{
	return (fCachedTypesList && fDragMessage);
}


inline const BMessage*
TBarView::DragMessage() const
{
	return fDragMessage;
}


inline BObjectList<BString>*
TBarView::CachedTypesList() const
{
	return fCachedTypesList;
}


#endif	// BARVIEW_H
