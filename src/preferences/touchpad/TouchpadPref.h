/*
 * Copyright 2008-2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler (haiku@Clemens-Zeidler.de)
 */
#ifndef TOUCHPAD_PREF_H
#define TOUCHPAD_PREF_H


#define DEBUG 1
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
								TouchpadPref();
								~TouchpadPref();

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

			status_t			ConnectToTouchPad();

			bool 				fConnected;
			BInputDevice* 		fTouchPad;

			touchpad_settings	fSettings;
			touchpad_settings	fStartSettings;
			BPoint				fWindowPosition;
};


#endif	// TOUCHPAD_PREF_H
