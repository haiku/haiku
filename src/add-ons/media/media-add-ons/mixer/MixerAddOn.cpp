// MixerAddOn.cpp
//
// David Shipman, 2002
// allows AudioMixer to be used as an addon 
//

#include "AudioMixer.h"
#include "MixerAddOn.h"
#include <cstring>
#include <cstdlib>

// instantiation function
extern "C" _EXPORT BMediaAddOn* make_media_addon(image_id image) {
	return new AudioMixerAddon(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

AudioMixerAddon::~AudioMixerAddon() {}
AudioMixerAddon::AudioMixerAddon(image_id image) :
	BMediaAddOn(image) {}
	
// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t AudioMixerAddon::InitCheck(
	const char** out_failure_text) {
	return B_OK;
}
	
int32 AudioMixerAddon::CountFlavors() {
	return 1;
}

status_t AudioMixerAddon::GetFlavorAt(
	int32 n,
	const flavor_info** out_info) {
	if(n)
		return B_ERROR;
	
	flavor_info* pInfo = new flavor_info;
	pInfo->internal_id = n;
	pInfo->name = "AudioMixer";
	pInfo->info =
		"AudioMixer media addon\n";
		"By David Shipman, 2002";
	pInfo->kinds = B_BUFFER_PRODUCER | B_BUFFER_CONSUMER | B_SYSTEM_MIXER | B_CONTROLLABLE;
	pInfo->flavor_flags = 0;
	pInfo->possible_count = 0;
	
	media_format* pFormat = new media_format;
	pFormat->type = B_MEDIA_RAW_AUDIO;
	pFormat->u.raw_audio = media_raw_audio_format::wildcard;
	
	pInfo->in_format_count = 1;
	pInfo->in_formats = pFormat;
	
	pInfo->out_format_count = 1;
	pInfo->out_formats = pFormat;
	
	*out_info = pInfo;
	return B_OK;
}

BMediaNode* AudioMixerAddon::InstantiateNodeFor(
	const flavor_info* info,
	BMessage* config,
	status_t* out_error) {

	return new AudioMixer(this);	
}

status_t AudioMixerAddon::GetConfigurationFor(
	BMediaNode* your_node,
	BMessage* into_message) {
	
	// no config yet
	return B_OK;
}

// end