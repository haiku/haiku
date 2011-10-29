/*
 * Copyright 2006, 2011 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "support.h"

#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>
#include <View.h>


status_t
load_settings(BMessage* message, const char* fileName, const char* folder)
{
	status_t ret = B_BAD_VALUE;
	if (message) {
		BPath path;
		if ((ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path)) == B_OK) {
			// passing folder is optional
			if (folder)
				ret = path.Append( folder );
			if (ret == B_OK && (ret = path.Append(fileName)) == B_OK ) {
				BFile file(path.Path(), B_READ_ONLY);
				if ((ret = file.InitCheck()) == B_OK) {
					ret = message->Unflatten(&file);
					file.Unset();
				}
			}
		}
	}
	return ret;
}


status_t
save_settings(BMessage* message, const char* fileName, const char* folder)
{
	status_t ret = B_BAD_VALUE;
	if (message) {
		BPath path;
		if ((ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path)) == B_OK) {
			// passing folder is optional
			if (folder && (ret = path.Append(folder)) == B_OK)
				ret = create_directory(path.Path(), 0777);
			if (ret == B_OK && (ret = path.Append(fileName)) == B_OK) {
				BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE
					| B_ERASE_FILE);
				if ((ret = file.InitCheck()) == B_OK) {
					ret = message->Flatten(&file);
					file.Unset();
				}
			}
		}
	}
	return ret;
}


void
stroke_frame(BView* v, BRect r, rgb_color left, rgb_color top, rgb_color right,
	rgb_color bottom)
{
	if (v && r.IsValid()) {
		v->BeginLineArray(4);
			v->AddLine(BPoint(r.left, r.bottom),
					   BPoint(r.left, r.top), left);
			v->AddLine(BPoint(r.left + 1.0, r.top),
					   BPoint(r.right, r.top), top);
			v->AddLine(BPoint(r.right, r.top + 1.0),
					   BPoint(r.right, r.bottom), right);
			v->AddLine(BPoint(r.right - 1.0, r.bottom),
					   BPoint(r.left + 1.0, r.bottom), bottom);
		v->EndLineArray();
	}
}


bool
make_sure_frame_is_on_screen(BRect& frame, BWindow* window)
{
	BScreen* screen = window != NULL ? new BScreen(window)
		: new BScreen(B_MAIN_SCREEN_ID);

	bool success = false;
	if (frame.IsValid() && screen->IsValid()) {
		BRect screenFrame = screen->Frame();
		if (!screenFrame.Contains(frame)) {
			// make sure frame fits in the screen
			if (frame.Width() > screenFrame.Width())
				frame.right -= frame.Width() - screenFrame.Width() + 10.0;
			if (frame.Height() > screenFrame.Height())
				frame.bottom -= frame.Height() - screenFrame.Height() + 30.0;
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
		success = true;
	}
	delete screen;
	return success;
}

