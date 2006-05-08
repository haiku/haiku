/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SEEK_SLIDER_H
#define SEEK_SLIDER_H

#include <Box.h>
#include <Control.h>

class SeekSlider : public BControl {
 public:
								SeekSlider(BRect frame,
										   const char* name,
										   BMessage* message,
										   int32 minValue,
										   int32 maxValue);

	virtual						~SeekSlider();

	// BControl interface
	virtual	void				AttachedToWindow();
	virtual	void				SetValue(int32 value);
	virtual void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);
	virtual	void				ResizeToPreferred();

	// SeekSlider
			void				SetPosition(float position);

private:
			int32				_ValueFor(float x) const;
			void				_StrokeFrame(BRect frame,
											 rgb_color left,
											 rgb_color top,
											 rgb_color right,
											 rgb_color bottom);

			bool				fTracking;
			int32				fMinValue;
			int32				fMaxValue;
};


#endif	//SEEK_SLIDER_H
