/*
 * Copyright 2006, 2011, 2013 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "support.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>


status_t
load_settings(BMessage* message, const char* fileName, const char* folder)
{
	if (message == NULL || fileName == NULL || fileName[0] == '\0')
		return B_BAD_VALUE;

	BPath path;
	status_t ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK)
		return ret;

	// passing folder is optional
	if (folder != NULL)
		ret = path.Append(folder);

	if (ret == B_OK && (ret = path.Append(fileName)) == B_OK ) {
		BFile file(path.Path(), B_READ_ONLY);
		ret = file.InitCheck();
		if (ret == B_OK)
			ret = message->Unflatten(&file);
	}

	return ret;
}


status_t
save_settings(const BMessage* message, const char* fileName, const char* folder)
{
	if (message == NULL || fileName == NULL || fileName[0] == '\0')
		return B_BAD_VALUE;

	BPath path;
	status_t ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK)
		return ret;

	// passing folder is optional
	if (folder != NULL)
		ret = path.Append(folder);

	if (ret == B_OK)
		ret = create_directory(path.Path(), 0777);

	if (ret == B_OK)
		ret = path.Append(fileName);

	if (ret == B_OK) {
		BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		ret = file.InitCheck();
		if (ret == B_OK)
			ret = message->Flatten(&file);
	}

	return ret;
}


bool
make_sure_frame_is_on_screen(BRect& frame, float borderWidth,
	float tabHeight, BWindow* window)
{
	if (!frame.IsValid())
		return false;

	BScreen* screen = window != NULL ? new BScreen(window)
		: new BScreen(B_MAIN_SCREEN_ID);

	if (!screen->IsValid()) {
		delete screen;
		return false;
	}

	BRect screenFrame = screen->Frame();

	// Validate borderWidth and tabHeight
	if (borderWidth < 0.0f)
		borderWidth = 0.0f;
	else
		borderWidth = std::min(borderWidth, floorf(screenFrame.Width() / 4.0f));

	if (tabHeight < 0.0f)
		tabHeight = 0.0f;
	else
		tabHeight = std::min(tabHeight, floorf(screenFrame.Height() / 4.0f));

	// Account for window border and tab. It doesn't matter much if the
	// decorator frame is wider, just as long as the user can grab a
	// border to move the window
	screenFrame.InsetBy(borderWidth, borderWidth);
	screenFrame.top += tabHeight;

	if (!screenFrame.Contains(frame)) {
		// make sure frame fits in the screen
		if (frame.Width() > screenFrame.Width())
			frame.right -= frame.Width() - screenFrame.Width();
		if (frame.Height() > screenFrame.Height())
			frame.bottom -= frame.Height() - screenFrame.Height();

		// frame is now at the most the size of the screen
		if (frame.right > screenFrame.right)
			frame.OffsetBy(-(frame.right - screenFrame.right), 0.0);
		if (frame.bottom > screenFrame.bottom)
			frame.OffsetBy(0.0, -(frame.bottom - screenFrame.bottom));
		if (frame.left < screenFrame.left)
			frame.OffsetBy((screenFrame.left - frame.left), 0.0);
		if (frame.top < screenFrame.top)
			frame.OffsetBy(0.0, (screenFrame.top - frame.top));
	}

	delete screen;
	return true;
}


status_t
get_app_resources(BResources& resources)
{
	app_info info;
	status_t status = be_app->GetAppInfo(&info);
	if (status != B_OK)
		return status;

	return resources.SetTo(&info.ref);
}

