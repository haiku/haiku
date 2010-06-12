//--------------------------------------------------------------------
//	
//	ViewLayoutFactory.cpp
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "ViewLayoutFactory.h"

#include <Button.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <View.h>
#include <List.h>
#include <algorithm>
#include <assert.h>
#include <stdio.h>

template <class T>
inline T average(const T& x, const T& y)
{
	return (x + y) / 2;
}


//--------------------------------------------------------------------
//	ViewLayoutFactory factory generators

BButton* ViewLayoutFactory::MakeButton(const char* name, const char* label,
	uint32 msgID, BPoint pos, corner posRef)
{
	BRect dummyFrame(0,0,0,0);
	BButton* pButton = new BButton(dummyFrame, name, label, 
		new BMessage(msgID));
	pButton->ResizeToPreferred();
	MoveViewCorner(*pButton, pos, posRef);
	return pButton;
}

BCheckBox* ViewLayoutFactory::MakeCheckBox(const char* name,
	const char* label, uint32 msgID, BPoint pos, corner posRef)
{
	BRect dummyFrame(0,0,0,0);
	BCheckBox* pCheckBox = new BCheckBox(dummyFrame, name, label,
		new BMessage(msgID));
	pCheckBox->ResizeToPreferred();
	MoveViewCorner(*pCheckBox, pos, posRef);
	return pCheckBox;
}

BTextControl* ViewLayoutFactory::MakeTextControl(const char* name,
	const char* label, const char* text, BPoint pos, float controlWidth,
	corner posRef)
{
	// Note: this function is almost identical to the regular
	// BTextControl constructor, but it minimizes the label
	// width and maximizes the field width, instead of divvying
	// them up half-&-half or 2 : 3, and gives the standard
	// flexibility of corner positioning.
	BRect dummyFrame(0,0,0,0);
	BTextControl* pCtrl = new BTextControl(dummyFrame, name, label,
		text, NULL);
	LayoutTextControl(*pCtrl, pos, controlWidth, posRef);
	return pCtrl;
}

void ViewLayoutFactory::LayoutTextControl(BTextControl& control,
	BPoint pos, float controlWidth, corner posRef)
{
	control.ResizeToPreferred();
	BTextView* pTextView = control.TextView();
	float widthExpand = controlWidth;
	widthExpand -= control.Bounds().Width();
	if (widthExpand > 0) {
		control.ResizeBy(widthExpand, 0);
		pTextView->ResizeBy(widthExpand, 0);
	}
	MoveViewCorner(control, pos, posRef);
}



//--------------------------------------------------------------------
//	ViewLayoutFactory implementation member functions

void ViewLayoutFactory::MoveViewCorner(BView& view, BPoint pos, corner posRef)
{
	BRect frame = view.Frame();
	BPoint topLeft;
	switch (posRef) {
	case CORNER_TOPLEFT:
		topLeft = pos;
		break;
	case CORNER_BOTTOMLEFT:
		topLeft.x = pos.x;
		topLeft.y = pos.y - frame.Height();
		break;
	case CORNER_TOPRIGHT:
		topLeft.x = pos.x - frame.Width();
		topLeft.y = pos.y;
		break;
	case CORNER_BOTTOMRIGHT:
		topLeft.x = pos.x - frame.Width();
		topLeft.y = pos.y - frame.Height();
		break;
	}
	
	view.MoveTo(topLeft);			
}

