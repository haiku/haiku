/*
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *
 */

#ifndef COLOR_SLIDER_H
#define COLOR_SLIDER_H

#include <Control.h>

#include "SelectedColorMode.h"

#define	MSG_COLOR_SLIDER	'ColS'

class BBitmap;

class ColorSlider : public BControl {
public:
								ColorSlider(SelectedColorMode mode,
									float value1, float value2,
									orientation dir = B_VERTICAL,
									border_style border = B_FANCY_BORDER);
								ColorSlider(BPoint offsetPoint,
									SelectedColorMode mode,
									float value1, float value2,
									orientation dir = B_VERTICAL,
									border_style border = B_FANCY_BORDER);
	virtual						~ColorSlider();

								// BControl
	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

	virtual	void				AttachedToWindow();

	virtual	status_t			Invoke(BMessage* message = NULL);

	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
										   const BMessage* dragMessage);

	virtual	void				SetValue(int32 value);

	// ColorSlider
			bool				IsTracking() const
									{ return fMouseDown; }

			void				SetModeAndValues(SelectedColorMode mode,
									float value1, float value2);
			void				SetOtherValues(float value1, float value2);
			void				GetOtherValues(float* value1,
									float* value2) const;

			void				SetMarkerToColor(rgb_color color);

private:
			void				_Init(SelectedColorMode mode,
						 			float value1, float value2,
						 			orientation dir, border_style border);

			void				_AllocBitmap(int32 width, int32 height);
			void				_Update();
			BRect				_BitmapRect() const;
			void				_FillBitmap(BBitmap* bitmap,
									SelectedColorMode mode,
									float fixedValue1, float fixedValue2,
									orientation orient) const;

	static	inline void			_DrawColorLineY(uint8* bits, int width,
									int r, int g, int b);
	static	inline void			_DrawColorLineX(uint8* bits, int height,
									int bpr, int r, int g, int b);
			void				_DrawTriangle(BPoint point1, BPoint point2,
									BPoint point3);

			void				_TrackMouse(BPoint where);

private:
	SelectedColorMode			fMode;
	float						fFixedValue1;
	float						fFixedValue2;

	bool						fMouseDown;

	BBitmap*					fBitmap;
	bool						fBitmapDirty;

	orientation					fOrientation;
	border_style				fBorderStyle;
};

#endif // COLOR_SLIDER_H
