/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CANVAS_VIEW_H
#define CANVAS_VIEW_H

#include "Icon.h"
#include "Scrollable.h"
#include "StateView.h"

class BBitmap;
class IconRenderer;

enum {
	SNAPPING_OFF	= 0,
	SNAPPING_64,
	SNAPPING_32,
	SNAPPING_16,
};

class CanvasView : public StateView,
				   public Scrollable,
				   public IconListener {
 public:
								CanvasView(BRect frame);
	virtual						~CanvasView();

	// StateView interface
	virtual	void				AttachedToWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				FilterMouse(BPoint* where) const;

	// Scrollable interface
 protected:
	virtual	void				ScrollOffsetChanged(BPoint oldOffset,
													BPoint newOffset);
	virtual	void				VisibleSizeChanged(float oldWidth,
												   float oldHeight,
												   float newWidth,
												   float newHeight);
	// IconListener interface
 public:
	virtual	void				AreaInvalidated(const BRect& area);

	// CanvasView
			void				SetIcon(Icon* icon);

	inline	float				ZoomLevel() const
									{ return fZoomLevel; }

			void				SetMouseFilterMode(uint32 mode);
			bool				MouseFilterMode() const
									{ return fMouseFilterMode; }

			void				ConvertFromCanvas(BPoint* point) const;
			void				ConvertToCanvas(BPoint* point) const;

			void				ConvertFromCanvas(BRect* rect) const;
			void				ConvertToCanvas(BRect* rect) const;

 protected:
	// StateView interface
	virtual	bool				_HandleKeyDown(uint32 key, uint32 modifiers);

	// CanvasView
			BRect				_CanvasRect() const;

			void				_AllocBackBitmap(float width,
												 float height);
			void				_FreeBackBitmap();
			void				_DrawInto(BView* view,
										  BRect updateRect);

			void				_MakeBackground();

 private:
			double				_NextZoomInLevel(double zoom) const;
			double				_NextZoomOutLevel(double zoom) const;
			void				_SetZoom(double zoomLevel);
			BRect				_LayoutCanvas();

			BBitmap*			fBitmap;
			BBitmap*			fBackground;

			Icon*				fIcon;
			IconRenderer*		fRenderer;
			BRect				fDirtyIconArea;

			BPoint				fCanvasOrigin;
			double				fZoomLevel;

			uint32				fMouseFilterMode;

			BBitmap*			fOffsreenBitmap;
			BView*				fOffsreenView;
};

#endif // CANVAS_VIEW_H
