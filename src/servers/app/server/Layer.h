//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		Layer.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@myrealbox.com>
//	Description:	Class used for rendering to the frame buffer. One layer per 
//					view on screen and also for window decorators
//  
//------------------------------------------------------------------------------
#ifndef _LAYER_H_
#define _LAYER_H_

#include <GraphicsDefs.h>
#include <Rect.h>
#include <Region.h>
#include <List.h>
#include <String.h>
#include <OS.h>
#include <Locker.h>
#include "LayerData.h"
#include "TokenHandler.h"

class ServerWindow;
class PortLink;
class RootLayer;
class WinBorder;
class Screen;
class ServerCursor;
class DisplayDriver;
class Desktop;
class Workspace;

/*!
	\class Layer Layer.h
	\brief Shadow BView class
	
	Layers provide all sorts of functionality. They are the shadow class for BViews, 
	but they also provide the base class for other classes which handle drawing to 
	the frame buffer, like RootLayer and WinBorder.
*/
class Layer
{
public:
								Layer(BRect frame, const char *name, int32 token, uint32 resize, 
										uint32 flags, ServerWindow *win);
	virtual						~Layer(void);

			void				AddChild(Layer *child, RootLayer *rootLayer);
			void				RemoveChild(Layer *child);
			void				RemoveSelf();
			bool				HasChild(Layer* layer) const;
			RootLayer*			GetRootLayer() const;

			uint32				CountChildren(void) const;
			Layer*				FindLayer(const int32 token);
			Layer*				LayerAt(const BPoint &pt);

	virtual	Layer*				TopLayer() const { return _topchild; }
	virtual	Layer*				LowerSibling() const { return _lowersibling; }
	virtual	Layer*				UpperSibling() const { return _uppersibling; }
	virtual	Layer*				BottomLayer() const { return _bottomchild; }

	virtual	Layer*				VirtualTopChild() const{ return _topchild; }
	virtual	Layer*				VirtualLowerSibling() const{ return _lowersibling; }
	virtual	Layer*				VirtualUpperSibling() const{ return _uppersibling; }
	virtual	Layer*				VirtualBottomChild() const{ return _bottomchild; }


			const char*			GetName(void) const { return (_name)?_name->String():NULL; }
			LayerData*			GetLayerData(void) const { return _layerdata; }
	
			void				SetLayerCursor(ServerCursor *csr);
			ServerCursor*		GetLayerCursor(void) const;
	virtual	void				MouseTransit(uint32 transit);

			void				Invalidate(const BRect &rect);
			void				Invalidate(const BRegion &region);
			void				DoInvalidate(const BRegion &reg, Layer *start);
			
	virtual	void				RebuildRegions( const BRect& r );
			void				RebuildChildRegions( const BRect &r, Layer* startFrom );
			void				RebuildFullRegion(void);
			void				MoveRegionsBy(float x, float y);
			void				ResizeRegionsBy(float x, float y);
	
			void				RequestClientUpdate(const BRect &rect);
			void				RequestDraw(const BRect &r);
	virtual	void				Draw(const BRect &r);

	virtual	void				Show(void);
	virtual	void				Hide(void);
			bool				IsHidden(void) const;

			BRect				Bounds(void) const;
			BRect				Frame(void) const;

	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);

			BRect				ConvertToParent(BRect rect);
			BRegion				ConvertToParent(BRegion *reg);
			BRect				ConvertFromParent(BRect rect);
			BRegion				ConvertFromParent(BRegion *reg);
			BRegion				ConvertToTop(BRegion *reg);
			BRect				ConvertToTop(BRect rect);
			BRegion				ConvertFromTop(BRegion *reg);
			BRect				ConvertFromTop(BRect rect);

			void				PruneTree(void);

			void				PrintToStream(void);
			void				PrintNode(void);
			void				PrintTree();

	// server "private" - should not be used
			void				SetRootLayer(RootLayer* rl);
protected:
	friend class RootLayer;
	friend class WinBorder;
	friend class Screen;
	friend class ServerWindow;
	friend class Desktop;
	friend class Workspace;
	
			BRect				_frame;
			BPoint				_boundsLeftTop;
			Layer				*_parent,
								*_uppersibling,
								*_lowersibling,
								*_topchild,
								*_bottomchild;

			BRegion				_visible,
								_fullVisible,
								_full;

			BRegion				*clipToPicture;
			bool				clipToPictureInverse;

			ServerWindow		*_serverwin;
			BString				*_name;	
			int32				_view_token;
			int32				_level;
			uint32				_flags;
			uint32				_resize_mode;
			bool				_hidden;
			bool				_is_updating;
			LayerData			*_layerdata;
			ServerCursor		*_cursor;
			DisplayDriver*		fDriver;
			RootLayer*			fRootLayer;
};

#endif
/*
 @log
 	* added a new member, BPoint _boundsLeftTop. Beside other uses, (DW don't forget!)it will be needed in redraw code.
 	* _flags is now declared as uint32
*/
