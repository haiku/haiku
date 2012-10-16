/*
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Entry.h>

#include "EqualizerNode.h"
#include "EqualizerAddOn.h"

extern "C" _EXPORT BMediaAddOn* make_media_addon(image_id image)
{
	return new EqualizerAddOn(image);
}

EqualizerAddOn::~EqualizerAddOn()
{	
}

EqualizerAddOn::EqualizerAddOn(image_id image) 
	:
	BMediaAddOn(image)
{
}
	
status_t 
EqualizerAddOn::InitCheck(const char **text) 
{
	return B_OK;
}
	
int32 
EqualizerAddOn::CountFlavors()
{
	return 1;
}

status_t EqualizerAddOn::GetFlavorAt(int32 idx, const flavor_info** info)
{
	if (idx < 0 || idx >= CountFlavors())
		return B_ERROR;
	
	flavor_info* f_info = new flavor_info;
	f_info->internal_id = idx;
	f_info->kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER | B_CONTROLLABLE;
	f_info->possible_count = 0;
	f_info->flavor_flags = 0;
	f_info->info = (char *)"10 Band Equalizer.\nby 3dEyes**";
	f_info->name = (char *)"Equalizer (10 Band)";
	
	media_format* format = new media_format;
	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio = media_raw_audio_format::wildcard;
	format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	
	f_info->in_format_count = 1;
	f_info->in_formats = format;

	format = new media_format;
	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio = media_raw_audio_format::wildcard;
	format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;

	f_info->out_format_count = 1;
	f_info->out_formats = format;
	
	*info = f_info;
	
	return B_OK;
}

BMediaNode* 
EqualizerAddOn::InstantiateNodeFor(const flavor_info* info, BMessage* config,
	status_t* err) 
{
	EqualizerNode* node = new EqualizerNode(this);
	return node;
}

status_t 
EqualizerAddOn::GetConfigurationFor(BMediaNode* node, BMessage* message)
{
	return B_OK;
}

bool
EqualizerAddOn::WantsAutoStart()
{ 
	return false; 
}

status_t 	
EqualizerAddOn::AutoStart(int count, BMediaNode** node,	int32* id, bool* more)
{ 
	return B_OK; 
}
