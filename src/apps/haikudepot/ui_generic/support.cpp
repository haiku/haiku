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
#include <Font.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <View.h>


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


status_t
get_app_resources(BResources& resources)
{
	app_info info;
	status_t status = be_app->GetAppInfo(&info);
	if (status != B_OK)
		return status;

	return resources.SetTo(&info.ref);
}


void
set_small_font(BView* view)
{
	BFont font;
	view->GetFont(&font);
	font.SetSize(ceilf(font.Size() * 0.8));
	view->SetFont(&font);
}




