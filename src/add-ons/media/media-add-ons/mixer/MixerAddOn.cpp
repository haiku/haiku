/*
 * Copyright 2002 David Shipman,
 * Copyright 2003-2007 Marcus Overhagen
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MixerAddOn.h"

#include <stdio.h>
#include <string.h>

#include <MediaRoster.h>
#include <MediaNode.h>

#include "AudioMixer.h"


// instantiation function
extern "C" _EXPORT BMediaAddOn* make_media_addon(image_id image) {
	return new AudioMixerAddon(image);
}


AudioMixerAddon::AudioMixerAddon(image_id image)
	:
	BMediaAddOn(image),
	fFormat(new media_format),
	fInfo(new flavor_info)
{
	fFormat->type = B_MEDIA_RAW_AUDIO;
	fFormat->u.raw_audio = media_raw_audio_format::wildcard;

	fInfo->internal_id = 0;
	fInfo->name = (char *)"Audio Mixer (Haiku)";
	fInfo->info = (char *)"Haiku Audio Mixer media addon";
	fInfo->kinds = B_BUFFER_PRODUCER | B_BUFFER_CONSUMER | B_SYSTEM_MIXER | B_CONTROLLABLE;
	fInfo->flavor_flags = 0;	// 0 = global or local instantiation allowed, no restrictions
	fInfo->possible_count = 0;	// 0 = infinite
	fInfo->in_format_count = 1;
	fInfo->in_formats = fFormat;
	fInfo->out_format_count = 1;
	fInfo->out_formats = fFormat;
}


AudioMixerAddon::~AudioMixerAddon()
{
	delete fFormat;
	delete fInfo;
}


// #pragma mark - BMediaAddOn implementation


status_t
AudioMixerAddon::InitCheck(const char** out_failure_text)
{
	return B_OK;
}


int32
AudioMixerAddon::CountFlavors()
{
	return 1;
}


status_t
AudioMixerAddon::GetFlavorAt(int32 n, const flavor_info** out_info)
{
	// only the 0th flavor exists
	if (n != 0)
		return B_ERROR;

	*out_info = fInfo;
	return B_OK;
}


BMediaNode *
AudioMixerAddon::InstantiateNodeFor(const flavor_info* info, BMessage* config,
	status_t* out_error)
{
	return new AudioMixer(this, false);
}


status_t
AudioMixerAddon::GetConfigurationFor(BMediaNode* your_node,
	BMessage* into_message)
{
	// no config yet
	return B_ERROR;
}


bool
AudioMixerAddon::WantsAutoStart()
{
	return true;
}


status_t
AudioMixerAddon::AutoStart(int in_index, BMediaNode **out_node,
	int32 *out_internal_id, bool *out_has_more)
{
	*out_has_more = false;

	if (in_index != 0)
		return B_ERROR;

	*out_internal_id = 0;
	AudioMixer *mixer = new AudioMixer(this, true);
	*out_node = mixer;

	return B_OK;
}
