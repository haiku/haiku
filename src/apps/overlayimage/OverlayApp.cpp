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
 *			Humdinger		<humdingerb@gmail.com>
 */

#include "OverlayApp.h"
#include "OverlayWindow.h"


OverlayApp::OverlayApp()
	: BApplication("application/x-vnd.Haiku-OverlayImage")
{
	OverlayWindow *theWindow = new OverlayWindow();
	theWindow->Show();
}


int
main()
{
	OverlayApp theApp;
	theApp.Run();
	return (0);
}
