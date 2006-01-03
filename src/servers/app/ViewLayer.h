/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef	VIEW_LAYER_H
#define VIEW_LAYER_H


#include "RGBColor.h"

#include <GraphicsDefs.h>
#include <Region.h>
#include <String.h>

class BList;

class DrawState;
class DrawingEngine;
class WindowLayer;
class ServerBitmap;
class ServerPicture;

class ViewLayer {
 public:
							ViewLayer(BRect frame, const char* name,
								int32 token, uint32 resizeMode,
								uint32 flags);

	virtual					~ViewLayer();

			void			SetName(const char* string);
			const char*		Name() const
								{ return fName.String(); }

			int32			Token() const
								{ return fToken; }

			BRect			Frame() const
								{ return fFrame; }
			BRect			Bounds() const;

			void			SetResizeMode(uint32 resizeMode)
								{ fResizeMode = resizeMode; }
			uint32			Flags() const
								{ return fFlags; }
			void			SetFlags(uint32 flags);

			BPoint			ScrollingOffset() const;

			void			SetDrawingOrigin(BPoint origin);
			BPoint			DrawingOrigin() const;

			void			SetScale(float scale);
			float			Scale() const;

			void			SetUserClipping(const BRegion& region);
				// region is expected in layer coordinates

			// converts the given frame up the view hierarchy and
			// clips to each views bounds
			void			ConvertToVisibleInTopView(BRect* bounds) const;

			void			AttachedToWindow(WindowLayer* window);
			void			DetachedFromWindow();
			WindowLayer*	Window() const { return fWindow; }

			// tree stuff
			void			AddChild(ViewLayer* layer);
			bool			RemoveChild(ViewLayer* layer);

	inline	ViewLayer*		Parent() const
								{ return fParent; }

			ViewLayer*		FirstChild() const;
			ViewLayer*		LastChild() const;
			ViewLayer*		PreviousSibling() const;
			ViewLayer*		NextSibling() const;

			ViewLayer*		TopLayer();

			uint32			CountChildren(bool deep = false) const;
			void			CollectTokensForChildren(BList* tokenMap) const;

			ViewLayer*		ViewAt(const BPoint& where,
								   BRegion* windowContentClipping);

			// coordinate conversion
			void			ConvertToParent(BPoint* point) const;
			void			ConvertToParent(BRect* rect) const;
			void			ConvertToParent(BRegion* region) const; 

			void			ConvertFromParent(BPoint* point) const;
			void			ConvertFromParent(BRect* rect) const;
			void			ConvertFromParent(BRegion* region) const; 

			void			ConvertToScreen(BPoint* point) const;
			void			ConvertToScreen(BRect* rect) const;
			void			ConvertToScreen(BRegion* region) const;

			void			ConvertFromScreen(BPoint* point) const;
			void			ConvertFromScreen(BRect* rect) const;
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

			void			CopyBits(BRect src, BRect dst,
									 BRegion& windowContentClipping);

			const BRegion&	LocalClipping() const { return fLocalClipping; }

			const RGBColor&	ViewColor() const
								{ return fViewColor; }
			void			SetViewColor(const RGBColor& color)
								{ fViewColor = color; }
			void			SetViewBitmap(ServerBitmap* bitmap, BRect sourceRect,
								BRect destRect, int32 resizingMode, int32 options);

			void			PushState();
			void			PopState();
			DrawState*		CurrentState() const { return fDrawState; }

			void			SetEventMask(uint32 eventMask, uint32 options);
			uint32			EventMask() const
								{ return fEventMask; }
			uint32			EventOptions() const
								{ return fEventOptions; }

			void			SetPicture(ServerPicture *picture);
			ServerPicture		*Picture() const;

			// for background clearing
			virtual void	Draw(DrawingEngine* drawingEngine,
								BRegion* effectiveClipping,
								BRegion* windowContentClipping,
								bool deep = false);

			void			SetHidden(bool hidden);
			bool			IsHidden() const;

			// takes into account parent views hidden status
			bool			IsVisible() const
								{ return fVisible; }
			// update visible status for this view and all children
			// according to the parents visibility
			void			UpdateVisibleDeep(bool parentVisible);

			void			MarkBackgroundDirty();
			bool			IsBackgroundDirty() const
								{ return fBackgroundDirty; }

			// clipping
			void			RebuildClipping(bool deep);
			BRegion&		ScreenClipping(BRegion* windowContentClipping,
										   bool force = false) const;
			void			InvalidateScreenClipping(bool deep);
			bool			IsScreenClippingValid() const
								{ return fScreenClippingValid; }

			// debugging
			void			PrintToStream() const;

	protected:
			void			_MoveScreenClipping(int32 x, int32 y,
												bool deep);

			BString			fName;
			int32			fToken;
			// area within parent coordinate space
			BRect			fFrame;
			// scrolling offset
			BPoint			fScrollingOffset;

			RGBColor		fViewColor;
			DrawState*		fDrawState;
			ServerBitmap*	fViewBitmap;
			BRect			fBitmapSource;
			BRect			fBitmapDestination;
			int32			fBitmapResizingMode;
			int32			fBitmapOptions;

			uint32			fResizeMode;
			uint32			fFlags;
			bool			fHidden;
			bool			fVisible;
			bool			fBackgroundDirty;

			uint32			fEventMask;
			uint32			fEventOptions;

			WindowLayer*	fWindow;
			ViewLayer*		fParent;

			ViewLayer*		fFirstChild;
			ViewLayer*		fPreviousSibling;
			ViewLayer*		fNextSibling;
			ViewLayer*		fLastChild;

			ServerPicture		*fPicture;
			// clipping
			BRegion			fLocalClipping;

	mutable	BRegion			fScreenClipping;
	mutable	bool			fScreenClippingValid;

};

#endif // LAYER_H
