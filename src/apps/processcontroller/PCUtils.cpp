/*
	PCUtils.cpp

	ProcessController © 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
*/

#include "PCUtils.h"
#include "PCWorld.h"
#include "PCView.h"
#include "icons.data"

#include <Alert.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>

#include <stdio.h>
#include <string.h>


#define snooze_time 1000000

team_id	gAppServerTeamID = -1;


bool
get_team_name_and_icon(infosPack& infopack, bool icon)
{
	if (gAppServerTeamID == -1 && !strcmp(infopack.tminfo.args, "/boot/beos/system/servers/app_server"))
		gAppServerTeamID = infopack.tminfo.team;

	bool nameFromArgs = false;
	bool tryTrackerIcon = true;

	for (int len = strlen(infopack.tminfo.args) - 1; len >= 0 && infopack.tminfo.args[len] == ' '; len--) {
		infopack.tminfo.args[len] = 0;
	}

	app_info info;
	status_t status = be_roster->GetRunningAppInfo(infopack.tminfo.team, &info);
	if (status == B_OK || infopack.tminfo.team == B_SYSTEM_TEAM) {
		if (infopack.tminfo.team == B_SYSTEM_TEAM) {
			BPath	kernel;
			find_directory(B_BEOS_SYSTEM_DIRECTORY, &kernel, true);
			system_info	sinfo;
			get_system_info(&sinfo);
			kernel.Append(sinfo.kernel_name);
			get_ref_for_path(kernel.Path(), &info.ref);
			nameFromArgs = true;
		}
	} else {
		BEntry	entry(infopack.tminfo.args, true);
		status = entry.GetRef(&info.ref);
		if (status != B_OK || strncmp(infopack.tminfo.args, "/boot/beos/system/", 18) != 0)
			nameFromArgs = true;
		tryTrackerIcon = (status == B_OK);
	}

	strncpy(infopack.tmname, nameFromArgs ? infopack.tminfo.args : info.ref.name,
		B_PATH_NAME_LENGTH - 1);

	if (icon) {
		infopack.tmicon = new BBitmap(BRect(0, 0, 15, 15), B_COLOR_8_BIT);
		if (!tryTrackerIcon || BNodeInfo::GetTrackerIcon(&info.ref, infopack.tmicon, B_MINI_ICON) != B_OK)
			infopack.tmicon->SetBits(k_app_mini, 256, 0, B_COLOR_8_BIT);
	} else
		infopack.tmicon = NULL;

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
		if (((uint32) info.text <= (uint32) move_to_deskbar
			&& (uint32) info.text + (uint32) info.text_size > (uint32) move_to_deskbar)
			|| ((uint32) info.data <= (uint32) move_to_deskbar
			&& (uint32) info.data + (uint32) info.data_size > (uint32) move_to_deskbar)) {
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
