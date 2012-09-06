/*
 * Copyright 2011, Hamish Morrison, hamish@lavabit.com
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <stdio.h>
#include <stdlib.h>

#include <Point.h>


static const int32 kErrorSettingsNotFound = B_ERRORS_END + 1;
static const int32 kErrorSettingsInvalid = B_ERRORS_END + 2;
static const int32 kErrorVolumeNotFound = B_ERRORS_END + 3;



class Settings {
public:
							Settings();

			bool			SwapEnabled() const
								{ return fCurrentSettings.enabled; }
			off_t			SwapSize() const { return fCurrentSettings.size; }
			dev_t			SwapVolume() { return fCurrentSettings.volume; }
			BPoint			WindowPosition() const { return fWindowPosition; }

			void			SetSwapEnabled(bool enabled,
								bool revertable = true);
			void			SetSwapSize(off_t size, bool revertable = true);
			void			SetSwapVolume(dev_t volume,
								bool revertable = true);
			void			SetWindowPosition(BPoint position);

			status_t		ReadWindowSettings();
			status_t		WriteWindowSettings();
			status_t		ReadSwapSettings();
			status_t		WriteSwapSettings();

			bool			IsRevertable();
			void			RevertSwapSettings();

			bool			IsDefaultable();
			void			DefaultSwapSettings(bool revertable = true);
private:
			struct SwapSettings {
				bool enabled;
				off_t size;
				dev_t volume;
			};

			BPoint			fWindowPosition;

			SwapSettings	fCurrentSettings;
			SwapSettings	fInitialSettings;
			SwapSettings	fDefaultSettings;
};

#endif	/* SETTINGS_H */
