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
#include "StateView.h"

class BBitmap;
class IconRenderer;

class CanvasView : public StateView,
				   public IconListener {
 public:
								CanvasView(BRect frame);
	virtual						~CanvasView();

	// StateView interface
	virtual	void				AttachedToWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);

	// IconListener interface
	virtual	void				AreaInvalidated(const BRect& area);

	// CanvasView
			void				SetIcon(Icon* icon);

	inline	float				ZoomLevel() const
									{ return fZoomLevel; }

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

 private:
			BBitmap*			fBitmap;
			Icon*				fIcon;
			IconRenderer*		fRenderer;
			BRect				fDirtyIconArea;

			BPoint				fCanvasOrigin;
			float				fZoomLevel;

			BBitmap*			fOffsreenBitmap;
			BView*				fOffsreenView;
};

#endif // CANVAS_VIEW_H
