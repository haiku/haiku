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


// MediaIcon.cpp

#include "MediaIcon.h"
#include "MediaIconBits.h"
#include "AddOnHostProtocol.h"

// Application Kit
#include <Application.h>
#include <Roster.h>
// Media Kit
#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <MediaAddOn.h>
// Storage Kit
#include <NodeInfo.h>
// Support Kit
#include <String.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ALLOC(x) //PRINT (x)
#define D_INTERNAL(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

MediaIcon::MediaIcon(
	const live_node_info &nodeInfo,
	icon_size size)
	: BBitmap(BRect(0.0, 0.0, size - 1.0, size - 1.0), B_CMAP8),
	  m_size(size),
	  m_nodeKind(nodeInfo.node.kind) {
	D_ALLOC(("MediaIcon::MediaIcon(live_node_info '%s')\n", nodeInfo.name));

	_findIconFor(nodeInfo);
}

MediaIcon::MediaIcon(
	const dormant_node_info &nodeInfo,
	icon_size size)
	: BBitmap(BRect(0.0, 0.0, size - 1.0, size - 1.0), B_CMAP8),
	  m_size(size),
	  m_nodeKind(0) {
	D_ALLOC(("MediaIcon::MediaIcon(dormant_node_info '%s')\n", nodeInfo.name));

	_findIconFor(nodeInfo);
}

MediaIcon::~MediaIcon() {
	D_ALLOC(("MediaIcon::~MediaIcon()\n"));

}

// -------------------------------------------------------- //
// *** internal accessors (private)
// -------------------------------------------------------- //

bool MediaIcon::_isPhysicalInput() const {
	D_INTERNAL(("MediaIcon::_isPhysicalInput()\n"));

	return ((m_nodeKind & B_PHYSICAL_INPUT)
	     && (m_nodeKind & B_BUFFER_PRODUCER));
}

bool MediaIcon::_isPhysicalOutput() const {
	D_INTERNAL(("MediaIcon::_isPhysicalOutput()\n"));

	return ((m_nodeKind & B_PHYSICAL_OUTPUT)
	     && (m_nodeKind & B_BUFFER_CONSUMER));;
}

bool MediaIcon::_isProducer() const {
	D_INTERNAL(("MediaIcon::_isProducer()\n"));

	return (!(m_nodeKind & B_BUFFER_CONSUMER) && 
			 (m_nodeKind & B_BUFFER_PRODUCER) && 
			!(m_nodeKind & B_PHYSICAL_INPUT)  &&
			!(m_nodeKind & B_PHYSICAL_OUTPUT));
}

bool MediaIcon::_isFilter() const {
	D_INTERNAL(("MediaIcon::_isFilter()\n"));

	return ( (m_nodeKind & B_BUFFER_CONSUMER) && 
			 (m_nodeKind & B_BUFFER_PRODUCER) && 
		    !(m_nodeKind & B_PHYSICAL_INPUT)  &&
		    !(m_nodeKind & B_PHYSICAL_OUTPUT));
}

bool MediaIcon::_isConsumer() const {
	D_INTERNAL(("MediaIcon::_isConsumer()\n"));

	return ( (m_nodeKind & B_BUFFER_CONSUMER) && 
		    !(m_nodeKind & B_BUFFER_PRODUCER) && 
		    !(m_nodeKind & B_PHYSICAL_INPUT)  &&
		    !(m_nodeKind & B_PHYSICAL_OUTPUT));
}

bool MediaIcon::_isSystemMixer() const {
	D_INTERNAL(("MediaIcon::_isSystemMixer()\n"));

	return (m_nodeKind & B_SYSTEM_MIXER);
}

bool MediaIcon::_isTimeSource() const {
	D_INTERNAL(("MediaIcon::_isTimeSource()\n"));

	return ((m_nodeKind & B_TIME_SOURCE) &&
		   !(m_nodeKind & B_PHYSICAL_INPUT) &&
		   !(m_nodeKind & B_PHYSICAL_OUTPUT));
}

