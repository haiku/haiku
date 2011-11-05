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


// route_app_io.cpp

#include "route_app_io.h"

#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaRoster.h>

#include "NodeManager.h"
#include "NodeRef.h"
#include "NodeSetIOContext.h"

__BEGIN_CORTEX_NAMESPACE

// -------------------------------------------------------- //

const char* const _NODE_SET_ELEMENT = "cortex_node_set";
const char* const _UI_STATE_ELEMENT = "cortex_ui_state";

const char* const _DORMANT_NODE_ELEMENT = "dormant_node";
const char* const _KIND_ELEMENT = "kind";
const char* const _FLAVOR_ID_ELEMENT = "flavor_id";
const char* const _CYCLE_ELEMENT = "cycle";
const char* const _FLAG_ELEMENT = "flag";
const char* const _RUN_MODE_ELEMENT = "run_mode";
const char* const _TIME_SOURCE_ELEMENT = "time_source";
const char* const _RECORDING_DELAY_ELEMENT = "recording_delay";
const char* const _CONNECTION_ELEMENT = "connection";
const char* const _OUTPUT_ELEMENT = "output";
const char* const _INPUT_ELEMENT = "input";
const char* const _NODE_GROUP_ELEMENT = "node_group";
const char* const _NAME_ELEMENT = "name";
const char* const _REF_ELEMENT = "ref";
const char* const _LIVE_NODE_ELEMENT = "live_node";

const char* const _AUDIO_INPUT_KEY = "AUDIO_INPUT";
const char* const _AUDIO_OUTPUT_KEY = "AUDIO_OUTPUT";
const char* const _AUDIO_MIXER_KEY = "AUDIO_MIXER";
const char* const _VIDEO_INPUT_KEY = "VIDEO_INPUT";
const char* const _VIDEO_OUTPUT_KEY = "VIDEO_OUTPUT";

__END_CORTEX_NAMESPACE

// -------------------------------------------------------- //
__USE_CORTEX_NAMESPACE
// -------------------------------------------------------- //

void __CORTEX_NAMESPACE__ _write_simple(
	const char* element,
	const char* value,
	ExportContext& context) {

	context.beginElement(element);
	context.beginContent();
	context.writeString(value);
	context.endElement();
}

void __CORTEX_NAMESPACE__ _write_node_kinds(
	int64 kinds,
	ExportContext& context) {

	if(kinds & B_BUFFER_PRODUCER)
		_write_simple(_KIND_ELEMENT, "B_BUFFER_PRODUCER", context);
	if(kinds & B_BUFFER_CONSUMER)
		_write_simple(_KIND_ELEMENT, "B_BUFFER_CONSUMER", context);
	if(kinds & B_TIME_SOURCE)
		_write_simple(_KIND_ELEMENT, "B_TIME_SOURCE", context);
	if(kinds & B_CONTROLLABLE)
		_write_simple(_KIND_ELEMENT, "B_CONTROLLABLE", context);
	if(kinds & B_FILE_INTERFACE)
		_write_simple(_KIND_ELEMENT, "B_FILE_INTERFACE", context);
	if(kinds & B_ENTITY_INTERFACE)
		_write_simple(_KIND_ELEMENT, "B_ENTITY_INTERFACE", context);
	if(kinds & B_PHYSICAL_INPUT)
		_write_simple(_KIND_ELEMENT, "B_PHYSICAL_INPUT", context);
	if(kinds & B_PHYSICAL_OUTPUT)
		_write_simple(_KIND_ELEMENT, "B_PHYSICAL_OUTPUT", context);
	if(kinds & B_SYSTEM_MIXER)
		_write_simple(_KIND_ELEMENT, "B_SYSTEM_MIXER", context);
}

void __CORTEX_NAMESPACE__ _read_node_kind(
	int64& ioKind,
	const char* data,
	ImportContext& context) {

	if(!strcmp(data, "B_BUFFER_PRODUCER"))
		ioKind |= B_BUFFER_PRODUCER;
	else if(!strcmp(data, "B_BUFFER_CONSUMER"))
		ioKind |= B_BUFFER_CONSUMER;
	else if(!strcmp(data, "B_TIME_SOURCE"))
		ioKind |= B_TIME_SOURCE;
	else if(!strcmp(data, "B_CONTROLLABLE"))
		ioKind |= B_CONTROLLABLE;
	else if(!strcmp(data, "B_FILE_INTERFACE"))
		ioKind |= B_FILE_INTERFACE;
	else if(!strcmp(data, "B_ENTITY_INTERFACE"))
		ioKind |= B_ENTITY_INTERFACE;
	else if(!strcmp(data, "B_PHYSICAL_INPUT"))
		ioKind |= B_PHYSICAL_INPUT;
	else if(!strcmp(data, "B_PHYSICAL_OUTPUT"))
		ioKind |= B_PHYSICAL_OUTPUT;
	else if(!strcmp(data, "B_SYSTEM_MIXER"))
		ioKind |= B_SYSTEM_MIXER;
	else {
		BString err;
		err << "_read_noderef_kind(): unknown node kind '" << data << "'\n";
		context.reportWarning(err.String());
	}
}

