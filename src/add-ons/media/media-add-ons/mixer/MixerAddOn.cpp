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

// ------------------------------------------------------- //
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
	
	flavor_info* fInfo = new flavor_info;
	fInfo->internal_id = n;
	fInfo->name = "AudioMixer";
	fInfo->info = "AudioMixer media addon\n";
	fInfo->kinds = B_BUFFER_PRODUCER | B_BUFFER_CONSUMER | B_SYSTEM_MIXER | B_CONTROLLABLE;
	fInfo->flavor_flags = 0;	// 0 = global or local instantiation allowed, no restrictions
	fInfo->possible_count = 0;	// 0 = infinite
	
	media_format* fFormat = new media_format;
	fFormat->type = B_MEDIA_RAW_AUDIO;
	fFormat->u.raw_audio = media_raw_audio_format::wildcard;
	
	fInfo->in_format_count = 1;
	fInfo->in_formats = fFormat;
	
	fInfo->out_format_count = 1;
	fInfo->out_formats = fFormat;
	
	*out_info = fInfo;
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
	return B_ERROR;
}

// end
