
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

	inline	rgb_color		ViewColor() const
								{ return fViewColor; }

			void			AttachedToWindow(WindowLayer* window,
											 bool topLayer = false);
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

			bool			IsHidden() const;
			void			Hide();
			void			Show();

			// clipping
			void			RebuildClipping(bool deep);
			BRegion&		ScreenClipping(BRegion* windowContentClipping,
										   bool force = false) const;

			// debugging
			void			PrintToStream() const;

			void			InvalidateScreenClipping(bool deep);

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

			WindowLayer*	fWindow;
			ViewLayer*		fParent;
			bool			fIsTopLayer;

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
