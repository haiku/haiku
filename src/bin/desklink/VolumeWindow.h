/*
 * Copyright 2003-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Axel Dörfler, axeld@pinc-software.de.
 */
#ifndef VOLUME_WINDOW_H
#define VOLUME_WINDOW_H


#include <Window.h>

#include "MixerControl.h"


class VolumeWindow : public BWindow {
public:
							VolumeWindow(BRect frame, bool dontBeep = false,
								int32 volumeWhich = VOLUME_USE_MIXER);
	virtual					~VolumeWindow();

protected:
	virtual	void			MessageReceived(BMessage* message);

private:
			int32			fUpdatedCount;
};

#endif	// VOLUME_WINDOW_H
