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
//	Description:	Shadow BView class
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
#include "DesktopClasses.h"
#include "TokenHandler.h"

class ServerWindow;
class PortLink;
class RootLayer;
class WinBorder;
class Screen;
class ServerCursor;

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
	Layer(BRect frame, const char *name, int32 token, int32 resize, 
			int32 flags, ServerWindow *win);
	virtual ~Layer(void);

	void AddChild(Layer *child, Layer *before=NULL, bool rebuild=true);
	void RemoveChild(Layer *child, bool rebuild=true);
	void RemoveSelf(bool rebuild=true);
	uint32 CountChildren(void);
	void MakeTopChild(void);
	void MakeBottomChild(void);
	Layer *FindLayer(int32 token);
	Layer *GetChildAt(BPoint pt, bool recursive=false);
	Layer *GetUpperSibling() { return _uppersibling; }
	Layer *GetLowerSibling() { return _lowersibling; }
	PortLink *GetLink(void);
	const char *GetName(void) { return (_name)?_name->String():NULL; }
	LayerData *GetLayerData(void) { return _layerdata; }
	
	void SetLayerCursor(ServerCursor *csr);
	ServerCursor *GetLayerCursor(void) const;
	virtual void MouseTransit(uint32 transit);

	void Invalidate(const BRect &rect);
	void Invalidate(BRegion& region);
	void RebuildRegions(bool include_children=true);
	virtual void RequestDraw(const BRect &r);
	virtual void RequestDraw(void);
	bool IsDirty(void) const;
	void UpdateIfNeeded(bool force_update=false);
	void MarkModified(BRect rect);
	void UpdateRegions(bool force=false);

	void Show(void);
	void Hide(void);
	bool IsHidden(void);

	BRect Bounds(void);
	BRect Frame(void);

			void				DoMoveTo(float x, float y);
			void				DoResizeTo(float newWidth, float newHeight);

	virtual void MoveBy(float x, float y);
	void ResizeBy(float x, float y);

	BRect ConvertToParent(BRect rect);
	BRegion ConvertToParent(BRegion *reg);
	BRect ConvertFromParent(BRect rect);
	BRegion ConvertFromParent(BRegion *reg);
	BRegion ConvertToTop(BRegion *reg);
	BRect ConvertToTop(BRect rect);
	BRegion ConvertFromTop(BRegion *reg);
	BRect ConvertFromTop(BRect rect);
	
	void PrintToStream(void);
	void PrintNode(void);
	void PruneTree(void);
	void PrintTree();
	
protected:
	friend class RootLayer;
	friend class WinBorder;
	friend class Screen;
	friend class ServerWindow;
	
	BRect		_frame;
	BPoint		_boundsLeftTop;
	Layer		*_parent,
				*_uppersibling,
				*_lowersibling,
				*_topchild,
				*_bottomchild;
	BRegion		*_visible,
				*_invalid,
				*_full;
	ServerWindow	*_serverwin;
	BString		*_name;	
	int32		_view_token;
	int32		_level;
	uint32		_flags;
	bool		_hidden;
	bool		_is_dirty;
	bool		_is_updating;
	bool		_regions_invalid;
	LayerData	*_layerdata;
	ServerCursor *_cursor;
};

#endif
/*
 @log
 	* added a new member, BPoint _boundsLeftTop. Beside other uses, (DW don't forget!)it will be needed in redraw code.
 	* _flags is now declared as uint32
*/
