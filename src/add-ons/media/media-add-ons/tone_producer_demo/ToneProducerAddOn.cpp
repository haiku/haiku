/* Written by Eric Moon, from the Cortex Source code archive.
 * Distributed under the terms of the Be Sample Code License
 */

// ToneProducerAddOn.cpp
// e.moon 4jun99

#include "ToneProducer.h"
#include "ToneProducerAddOn.h"
#include <cstring>
#include <cstdlib>

// instantiation function
extern "C" _EXPORT BMediaAddOn* make_media_addon(image_id image) {
	return new ToneProducerAddOn(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

ToneProducerAddOn::~ToneProducerAddOn() {}
ToneProducerAddOn::ToneProducerAddOn(image_id image) :
	BMediaAddOn(image) {}
	
// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t ToneProducerAddOn::InitCheck(
	const char** out_failure_text) {
	return B_OK;
}
	
int32 ToneProducerAddOn::CountFlavors() {
	return 1;
}

status_t ToneProducerAddOn::GetFlavorAt(
	int32 n,
	const flavor_info** out_info) {
	if(n)
		return B_ERROR;
	
	flavor_info* pInfo = new flavor_info;
	pInfo->internal_id = n;
	pInfo->name = (char *)"Demo Audio Producer";
	pInfo->info = (char *)
		"An add-on version of the ToneProducer node.\n"
		"See the Be Developer Newsletter: 2 June, 1999\n"
		"adapted by Eric Moon (4 June, 1999)";
	pInfo->kinds = B_BUFFER_PRODUCER | B_CONTROLLABLE;
	pInfo->flavor_flags = 0;
	pInfo->possible_count = 0;
	
	pInfo->in_format_count = 0;
	pInfo->in_formats = 0;
	
	pInfo->out_format_count = 1;
	media_format* pFormat = new media_format;
	pFormat->type = B_MEDIA_RAW_AUDIO;
	pFormat->u.raw_audio = media_raw_audio_format::wildcard;
	pInfo->out_formats = pFormat;
	
	*out_info = pInfo;
	return B_OK;
}

BMediaNode* ToneProducerAddOn::InstantiateNodeFor(
	const flavor_info* info,
	BMessage* config,
	status_t* out_error) {

	return new ToneProducer(this);	
}

status_t ToneProducerAddOn::GetConfigurationFor(
	BMediaNode* your_node,
	BMessage* into_message) {
	
	// no config yet
	return B_OK;
}

// END -- ToneProducerAddOn.cpp
