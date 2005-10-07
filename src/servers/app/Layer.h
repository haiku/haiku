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

//#define NEW_CLIPPING 1
#define NEW_INPUT_HANDLING 1

enum {
	B_LAYER_NONE		= 1,
	B_LAYER_MOVE		= 2,
	B_LAYER_SIMPLE_MOVE	= 3,
	B_LAYER_RESIZE		= 4,
	B_LAYER_MASK_RESIZE	= 5,
};

enum {
	B_LAYER_ACTION_NONE = 0,
	B_LAYER_ACTION_MOVE,
	B_LAYER_ACTION_RESIZE
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
class ServerBitmap;

class PointerEvent {
 public:
	int32		code;		// B_MOUSE_UP, B_MOUSE_DOWN, B_MOUSE_MOVED,
							// B_MOUSE_WHEEL_CHANGED
	bigtime_t	when;
	BPoint		where;
	float		wheel_delta_x;
	float		wheel_delta_y;
	int32		modifiers;
	int32		buttons;	// B_PRIMARY_MOUSE_BUTTON, B_SECONDARY_MOUSE_BUTTON
							// B_TERTIARY_MOUSE_BUTTON
	int32		clicks;
};

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

			void				SetRootLayer(RootLayer* rl)
									{ fRootLayer = rl; }
			RootLayer*			GetRootLayer() const
									{ return fRootLayer; }
	
			uint32				CountChildren() const;
			Layer*				FindLayer(const int32 token);
			Layer*				LayerAt(const BPoint &pt, bool recursive = true);

	virtual	Layer*				FirstChild() const;
	virtual	Layer*				NextChild() const;
	virtual	Layer*				PreviousChild() const;
	virtual	Layer*				LastChild() const;

	virtual	void				SetName(const char* name);	
	inline	const char*			Name() const
									{ return fName.String(); }

	inline	uint32				ResizeMode() const
									{ return fResizeMode; }

	virtual	void				SetFlags(uint32 flags);
	inline	uint32				Flags() const
									{ return fFlags; }

#ifndef NEW_CLIPPING
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

			void				EmptyGlobals();

#endif
			void				Redraw(const BRegion& reg,
									   Layer* startFrom = NULL);
	
	virtual	void				Draw(const BRect& r);
	
			void				Show(bool invalidate = true);
			void				Hide(bool invalidate = true);
			bool				IsHidden() const;
			
	// graphic state
			void				PushState();
			void				PopState();
			
	// coordinate system	
			BRect				Bounds() const;
			BRect				Frame() const;
	
	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);
	virtual	void				ScrollBy(float x, float y);
#ifdef NEW_INPUT_HANDLING
	virtual	void				MouseDown(const PointerEvent& evt);
	virtual	void				MouseUp(const PointerEvent& evt);
	virtual	void				MouseMoved(const PointerEvent& evt, uint32 transit);
	virtual	void				WorkspaceActivated(int32 index, bool active);
	virtual	void				WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces);
#endif
			BPoint				BoundsOrigin() const; // BoundsFrameDiff()?
			float				Scale() const;

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

	inline	WinBorder*			Owner() const
									{ return fOwner; }

	virtual	bool				HasClient()
									{ return true; }

			uint32				EventMask() const
									{ return fEventMask; }

			uint32				EventOptions() const
									{ return fEventOptions; }

	inline	void				QuietlySetEventMask(uint32 em)
									{ fEventMask = em; }

	inline	void				QuietlySetEventOptions(uint32 eo)
									{ fEventOptions = eo; }

			void				PruneTree();
	
	// debugging
			void				PrintToStream();
			void				PrintNode();
			void				PrintTree();
	
	// server "private" - should not be used
			void				SetAsTopLayer(bool option)
									{ fIsTopLayer = option; }

	inline	bool				IsTopLayer() const
									{ return fIsTopLayer; }

			BRegion*			ClippingRegion() const
									{ return fClipReg; }

								// automatic background blanking by app_server
			void				SetViewColor(const RGBColor& color);
	inline	const RGBColor&		ViewColor() const
									{ return fViewColor; }

			void				SetBackgroundBitmap(const ServerBitmap* bitmap);
	inline	const ServerBitmap*	BackgroundBitmap() const
									{ return fBackgroundBitmap; }

								// overlay support
								// TODO: This can't be all, what about color key?
			void				SetOverlayBitmap(const ServerBitmap* bitmap);
	inline	const ServerBitmap*	OverlayBitmap() const
									{ return fOverlayBitmap; }

			void				CopyBits(BRect& src, BRect& dst,
										 int32 xOffset, int32 yOffset);
