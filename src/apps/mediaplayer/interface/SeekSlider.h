/*
 * Copyright © 2006-2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef SEEK_SLIDER_H
#define SEEK_SLIDER_H

#include <Box.h>
#include <Control.h>

class SeekSlider : public BControl {
 public:
								SeekSlider(BRect frame,
									const char* name, BMessage* message,
									int32 minValue, int32 maxValue);

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
	virtual	void				FrameResized(float width, float height);

	// SeekSlider
			void				SetPosition(float position);
			bool				IsTracking() const;

private:
			int32				_ValueFor(float x) const;
			int32				_KnobPosFor(BRect bounds,
											int32 value) const;
			void				_StrokeFrame(BRect frame,
									rgb_color left, rgb_color top,
									rgb_color right, rgb_color bottom);
			void				_SetKnobPosition(int32 knobPos);


			bool				fTracking;
			bigtime_t			fLastTrackTime;
			int32				fKnobPos;
			int32				fMinValue;
			int32				fMaxValue;
};


#endif	//SEEK_SLIDER_H
