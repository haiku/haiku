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
#include "PatternHandler.h" // for pattern_union
#include "ServerConfig.h"


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
	_bgcolor=new RGBColor();
}

//! Frees all allocated heap memory (which happens to be none) ;)
RootLayer::~RootLayer()
{
	delete _bgcolor;
}

/*!
	\brief Requests that the layer be drawn on screen
	\param r The bounding rectangle of the area given in the Layer's coordinates
*/
void RootLayer::RequestDraw(const BRect &r)
{
	Invalidate(r);
	RequestDraw();
}

/*!
	\brief Requests that the layer be drawn on screen
*/
void RootLayer::RequestDraw(void)
{
	if(!_is_dirty)
		return;
	
	// Redraw the base
	if(_invalid)
	{
		for(int32 i=0; _invalid->CountRects();i++)
		{
			if(_invalid->RectAt(i).IsValid())
				_driver->FillRect(_invalid->RectAt(i),_layerdata, pat_solidlow);
			else
				break;
		}
	
		delete _invalid;
		_invalid=NULL;
	}

	// force redraw of all dirty windows	
	for(Layer *lay=_topchild; lay!=NULL; lay=lay->_lowersibling)
	{
		if(lay->IsDirty())
			lay->RequestDraw(lay->Bounds());
	}

	_is_dirty=false;
#ifdef DISPLAYDRIVER_TEST_HACK
	int8 pattern[8];
	int8 pattern2[8];
	memset(pattern,255,8);
	memset(pattern2,128+64+32+16,8);
	BRect r1(100,100,1500,1100);
	BPoint pts[4];
	pts[0].x = 200;
	pts[0].y = 200;
	pts[1].x = 400;
	pts[1].y = 1000;
	pts[2].x = 600;
	pts[2].y = 400;
	pts[3].x = 1200;
	pts[3].y = 800;
	BPoint triangle[3];
	BRect triangleRect(100,100,400,300);
	triangle[0].x = 100;
	triangle[0].y = 100;
	triangle[1].x = 100;
	triangle[1].y = 300;
	triangle[2].x = 400;
	triangle[2].y = 300;
	BPoint polygon[6];
	BRect polygonRect(100,100,300,400);
	polygon[0].x = 100;
	polygon[0].y = 100;
	polygon[1].x = 100;
	polygon[1].y = 400;
	polygon[2].x = 200;
	polygon[2].y = 300;
	polygon[3].x = 300;
	polygon[3].y = 400;
	polygon[4].x = 300;
	polygon[4].y = 100;
	polygon[5].x = 200;
	polygon[5].y = 200;

	_layerdata->highcolor.SetColor(255,0,0,255);
	_layerdata->lowcolor.SetColor(255,255,255,255);
	_driver->FillRect(r1,_layerdata,pattern);

	_layerdata->highcolor.SetColor(255,255,0,255);
	_driver->StrokeLine(BPoint(100,100),BPoint(1500,1100),_layerdata,pattern);

	_layerdata->highcolor.SetColor(0,0,255,255);
	_driver->StrokeBezier(pts,_layerdata,pattern);
	_driver->StrokeArc(BRect(200,300,400,600),30,270,_layerdata,pattern);
	_driver->StrokeEllipse(BRect(200,700,400,900),_layerdata,pattern);
	_driver->StrokeRect(BRect(650,1000,750,1090),_layerdata,pattern);
	_driver->StrokeRoundRect(BRect(200,1000,600,1090),30,40,_layerdata,pattern);
//	_driver->StrokePolygon(polygon,6,polygonRect,_layerdata,pattern);
//	_driver->StrokeTriangle(triangle,triangleRect,_layerdata,pattern);
	_layerdata->highcolor.SetColor(255,0,255,255);
	_driver->FillArc(BRect(1250,300,1450,600),30,270,_layerdata,pattern);
//	_driver->FillBezier(pts,_layerdata,pattern);
	_driver->FillEllipse(BRect(800,300,1200,600),_layerdata,pattern);
	_driver->FillRoundRect(BRect(800,1000,1200,1090),30,40,_layerdata,pattern2);
	_driver->FillPolygon(polygon,6,polygonRect,_layerdata,pattern);
//	_driver->FillTriangle(triangle,triangleRect,_layerdata,pattern);
#endif
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
	_driver=driver;
}

/*!
	\brief Rebuilds the visible and invalid layers based on the layer hierarchy
	\param recursive (Defaults to false)
*/
void RootLayer::RebuildRegions(bool recursive)
{
	// Unlike the other layers, RootLayers can't depend on their parent layer to reset its
	// visible region, so we have to do it itself.
	_visible->MakeEmpty();
	_visible->Include(_full);

	Layer::RebuildRegions(recursive);
}