// -------------------------------------------------------- //
// *** internal operations (private)
// -------------------------------------------------------- //

void MediaIcon::_findIconFor(
	const live_node_info &nodeInfo) {
	D_INTERNAL(("MediaIcon::_findIconFor(live_node_info)\n"));

	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (m_nodeKind & B_FILE_INTERFACE) {
		entry_ref ref;
		if ((roster && (roster->GetRefFor(nodeInfo.node, &ref) == B_OK))
		 && (BNodeInfo::GetTrackerIcon(&ref, this, m_size) == B_OK)) {
			return;
		}
	}
	dormant_node_info dormantNodeInfo;
	if  (roster
	 && (roster->GetDormantNodeFor(nodeInfo.node, &dormantNodeInfo) == B_OK)) {
		D_INTERNAL((" -> instantiated from dormant node\n"));
		_findIconFor(dormantNodeInfo);
	}
	else {
		D_INTERNAL((" -> application internal node\n"));
		port_info portInfo;
		app_info appInfo;
		if ((get_port_info(nodeInfo.node.port, &portInfo) == B_OK)
		 && (be_roster->GetRunningAppInfo(portInfo.team, &appInfo) == B_OK)) {
			D_INTERNAL((" -> application info found: %s\n", appInfo.signature));
			app_info thisAppInfo;
			if ((be_app->GetAppInfo(&thisAppInfo) != B_OK)
			 || ((strcmp(appInfo.signature, thisAppInfo.signature) != 0)
			 && (strcmp(appInfo.signature, addon_host::g_appSignature) != 0))) {
				// only use app icon if the node doesn't belong to our team
				// or the addon-host
				BNodeInfo::GetTrackerIcon(&appInfo.ref, this, m_size);
				return;
			}
		}
		bool audioIn = false, audioOut = false, videoIn = false, videoOut = false;
		_getMediaTypesFor(nodeInfo, &audioIn, &audioOut, &videoIn, &videoOut);
		_findDefaultIconFor(audioIn, audioOut, videoIn, videoOut);
	}
}


void
MediaIcon::_findIconFor(const dormant_node_info &nodeInfo)
{
	D_INTERNAL(("MediaIcon::_findIconFor(dormant_node_info)\n"));

	dormant_flavor_info flavorInfo;
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	status_t error = roster->GetDormantFlavorInfoFor(nodeInfo, &flavorInfo);
	if (error == B_OK) {
		m_nodeKind = flavorInfo.kinds;
		bool audioIn = false, audioOut = false;
		bool videoIn = false, videoOut = false;
		_getMediaTypesFor(flavorInfo, &audioIn, &audioOut, &videoIn, &videoOut);
		_findDefaultIconFor(audioIn, audioOut, videoIn, videoOut);
	} else {
		// use generic icon in case we couldn't get any info
		if (m_size == B_LARGE_ICON)
			SetBits(M_GENERIC_ICON.large, 1024, 0, B_CMAP8);
		else if (m_size == B_MINI_ICON)
			SetBits(M_GENERIC_ICON.small, 256, 0, B_CMAP8);
	}
}


