/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef TOUCHPAD_PREF_H
#define TOUCHPAD_PREF_H


#include <Debug.h>

#include "touchpad_settings.h"
#include <Input.h>
#include <Path.h>


#if DEBUG
#	define LOG(text...) PRINT((text))
#else
#	define LOG(text...)
#endif


class TouchpadPref {
public:
								TouchpadPref(BInputDevice* device);
			virtual				~TouchpadPref();

			void				Revert();
			void				Defaults();

			BPoint 				WindowPosition()
									{ return fWindowPosition; }
			void				SetWindowPosition(BPoint position)
									{ fWindowPosition = position; }

			touchpad_settings&	Settings()
									{ return fSettings; }
			status_t			UpdateSettings();

private:
			status_t			GetSettingsPath(BPath& path);
			status_t			LoadSettings();
			status_t			SaveSettings();

			BInputDevice* 		fTouchPad;

			touchpad_settings	fSettings;
			touchpad_settings	fStartSettings;
			BPoint				fWindowPosition;
};


#endif	// TOUCHPAD_PREF_H
