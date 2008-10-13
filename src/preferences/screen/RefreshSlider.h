/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef REFRESH_SLIDER_H
#define REFRESH_SLIDER_H


#include <Slider.h>


class RefreshSlider : public BSlider {
	public:
		RefreshSlider(BRect frame, float min, float max, uint32 resizingMode);
		virtual ~RefreshSlider();

		virtual void DrawFocusMark();
		virtual const char* UpdateText() const;
		virtual void KeyDown(const char* bytes, int32 numBytes);

	private:
		char* fStatus;
};

#endif	// REFRESH_SLIDER_H
