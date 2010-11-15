/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MediaIcons.h"

#include <Application.h>
#include <File.h>
#include <Resources.h>
#include <Roster.h>

#include "IconHandles.h"


const BRect MediaIcons::sBounds(0, 0, 15, 15);


MediaIcons::IconSet::IconSet()
	:
	inputIcon(MediaIcons::sBounds, B_CMAP8),
	outputIcon(MediaIcons::sBounds, B_CMAP8)
{
}



MediaIcons::MediaIcons()
	:
	devicesIcon(sBounds, B_CMAP8),
	mixerIcon(sBounds, B_CMAP8)
{
	app_info info;
	be_app->GetAppInfo(&info);
	BFile executableFile(&info.ref, B_READ_ONLY);
	BResources resources(&executableFile);
	resources.PreloadResourceType(B_COLOR_8_BIT_TYPE);

	_LoadBitmap(&resources, devices_icon, &devicesIcon);
	_LoadBitmap(&resources, mixer_icon, &mixerIcon);
	_LoadBitmap(&resources, tv_icon, &videoIcons.outputIcon);
	_LoadBitmap(&resources, cam_icon, &videoIcons.inputIcon);
	_LoadBitmap(&resources, mic_icon, &audioIcons.inputIcon);
	_LoadBitmap(&resources, speaker_icon, &audioIcons.outputIcon);
}


void
MediaIcons::_LoadBitmap(BResources* resources, int32 id, BBitmap* bitmap)
{
	size_t size;
	const void* bits = resources->LoadResource(B_COLOR_8_BIT_TYPE, id, &size);
	bitmap->SetBits(bits, size, 0, B_CMAP8);
}


BRect
MediaIcons::IconRectAt(const BPoint& topLeft)
{
	return BRect(sBounds).OffsetToSelf(topLeft);
}
