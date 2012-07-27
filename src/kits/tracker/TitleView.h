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
#ifndef	_TITLE_VIEW_H
#define _TITLE_VIEW_H


#include <Cursor.h>
#include <DataIO.h>
#include <View.h>

#include "ObjectList.h"


namespace BPrivate {


class BPoseView;
class BColumn;
class BColumnTitle;
class ColumnTrackState;
class OffscreenBitmap;

const int32 kTitleViewHeight = 16;
const int32 kEdgeSize = 6;
const int32 kTitleColumnLeftExtraMargin = 11;
const int32 kTitleColumnRightExtraMargin = 5;
const int32 kTitleColumnExtraMargin = kTitleColumnLeftExtraMargin
	+ kTitleColumnRightExtraMargin;
const int32 kMinColumnWidth = 20;
const int32 kRemoveTitleMargin = 10;
const int32 kColumnStart = 40;


class BTitleView : public BView {
public:
	BTitleView(BRect, BPoseView*);
	virtual ~BTitleView();

	virtual	void MouseDown(BPoint where);
	virtual	void MouseUp(BPoint where);
	virtual	void Draw(BRect updateRect);

	void Draw(BRect, bool useOffscreen = false,
		bool updateOnly = true,
		const BColumnTitle* pressedColumn = 0,
		void (*trackRectBlitter)(BView*, BRect) = 0,
		BRect passThru = BRect(0, 0, 0, 0));

	void AddTitle(BColumn*, const BColumn* after = 0);
	void RemoveTitle(BColumn*);
	void Reset();

	BPoseView* PoseView() const;

protected:
	void MouseMoved(BPoint, uint32, const BMessage*);
	
private:
	BColumnTitle* FindColumnTitle(BPoint) const;
	BColumnTitle* InColumnResizeArea(BPoint) const;
	BColumnTitle* FindColumnTitle(const BColumn*) const;

	BPoseView* fPoseView;
	BObjectList<BColumnTitle> fTitleList;
	BCursor fHorizontalResizeCursor;

	BColumnTitle* fPreviouslyClickedColumnTitle;
	bigtime_t fPreviousLeftClickTime;
	ColumnTrackState* fTrackingState;

	static OffscreenBitmap* sOffscreen;
	
	typedef BView _inherited;

	friend class ColumnTrackState;
	friend class ColumnDragState;
};


class BColumnTitle {
public:
	BColumnTitle(BTitleView*, BColumn*);
	virtual ~BColumnTitle() {}
	
	virtual void Draw(BView*, bool pressed = false);

	BColumn* Column() const;
	BRect Bounds() const;

	bool InColumnResizeArea(BPoint) const;

private:
	BColumn* fColumn;
	BTitleView* fParent;

	friend class ColumnResizeState;
};


// Utility classes to handle dragging state
class ColumnTrackState {
public:
	ColumnTrackState(BTitleView* titleView, BColumnTitle* columnTitle,
		BPoint where, bigtime_t pastClickTime);
	virtual ~ColumnTrackState() {}

	void MouseMoved(BPoint where, uint32 buttons);
	void MouseUp(BPoint where);

protected:
	virtual void Moved(BPoint where, uint32 buttons) = 0;
	virtual void Clicked(BPoint where) = 0;
	virtual void Done(BPoint where) = 0;
	virtual bool ValueChanged(BPoint where) = 0;

	BTitleView*		fTitleView;
	BColumnTitle*	fTitle;
	BPoint			fFirstClickPoint;
	bigtime_t		fPastClickTime;
	bool			fHasMoved;
};


class ColumnResizeState : public ColumnTrackState {
public:
	ColumnResizeState(BTitleView* titleView, BColumnTitle* columnTitle,
		BPoint where, bigtime_t pastClickTime);

protected:
	virtual void Moved(BPoint where, uint32 buttons);
	virtual void Done(BPoint where);
	virtual void Clicked(BPoint where);
	virtual bool ValueChanged(BPoint);

	void DrawLine();
	void UndrawLine();

private:
	float fLastLineDrawPos;
	float fInitialTrackOffset;

	typedef ColumnTrackState _inherited;
};


class ColumnDragState : public ColumnTrackState {
public:
	ColumnDragState(BTitleView* titleView, BColumnTitle* columnTitle,
		BPoint where, bigtime_t pastClickTime);

protected:
	virtual void Moved(BPoint where, uint32 buttons);
	virtual void Done(BPoint where);
	virtual void Clicked(BPoint where);
	virtual bool ValueChanged(BPoint);

	void DrawOutline(float);
	void UndrawOutline();
	void DrawPressNoOutline();

private:
	float fInitialMouseTrackOffset;
	bool fTrackingRemovedColumn;
	BMallocIO fColumnArchive;

	typedef ColumnTrackState _inherited;
};


inline BColumn*
BColumnTitle::Column() const
{
	return fColumn;
}


inline BPoseView*
BTitleView::PoseView() const
{
	return fPoseView;
}


} // namespace BPrivate

using namespace BPrivate;

#endif	// _TITLE_VIEW_H
