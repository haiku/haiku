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


#ifndef HEADERVIEW_H
#define HEADERVIEW_H


#include <Bitmap.h>
#include <Handler.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Rect.h>
#include <TextView.h>
#include <View.h>

#include "Model.h"


class HeaderView: public BView {
public:
	HeaderView(Model*);
	~HeaderView();

	void ModelChanged(Model*, BMessage*);
	void ReLinkTargetModel(Model*);
	void BeginEditingTitle();
	void FinishEditingTitle(bool);
	virtual void Draw(BRect);
	virtual void MakeFocus(bool focus);
	virtual void WindowActivated(bool active);

	BTextView* TextView() const { return fTitleEditView; }

protected:
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32, const BMessage* dragMessage);
	virtual void MouseUp(BPoint where);
	virtual void MessageReceived(BMessage* message);

	status_t BuildContextMenu(BMenu* parent);
	static filter_result TextViewFilter(BMessage*, BHandler**,
		BMessageFilter*);

	float CurrentFontHeight();

private:
	// States for tracking the mouse
	enum track_state {
		no_track = 0,
		icon_track,
		open_only_track
			// This is for items that can be opened, but can't be
			// drag and dropped or renamed (Trash, Desktop Folder...)
	};

	// Layouting
	BRect fTitleRect;
	BRect fIconRect;
	BPoint fClickPoint;

	// Model data
	Model* fModel;
	Model* fIconModel;
	BBitmap* fIcon;
	BTextView* fTitleEditView;

	// Mouse tracking
	track_state fTrackingState;
	bool fMouseDown;
	bool fIsDropTarget;
	bool fDoubleClick;
	bool fDragging;

	typedef BView _inherited;
};


#endif /* !HEADERVIEW_H */
