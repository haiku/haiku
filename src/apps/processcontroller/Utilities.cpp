/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Utilities.h"
#include "PCWorld.h"
#include "ProcessController.h"
#include "icons.h"

#include <AppMisc.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <Window.h>

#include <stdio.h>
#include <string.h>


bool
get_team_name_and_icon(info_pack& infoPack, bool icon)
{
	BPath systemPath;
	find_directory(B_BEOS_SYSTEM_DIRECTORY, &systemPath);

	bool nameFromArgs = false;
	bool tryTrackerIcon = true;

	for (int len = strlen(infoPack.team_info.args) - 1;
			len >= 0 && infoPack.team_info.args[len] == ' '; len--) {
		infoPack.team_info.args[len] = 0;
	}

	app_info info;
	status_t status = be_roster->GetRunningAppInfo(infoPack.team_info.team, &info);
	if (status != B_OK) {
		if (infoPack.team_info.team == B_SYSTEM_TEAM) {
			// Get icon and name from kernel image
			system_info	systemInfo;
			get_system_info(&systemInfo);

			BPath kernelPath(systemPath);
			kernelPath.Append(systemInfo.kernel_name);
			get_ref_for_path(kernelPath.Path(), &info.ref);
			nameFromArgs = true;
		} else {
			status = BPrivate::get_app_ref(infoPack.team_info.team, &info.ref);
			nameFromArgs = true;
			tryTrackerIcon = (status == B_OK);
		}
	}

	strncpy(infoPack.team_name, nameFromArgs ? infoPack.team_info.args : info.ref.name,
		B_PATH_NAME_LENGTH - 1);

	if (icon) {
		infoPack.team_icon = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
		if (!tryTrackerIcon
			|| BNodeInfo::GetTrackerIcon(&info.ref, infoPack.team_icon,
				B_MINI_ICON) != B_OK) {
			BMimeType genericAppType(B_APP_MIME_TYPE);
			status = genericAppType.GetIcon(infoPack.team_icon, B_MINI_ICON);
		}
	} else
		infoPack.team_icon = NULL;

	return true;
}


bool
launch(const char* signature, const char* path)
{
	status_t status = be_roster->Launch(signature);
	if (status != B_OK && path) {
		entry_ref ref;
		if (get_ref_for_path(path, &ref) == B_OK)
			status = be_roster->Launch(&ref);
	}
	return status == B_OK;
}


void
mix_colors(rgb_color &target, rgb_color & first, rgb_color & second, float mix)
{
	target.red = (uint8)(second.red * mix + (1. - mix) * first.red);
	target.green = (uint8)(second.green * mix + (1. - mix) * first.green);
	target.blue = (uint8)(second.blue * mix + (1. - mix) * first.blue);
	target.alpha = (uint8)(second.alpha * mix + (1. - mix) * first.alpha);
}


void
find_self(entry_ref& ref)
{
	int32 cookie = 0;
	image_info info;
	while (get_next_image_info (0, &cookie, &info) == B_OK) {
		if (((addr_t)info.text <= (addr_t)move_to_deskbar
			&& (addr_t)info.text + (size_t)info.text_size > (addr_t)move_to_deskbar)
			|| ((addr_t)info.data <= (addr_t)move_to_deskbar
			&& (addr_t)info.data + (size_t)info.data_size > (addr_t)move_to_deskbar)) {
			if (get_ref_for_path (info.name, &ref) == B_OK)
				return;
		}
	}

	// This works, but not always... :(
	app_info appInfo;
	be_roster->GetAppInfo(kSignature, &appInfo);
	ref = appInfo.ref;
}


void
move_to_deskbar(BDeskbar& deskbar)
{
	entry_ref ref;
	find_self(ref);

	deskbar.AddItem(&ref);
}


void
make_window_visible(BWindow* window, bool mayResize)
{
	uint32 flags = window->Flags();
	BRect screen = BScreen(window).Frame();
	BRect frame = window->Frame();
	screen.InsetBy(4, 8);
	screen.OffsetBy(0, 8);

	if (mayResize) {
		float width = frame.Width();
		float height = frame.Height();
		if (screen.Width() < width && !(flags & B_NOT_H_RESIZABLE))
			width = screen.Width();
		if (screen.Height() < height && !(flags & B_NOT_V_RESIZABLE))
			height = screen.Height();
		if (width != frame.Width() || height != frame.Height()) {
			window->ResizeTo(width, height);
			frame.right = frame.left + width;
			frame.bottom = frame.top + height;
		}
	}
	if (frame.right > screen.right)
		window->MoveBy(screen.right-frame.right, 0);
	if (frame.bottom > screen.bottom)
		window->MoveBy(0, screen.bottom-frame.bottom);
	if (frame.left < screen.left)
		window->MoveTo(screen.left, frame.top);
	if (frame.top < screen.top)
		window->MoveBy(0, screen.top-frame.top);
}


BRect
bar_rect(BRect& frame, BFont* font)
{
	BRect rect(frame);
	font_height metrics;
	font->GetHeight(&metrics);
	float barHeight = metrics.ascent;
	rect.top = frame.top + (frame.Height() - barHeight) / 2;
	rect.bottom = frame.top + (frame.Height() + barHeight) / 2;

	rect.left = frame.right - kMargin - kBarWidth;
	rect.right = frame.right - kMargin;

	return rect;
}

