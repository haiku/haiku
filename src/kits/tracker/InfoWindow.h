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

#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H

#include <String.h>
#include <Window.h>
#include <MessageFilter.h>

#include "DialogPane.h"
#include "FilePermissionsView.h"
#include "LockingList.h"
#include "Utilities.h"

class BMenuField;

namespace BPrivate {

class Model;
class AttributeView;
class TrackingView;

// States for tracking the mouse
enum track_state {
	no_track = 0,
	link_track,
	path_track,
	icon_track,
	size_track,
	open_only_track		// This is for items that can be opened, but can't be
						// drag and dropped or renamed (Trash, Desktop Folder...)
};

class TrackingView : public BControl {
	public:
		TrackingView(BRect, const char *str, const BFont *font, BMessage *message);

		virtual void MouseDown(BPoint);
		virtual void MouseMoved(BPoint, uint32 transit, const BMessage *message);
		virtual void MouseUp(BPoint);
		virtual void Draw(BRect);

	private:
		bool fMouseDown;
		bool fMouseInView;
		BFont fFont;
};

class AttributeView : public BView {	
	public:
		AttributeView(BRect, Model *);
		~AttributeView();

		void ModelChanged(Model *, BMessage *);
		void ReLinkTargetModel(Model *);
		void BeginEditingTitle();
		void FinishEditingTitle(bool);
		float CurrentFontHeight(float size = -1);

		BTextView *TextView() const { return fTitleEditView; }

		static filter_result TextViewFilter(BMessage *, BHandler **, BMessageFilter *);

		off_t LastSize() const;
		void SetLastSize(off_t);

		void SetSizeStr(const char *);

		status_t BuildContextMenu(BMenu *parent);

		void SetPermissionsSwitchState(int32 state);

	protected:
		virtual void MouseDown(BPoint);
		virtual void MouseMoved(BPoint, uint32, const BMessage *);
		virtual void MouseUp(BPoint);
		virtual void MessageReceived(BMessage *);
		virtual void AttachedToWindow();
		virtual void Draw(BRect);
		virtual void Pulse();
		virtual void MakeFocus(bool);
		virtual void WindowActivated(bool);

	private:
		void InitStrings(const Model *);
		void CheckAndSetSize();
		void OpenLinkSource();
		void OpenLinkTarget();

		BString fPathStr;
		BString fLinkToStr;
		BString fSizeStr;
		BString fModifiedStr;
		BString fCreatedStr;
		BString fKindStr;
		BString fDescStr;

		off_t fFreeBytes;
		off_t fLastSize;

		BRect fPathRect;
		BRect fLinkRect;
		BRect fDescRect;
		BRect fTitleRect;
		BRect fIconRect;
		BRect fSizeRect;
		BPoint fClickPoint;
		float fDivider;

		BMenuField *fPreferredAppMenu;
		Model *fModel;
		Model *fIconModel;
		BBitmap *fIcon;
		bool fMouseDown;
		bool fDragging;
		bool fDoubleClick;
		track_state fTrackingState;
		bool fIsDropTarget;
		BTextView *fTitleEditView;
		PaneSwitch *fPermissionsSwitch;
		BWindow *fPathWindow;
		BWindow *fLinkWindow;
		BWindow *fDescWindow;

		typedef BView _inherited;
};


class BInfoWindow : public BWindow {
	public:
		BInfoWindow(Model *, int32 groupIndex, LockingList<BWindow> *list = NULL);
		~BInfoWindow();

		virtual bool IsShowing(const node_ref *) const;
		Model *TargetModel() const;
		void SetSizeStr(const char *);
		bool StopCalc();
		void OpenFilePanel(const entry_ref *);

		static void GetSizeString(BString &result, off_t size, int32 fileCount);

	protected:
		virtual void Quit();
		virtual void MessageReceived(BMessage *);
		virtual void Show();

	private:
		static BRect InfoWindowRect(bool displayingSymlink);
		static int32 CalcSize(void *); 

		Model *fModel;
		volatile bool fStopCalc;
		int32 fIndex; 				// tells where it lives with respect to other
		thread_id fCalcThreadID;
		LockingList<BWindow> *fWindowList;
		FilePermissionsView *fPermissionsView;
		AttributeView *fAttributeView;
		BFilePanel *fFilePanel;
		bool fFilePanelOpen;

		typedef BWindow _inherited;
};


inline bool
BInfoWindow::StopCalc()
{
	return fStopCalc;
}

inline Model *
BInfoWindow::TargetModel() const
{
	return fModel;
}

} // namespace BPrivate

using namespace BPrivate;

#endif
