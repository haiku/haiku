// AudioAdapterAddOn.cpp

#include "AudioAdapterAddOn.h"

#include "AudioAdapterNode.h"
//#include "AudioFilterNode.h"
#include "AudioAdapterOp.h"

#include <Entry.h>
#include <Debug.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// -------------------------------------------------------- //
// instantiation function
// -------------------------------------------------------- //

extern "C" _EXPORT BMediaAddOn* make_media_addon(image_id image) {
	return new AudioAdapterAddOn(image);
}

// -------------------------------------------------------- //
// main() stub
// -------------------------------------------------------- //

int main() {
	fputs("[AudioAdapter.media_addon]", stderr);
	return 1;
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

//AudioAdapterAddOn::~AudioAdapterAddOn() {}
AudioAdapterAddOn::AudioAdapterAddOn(image_id id) :
	BMediaAddOn(id) {}

// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t AudioAdapterAddOn::InitCheck(
	const char** out_failure_text) {
	return B_OK;
}
	
int32 AudioAdapterAddOn::CountFlavors() {
	return 1;
}

status_t AudioAdapterAddOn::GetFlavorAt(
	int32 n,
	const flavor_info** out_info) {
	if(n)
		return B_ERROR;
	
	flavor_info* pInfo = new flavor_info;
	pInfo->internal_id = n;
	pInfo->name = "AudioAdapter";
	pInfo->info =
		"AudioAdapter (generic raw-audio format conversion).\n"
		"by Eric Moon (8 September 1999)";
	pInfo->kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER | B_CONTROLLABLE;
	pInfo->flavor_flags = 0;
	pInfo->possible_count = 0;
	
	pInfo->in_format_count = 1;
	media_format* pFormat = new media_format;
	pFormat->type = B_MEDIA_RAW_AUDIO;
	pFormat->u.raw_audio = media_raw_audio_format::wildcard;
	pInfo->in_formats = pFormat;

	pInfo->out_format_count = 1;
	pFormat = new media_format;
	pFormat->type = B_MEDIA_RAW_AUDIO;
	pFormat->u.raw_audio = media_raw_audio_format::wildcard;
	pInfo->out_formats = pFormat;
	
	*out_info = pInfo;
	return B_OK;
}

BMediaNode* AudioAdapterAddOn::InstantiateNodeFor(
	const flavor_info* info,
	BMessage* config,
	status_t* out_error) {

	return new _AudioAdapterNode(
		"AudioAdapter",
		new AudioAdapterOpFactory(),
		this);
}

status_t AudioAdapterAddOn::GetConfigurationFor(
	BMediaNode* your_node,
	BMessage* into_message) {
	
	// no config yet
	return B_OK;
}


// END -- AudioAdapterAddOn.cpp --