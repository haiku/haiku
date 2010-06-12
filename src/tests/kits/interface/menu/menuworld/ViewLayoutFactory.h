//--------------------------------------------------------------------
//	
//	ViewLayoutFactory.h
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _ViewLayoutFactory_h
#define _ViewLayoutFactory_h

#include <InterfaceKit.h>
#include <SupportDefs.h>

typedef enum { CORNER_TOPLEFT, CORNER_BOTTOMLEFT,
	CORNER_TOPRIGHT, CORNER_BOTTOMRIGHT } corner;

// bit flags
typedef enum { RECT_WIDTH = 1, RECT_HEIGHT,
	RECT_WIDTH_AND_HEIGHT } rect_dim;

typedef enum { ALIGN_LEFT, ALIGN_TOP, ALIGN_RIGHT,
	ALIGN_BOTTOM, ALIGN_HCENTER, ALIGN_VCENTER } align_side;

//====================================================================
//	CLASS: ViewLayoutFactory
// ---------------------------
//  A class, still under construction, that handles some
//	simple view layout problems. Primarily, it creates
//	standard Interface Kit views that are automatically
//	set to their preferred size. In addition, it provides:
//
//	* Positioning of views using any one of the corners of
//	the view's rectangle
//	* Alignment of a group of views along one edge or center,
//	to a specified distance between views.
//	* Adjustment of the width or height of a group of views
//	to the maximum value represented by the group.
//	* Resizing of a view to surround all of its children,
//	plus a specified margin on the right and bottom sides.

class ViewLayoutFactory
{
	//----------------------------------------------------------------
	//	Factory generators
	
public:
	BButton*		MakeButton(const char* name, const char* label,
						uint32 msgID, BPoint pos,
						corner posRef = CORNER_TOPLEFT);
	BCheckBox*		MakeCheckBox(const char* name, const char* label,
						uint32 msgID, BPoint pos,
						corner posRef = CORNER_TOPLEFT);
	BTextControl*	MakeTextControl(const char* name, const char* label,
						const char* text, BPoint pos, float controlWidth,
						corner posRef = CORNER_TOPLEFT);

	void			LayoutTextControl(BTextControl& control,
						BPoint pos, float controlWidth,
						corner posRef = CORNER_TOPLEFT);
						
	//----------------------------------------------------------------
	//	Other operations
	
public:
	void			MoveViewCorner(BView& view, BPoint pos,
						corner posRef = CORNER_TOPLEFT);
	void			Align(BList& viewList, align_side side,
						float alignLen);
	void			ResizeToListMax(BList& viewList, rect_dim resizeDim,
						corner anchor = CORNER_TOPLEFT);
	void			ResizeAroundChildren(BView& view, BPoint margin);
};

#endif /* _ViewLayoutFactory_h */
