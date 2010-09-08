/*
 * Copyright 2006-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SEEK_SLIDER_H
#define SEEK_SLIDER_H


#include <Slider.h>
#include <String.h>


class SeekSlider : public BSlider {
public:
								SeekSlider(const char* name, BMessage* message,
									int32 minValue, int32 maxValue);

	virtual						~SeekSlider();

	// BSlider interface
	virtual	status_t			Invoke(BMessage* message);
	virtual void				DrawBar();
	virtual	void				DrawThumb();
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				GetPreferredSize(float* _width,
									float* _height);

	// SeekSlider
			bool				IsTracking() const;
			void				SetDisabledString(const char* string);

private:
			bool				fTracking;
			bigtime_t			fLastTrackTime;

			BString				fDisabledString;
};


#endif	//SEEK_SLIDER_H
