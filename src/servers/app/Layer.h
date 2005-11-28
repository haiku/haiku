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


#include "RGBColor.h"
#include "ServerWindow.h"

#include <GraphicsDefs.h>
#include <List.h>
#include <Locker.h>
#include <OS.h>
#include <String.h>
#include <Rect.h>
#include <Region.h>


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

			int32				ViewToken() const { return fViewToken; }

	// children handling
			void				AddChild(Layer* child, ServerWindow* serverWin);
			void				RemoveChild(Layer* child);
			void				RemoveSelf();
			bool				HasChild(Layer* layer);
			Layer*				Parent() const
									{ return fParent; }

			uint32				CountChildren() const;
			Layer*				LayerAt(BPoint where);

			Layer*				FirstChild() const;
			Layer*				LastChild() const;
			Layer*				NextLayer() const;
			Layer*				PreviousLayer() const;

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
			BPoint				ScrollingOffset() const;

			void				SetDrawingOrigin(BPoint origin);
			BPoint				DrawingOrigin() const;

			void				SetScale(float scale);
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

			void				ConvertToScreenForDrawing(BPoint* pt) const;
			void				ConvertToScreenForDrawing(BRect* rect) const;
			void				ConvertToScreenForDrawing(BRegion* reg) const;

			void				ConvertFromScreenForDrawing(BPoint* pt) const;

	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);
	virtual	void				ScrollBy(float x, float y);
			void				CopyBits(BRect& src, BRect& dst,
										 int32 xOffset, int32 yOffset);

	// input handling
	virtual	void				MouseDown(BMessage *msg, BPoint where);
	virtual	void				MouseUp(BMessage *msg, BPoint where);
	virtual	void				MouseMoved(BMessage *msg, BPoint where);
	virtual	void				MouseWheelChanged(BMessage *msg, BPoint where);

	virtual	void				WorkspaceActivated(int32 index, bool active);
	virtual	void				WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces);
	virtual	void				Activated(bool active);

	// action hooks
	virtual	void				MovedByHook(float dx, float dy);
	virtual	void				ResizedByHook(float dx, float dy, bool automatic);
	virtual	void				ScrolledByHook(float dx, float dy);

	// app_server objects getters
			ServerWindow*		Window() const
									{ return fWindow; }
			ServerApp*			App() const
									{ return fWindow ? fWindow->App() : NULL; }
			RootLayer*			GetRootLayer() const
									{ return fRootLayer; }
			void				SetRootLayer(RootLayer* rl)
									{ fRootLayer = rl; }
			DrawingEngine*		GetDrawingEngine() const
									{ return fDriver; }

			void				SetEventMask(uint32 eventMask, uint32 options);
			uint32				EventMask() const
									{ return fEventMask; }
			uint32				EventOptions() const
									{ return fEventOptions; }

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

	virtual	void				Draw(const BRect& updateRect);
	virtual void				_AllRedraw(const BRegion& invalid);

 protected:
 	friend class RootLayer;
	friend class ServerWindow;

			bool				_AddChildToList(Layer* child, Layer* before = NULL);
			void				_RemoveChildFromList(Layer* child);

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
			void				_AddToViewsWithInvalidCoords() const;
			void				_SendViewCoordUpdateMsg() const;


			BString				fName;	
			BRect				fFrame;
			BPoint				fScrollingOffset;

			BRegion				fVisible;
			BRegion				fFullVisible;
			BRegion				fDirtyForRebuild;
			BRegion				fDrawingRegion;

			DrawingEngine*		fDriver;
			RootLayer*			fRootLayer;
			ServerWindow*		fWindow;

			DrawState*			fDrawState;

			Layer*				fParent;
			Layer*				fPreviousSibling;
			Layer*				fNextSibling;
			Layer*				fFirstChild;
			Layer*				fLastChild;

			int32				fViewToken;
			uint32				fFlags;
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
