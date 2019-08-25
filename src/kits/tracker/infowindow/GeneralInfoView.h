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

#ifndef GENERALINFOVIEW_H
#define GENERALINFOVIEW_H


#include <MenuField.h>
#include <Message.h>
#include <Rect.h>
#include <TextView.h>
#include <View.h>

#include "DialogPane.h"
#include "Model.h"


namespace BPrivate {

// States for tracking the mouse
enum track_state {
	no_track = 0,
	link_track,
	path_track,
	icon_track,
	size_track,
	open_only_track
		// This is for items that can be opened, but can't be
		// drag and dropped or renamed (Trash, Desktop Folder...)
};


class GeneralInfoView : public BView {
public:
	GeneralInfoView(BRect, Model*);
	~GeneralInfoView();

	void ModelChanged(Model*, BMessage*);
	void ReLinkTargetModel(Model*);
	void BeginEditingTitle();
	void FinishEditingTitle(bool);
	float CurrentFontHeight();

	BTextView* TextView() const { return fTitleEditView; }

	static filter_result TextViewFilter(BMessage*, BHandler**,
		BMessageFilter*);

	off_t LastSize() const;
	void SetLastSize(off_t);

	void SetSizeString(const char*);

	status_t BuildContextMenu(BMenu* parent);

	void SetPermissionsSwitchState(int32 state);

protected:
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32, const BMessage* dragMessage);
	virtual void MouseUp(BPoint where);
	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();
	virtual void Draw(BRect);
	virtual void Pulse();
	virtual void MakeFocus(bool focus);
	virtual void WindowActivated(bool active);

private:
	void InitStrings(const Model*);
	void CheckAndSetSize();
	void OpenLinkSource();
	void OpenLinkTarget();

	BString fPathStr;
	BString fLinkToStr;
	BString fSizeString;
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

	BMenuField* fPreferredAppMenu;
	Model* fModel;
	Model* fIconModel;
	BBitmap* fIcon;
	bool fMouseDown;
	bool fDragging;
	bool fDoubleClick;
	track_state fTrackingState;
	bool fIsDropTarget;
	BTextView* fTitleEditView;
	PaneSwitch* fPermissionsSwitch;
	BWindow* fPathWindow;
	BWindow* fLinkWindow;
	BWindow* fDescWindow;
	color_which fCurrentLinkColorWhich;
	color_which fCurrentPathColorWhich;

	typedef BView _inherited;
};

}	// namespace BPrivate


const uint32 kPermissionsSelected = 'sepe';
const uint32 kRecalculateSize = 'resz';
const uint32 kSetLinkTarget = 'link';

const float kBorderWidth = 32.0f;


#endif /* !GENERALINFOVIEW_H */
