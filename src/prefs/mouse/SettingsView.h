// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			SettingsView.h
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk)
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Mouse Preferences
//  Created:		December 10, 2003
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
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
		virtual void GetPreferredSize(float *_width, float *_height);
		virtual void Draw(BRect frame);

		void SetMouseType(int32 type);
		void UpdateFromSettings();

	private:
		typedef BBox inherited;

		const MouseSettings &fSettings;

		BPopUpMenu	*fTypeMenu, *fFocusMenu;
		MouseView	*fMouseView;
		BSlider		*dcSpeedSlider, *mouseSpeedSlider, *mouseAccSlider;

		BBitmap 	*fDoubleClickBitmap, *fSpeedBitmap, *fAccelerationBitmap;
};

#endif	/* SETTINGS_VIEW_H */
