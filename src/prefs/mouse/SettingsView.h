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

class SettingsView : public BBox {
	public:
		SettingsView(BRect frame, MouseSettings &settings);

		virtual void AttachedToWindow();
		virtual void MouseDown(BPoint where);
		virtual void Draw(BRect frame);
		virtual void Pulse();

		void Init();

		BSlider		*dcSpeedSlider, *mouseSpeedSlider, *mouseAccSlider;
		BPopUpMenu	*mouseTypeMenu, *focusMenu, *mouseMapMenu;

		bigtime_t 	fClickSpeed;
		int32 		fMouseSpeed;
		int32 		fMouseAcc;
		int32 		fMouseType;
		mode_mouse 	fMouseMode;
		mouse_map 	fMouseMap, fCurrentMouseMap;
		int32		fCurrentButton;

	private:
		typedef BBox inherited;

		MouseSettings	&fSettings;

		uint32		fButtons;
		uint32		fOldButtons;

		BBitmap 		*fDoubleClickBitmap, *fSpeedBitmap, *fAccelerationBitmap;
		BBitmap			*fMouseBitmap, *fMouseDownBitmap;
};

#endif	/* SETTINGS_VIEW_H */