void ViewLayoutFactory::Align(BList& viewList, align_side side,
	float alignLen)
{
	int32 i, len = viewList.CountItems();
	if (len <= 1) {
		return;
	}

	// make sure alignment is recognized
	if ((side != ALIGN_LEFT) && (side != ALIGN_TOP)
		&& (side != ALIGN_RIGHT) && (side != ALIGN_BOTTOM)
		&& (side != ALIGN_HCENTER) && (side != ALIGN_VCENTER))
	{
		return;
	}
	
	// find initial location for placement, based on head item
	BPoint viewLoc;
	BView* pView = reinterpret_cast<BView*>(viewList.ItemAt(0));
	if (! pView) {
		return;
	}
	
	BRect frame = pView->Frame();
	switch (side) {
	case ALIGN_LEFT:
	case ALIGN_TOP:
		viewLoc.Set(frame.left, frame.top);
		break;
	case ALIGN_RIGHT:
		viewLoc.Set(frame.right, frame.top);
		break;
	case ALIGN_BOTTOM:
		viewLoc.Set(frame.left, frame.bottom);
		break;
	case ALIGN_HCENTER:
		viewLoc.Set(frame.left, average(frame.top, frame.bottom));
		break;
	case ALIGN_VCENTER:
		viewLoc.Set(average(frame.left, frame.right), frame.top);
		printf("Aligning along vcenter\nInitial position: ");
		viewLoc.PrintToStream();
		break;
	}

	// align items relative to head item
	for (i=1; i<len; i++) {
		BView* pView = reinterpret_cast<BView*>(viewList.ItemAt(i));
		if (pView) {
			switch (side) {
			case ALIGN_LEFT:
				viewLoc.y += alignLen;
				MoveViewCorner(*pView, viewLoc, CORNER_TOPLEFT);
				break;
			case ALIGN_TOP:
				viewLoc.x += alignLen;
				MoveViewCorner(*pView, viewLoc, CORNER_TOPLEFT);
				break;
			case ALIGN_RIGHT:
				viewLoc.y += alignLen;
				MoveViewCorner(*pView, viewLoc, CORNER_TOPRIGHT);
				break;
			case ALIGN_BOTTOM:
				viewLoc.x += alignLen;
				MoveViewCorner(*pView, viewLoc, CORNER_BOTTOMLEFT);
				break;
			case ALIGN_HCENTER:
				{
					viewLoc.x += alignLen;
					BPoint moveLoc = viewLoc;
					BRect r = pView->Frame();
					moveLoc.y -= (r.bottom - r.top) / 2;
					MoveViewCorner(*pView, moveLoc, CORNER_TOPLEFT);
					break;
				}
			case ALIGN_VCENTER:
				{
					viewLoc.y += alignLen;
					BPoint moveLoc = viewLoc;
					BRect r = pView->Frame();
					moveLoc.x -= (r.right - r.left) / 2;
					MoveViewCorner(*pView, moveLoc, CORNER_TOPLEFT);
					break;
				}
			}
		}
	}
}

void ViewLayoutFactory::ResizeToListMax(BList& viewList, rect_dim resizeDim,
	corner anchor)
{
	int32 i, len = viewList.CountItems();
	float maxWidth = 0.0f, maxHeight = 0.0f;
	float curWidth = 0.0f, curHeight = 0.0f;

	// find maximum dimensions
	for (i=0; i<len; i++) {
		BView* pView = reinterpret_cast<BView*>(viewList.ItemAt(i));
		if (pView) {
			BRect frame = pView->Frame();
			curWidth = frame.Width();
			curHeight = frame.Height();
			if (curWidth > maxWidth) {
				maxWidth = curWidth;
			}
			if (curHeight > maxHeight) {
				maxHeight = curHeight;
			}
		}
	}

	// now resize everything in the list based on selected dimension
	for (i=0; i<len; i++) {
		BView* pView = reinterpret_cast<BView*>(viewList.ItemAt(i));
		if (pView) {
			float newWidth, newHeight;
			BRect frame = pView->Frame();
			newWidth = (resizeDim & RECT_WIDTH)
				? maxWidth : frame.Width();
			newHeight = (resizeDim & RECT_HEIGHT)
				? maxHeight : frame.Height();
			pView->ResizeTo(newWidth, newHeight);
		}
	}
}

void ViewLayoutFactory::ResizeAroundChildren(BView& view, BPoint margin)
{
	float fMax_x = 0.0f, fMax_y = 0.0f;
	int32 numChild = view.CountChildren();
	for (int32 i = 0; i < numChild; i++)
	{
		BView* childView = view.ChildAt(i);
		if (childView) {
			BRect r = childView->Frame();
			fMax_x = std::max(fMax_x, r.right);
			fMax_y = std::max(fMax_y, r.bottom);
		}
	}
	
	fMax_x += margin.x;
	fMax_y += margin.y;
	view.ResizeTo(fMax_x, fMax_y);
}
