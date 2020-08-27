/*
 * Copyright (c) 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Marcus Overhagen <marcus@overhagen.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Joseph Groover <looncraz@looncraz.net>
 */
#ifndef	VIEW_H
#define VIEW_H


#include "Canvas.h"
#include "IntRect.h"

#include <AutoDeleter.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <ObjectList.h>
#include <Region.h>
#include <Referenceable.h>
#include <String.h>

class BList;
class BMessage;

namespace BPrivate {
	class PortLink;
};

class DrawingEngine;
class Overlay;
class Window;
class ServerBitmap;
class ServerCursor;
class ServerPicture;
class BGradient;

class View: public Canvas {
public:
							View(IntRect frame, IntPoint scrollingOffset,
								const char* name, int32 token,
								uint32 resizeMode, uint32 flags);
	virtual					~View();

			int32			Token() const
								{ return fToken; }

			IntRect			Frame() const
								{ return fFrame; }
	virtual	IntRect			Bounds() const;

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

			// converts the given frame up the view hierarchy and
			// clips to each views bounds
			void			ConvertToVisibleInTopView(IntRect* bounds) const;

	virtual	void			AttachedToWindow(::Window* window);
	virtual void			DetachedFromWindow();
			::Window*		Window() const { return fWindow; }

			// Shorthands for opaque Window access
			DrawingEngine*	GetDrawingEngine() const;
			ServerPicture*	GetPicture(int32 token) const;
			void			ResyncDrawState();
			void			UpdateCurrentDrawingRegion();

			// tree stuff
			void			AddChild(View* view);
			bool			RemoveChild(View* view);

	inline	bool			HasParent(View* candidate) const
							{
								return fParent == candidate
									|| (fParent != NULL
										&& fParent->HasParent(candidate));
							}

	inline	View*			Parent() const
								{ return fParent; }

	inline	View*			FirstChild() const
								{ return fFirstChild; }
	inline	View*			LastChild() const
								{ return fLastChild; }
	inline	View*			PreviousSibling() const
								{ return fPreviousSibling; }
	inline	View*			NextSibling() const
								{ return fNextSibling; }

			View*			TopView();

			uint32			CountChildren(bool deep = false) const;
			void			CollectTokensForChildren(BList* tokenMap) const;
			void			FindViews(uint32 flags, BObjectList<View>& list,
								int32& left);

			bool			HasView(View* view);
			View*			ViewAt(const BPoint& where);

public:
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

			const rgb_color& ViewColor() const
								{ return fViewColor; }
			void			SetViewColor(const rgb_color& color)
								{ fViewColor = color; }

			void			ColorUpdated(color_which which, rgb_color color);
			void			SetViewUIColor(color_which which, float tint);
			color_which		ViewUIColor(float* tint);

			ServerBitmap*	ViewBitmap() const
								{ return fViewBitmap; }
			void			SetViewBitmap(ServerBitmap* bitmap,
								IntRect sourceRect, IntRect destRect,
								int32 resizingMode, int32 options);

			void			PushState();
			void			PopState();

			void			SetEventMask(uint32 eventMask, uint32 options);
			uint32			EventMask() const
								{ return fEventMask; }
			uint32			EventOptions() const
								{ return fEventOptions; }

			void			SetCursor(ServerCursor* cursor);
			ServerCursor*	Cursor() const { return fCursor; }

			void			SetPicture(ServerPicture* picture);
			ServerPicture*	Picture() const
								{ return fPicture; }

			void			BlendAllLayers();

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

			void			AddTokensForViewsInRegion(BPrivate::PortLink& link,
								BRegion& region,
								BRegion* windowContentClipping);

			// clipping
			void			RebuildClipping(bool deep);
			BRegion&		ScreenAndUserClipping(
								BRegion* windowContentClipping,
								bool force = false) const;
			void			InvalidateScreenClipping();
	inline	bool			IsScreenClippingValid() const
								{
									return fScreenClippingValid
										&& (fUserClipping.Get() == NULL
										|| (fUserClipping.Get() != NULL
										&& fScreenAndUserClipping.Get() != NULL));
								}

			// debugging
			void			PrintToStream() const;
#if 0
			bool			MarkAt(DrawingEngine* engine, const BPoint& where,
								int32 level = 0);
#endif

protected:
	virtual	void			_LocalToScreenTransform(
								SimpleTransform& transform) const;
	virtual	void			_ScreenToLocalTransform(
								SimpleTransform& transform) const;

			BRegion&		_ScreenClipping(BRegion* windowContentClipping,
								bool force = false) const;
			void			_MoveScreenClipping(int32 x, int32 y,
								bool deep);
			Overlay*		_Overlay() const;
			void			_UpdateOverlayView() const;

			BString			fName;
			int32			fToken;
			// area within parent coordinate space
			IntRect			fFrame;
			// offset of the local area (bounds)
			IntPoint		fScrollingOffset;

			rgb_color		fViewColor;
			color_which		fWhichViewColor;
			float			fWhichViewColorTint;
			BReference<ServerBitmap>
							fViewBitmap;
			IntRect			fBitmapSource;
			IntRect			fBitmapDestination;
			int32			fBitmapResizingMode;
			int32			fBitmapOptions;

			uint32			fResizeMode;
			uint32			fFlags;
			bool			fHidden : 1;
			bool			fVisible : 1;
			bool			fBackgroundDirty : 1;
			bool			fIsDesktopBackground : 1;

			uint32			fEventMask;
			uint32			fEventOptions;

			::Window*		fWindow;
			View*			fParent;

			View*			fFirstChild;
			View*			fPreviousSibling;
			View*			fNextSibling;
			View*			fLastChild;

			BReference<ServerCursor>
							fCursor;
			BReference<ServerPicture>
							fPicture;

			// clipping
			BRegion			fLocalClipping;

	mutable	BRegion			fScreenClipping;
	mutable	bool			fScreenClippingValid;

			ObjectDeleter<BRegion>
							fUserClipping;
	mutable	ObjectDeleter<BRegion>
							fScreenAndUserClipping;
};

#endif	// VIEW_H
