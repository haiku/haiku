/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
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
	B_LAYER_ACTION_NONE = 0,
	B_LAYER_ACTION_MOVE,
	B_LAYER_ACTION_RESIZE
};

enum {
	B_LAYER_CHILDREN_DEPENDANT = 0x1000U,
};

class ServerApp;
class RootLayer;
class DrawingEngine;
class DrawState;
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
									  uint32 flags, DrawingEngine* driver);
	virtual						~Layer();

	// name
	virtual	void				SetName(const char* name);	
	inline	const char*			Name() const
									{ return fName.String(); }

	// children handling
			void				AddChild(Layer* child, ServerWindow* serverWin);
			void				RemoveChild(Layer* child);
			void				RemoveSelf();
			bool				HasChild(Layer* layer);
			Layer*				Parent() const
									{ return fParent; }

			uint32				CountChildren() const;
			Layer*				FindLayer(const int32 token);
			Layer*				LayerAt(const BPoint &pt, bool recursive = true);

	virtual	Layer*				FirstChild() const;
	virtual	Layer*				NextChild() const;
	virtual	Layer*				PreviousChild() const;
	virtual	Layer*				LastChild() const;

			void				SetAsTopLayer(bool option)
									{ fIsTopLayer = option; }
	inline	bool				IsTopLayer() const
									{ return fIsTopLayer; }

	// visible state	
			void				Show(bool invalidate = true);
			void				Hide(bool invalidate = true);
			bool				IsHidden() const;
			bool				IsVisuallyHidden() const;
			
	// graphic state and attributes
			void				PushState();
			void				PopState();
			DrawState*			CurrentState() const { return fDrawState; }

			void				SetViewColor(const RGBColor& color);
	inline	const RGBColor&		ViewColor() const
									{ return fViewColor; }
			void				SetBackgroundBitmap(const ServerBitmap* bitmap);
	inline	const ServerBitmap*	BackgroundBitmap() const
									{ return fBackgroundBitmap; }
			void				SetOverlayBitmap(const ServerBitmap* bitmap);
	inline	const ServerBitmap*	OverlayBitmap() const
									{ return fOverlayBitmap; }

	// coordinate system	
			BRect				Bounds() const;
			BRect				Frame() const;
			BPoint				BoundsOrigin() const; // BoundsFrameDiff()?
			float				Scale() const;

			void				ConvertToParent(BPoint* pt) const;
			void				ConvertToParent(BRect* rect) const;
			void				ConvertToParent(BRegion* reg) const;
			void				ConvertFromParent(BPoint* pt) const;
			void				ConvertFromParent(BRect* rect) const;
			void				ConvertFromParent(BRegion* reg) const;

			void				ConvertToScreen(BPoint* pt) const;
			void				ConvertToScreen(BRect* rect) const;
			void				ConvertToScreen(BRegion* reg) const;
			void				ConvertFromScreen(BPoint* pt) const;
			void				ConvertFromScreen(BRect* rect) const;
			void				ConvertFromScreen(BRegion* reg) const;

	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);
	virtual	void				ScrollBy(float x, float y);
			void				CopyBits(BRect& src, BRect& dst,
										 int32 xOffset, int32 yOffset);

	// input handling
	virtual	void				MouseDown(const BMessage *msg);
	virtual	void				MouseUp(const BMessage *msg);
	virtual	void				MouseMoved(const BMessage *msg);
	virtual	void				MouseWheelChanged(const BMessage *msg);

	virtual	void				KeyDown(const BMessage *msg);
	virtual	void				KeyUp(const BMessage *msg);
	virtual	void				UnmappedKeyDown(const BMessage *msg);
	virtual	void				UnmappedKeyUp(const BMessage *msg);
	virtual	void				ModifiersChanged(const BMessage *msg);

	virtual	void				WorkspaceActivated(int32 index, bool active);
	virtual	void				WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces);
	virtual	void				Activated(bool active);

	// action hooks
	virtual	void				MovedByHook(float dx, float dy);
	virtual	void				ResizedByHook(float dx, float dy, bool automatic);
	virtual	void				ScrolledByHook(float dx, float dy);

	// app_server objects getters
			ServerWindow*		Window() const
									{ return fServerWin; }
			ServerApp*			App() const
									{ return fServerWin? fServerWin->App(): NULL; }
	inline	WinBorder*			Owner() const
									{ return fOwner; }
			RootLayer*			GetRootLayer() const
									{ return fRootLayer; }
			void				SetRootLayer(RootLayer* rl)
									{ fRootLayer = rl; }
			DrawingEngine*		GetDrawingEngine() const
									{ return fDriver; }

	// flags
			uint32				EventMask() const
									{ return fEventMask; }
			uint32				EventOptions() const
									{ return fEventOptions; }
	inline	void				QuietlySetEventMask(uint32 em)
									{ fEventMask = em; }
	inline	void				QuietlySetEventOptions(uint32 eo)
									{ fEventOptions = eo; }
	inline	uint32				ResizeMode() const
									{ return fResizeMode; }
	inline	uint32				Flags() const
									{ return fFlags; }
			void				SetFlags(uint32 flags);

	// clipping stuff and redraw
			void				SetUserClipping(const BRegion& region);
				// region is expected in layer coordinates

	inline	const BRegion&		VisibleRegion() const { return fVisible; }
	inline	const BRegion&		FullVisible() const { return fFullVisible; }
	inline	const BRegion&		DrawingRegion() const { return fDrawingRegion; }

	virtual	void				GetOnScreenRegion(BRegion& region);

			void				MarkForRebuild(const BRegion &dirty);
			void				TriggerRebuild();
			void				_GetAllRebuildDirty(BRegion *totalReg);

	virtual	void				Draw(const BRect& r);
			void				_AllRedraw(const BRegion &invalid);

 private:
	friend class RootLayer;
	friend class WinBorder;
	friend class ServerWindow;

 			void				do_Hide();
 			void				do_Show();
			void				do_CopyBits(BRect& src, BRect& dst,
											int32 xOffset, int32 yOffset);

	// private clipping stuff
	virtual	void				_ReserveRegions(BRegion &reg);
			void				_RebuildVisibleRegions( const BRegion &invalid,
														const BRegion &parentLocalVisible,
														const Layer *startFrom);
			void				_RebuildDrawingRegion();
			void				_ClearVisibleRegions();
			void				_ResizeLayerFrameBy(float x, float y);
			void				_RezizeLayerRedrawMore(BRegion &reg, float dx, float dy);
			void				_ResizeLayerFullUpdateOnResize(BRegion &reg, float dx, float dy);

	// for updating client-side coordinates
			void				AddToViewsWithInvalidCoords() const;
			void				SendViewCoordUpdateMsg() const;


			BString				fName;	
			BRect				fFrame;
// TODO: should be removed or reused in a similar fashion
// to hold the accumulated origins from the graphics state stack.
// The same needs to be done for "scale". (Keeping an accumulated
// value.)
//			BPoint				fBoundsLeftTop;

			BRegion				fVisible;
			BRegion				fFullVisible;
			BRegion				fDirtyForRebuild;
			BRegion				fDrawingRegion;

			DrawingEngine*		fDriver;
			RootLayer*			fRootLayer;
			ServerWindow*		fServerWin;
			WinBorder*			fOwner;

			DrawState*			fDrawState;

			Layer*				fParent;
			Layer*				fPreviousSibling;
			Layer*				fNextSibling;
			Layer*				fFirstChild;
			Layer*				fLastChild;
	
	mutable	Layer*				fCurrent;

			int32				fViewToken;
			uint32				fFlags;
			uint16				fAdFlags;
			uint32				fResizeMode;
			uint32				fEventMask;
			uint32				fEventOptions;

			bool				fHidden;
			bool				fIsTopLayer;
			RGBColor			fViewColor;

	const	ServerBitmap*		fBackgroundBitmap;
	const	ServerBitmap*		fOverlayBitmap;

};

#endif // _LAYER_H_