void
MediaIcon::_getMediaTypesFor(const live_node_info &nodeInfo, bool *audioIn,
	bool *audioOut, bool *videoIn, bool *videoOut)
{
	D_INTERNAL(("MediaIcon::_getMediaTypeFor(live_node_info)\n"));

	// get the media_types supported by this node
	const int32 numberOfInputs = 4;
	int32 numberOfFreeInputs, numberOfConnectedInputs;
	media_input inputs[numberOfInputs];
	const int32 numberOfOutputs = 4;
	int32 numberOfFreeOutputs, numberOfConnectedOutputs;
	media_output outputs[numberOfOutputs];
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (roster->GetFreeInputsFor(nodeInfo.node, inputs, numberOfInputs, &numberOfFreeInputs) == B_OK) {
		for (int32 i = 0; i < numberOfFreeInputs; i++) {
			if ((inputs[i].format.type == B_MEDIA_RAW_AUDIO)
			 || (inputs[i].format.type == B_MEDIA_ENCODED_AUDIO)) {
				*audioIn = true;
				continue;
			}
			if ((inputs[i].format.type == B_MEDIA_RAW_VIDEO)
			 || (inputs[i].format.type == B_MEDIA_ENCODED_VIDEO)) {
				*videoIn = true;
			}
		}
	}
	if (roster->GetConnectedInputsFor(nodeInfo.node, inputs, numberOfInputs, &numberOfConnectedInputs) == B_OK) {
		for (int32 i = 0; i < numberOfConnectedInputs; i++) {
			if ((inputs[i].format.type == B_MEDIA_RAW_AUDIO)
			 || (inputs[i].format.type == B_MEDIA_ENCODED_AUDIO)) {
				*audioIn = true;
				continue;
			}
			if ((inputs[i].format.type == B_MEDIA_RAW_VIDEO)
			 || (inputs[i].format.type == B_MEDIA_ENCODED_VIDEO)) {
				*videoIn = true;
			}
		}
	}
	if (roster->GetFreeOutputsFor(nodeInfo.node, outputs, numberOfOutputs, &numberOfFreeOutputs) == B_OK) {
		for (int32 i = 0; i < numberOfFreeOutputs; i++) {
			if ((outputs[i].format.type == B_MEDIA_RAW_AUDIO)
			 || (outputs[i].format.type == B_MEDIA_ENCODED_AUDIO)) {
				*audioOut = true;
				continue;
			}
			if ((outputs[i].format.type == B_MEDIA_RAW_VIDEO)
			 || (outputs[i].format.type == B_MEDIA_ENCODED_VIDEO)) {
				*videoOut = true;
			}
		}
	}
	if (roster->GetConnectedOutputsFor(nodeInfo.node, outputs, numberOfOutputs, &numberOfConnectedOutputs) == B_OK) {
		for (int32 i = 0; i < numberOfConnectedOutputs; i++) {
			if ((outputs[i].format.type == B_MEDIA_RAW_AUDIO)
			 || (outputs[i].format.type == B_MEDIA_ENCODED_AUDIO)) {
				*audioOut = true;
				continue;
			}
			if ((outputs[i].format.type == B_MEDIA_RAW_VIDEO)
			 || (outputs[i].format.type == B_MEDIA_ENCODED_VIDEO)) {
				*videoOut = true;
			}
		}
	}
}

void MediaIcon::_getMediaTypesFor(
	const dormant_flavor_info &flavorInfo,
	bool *audioIn,
	bool *audioOut,
	bool *videoIn,
	bool *videoOut) {
	D_INTERNAL(("MediaIcon::_getMediaTypeFor(dormant_flavor_info)\n"));

	for (int32 i = 0; i < flavorInfo.in_format_count; i++) {
		if ((flavorInfo.in_formats[i].type == B_MEDIA_RAW_AUDIO)
		 || (flavorInfo.in_formats[i].type == B_MEDIA_ENCODED_AUDIO)) {
			*audioIn = true;
			continue;
		}
		if ((flavorInfo.in_formats[i].type == B_MEDIA_RAW_VIDEO)
		 || (flavorInfo.in_formats[i].type == B_MEDIA_ENCODED_VIDEO)) {
			*videoIn = true;
		}
	}
	for (int32 i = 0; i < flavorInfo.out_format_count; i++)	{
		if ((flavorInfo.out_formats[i].type == B_MEDIA_RAW_AUDIO)
		 || (flavorInfo.out_formats[i].type == B_MEDIA_ENCODED_AUDIO)) {
			*audioOut = true;
			continue;
		}
		if ((flavorInfo.out_formats[i].type == B_MEDIA_RAW_VIDEO)
		 || (flavorInfo.out_formats[i].type == B_MEDIA_ENCODED_VIDEO)) {
			*videoOut = true;
		}
	}
}