#ifndef NEW_CLIPPING
	inline	const BRegion&		VisibleRegion() const { return fVisible; }
	inline	const BRegion&		FullVisible() const { return fFullVisible; }

#else
	inline	const BRegion&		VisibleRegion() const { return fVisible2; }
	inline	const BRegion&		FullVisible() const { return fFullVisible2; }

	virtual	void				GetWantedRegion(BRegion& reg) const;

	virtual	void				MovedByHook(float dx, float dy);
	virtual	void				ResizedByHook(float dx, float dy, bool automatic);
	virtual	void				ScrolledByHook(float dx, float dy);

			void				ConvertToParent2(BPoint* pt) const;
			void				ConvertToParent2(BRect* rect) const;
			void				ConvertToParent2(BRegion* reg) const;
			void				ConvertFromParent2(BPoint* pt) const;
			void				ConvertFromParent2(BRect* rect) const;
			void				ConvertFromParent2(BRegion* reg) const;

			void				ConvertToScreen2(BPoint* pt) const;
			void				ConvertToScreen2(BRect* rect) const;
			void				ConvertToScreen2(BRegion* reg) const;
			void				ConvertFromScreen2(BPoint* pt) const;
			void				ConvertFromScreen2(BRect* rect) const;
			void				ConvertFromScreen2(BRegion* reg) const;
			bool				IsVisuallyHidden() const;
 private:
 			void				do_Hide();
 			void				do_Show();
			void				do_RebuildVisibleRegions(const BRegion &invalid,
														const Layer *startFrom);
			void				do_MoveBy(float dx, float dy);
			void				do_ResizeBy(float dx, float dy);
			void				do_ScrollBy(float dx, float dy);

			void				do_Invalidate(	const BRegion &invalid,
												const Layer *startFrom = NULL);

			void				do_Redraw(	const BRegion &invalid,
											const Layer *startFrom = NULL);

			void				rebuild_visible_regions(const BRegion &invalid,
													const BRegion &parentLocalVisible,
													const Layer *startFrom);

	virtual	void				_ReserveRegions(BRegion &reg);
	virtual	void				_GetWantedRegion(BRegion &reg);

			void				clear_visible_regions();
			void				resize_layer_frame_by(float x, float y);
			void				rezize_layer_redraw_more(BRegion &reg, float dx, float dy);
			void				resize_layer_full_update_on_resize(BRegion &reg, float dx, float dy);

#endif
 private:
			void				do_CopyBits(BRect& src, BRect& dst,
											int32 xOffset, int32 yOffset);

 protected:
	friend class RootLayer;
	friend class WinBorder;
	friend class ServerWindow;
// TODO: remove, is here for debugging purposes only
friend class OffscreenWinBorder;

#ifndef NEW_CLIPPING
			void				move_layer(float x, float y);
			void				resize_layer(float x, float y);

			void				FullInvalidate(const BRect& rect);
			void				FullInvalidate(const BRegion& region);
			void				Invalidate(const BRegion& region);
#endif

			BRect				fFrame;
// TODO: should be removed or reused in a similar fashion
// to hold the accumulated origins from the graphics state stack.
// The same needs to be done for "scale". (Keeping an accumulated
// value.)
//			BPoint				fBoundsLeftTop;
			WinBorder*			fOwner;

			Layer*				fParent;
			Layer*				fPreviousSibling;
			Layer*				fNextSibling;
			Layer*				fFirstChild;
			Layer*				fLastChild;
	
	mutable	Layer*				fCurrent;

#ifndef NEW_CLIPPING	
			BRegion				fVisible;
			BRegion				fFullVisible;
			BRegion				fFull;
			int8				fFrameAction;
#else
 private:
			BRegion				fVisible2;
			BRegion				fFullVisible2;
 protected:
#endif
			BRegion*			fClipReg;
	
			ServerWindow*		fServerWin;
			BString				fName;	
			int32				fViewToken;
			uint32				fFlags;
			uint32				fResizeMode;
			uint32				fEventMask;
			uint32				fEventOptions;
			bool				fHidden;
			bool				fIsTopLayer;
			uint16				fAdFlags;
			int8				fClassID;
	
			DisplayDriver*		fDriver;
			LayerData*			fLayerData;
	
			RootLayer*			fRootLayer;

 private:
			void				RequestDraw(const BRegion& reg,
											Layer* startFrom);
			ServerWindow*		SearchForServerWindow();

			void				SendUpdateMsg(BRegion& reg);
			void				AddToViewsWithInvalidCoords() const;
			void				SendViewCoordUpdateMsg() const;

			RGBColor			fViewColor;

	const	ServerBitmap*		fBackgroundBitmap;
	const	ServerBitmap*		fOverlayBitmap;

};

#endif // _LAYER_H_
