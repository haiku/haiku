/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 */
#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <Slider.h>
#include <PopUpMenu.h>


class MouseSettings;
class MouseView;


class SettingsView : public BBox {
public:
		SettingsView(BRect frame, MouseSettings &settings);
		virtual ~SettingsView();

		virtual void AttachedToWindow();
		virtual void GetPreferredSize(float* _width, float* _height);
		virtual void Draw(BRect frame);

		void SetMouseType(int32 type);
		void MouseMapUpdated();
		void UpdateFromSettings();

private:
		typedef BBox inherited;

		const MouseSettings &fSettings;

		BPopUpMenu	*fTypeMenu, *fFocusMenu;
		MouseView	*fMouseView;
		BSlider		*fClickSpeedSlider, *fMouseSpeedSlider, *fAccelerationSlider;

		BBitmap 	*fDoubleClickBitmap, *fSpeedBitmap, *fAccelerationBitmap;

		BRect 		fLeftArea, fRightArea;
		BPoint		fLeft, fRight;
		BPoint		fDoubleClickBmpPoint, fSpeedBmpPoint, fAccelerationBmpPoint;
};

#endif	/* SETTINGS_VIEW_H */