void MediaIcon::_findDefaultIconFor(
	bool audioIn,
	bool audioOut,
	bool videoIn,
	bool videoOut) {
	D_INTERNAL(("MediaIcon::_findDefaultIcon()\n"));

	if (_isTimeSource()) {
		if (m_size == B_LARGE_ICON)
			SetBits(M_TIME_SOURCE_ICON.large, 1024, 0, B_CMAP8);
		else if (m_size == B_MINI_ICON)
			SetBits(M_TIME_SOURCE_ICON.small, 256, 0, B_CMAP8);
		return;
	}

	if (_isSystemMixer()) {
		if (m_size == B_LARGE_ICON)
			SetBits(M_AUDIO_MIXER_ICON.large, 1024, 0, B_CMAP8);
		else if (m_size == B_MINI_ICON)
			SetBits(M_AUDIO_MIXER_ICON.small, 256, 0, B_CMAP8);
		return;
	}

	if (m_nodeKind & B_FILE_INTERFACE) {
		if (_isProducer()) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_FILE_READER_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_FILE_READER_ICON.small, 256, 0, B_CMAP8);
			return;
		}
		else {
			if (m_size == B_LARGE_ICON)
				SetBits(M_FILE_WRITER_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_FILE_WRITER_ICON.small, 256, 0, B_CMAP8);
			return;
		}
	}

	if (_isPhysicalInput()) {
		if (audioOut) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_AUDIO_INPUT_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_AUDIO_INPUT_ICON.small, 256, 0, B_CMAP8);
			return;
		}
		else if (videoOut) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_VIDEO_INPUT_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_VIDEO_INPUT_ICON.small, 256, 0, B_CMAP8);
			return;
		}
	}
	
	if (_isPhysicalOutput()) {
		if (audioIn) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_AUDIO_OUTPUT_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_AUDIO_OUTPUT_ICON.small, 256, 0, B_CMAP8);
			return;
		}
		else if (videoIn) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_VIDEO_OUTPUT_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_VIDEO_OUTPUT_ICON.small, 256, 0, B_CMAP8);
			return;
		}
	}

	if (_isProducer()) {
		if (audioOut) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_AUDIO_PRODUCER_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_AUDIO_PRODUCER_ICON.small, 256, 0, B_CMAP8);
			return;
		}
		else if (videoOut) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_VIDEO_PRODUCER_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_VIDEO_PRODUCER_ICON.small, 256, 0, B_CMAP8);
			return;
		}
	}

	if (_isFilter()) {
		if (audioIn && audioOut && !videoIn && !videoOut) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_AUDIO_FILTER_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_AUDIO_FILTER_ICON.small, 256, 0, B_CMAP8);
			return;
		}
		else if (audioIn && !videoIn && videoOut) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_AUDIO_CONSUMER_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_AUDIO_CONSUMER_ICON.small, 256, 0, B_CMAP8);
			return;
		}
		else if (!audioIn && !audioOut && videoIn && videoOut) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_VIDEO_FILTER_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_VIDEO_FILTER_ICON.small, 256, 0, B_CMAP8);
			return;
		}
	}

	if (_isConsumer()) {
		if (audioIn) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_AUDIO_CONSUMER_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_AUDIO_CONSUMER_ICON.small, 256, 0, B_CMAP8);
			return;
		}
		else if (videoIn) {
			if (m_size == B_LARGE_ICON)
				SetBits(M_VIDEO_CONSUMER_ICON.large, 1024, 0, B_CMAP8);
			else if (m_size == B_MINI_ICON)
				SetBits(M_VIDEO_CONSUMER_ICON.small, 256, 0, B_CMAP8);
			return;
		}
	}

	// assign a default icon
	if (m_size == B_LARGE_ICON)
		SetBits(M_GENERIC_ICON.large, 1024, 0, B_CMAP8);
	else if (m_size == B_MINI_ICON)
		SetBits(M_GENERIC_ICON.small, 256, 0, B_CMAP8);
}