// fills in either key or outName/kind for the provided
// node.  If the given node is one of the default system nodes,
// an appropriate 'preset' key value will be returned.

status_t __CORTEX_NAMESPACE__ _get_node_signature(
	const NodeManager*			manager,
	const NodeSetIOContext*	context,
	media_node_id						node,
	BString&								outKey,
	BString&								outName,
	int64&									outKind) {

	ASSERT(manager);
	ASSERT(context);
	
	// get ref
	NodeRef* ref;
	status_t err = manager->getNodeRef(node, &ref);
	if(err < B_OK) {
		PRINT((
			"!!! ConnectionIO::_getNodeSignature(): node %ld not found\n",
			node));
		return err;
	}
	
	// check against system-default nodes
	if(ref == manager->audioInputNode()) {
		outKey = _AUDIO_INPUT_KEY;
		return B_OK;
	}
	else if(ref == manager->audioOutputNode()) {
		outKey = _AUDIO_OUTPUT_KEY;
		return B_OK;
	}
	else if(ref == manager->audioMixerNode()) {
		outKey = _AUDIO_MIXER_KEY;
		return B_OK;
	}
	else if(ref == manager->videoInputNode()) {
		outKey = _VIDEO_INPUT_KEY;
		return B_OK;
	}
	else if(ref == manager->videoOutputNode()) {
		outKey = _VIDEO_OUTPUT_KEY;
		return B_OK;
	}
	
	// check context for a key (found if the node has already
	// been exported)
	const char* key;
	err = context->getKeyFor(node, &key);
	if(err == B_OK) {
		outKey = key;
		return B_OK;
	}
	
	// return name/kind signature
	outName = ref->name();
	outKind = ref->kind();
	return B_OK;
}

	
// given a name and kind, looks for a matching node

status_t __CORTEX_NAMESPACE__ _match_node_signature(
	const char*							name,
	int64										kind,
	media_node_id*					outNode) {

	// fetch matching nodes
	BMediaRoster* roster = BMediaRoster::Roster();
	const int32 bufferSize = 64;
	live_node_info buffer[bufferSize];
	int32 count = bufferSize;
	status_t err = roster->GetLiveNodes(
		buffer,
		&count,
		0,
		0,
		name,
		kind); // is this argument supported yet? +++++
			
	if(err < B_OK)
		return err;
	
	for(int32 n = 0; n < count; ++n) {
		PRINT(("# %ld\n", buffer[n].node.node));
		if((buffer[n].node.kind & kind) != kind) {
			PRINT(("# - kind mismatch\n"));
			continue;
		}
		
		*outNode = buffer[n].node.node;
		return B_OK;
	}
	
	return B_NAME_NOT_FOUND;
}

// given a key, looks for a system-default node

status_t __CORTEX_NAMESPACE__ _match_system_node_key(
	const char*							key,
	const NodeManager*			manager,
	media_node_id*					outNode) {
	
	if(!strcmp(key, _AUDIO_INPUT_KEY)) {
		if(manager->audioInputNode()) {
			*outNode = manager->audioInputNode()->id();
			return B_OK;
		}
		return B_NAME_NOT_FOUND;
	}
	else if(!strcmp(key, _AUDIO_OUTPUT_KEY)) {
		if(manager->audioOutputNode()) {
			*outNode = manager->audioOutputNode()->id();
			return B_OK;
		}
		return B_NAME_NOT_FOUND;
	}
	else if(!strcmp(key, _AUDIO_MIXER_KEY)) {
		if(manager->audioMixerNode()) {
			*outNode = manager->audioMixerNode()->id();
			return B_OK;
		}
		return B_NAME_NOT_FOUND;
	}
	else if(!strcmp(key, _VIDEO_INPUT_KEY)) {
		if(manager->videoInputNode()) {
			*outNode = manager->videoInputNode()->id();
			return B_OK;
		}
		return B_NAME_NOT_FOUND;
	}
	else if(!strcmp(key, _VIDEO_OUTPUT_KEY)) {
		if(manager->videoOutputNode()) {
			*outNode = manager->videoOutputNode()->id();
			return B_OK;
		}
		return B_NAME_NOT_FOUND;
	}
	return B_NAME_NOT_FOUND;
}

// adds mappings for the simple string-content elements to the
// given document type

void __CORTEX_NAMESPACE__ _add_string_elements(
	XML::DocumentType*			docType) {

	docType->addMapping(new Mapping<StringContent>(_NAME_ELEMENT));
	docType->addMapping(new Mapping<StringContent>(_KIND_ELEMENT));
	docType->addMapping(new Mapping<StringContent>(_FLAG_ELEMENT));
	docType->addMapping(new Mapping<StringContent>(_FLAVOR_ID_ELEMENT));
	docType->addMapping(new Mapping<StringContent>(_CYCLE_ELEMENT));
	docType->addMapping(new Mapping<StringContent>(_RUN_MODE_ELEMENT));
	docType->addMapping(new Mapping<StringContent>(_TIME_SOURCE_ELEMENT));
	docType->addMapping(new Mapping<StringContent>(_RECORDING_DELAY_ELEMENT));
	docType->addMapping(new Mapping<StringContent>(_REF_ELEMENT));
}

// END -- route_app_io.cpp --

