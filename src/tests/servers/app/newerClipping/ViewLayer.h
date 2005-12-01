
#ifndef	VIEW_LAYER_H
#define VIEW_LAYER_H

#include <GraphicsDefs.h>
#include <Region.h>
#include <String.h>

class BList;
class DrawingEngine;
class WindowLayer;

class ViewLayer {
 public:
							ViewLayer(BRect frame,
									  const char* name,
									  uint32 reizeMode,
									  uint32 flags,
									  rgb_color viewColor);

	virtual					~ViewLayer();

	inline	BRect			Frame() const
								{ return fFrame; }
			BRect			Bounds() const;
			// converts the given frame up the view hirarchy and
			// clips to each views bounds
			void			ConvertToVisibleInTopView(BRect* bounds) const;

	inline	rgb_color		ViewColor() const
								{ return fViewColor; }

			void			AttachedToWindow(WindowLayer* window);
			void			DetachedFromWindow();

			// tree stuff
			void			AddChild(ViewLayer* layer);
			bool			RemoveChild(ViewLayer* layer);

	inline	ViewLayer*		Parent() const
								{ return fParent; }

			ViewLayer*		FirstChild() const;
			ViewLayer*		PreviousChild() const;
			ViewLayer*		NextChild() const;
			ViewLayer*		LastChild() const;

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

			void			ConvertToTop(BPoint* point) const;
			void			ConvertToTop(BRect* rect) const;
			void			ConvertToTop(BRegion* region) const; 

			void			ConvertFromTop(BPoint* point) const;
			void			ConvertFromTop(BRect* rect) const;
			void			ConvertFromTop(BRegion* region) const; 

			// settings
			void			SetName(const char* string);
	inline	const char*		Name() const
								{ return fName.String(); }

			void			MoveBy(			int32 dx, int32 dy,
											BRegion* dirtyRegion);

			void			ResizeBy(		int32 dx, int32 dy,
											BRegion* dirtyRegion);

			void			ScrollBy(		int32 dx, int32 dy,
											BRegion* dirtyRegion);

			void			ParentResized(	int32 dx, int32 dy,
											BRegion* dirtyRegion);

			// for background clearing
			void			Draw(			DrawingEngine* drawingEngine,
											BRegion* effectiveClipping,
											BRegion* windowContentClipping,
											bool deep = false);

			// to simulate drawing triggered from client side
			void			ClientDraw(		DrawingEngine* drawingEngine,
											BRegion* effectiveClipping);

			void			SetHidden(bool hidden);
			bool			IsHidden() const;

			// takes into account parent views hidden status
			bool			IsVisible() const
								{ return fVisible; }
			// update visible status for this view and all children
			// according to the parents visibility
			void			UpdateVisibleDeep(bool parentVisible);

			// clipping
			void			RebuildClipping(bool deep);
			BRegion&		ScreenClipping(BRegion* windowContentClipping,
										   bool force = false) const;
			void			InvalidateScreenClipping(bool deep);

			// debugging
			void			PrintToStream() const;

private:
			void			_MoveScreenClipping(int32 x, int32 y,
												bool deep);

			BString			fName;
			// area within parent coordinate space
			BRect			fFrame;
			// scrolling offset
			BPoint			fScrollingOffset;

			rgb_color		fViewColor;

			uint32			fResizeMode;
			uint32			fFlags;
			bool			fHidden;
			bool			fVisible;

			WindowLayer*	fWindow;
			ViewLayer*		fParent;

			ViewLayer*		fFirstChild;
			ViewLayer*		fPreviousSibling;
			ViewLayer*		fNextSibling;
			ViewLayer*		fLastChild;

			// used for traversing the childs
	mutable	ViewLayer*		fCurrentChild;

			// clipping
			BRegion			fLocalClipping;

	mutable	BRegion			fScreenClipping;
	mutable	bool			fScreenClippingValid;

};

#endif // LAYER_H
