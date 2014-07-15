/* EndpointInfo.cpp
 * ----------------
 * Implements the EndpointInfo object.
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#include "EndpointInfo.h"

#include <Bitmap.h>
#include <Debug.h>
#include <IconUtils.h>
#include <Message.h>
#include <MidiRoster.h>
#include <MidiEndpoint.h>

const char* LARGE_ICON_NAME = "be:large_icon";
const char* MINI_ICON_NAME = "be:mini_icon";
const char* VECTOR_ICON_NAME = "icon";
const uint32 LARGE_ICON_TYPE = 'ICON';
const uint32 MINI_ICON_TYPE = 'MICN';
const uint32 VECTOR_ICON_TYPE = 'VICN';
extern const uint8 LARGE_ICON_SIZE = 32;
extern const uint8 MINI_ICON_SIZE = 16;
extern const icon_size DISPLAY_ICON_SIZE = B_LARGE_ICON;
extern const color_space ICON_COLOR_SPACE = B_CMAP8;

static BBitmap* CreateIcon(const BMessage* msg, icon_size which);


EndpointInfo::EndpointInfo()
	:
	fId(-1),
	fIcon(NULL)
{}


EndpointInfo::EndpointInfo(int32 id)
	:
	fId(id),
	fIcon(NULL)
{
	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (roster != NULL) {
		BMidiEndpoint* endpoint = roster->FindEndpoint(id);
		if (endpoint != NULL) {
			printf("endpoint %" B_PRId32 " = %p\n", id, endpoint);
			BMessage msg;
			if (endpoint->GetProperties(&msg) == B_OK) {
				fIcon = CreateIcon(&msg, DISPLAY_ICON_SIZE);
			}
			endpoint->Release();
		}
	}
}


EndpointInfo::EndpointInfo(const EndpointInfo& info)
	:
	fId(info.fId)
{
	fIcon = (info.fIcon) ? new BBitmap(info.fIcon) : NULL;
}


EndpointInfo&
EndpointInfo::operator=(const EndpointInfo& info)
{
	if (&info != this) {
		fId = info.fId;
		delete fIcon;
		fIcon = (info.fIcon) ? new BBitmap(info.fIcon) : NULL;
	}
	return *this;
}


EndpointInfo::~EndpointInfo()
{
	delete fIcon;
}


void
EndpointInfo::UpdateProperties(const BMessage* props)
{
	delete fIcon;
	fIcon = CreateIcon(props, DISPLAY_ICON_SIZE);
}


static BBitmap*
CreateIcon(const BMessage* msg, icon_size which)
{

	const void* data;
	ssize_t size;
	BBitmap* bitmap = NULL;

	// See if a Haiku Vector Icon available
	if (msg->FindData(VECTOR_ICON_NAME, VECTOR_ICON_TYPE, &data,
		&size) == B_OK)  {
		BRect r(0, 0, LARGE_ICON_SIZE - 1, LARGE_ICON_SIZE - 1);
		bitmap = new BBitmap(r, B_RGBA32);
		if (BIconUtils::GetVectorIcon((const uint8*)data, size,
			bitmap) == B_OK) {
			printf("Created vector icon bitmap\n");
			return bitmap;
		} else {
			delete bitmap;
			bitmap = NULL;
		}
	}

	// If not, look for BeOS style icon
	float bmapSize;
	uint32 iconType;
	const char* iconName;

	if (which == B_LARGE_ICON) {
		bmapSize = LARGE_ICON_SIZE - 1;
		iconType = LARGE_ICON_TYPE;
		iconName = LARGE_ICON_NAME;
	} else if (which == B_MINI_ICON) {
		bmapSize = MINI_ICON_SIZE - 1;
		iconType = MINI_ICON_TYPE;
		iconName = MINI_ICON_NAME;
	} else
		return NULL;

	if (msg->FindData(iconName, iconType, &data, &size) == B_OK) {
		bitmap = new BBitmap(BRect(0, 0, bmapSize, bmapSize),
			ICON_COLOR_SPACE);
		ASSERT((bitmap->BitsLength() == size));
		memcpy(bitmap->Bits(), data, size);
	}
	return bitmap;
}
