/*
 * Copyright (c) 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef	VIEW_LAYER_H
#define VIEW_LAYER_H


#include "RGBColor.h"
#include "IntRect.h"

#include <GraphicsDefs.h>
#include <Region.h>
#include <String.h>

class BList;

namespace BPrivate {
	class PortLink;
};

class DrawState;
class DrawingEngine;
class Overlay;
class WindowLayer;
class ServerBitmap;
class ServerCursor;
class ServerPicture;

class ViewLayer {
 public:
							ViewLayer(IntRect frame, IntPoint scrollingOffset,
								const char* name, int32 token, uint32 resizeMode,
								uint32 flags);

	virtual					~ViewLayer();

			int32			Token() const
								{ return fToken; }

			IntRect			Frame() const
								{ return fFrame; }
			IntRect			Bounds() const;

			void			SetResizeMode(uint32 resizeMode)
								{ fResizeMode = resizeMode; }

			void			SetName(const char* string);
			const char*		Name() const
								{ return fName.String(); }

			void			SetFlags(uint32 flags);
			uint32			Flags() const
								{ return fFlags; }

	inline	IntPoint		ScrollingOffset() const
								{ return fScrollingOffset; }

			void			SetDrawingOrigin(BPoint origin);
			BPoint			DrawingOrigin() const;

			void			SetScale(float scale);
			float			Scale() const;

			void			SetUserClipping(const BRegion* region);
				// region is expected in layer coordinates

			// converts the given frame up the view hierarchy and
			// clips to each views bounds
			void			ConvertToVisibleInTopView(IntRect* bounds) const;

			void			AttachedToWindow(WindowLayer* window);
			void			DetachedFromWindow();
			WindowLayer*	Window() const { return fWindow; }

			// tree stuff
			void			AddChild(ViewLayer* layer);
			bool			RemoveChild(ViewLayer* layer);

	inline	ViewLayer*		Parent() const
								{ return fParent; }

	inline	ViewLayer*		FirstChild() const
								{ return fFirstChild; }
	inline	ViewLayer*		LastChild() const
								{ return fLastChild; }
	inline	ViewLayer*		PreviousSibling() const
								{ return fPreviousSibling; }
	inline	ViewLayer*		NextSibling() const
								{ return fNextSibling; }

			ViewLayer*		TopLayer();

			uint32			CountChildren(bool deep = false) const;
			void			CollectTokensForChildren(BList* tokenMap) const;

			ViewLayer*		ViewAt(const BPoint& where,
								   BRegion* windowContentClipping);

			// coordinate conversion
			void			ConvertToParent(BPoint* point) const;
			void			ConvertToParent(IntPoint* point) const;
			void			ConvertToParent(BRect* rect) const;
			void			ConvertToParent(IntRect* rect) const;
			void			ConvertToParent(BRegion* region) const; 

			void			ConvertFromParent(BPoint* point) const;
			void			ConvertFromParent(IntPoint* point) const;
			void			ConvertFromParent(BRect* rect) const;
			void			ConvertFromParent(IntRect* rect) const;
			void			ConvertFromParent(BRegion* region) const; 

			void			ConvertToScreen(BPoint* point) const;
			void			ConvertToScreen(IntPoint* point) const;
			void			ConvertToScreen(BRect* rect) const;
			void			ConvertToScreen(IntRect* rect) const;
			void			ConvertToScreen(BRegion* region) const;

			void			ConvertFromScreen(BPoint* point) const;
			void			ConvertFromScreen(IntPoint* point) const;
			void			ConvertFromScreen(BRect* rect) const;
			void			ConvertFromScreen(IntRect* rect) const;
			void			ConvertFromScreen(BRegion* region) const;

			void			ConvertToScreenForDrawing(BPoint* point) const;
			void			ConvertToScreenForDrawing(BRect* rect) const;
			void			ConvertToScreenForDrawing(BRegion* region) const;

			void			ConvertFromScreenForDrawing(BPoint* point) const;
				// used when updating the pen position

			void			MoveBy(int32 dx, int32 dy,
								BRegion* dirtyRegion);

			void			ResizeBy(int32 dx, int32 dy,
								BRegion* dirtyRegion);

			void			ScrollBy(int32 dx, int32 dy,
								BRegion* dirtyRegion);

			void			ParentResized(int32 dx, int32 dy,
								BRegion* dirtyRegion);

			void			CopyBits(IntRect src, IntRect dst,
									 BRegion& windowContentClipping);

			const BRegion&	LocalClipping() const { return fLocalClipping; }

			const RGBColor&	ViewColor() const
								{ return fViewColor; }
			void			SetViewColor(const RGBColor& color)
								{ fViewColor = color; }
			void			SetViewColor(const rgb_color& color)
								{ fViewColor = color; }

			ServerBitmap*	ViewBitmap() const
								{ return fViewBitmap; }
			void			SetViewBitmap(ServerBitmap* bitmap, IntRect sourceRect,
								IntRect destRect, int32 resizingMode, int32 options);

			void			PushState();
			void			PopState();
			DrawState*		CurrentState() const { return fDrawState; }

			void			SetEventMask(uint32 eventMask, uint32 options);
			uint32			EventMask() const
								{ return fEventMask; }
			uint32			EventOptions() const
								{ return fEventOptions; }

			ServerCursor*	Cursor() const { return fCursor; }
			void			SetCursor(ServerCursor* cursor);

			ServerPicture*	Picture() const;
			void			SetPicture(ServerPicture* picture);

			// for background clearing
			virtual void	Draw(DrawingEngine* drawingEngine,
								BRegion* effectiveClipping,
								BRegion* windowContentClipping,
								bool deep = false);

			virtual void	MouseDown(BMessage* message, BPoint where);
			virtual void	MouseUp(BMessage* message, BPoint where);
			virtual void	MouseMoved(BMessage* message, BPoint where);

			void			SetHidden(bool hidden);
			bool			IsHidden() const;

			// takes into account parent views hidden status
			bool			IsVisible() const
								{ return fVisible; }
			// update visible status for this view and all children
			// according to the parents visibility
			void			UpdateVisibleDeep(bool parentVisible);

			void			UpdateOverlay();

			void			MarkBackgroundDirty();
			bool			IsBackgroundDirty() const
								{ return fBackgroundDirty; }

			bool			IsDesktopBackground() const
								{ return fIsDesktopBackground; }

			void			AddTokensForLayersInRegion(BMessage* message,
								BRegion& region,
								BRegion* windowContentClipping);

			void			AddTokensForLayersInRegion(BPrivate::PortLink& link,
								BRegion& region,
								BRegion* windowContentClipping);

			// clipping
			void			RebuildClipping(bool deep);
			BRegion&		ScreenClipping(BRegion* windowContentClipping,
										   bool force = false) const;
			void			InvalidateScreenClipping();
	inline	bool			IsScreenClippingValid() const
								{ return fScreenClippingValid; }

			// debugging
			void			PrintToStream() const;

	protected:
			void			_MoveScreenClipping(int32 x, int32 y,
												bool deep);
			Overlay*		_Overlay() const;
			void			_UpdateOverlayView() const;

			BString			fName;
			int32			fToken;
			// area within parent coordinate space
			IntRect			fFrame;
			// scrolling offset
			IntPoint		fScrollingOffset;

			RGBColor		fViewColor;
			DrawState*		fDrawState;
			ServerBitmap*	fViewBitmap;
			IntRect			fBitmapSource;
			IntRect			fBitmapDestination;
			int32			fBitmapResizingMode;
			int32			fBitmapOptions;

			uint32			fResizeMode;
			uint32			fFlags;
			bool			fHidden;
			bool			fVisible;
			bool			fBackgroundDirty;
			bool			fIsDesktopBackground;

			uint32			fEventMask;
			uint32			fEventOptions;

			WindowLayer*	fWindow;
			ViewLayer*		fParent;

			ViewLayer*		fFirstChild;
			ViewLayer*		fPreviousSibling;
			ViewLayer*		fNextSibling;
			ViewLayer*		fLastChild;

			ServerCursor*	fCursor;
			ServerPicture*	fPicture;

			// clipping
			BRegion			fLocalClipping;

	mutable	BRegion			fScreenClipping;
	mutable	bool			fScreenClippingValid;

};


#endif // LAYER_H
