/*
 * Copyright 1999-2010, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * OverlayImage is based on the code presented in this article:
 * http://www.haiku-os.org/documents/dev/replishow_a_replicable_image_viewer
 *
 * Authors:
 *			Seth Flexman
 *			Hartmuth Reh
 *			Humdinger
 */

#ifndef OVERLAY_WINDOW_H
#define OVERLAY_WINDOW_H

#include <TextView.h>
#include <Window.h>


class OverlayWindow : public BWindow {
public:
						OverlayWindow();
		virtual bool 	QuitRequested();
};

#endif // OVERLAY_WINDOW_H
