//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
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
//	File Name:		RootLayer.cpp
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class used for the top layer of each workspace's Layer tree
//  
//------------------------------------------------------------------------------
#include <View.h>
#include "RootLayer.h"
#include "Desktop.h"

/*!
	\brief Sets up internal variables needed by the RootLayer
	\param rect Frame of the root layer
	\param layername Name of the root layer. Not really used.
	\param gfxdriver Pointer to the related graphics driver
*/
RootLayer::RootLayer(BRect rect, const char *layername, DisplayDriver *gfxdriver)
	: Layer(rect,layername,B_FOLLOW_NONE,0, NULL)
{
	_driver=gfxdriver;
	_invalid=new BRegion(Bounds());
	_is_dirty=true;
}

//! Frees all allocated heap memory (which happens to be none) ;)
RootLayer::~RootLayer()
{
}

/*!
	\brief Requests that the layer be drawn on screen
	\param r The bounding rectangle of the area given in the Layer's coordinates
*/
void RootLayer::RequestDraw(const BRect &r)
{
  /*
     1) call the display driver's FillRect on the rectangle, filling with the layer's background color
2) recurse through each child and call its RequestDraw() function if it intersects the child's frame
*/
}

/*!
	\brief Requests that the layer be drawn on screen
*/
void RootLayer::RequestDraw(void)
{
	if(!_is_dirty)
		return;
	
	// Redraw the base
	for(int32 i=0; _invalid->CountRects();i++)
	{
		if(_invalid->RectAt(i).IsValid())
			_driver->FillRect(_invalid->RectAt(i),_layerdata, 0LL);
		else
			break;
	}

	delete _invalid;
	_invalid=NULL;
	_is_dirty=false;

	// force redraw of all dirty windows	
	for(Layer *lay=_topchild; lay!=NULL; lay=lay->_lowersibling)
	{
		if(lay->IsDirty())
			lay->RequestDraw(lay->Bounds());
	}

}

/*!
	\brief Sets the background color of the Screen
	\param col The new background color
*/
void RootLayer::SetColor(const RGBColor &col)
{
	_layerdata->lowcolor=col;
}

/*!
	\brief Returns the background color of the Screen
	\return The background color
*/
RGBColor RootLayer::GetColor(void) const
{	
	return _layerdata->lowcolor;
}

//! Empty function to disable moving the RootLayer
void RootLayer::MoveBy(float x, float y)
{
}

//! Empty function to disable moving the RootLayer
void RootLayer::MoveBy(BPoint pt)
{
}

//! Reimplemented for RootLayer special case
void RootLayer::ResizeBy(float x, float y)
{
	BRect oldframe=_frame;
	_frame.right+=x;
	_frame.bottom+=y;

	// We'll need to rebuild the regions of the child layers
	// because resizing will affect the visible regions
	RebuildRegions(true);
	
	// If we've gotten bigger, we'll need to repaint the new areas
	if(x>0)
	{
		BRect dx(oldframe.right,oldframe.top, _frame.right, _frame.bottom);
		Invalidate(dx);
	}
	if(y>0)
	{
		BRect dy(oldframe.left,oldframe.bottom, _frame.right, _frame.bottom);
		Invalidate(dy);
	}
}

//! Reimplemented for RootLayer special case
void RootLayer::ResizeBy(BPoint pt)
{
	ResizeBy(pt.x,pt.y);
}

/*!
	\brief Assigns a particular display driver to the RootLayer
	\param d The new display driver (ignored if NULL)
*/
void RootLayer::SetDriver(DisplayDriver *driver)
{
}

/*!
	\brief Rebuilds the visible and invalid layers based on the layer hierarchy
	\param recursive (Defaults to false)
*/
void RootLayer::RebuildRegions(bool recursive)
{
  /*
     1) get the frame
2) set full and visible regions to frame
3) iterate through each child and exclude its full region from the visible region if the child is visible.
     */
}
