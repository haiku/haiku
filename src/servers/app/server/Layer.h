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
#include "RGBColor.h"

enum
{
	B_LAYER_NONE		= 1,
	B_LAYER_MOVE		= 2,
	B_LAYER_SIMPLE_MOVE	= 3,
	B_LAYER_RESIZE		= 4,
	B_LAYER_MASK_RESIZE	= 5,
};

enum
{
	B_LAYER_CHILDREN_DEPENDANT = 0x1000U,
};

// easy way to determine class type
enum
{
	AS_LAYER_CLASS		= 1,
	AS_WINBORDER_CLASS	= 2,
	AS_ROOTLAYER_CLASS	= 3,
};

class ServerWindow;
class RootLayer;
class DisplayDriver;
class LayerData;

class Layer
{
public:
	Layer(BRect frame, const char *name, int32 token, uint32 resize, 
			uint32 flags, DisplayDriver *driver);
	virtual ~Layer(void);

	void AddChild(Layer *child, ServerWindow *serverWin);
	void RemoveChild(Layer *child);
	void RemoveSelf(void);
	bool HasChild(Layer *layer);
	RootLayer *GetRootLayer(void) const { return fRootLayer; }
	
	uint32 CountChildren(void) const;
	Layer *FindLayer(const int32 token);
	Layer *LayerAt(const BPoint &pt);
	bool IsTopLayer(void) { return fIsTopLayer; }
	
	virtual	Layer *VirtualTopChild(void) const;
	virtual	Layer *VirtualLowerSibling(void) const;
	virtual	Layer *VirtualUpperSibling(void) const;
	virtual	Layer *VirtualBottomChild(void) const;
	
	const char *GetName(void) const { return (fName)?fName->String():NULL; }
	
	void FullInvalidate(const BRect &rect);
	void FullInvalidate(const BRegion &region);
	void Invalidate(const BRegion &region);
	void RebuildAndForceRedraw( const BRegion& reg, Layer *target);
	
	virtual	void RebuildFullRegion(void);
	void StartRebuildRegions( const BRegion& reg, Layer *target, uint32 action, BPoint& pt);
	void RebuildRegions( const BRegion& reg, uint32 action, BPoint pt, BPoint ptOffset);
	uint32 ResizeOthers(float x, float y,  BPoint coords[], BPoint *ptOffset);
	
	void Redraw(const BRegion& reg, Layer *startFrom=NULL);
	
	void EmptyGlobals(void);
	
	virtual	void Draw(const BRect &r);
	
	virtual	void Show(bool invalidate=true);
	virtual	void Hide(bool invalidate=true);
	virtual bool IsHidden(void) const;
	
	BRect Bounds(void) const;
	BRect Frame(void) const;
	
	virtual	void MoveBy(float x, float y);
	virtual	void ResizeBy(float x, float y);
	
	BRect ConvertToParent(BRect rect);
	BRegion ConvertToParent(BRegion *reg);
	BRect ConvertFromParent(BRect rect);
	BRegion ConvertFromParent(BRegion *reg);
	
	BPoint ConvertToTop(BPoint pt);
	BRegion ConvertToTop(BRegion *reg);
	BRect ConvertToTop(BRect rect);

	BPoint ConvertFromTop(BPoint pt);
	BRegion ConvertFromTop(BRegion *reg);
	BRect ConvertFromTop(BRect rect);
	
	DisplayDriver *GetDisplayDriver(void) const { return fDriver; }
	ServerWindow *Window(void) const { return fServerWin; }
	virtual bool HasClient(void) { return true; }
	bool IsServerLayer() const;

	void PruneTree(void);
	
	void PrintToStream(void);
	void PrintNode(void);
	void PrintTree(void);
	
	// server "private" - should not be used
	void SetRootLayer(RootLayer *rl){ fRootLayer = rl; }
	void SetAsTopLayer(bool option) { fIsTopLayer = option; }
	bool IsTopLayer() const { return fIsTopLayer; }

	void UpdateStart();
	void UpdateEnd();
	bool InUpdate() const { return fInUpdate; }
	BRegion* ClippingRegion() const { return fClipReg; }

protected:
	friend class RootLayer;
	friend class WinBorder;
	friend class Screen;
	friend class ServerWindow;
	friend class Desktop;
	friend class Workspace;

	BRect fFrame;
	BPoint fBoundsLeftTop;
	Layer *fParent;
	Layer *fUpperSibling;
	Layer *fLowerSibling;
	Layer *fTopChild;
	Layer *fBottomChild;
	
	mutable	Layer *fCurrent;
	
	BRegion fVisible;
	BRegion	fFullVisible;
	BRegion	fFull;
	BRegion	fUpdateReg;
	BRegion *fClipReg;
	
	BRegion *clipToPicture;
	bool clipToPictureInverse;
	
	ServerWindow *fServerWin;
	BString *fName;	
	int32 fViewToken;
	int32 fLevel;
	uint32 fFlags;
	uint32 fResizeMode;
	bool fHidden;
	bool fInUpdate;
	bool fIsTopLayer;
	uint16 fAdFlags;
	int8 fClassID;
	int8 fFrameAction;
	
	DisplayDriver *fDriver;
	LayerData *fLayerData;
	//RGBColor fBackColor;
	
	RootLayer *fRootLayer;

private:
	void RequestDraw(const BRegion &reg, Layer *startFrom);
	ServerWindow *SearchForServerWindow(void);

	void Layer::SendUpdateMsg();
	void SendViewMovedMsg(void);
	void SendViewResizedMsg(void);

};

#endif
