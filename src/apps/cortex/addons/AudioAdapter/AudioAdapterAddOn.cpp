/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
	pInfo->name = (char *)"AudioAdapter";
	pInfo->info = (char *)
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
