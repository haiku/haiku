/*
 * Copyright 2006-2012 Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef ALPHA_SLIDER_H
#define ALPHA_SLIDER_H

#include <Control.h>

class AlphaSlider : public BControl {
public:
								AlphaSlider(orientation dir = B_HORIZONTAL,
									BMessage* message = NULL,
									border_style border = B_FANCY_BORDER);
	virtual						~AlphaSlider();

	// BControl interface
	virtual	void				WindowActivated(bool active);
	virtual	void				MakeFocus(bool focus);

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				SetValue(int32 value);
	virtual	void				SetEnabled(bool enabled);

	// AlphaSlider
			void				SetColor(const rgb_color& color);

			bool				IsTracking() const
									{ return fDragging; }

private:
			void				_UpdateColors();
			void				_AllocBitmap(int32 width, int32 height);
			BRect				_BitmapRect() const;
			int32				_ValueFor(BPoint where) const;

private:
			BBitmap*			fBitmap;
			rgb_color			fColor;
			bool				fDragging;
			orientation			fOrientation;
			border_style		fBorderStyle;
};

#endif // ALPHA_SLIDER_H
