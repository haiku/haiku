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
//	File Name:		Layer.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@cotty.iren.com>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Class used for rendering to the frame buffer. One layer per 
//					view on screen and also for window decorators
//  
//------------------------------------------------------------------------------
#ifndef _LAYER_H_
#define _LAYER_H_

#include <GraphicsDefs.h>
#include <List.h>
#include <Locker.h>
#include <OS.h>
#include <String.h>
#include <Rect.h>
#include <Region.h>

#include "RGBColor.h"
#include "ServerWindow.h"

enum {
	B_LAYER_NONE		= 1,
	B_LAYER_MOVE		= 2,
	B_LAYER_SIMPLE_MOVE	= 3,
	B_LAYER_RESIZE		= 4,
	B_LAYER_MASK_RESIZE	= 5,
};

enum {
	B_LAYER_CHILDREN_DEPENDANT = 0x1000U,
};

// easy way to determine class type
enum {
	AS_LAYER_CLASS		= 1,
	AS_WINBORDER_CLASS	= 2,
	AS_ROOTLAYER_CLASS	= 3,
};

class ServerApp;
class RootLayer;
class DisplayDriver;
class LayerData;

class Layer {
 public:
								Layer(BRect frame, const char* name,
									  int32 token, uint32 resize,
									  uint32 flags, DisplayDriver* driver);
	virtual						~Layer();

			void				AddChild(Layer* child, ServerWindow* serverWin);
			void				RemoveChild(Layer* child);
			void				RemoveSelf();
			bool				HasChild(Layer* layer);

			RootLayer*			GetRootLayer() const
									{ return fRootLayer; }
	
			uint32				CountChildren() const;
			Layer*				FindLayer(const int32 token);
			Layer*				LayerAt(const BPoint &pt);

	virtual	Layer*				VirtualTopChild() const;
	virtual	Layer*				VirtualLowerSibling() const;
	virtual	Layer*				VirtualUpperSibling() const;
	virtual	Layer*				VirtualBottomChild() const;
	
			const char*			GetName() const
									{ return (fName) ? fName->String() : NULL; }
	
	virtual	void				RebuildFullRegion();
			void				StartRebuildRegions(const BRegion& reg,
													Layer* target,
													uint32 action,
													BPoint& pt);

			void				RebuildRegions(const BRegion& reg,
											   uint32 action,
											   BPoint pt,
											   BPoint ptOffset);

			uint32				ResizeOthers(float x, float y,
											 BPoint coords[],
											 BPoint* ptOffset);
	
			void				Redraw(const BRegion& reg,
									   Layer* startFrom = NULL);
	
	virtual	void				Draw(const BRect& r);
	
			void				EmptyGlobals();
	
			void				Show(bool invalidate = true);
			void				Hide(bool invalidate = true);
			bool				IsHidden() const;

	// coordinate system	
			BRect				Bounds() const;
			BRect				Frame() const;
	
	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);

			BPoint				BoundsOrigin() const; // BoundsFrameDiff()?

			BPoint				ConvertToParent(BPoint pt);
			BRect				ConvertToParent(BRect rect);
			BRegion				ConvertToParent(BRegion* reg);

			BPoint				ConvertFromParent(BPoint pt);
			BRect				ConvertFromParent(BRect rect);
			BRegion				ConvertFromParent(BRegion* reg);
	
			BPoint				ConvertToTop(BPoint pt);
			BRect				ConvertToTop(BRect rect);
			BRegion				ConvertToTop(BRegion* reg);

			BPoint				ConvertFromTop(BPoint pt);
			BRect				ConvertFromTop(BRect rect);
			BRegion				ConvertFromTop(BRegion *reg);
	

			DisplayDriver*		GetDisplayDriver() const
									{ return fDriver; }

			ServerWindow*		Window() const
									{ return fServerWin; }

			ServerApp*			App() const
									{ return fServerWin? fServerWin->App(): NULL; }

	virtual	bool				HasClient()
									{ return true; }
//			bool				IsServerLayer() const;

			uint32				EventMask() const
									{ return fEventMask; }

			uint32				EventOptions() const
									{ return fEventOptions; }

			void				PruneTree();
	
	// debugging
			void				PrintToStream();
			void				PrintNode();
			void				PrintTree();
	
	// server "private" - should not be used
			void				SetRootLayer(RootLayer* rl)
									{ fRootLayer = rl; }

			void				SetAsTopLayer(bool option)
									{ fIsTopLayer = option; }

			bool				IsTopLayer() const
									{ return fIsTopLayer; }

			void				UpdateStart();
			void				UpdateEnd();

			BRegion*			ClippingRegion() const
									{ return fClipReg; }

 protected:
	friend class RootLayer;
	friend class WinBorder;
	friend class ServerWindow;

			void				move_layer(float x, float y);
			void				resize_layer(float x, float y);

			void				FullInvalidate(const BRect& rect);
			void				FullInvalidate(const BRegion& region);
			void				Invalidate(const BRegion& region);

			BRect				fFrame;
//			BPoint				fBoundsLeftTop;
			WinBorder*			fOwner;
			Layer*				fParent;
			Layer*				fUpperSibling;
			Layer*				fLowerSibling;
			Layer*				fTopChild;
			Layer*				fBottomChild;
	
	mutable	Layer*				fCurrent;
	
			BRegion				fVisible;
			BRegion				fFullVisible;
			BRegion				fFull;

			BRegion*			fClipReg;
	
			BRegion*			clipToPicture;
			bool				clipToPictureInverse;
	
			ServerWindow*		fServerWin;
			BString*			fName;	
			int32				fViewToken;
			uint32				fFlags;
			uint32				fResizeMode;
			uint32				fEventMask;
			uint32				fEventOptions;
			bool				fHidden;
			bool				fIsTopLayer;
			uint16				fAdFlags;
			int8				fClassID;
			int8				fFrameAction;
	
			DisplayDriver*		fDriver;
			LayerData*			fLayerData;
	
			RootLayer*			fRootLayer;

 private:
			void				RequestDraw(const BRegion& reg,
											Layer* startFrom);
			ServerWindow*		SearchForServerWindow();

			void				SendUpdateMsg(BRegion& reg);
			void				SendViewMovedMsg();
			void				SendViewResizedMsg();

};

#endif // _LAYER_H_
