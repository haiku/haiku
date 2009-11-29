// EndpointInfo.cpp
// ----------------
// Implements the EndpointInfo object.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

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
	: m_id(-1), m_icon(NULL)
{}

EndpointInfo::EndpointInfo(int32 id)
	: m_id(id), m_icon(NULL)
{
	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (roster) {
		BMidiEndpoint* endpoint = roster->FindEndpoint(id);
		if (endpoint) {
			printf("endpoint %ld = %p\n", id, endpoint);
			BMessage msg;
			if (endpoint->GetProperties(&msg) == B_OK) {
				m_icon = CreateIcon(&msg, DISPLAY_ICON_SIZE);
			}
			endpoint->Release();
		}
	}	
}

EndpointInfo::EndpointInfo(const EndpointInfo& info)
	: m_id(info.m_id)
{
	m_icon = (info.m_icon) ? new BBitmap(info.m_icon) : NULL;	
}

EndpointInfo& EndpointInfo::operator=(const EndpointInfo& info)
{
	if (&info != this) {
		m_id = info.m_id;
		delete m_icon;
		m_icon = (info.m_icon) ? new BBitmap(info.m_icon) : NULL;
	}
	return *this;
}

EndpointInfo::~EndpointInfo()
{
	delete m_icon;
}

void EndpointInfo::UpdateProperties(const BMessage* props)
{
	delete m_icon;
	m_icon = CreateIcon(props, DISPLAY_ICON_SIZE);
}

static BBitmap* CreateIcon(const BMessage* msg, icon_size which)
{
	float iconSize;
	uint32 iconType;
	const char* iconName;
	
	if (which == B_LARGE_ICON) {
		iconSize = LARGE_ICON_SIZE;
		iconType = LARGE_ICON_TYPE;
		iconName = LARGE_ICON_NAME;
	} else if (which == B_MINI_ICON) {
		iconSize = MINI_ICON_SIZE;
		iconType = MINI_ICON_TYPE;
		iconName = MINI_ICON_NAME;
	} else {
		return NULL;
	}
							
	const void* data;
	ssize_t size;
	BBitmap* bitmap = NULL;

#ifdef __HAIKU__
	iconSize = LARGE_ICON_SIZE;
	
	if (msg->FindData(VECTOR_ICON_NAME, VECTOR_ICON_TYPE, &data, 
		&size) == B_OK)  {
		BRect r(0, 0, iconSize-1, iconSize-1);
		bitmap = new BBitmap(r, B_RGBA32);
		if (BIconUtils::GetVectorIcon((const uint8*)data, size, 
			bitmap) == B_OK) {
			printf("Created vector icon bitmap\n");
			return bitmap;
		} else 
			delete bitmap;
	}
#endif

	if (msg->FindData(iconName, iconType, &data, &size) == B_OK)
	{
		BRect r(0, 0, iconSize-1, iconSize-1);
		bitmap = new BBitmap(r, ICON_COLOR_SPACE);
		ASSERT((bitmap->BitsLength() == size));
		memcpy(bitmap->Bits(), data, size);
	}
	return bitmap;
}
