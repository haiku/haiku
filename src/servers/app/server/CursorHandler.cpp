//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		CursorHandler.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	class for handling cursor display for DisplayDriver
//  
//------------------------------------------------------------------------------
#include "CursorHandler.h"
#include "DisplayDriver.h"
#include "RectUtils.h"

CursorHandler::CursorHandler(DisplayDriver *driver)
 : 	fDriver(driver),
	fOldPosition(0,0,0,0),
	fPosition(0,0,0,0),
	fCursorPos(0,0),
	fSavedData(NULL),
	fCursor(NULL),
	fHideLevel(0),
	fDriverHidden(false),
	fIsObscured(false),
	fValidSaveData(false)
{
}

//! Frees any memory used for saving screen data
CursorHandler::~CursorHandler(void)
{
	delete fSavedData;
}

/*!
	\brief Tests to see if the passed rectangle intersects the cursor's current position
	\param r The rectangle to test against
	\return true if r intersects the cursor's current screen footprint
*/
bool CursorHandler::IntersectsCursor(const BRect &r)
{
	return TestRectIntersection(r, fPosition);
}

void CursorHandler::SetCursor(ServerCursor *cursor)
{
	// we may start empty, but we will never need to stay that way
	if(!cursor)
		return;
	
	Hide();
	
	if(fSavedData && cursor->Bounds()!=fSavedData->Bounds())
	{
		delete fSavedData;
		fSavedData=NULL;
	}
	
	if(!fSavedData)
		fSavedData=new UtilityBitmap(cursor);
	
	fCursor=cursor;
	
	// We change the position rectangles to reflect the new cursor, compensating for the
	// new cursor's hotspot.
	fPosition=cursor->Bounds().OffsetToCopy(fCursorPos.x-fCursor->GetHotSpot().x,
		fCursorPos.y-fCursor->GetHotSpot().y);
	fOldPosition=fPosition;
	
	Show();
}

/*!
	\brief Moves the cursor to the specified place on screen
	\param pt The new cursor location
	
	This function fails if asked to move the cursor outside the screen boundaries
*/
void CursorHandler::MoveTo(const BPoint &pt)
{
	if(pt.x<0 || pt.y<0 || (pt.x>fDriver->fDisplayMode.virtual_width-1) ||
			(pt.y>fDriver->fDisplayMode.virtual_height-1))
		return;
	
	fCursorPos=pt;
	fDriver->CopyBitmap(fSavedData,fSavedData->Bounds(),fOldPosition,&(fDriver->fDrawData));
	fPosition.OffsetTo(fCursorPos.x-fCursor->GetHotSpot().x,
			fCursorPos.y-fCursor->GetHotSpot().y);
	fDriver->CopyToBitmap(fSavedData,fPosition);
	
	fDriver->fDrawData.draw_mode=B_OP_ALPHA;
	fDriver->CopyBitmap(fCursor,fCursor->Bounds(),fPosition,&(fDriver->fDrawData));
	fDriver->fDrawData.draw_mode=B_OP_COPY;
	
	fDriver->Invalidate(fOldPosition);
	fOldPosition=fPosition;
	
	fDriver->Invalidate(fPosition);
}

//! Shows the cursor. Unlike BView, calls are not cumulative
void CursorHandler::Show(void)
{
	// This call is pretty simple -- save the area underneath the cursor and draw the thing
	
	if(fHideLevel==0)
		return;
	
	fIsObscured=false;
	fHideLevel--;
	
	if(fHideLevel>0)
		return;
	
	fOldPosition=fPosition;
	
	if(!fCursor)
		return;
	
	fValidSaveData=true;
	
	if(!fSavedData)
		fSavedData=new UtilityBitmap(fCursor->Bounds(),(color_space)fDriver->fDisplayMode.space,0);
	
	fDriver->CopyToBitmap(fSavedData,fPosition);
	
	// Draw the data to the buffer
	fDriver->fDrawData.draw_mode=B_OP_ALPHA;
	fDriver->CopyBitmap(fCursor,fCursor->Bounds(),fPosition,&(fDriver->fDrawData));
	fDriver->fDrawData.draw_mode=B_OP_COPY;
	fDriver->Invalidate(fPosition);
}

//! Hides the cursor. Unlike BView, calls are not cumulative
void CursorHandler::Hide(void)
{
	fHideLevel++;
	
	// TODO: find out what the behavior is if a Hide is called on an obscured cursor
	
	// For now, we will change the state from obscured to hidden and leave it at that
	if(fIsObscured)
	{
		fIsObscured=false;
		return;
	}
	
	if(fHideLevel>1)
		return;
	
	if(!fCursor)
		return;
	
	if(!fSavedData)
		fSavedData=new UtilityBitmap(fCursor);
	
	fDriver->CopyBitmap(fSavedData,fSavedData->Bounds(),fOldPosition,&(fDriver->fDrawData));
	fDriver->Invalidate(fPosition);
}

//! Hides the cursor until the next MoveTo call. Unlike BView, calls are not cumulative
void CursorHandler::Obscure(void)
{
	if(IsObscured())
		return;
	
	fIsObscured=true;
	
	// TODO: find out what the behavior is if an Obscure is called on a hidden cursor
	
	// For now, we will change the state from obscured to hidden and leave it at that
	if(fHideLevel>0)
	{
		fHideLevel=0;
		return;
	}
	
	fDriver->CopyBitmap(fSavedData,fSavedData->Bounds(),fOldPosition,&(fDriver->fDrawData));
	fDriver->Invalidate(fPosition);
}

void CursorHandler::DriverHide(void)
{
	if(fDriverHidden)
		return;
		
	fDriverHidden=true;
	
	if(!fCursor)
		return;
	
	if(!fSavedData)
		fSavedData=new UtilityBitmap(fCursor);
	
	fDriver->CopyBitmap(fSavedData,fSavedData->Bounds(),fOldPosition,&(fDriver->fDrawData));
	fDriver->Invalidate(fPosition);
}

void CursorHandler::DriverShow(void)
{
	if(!fDriverHidden)
		return;
	
	fDriverHidden=false;
	
	fOldPosition=fPosition;
	
	if(!fCursor)
		return;
	
	fValidSaveData=true;
	
	if(!fSavedData)
		fSavedData=new UtilityBitmap(fCursor->Bounds(),(color_space)fDriver->fDisplayMode.space,0);
	
	fDriver->CopyToBitmap(fSavedData,fPosition);
	
	// Draw the data to the buffer
	fDriver->fDrawData.draw_mode=B_OP_ALPHA;
	fDriver->CopyBitmap(fCursor,fCursor->Bounds(),fPosition,&(fDriver->fDrawData));
	fDriver->fDrawData.draw_mode=B_OP_COPY;
	fDriver->Invalidate(fPosition);
}
