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


// DormantNodeIO.cpp

#include "DormantNodeIO.h"
#include "ImportContext.h"
#include "ExportContext.h"
#include "StringContent.h"

#include "NodeManager.h"

#include <Debug.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>
#include <MediaRoster.h>
#include <Path.h>

#include <cstdlib>

#include "route_app_io.h"

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

DormantNodeIO::~DormantNodeIO() {}

// initialize for import (to defaults)
DormantNodeIO::DormantNodeIO() :
	m_kinds(0LL),
	m_flavorID(0),
	m_flags(0),
	m_runMode(0),
	m_recordingDelay(0),
	m_cycle(false),
	m_exportValid(false) {}

// initialize for export
DormantNodeIO::DormantNodeIO(
	NodeRef*								ref,
	const char*							nodeKey) :
	m_exportValid(false) {
	
	ASSERT(ref);
	ASSERT(nodeKey);
	status_t err;

	m_nodeKey = nodeKey;

	// * extract dormant-node info 	
	dormant_node_info info;
	err = ref->getDormantNodeInfo(&info);
	if(err < B_OK) {
		PRINT((	
			"!!! DormantNodeIO(): getDormantNodeInfo() failed:\n"
			"    %s\n",
			strerror(err)));
		return;
	}

	dormant_flavor_info flavorInfo;
	err = BMediaRoster::Roster()->GetDormantFlavorInfoFor(
		info, &flavorInfo);
	if(err < B_OK) {
		PRINT((	
			"!!! DormantNodeIO(): GetDormantFlavorInfoFor() failed:\n"
			"    %s\n",
			strerror(err)));
		return;
	}

	m_dormantName = flavorInfo.name;
	m_flavorID = info.flavor_id;
	m_kinds = flavorInfo.kinds;
	
	m_flags = ref->flags();
	entry_ref file;
	if(ref->getFile(&file) == B_OK)
		m_entry.SetTo(&file);
	
	m_runMode = ref->runMode();
	m_recordingDelay = ref->recordingDelay();
	m_cycle = ref->isCycling();
	
	// done extracting node info; ready for export
	m_exportValid = true;
}

// -------------------------------------------------------- //
// *** document-type setup
// -------------------------------------------------------- //

/*static*/
void DormantNodeIO::AddTo(
	XML::DocumentType*			docType) {

	// map self
	docType->addMapping(new Mapping<DormantNodeIO>(_DORMANT_NODE_ELEMENT));

//	// map simple (content-only) elements
//	// +++++ should these be added at a higher level, since they're
//	//       shared?  no harm is done if one is added more than once,
//	//       since they'll always map to StringContent.
//	// +++++
//	docType->addMapping(new Mapping<StringContent>(_NAME_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_KIND_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_FLAVOR_ID_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_FLAG_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_RUN_MODE_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_RECORDING_DELAY_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_CYCLE_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_REF_ELEMENT));
}

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

// call when object imported to create the described node

// +++++

status_t DormantNodeIO::instantiate(
	NodeManager*						manager,
	NodeRef**								outRef) {

	status_t err;
	BPath p;
	if(m_entry.InitCheck() == B_OK)
		m_entry.GetPath(&p);

//	PRINT((
//		"DormantNodeIO:\n"
//		"  key:       %s\n"
//		"  name:      %s\n"
//		"  flavor:    %ld\n"
//		"  kinds:     %Lx\n"
//		"  flags:     %lx\n"
//		"  runMode:   %ld\n"
//		"  recDelay:  %Ld\n"
//		"  cycle:     %s\n"
//		"  entry:     %s\n\n",
//		m_nodeKey.String(),
//		m_dormantName.String(),
//		m_flavorID,
//		m_kinds,
//		m_flags,
//		m_runMode,
//		m_recordingDelay,
//		m_cycle ? "true" : "false",
//		p.Path()));
	
	// find matching dormant node
	dormant_node_info info;
	err = _matchDormantNode(&info);
	if(err < B_OK) {
		PRINT((
			"!!! _matchDormantNode() failed: %s\n", strerror(err)));
		return err;
	}
	
	// instantiate node
	err = manager->instantiate(
		info,
		outRef,
		B_INFINITE_TIMEOUT,
		m_flags);
	if(err < B_OK) {
		PRINT((
			"!!! instantiate() failed: %s\n", strerror(err)));
		return err;
	}
	
	entry_ref mediaRef;
	if(m_entry.InitCheck() == B_OK && m_entry.GetRef(&mediaRef) == B_OK) {
		// set ref
		err = (*outRef)->setFile(mediaRef);
		if(err < B_OK) {
			PRINT((
				"!!! WARNING: setFile() failed: %s\n", strerror(err)));
		}
	}

	// set run mode
	if(m_runMode)
		(*outRef)->setRunMode(m_runMode, m_recordingDelay);
	
	// set cycle state
	if(m_cycle)
		(*outRef)->setCycling(true);

	return B_OK;
}

status_t DormantNodeIO::_matchDormantNode(
	dormant_node_info*			outInfo) {

	status_t err;
	
	// fetch all dormant nodes matching the signature
	const int32 bufferSize = 32;
	dormant_node_info	buffer[bufferSize];
	int32 count = bufferSize;
	err = BMediaRoster::Roster()->GetDormantNodes(
		buffer,
		&count,
		0,
		0,
		m_dormantName.String(),
		m_kinds,
		0 /*~m_kinds*/);
	if(err < B_OK)
		return err;
	
	if(!count)
		return B_NAME_NOT_FOUND;
	
	for(int32 n = 0; n < count; ++n) {
		if(buffer[n].flavor_id == m_flavorID) {
			*outInfo = buffer[n];
			return B_OK;
		}
	}

	// didn't match flavor id
	return B_BAD_INDEX;
}


// -------------------------------------------------------- //
// *** IPersistent
// -------------------------------------------------------- //

// -------------------------------------------------------- //
// EXPORT:
// -------------------------------------------------------- //

//inline void _write_simple(
//	const char* element,
//	const char* value,
//	ExportContext& context) {
//
//	context.beginElement(element);
//	context.beginContent();
//	context.writeString(value);
//	context.endElement();
//}

// -------------------------------------------------------- //

void DormantNodeIO::xmlExportBegin(
	ExportContext&						context) const {
	
	if(!m_exportValid) {
		context.reportError(
			"DormantNodeIO::xmlExportBegin():\n"
			"*** invalid ***\n");
		return;
	}
	
	context.beginElement(_DORMANT_NODE_ELEMENT);
}
	
void DormantNodeIO::xmlExportAttributes(
	ExportContext&						context) const {

	context.writeAttr("key", m_nodeKey);	
}

void DormantNodeIO::xmlExportContent(
	ExportContext&						context) const {

	context.beginContent();
	BString buffer;
	
	// write dormant-node description
	context.beginElement(_NAME_ELEMENT);
	context.beginContent();
	context.writeString(m_dormantName);
	context.endElement();

	if(m_flavorID > 0) {
		buffer = "";
		buffer << m_flavorID;
		context.beginElement(_FLAVOR_ID_ELEMENT);
		context.beginContent();
		context.writeString(buffer);
		context.endElement();
	}	

	_write_node_kinds(m_kinds, context);
//	if(m_kinds & B_BUFFER_PRODUCER)
//		_write_simple(_KIND_ELEMENT, "B_BUFFER_PRODUCER", context);
//	if(m_kinds & B_BUFFER_CONSUMER)
//		_write_simple(_KIND_ELEMENT, "B_BUFFER_CONSUMER", context);
//	if(m_kinds & B_TIME_SOURCE)
//		_write_simple(_KIND_ELEMENT, "B_TIME_SOURCE", context);
//	if(m_kinds & B_CONTROLLABLE)
//		_write_simple(_KIND_ELEMENT, "B_CONTROLLABLE", context);
//	if(m_kinds & B_FILE_INTERFACE)
//		_write_simple(_KIND_ELEMENT, "B_FILE_INTERFACE", context);
//	if(m_kinds & B_ENTITY_INTERFACE)
//		_write_simple(_KIND_ELEMENT, "B_ENTITY_INTERFACE", context);
//	if(m_kinds & B_PHYSICAL_INPUT)
//		_write_simple(_KIND_ELEMENT, "B_PHYSICAL_INPUT", context);
//	if(m_kinds & B_PHYSICAL_OUTPUT)
//		_write_simple(_KIND_ELEMENT, "B_PHYSICAL_OUTPUT", context);
//	if(m_kinds & B_SYSTEM_MIXER)
//		_write_simple(_KIND_ELEMENT, "B_SYSTEM_MIXER", context);
	
	// write NodeRef flags
	if(m_flags & NodeRef::NO_START_STOP)
		_write_simple(_FLAG_ELEMENT, "NO_START_STOP", context);
	if(m_flags & NodeRef::NO_SEEK)
		_write_simple(_FLAG_ELEMENT, "NO_SEEK", context);
	if(m_flags & NodeRef::NO_PREROLL)
		_write_simple(_FLAG_ELEMENT, "NO_PREROLL", context);
	if(m_flags & NodeRef::NO_STOP)
		_write_simple(_FLAG_ELEMENT, "NO_STOP", context);
	if(m_flags & NodeRef::NO_ROSTER_WATCH)
		_write_simple(_FLAG_ELEMENT, "NO_ROSTER_WATCH", context);
	if(m_flags & NodeRef::NO_POSITION_REPORTING)
		_write_simple(_FLAG_ELEMENT, "NO_POSITION_REPORTING", context);
		
	// write transport settings
	if(m_runMode > 0) {
		switch(m_runMode) {
			case BMediaNode::B_OFFLINE:
				_write_simple(_RUN_MODE_ELEMENT, "B_OFFLINE", context);
				break;
			case BMediaNode::B_DECREASE_PRECISION:
				_write_simple(_RUN_MODE_ELEMENT, "B_DECREASE_PRECISION", context);
				break;
			case BMediaNode::B_INCREASE_LATENCY:
				_write_simple(_RUN_MODE_ELEMENT, "B_INCREASE_LATENCY", context);
				break;
			case BMediaNode::B_DROP_DATA:
				_write_simple(_RUN_MODE_ELEMENT, "B_DROP_DATA", context);
				break;
			case BMediaNode::B_RECORDING:
				_write_simple(_RUN_MODE_ELEMENT, "B_RECORDING", context);
				buffer = "";
				buffer << m_recordingDelay;
				_write_simple(_RECORDING_DELAY_ELEMENT, buffer.String(), context);
				break;
			default:
				buffer = "";
				buffer << m_runMode;
				_write_simple(_RUN_MODE_ELEMENT, buffer.String(), context);
		}
	}

	if(m_cycle) {
		context.beginElement(_CYCLE_ELEMENT);
		context.endElement();
	}

	BPath p;
	if(
		m_entry.InitCheck() == B_OK &&
		m_entry.GetPath(&p) == B_OK)
		_write_simple(_REF_ELEMENT, p.Path(), context);

}

void DormantNodeIO::xmlExportEnd(
	ExportContext&						context) const {

	// finish
	context.endElement();
}


// -------------------------------------------------------- //
// IMPORT:
// -------------------------------------------------------- //

//inline void _read_node_kind(
//	int64& ioKind,
//	const char* data,
//	ImportContext& context) {
//
//	if(!strcmp(data, "B_BUFFER_PRODUCER"))
//		ioKind |= B_BUFFER_PRODUCER;
//	else if(!strcmp(data, "B_BUFFER_CONSUMER"))
//		ioKind |= B_BUFFER_CONSUMER;
//	else if(!strcmp(data, "B_TIME_SOURCE"))
//		ioKind |= B_TIME_SOURCE;
//	else if(!strcmp(data, "B_CONTROLLABLE"))
//		ioKind |= B_CONTROLLABLE;
//	else if(!strcmp(data, "B_FILE_INTERFACE"))
//		ioKind |= B_FILE_INTERFACE;
//	else if(!strcmp(data, "B_ENTITY_INTERFACE"))
//		ioKind |= B_ENTITY_INTERFACE;
//	else if(!strcmp(data, "B_PHYSICAL_INPUT"))
//		ioKind |= B_PHYSICAL_INPUT;
//	else if(!strcmp(data, "B_PHYSICAL_OUTPUT"))
//		ioKind |= B_PHYSICAL_OUTPUT;
//	else if(!strcmp(data, "B_SYSTEM_MIXER"))
//		ioKind |= B_SYSTEM_MIXER;
//	else {
//		BString err;
//		err << "_read_noderef_kind(): unknown node kind '" << data << "'\n";
//		context.reportWarning(err.String());
//	}
//}

inline void _read_noderef_flag(
	int32& ioFlags,
	const char* data,
	ImportContext& context) {

	if(!strcmp(data, "NO_START_STOP"))
		ioFlags |= NodeRef::NO_START_STOP;
	else if(!strcmp(data, "NO_SEEK"))
		ioFlags |= NodeRef::NO_SEEK;
	else if(!strcmp(data, "NO_PREROLL"))
		ioFlags |= NodeRef::NO_PREROLL;
	else if(!strcmp(data, "NO_STOP"))
		ioFlags |= NodeRef::NO_STOP;
	else if(!strcmp(data, "NO_ROSTER_WATCH"))
		ioFlags |= NodeRef::NO_ROSTER_WATCH;
	else if(!strcmp(data, "NO_POSITION_REPORTING"))
		ioFlags |= NodeRef::NO_POSITION_REPORTING;	
	else {
		BString err;
		err << "_read_noderef_flag(): unknown node flag '" << data << "'\n";
		context.reportWarning(err.String());
	}
}

inline void _read_run_mode(
	int32& runMode,
	const char* data,
	ImportContext& context) {

	if(!strcmp(data, "B_OFFLINE"))
		runMode = BMediaNode::B_OFFLINE;
	else if(!strcmp(data, "B_DECREASE_PRECISION"))
		runMode = BMediaNode::B_DECREASE_PRECISION;
	else if(!strcmp(data, "B_INCREASE_LATENCY"))
		runMode = BMediaNode::B_INCREASE_LATENCY;
	else if(!strcmp(data, "B_DROP_DATA"))
		runMode = BMediaNode::B_DROP_DATA;
	else if(!strcmp(data, "B_RECORDING"))
		runMode = BMediaNode::B_RECORDING;
	else {
		BString err;
		err << "_read_run_mode(): unknown run mode '" << data << "'\n";
		context.reportWarning(err.String());
	}
}

inline void _read_entry(
	BEntry& entry,
	const char* data,
	ImportContext& context) {
	
	entry_ref r;
	status_t err = get_ref_for_path(data, &r);
	if(err < B_OK) {
		BString text;
		text << "_read_entry_ref(): get_ref_for_path('" << data << "') failed:\n"
			"   " << strerror(err) << "\n";
		context.reportWarning(text.String());
	}
	
	entry.SetTo(&r);
}


void DormantNodeIO::xmlImportBegin(
	ImportContext&						context) { TOUCH(context); }

void DormantNodeIO::xmlImportAttribute(
	const char*								key,
	const char*								value,
	ImportContext&						context) {

	if(!strcmp(key, "key")) {
		m_nodeKey = value;
	}
	else {
		BString err;
		err << "DormantNodeIO: unknown attribute '" << key << "'\n";
		context.reportError(err.String());		
	}	
}

void DormantNodeIO::xmlImportContent(
	const char*								data,
	uint32										length,
	ImportContext&						context) { TOUCH(data); TOUCH(length); TOUCH(context); }

void DormantNodeIO::xmlImportChild(
	IPersistent*							child,
	ImportContext&						context) {
	
	StringContent* obj = dynamic_cast<StringContent*>(child);
	if(!obj) {
		BString err;
		err << "DormantNodeIO: unexpected element '" <<
			context.element() << "'\n";
		context.reportError(err.String());
		return;			
	}
	
	if(!strcmp(context.element(), _NAME_ELEMENT))
		m_dormantName = obj->content;
	else if(!strcmp(context.element(), _KIND_ELEMENT))
		_read_node_kind(m_kinds, obj->content.String(), context);
	else if(!strcmp(context.element(), _FLAVOR_ID_ELEMENT))
		m_flavorID = atol(obj->content.String());
	else if(!strcmp(context.element(), _FLAG_ELEMENT))
		_read_noderef_flag(m_flags, obj->content.String(), context);
	else if(!strcmp(context.element(), _RUN_MODE_ELEMENT))
		_read_run_mode(m_runMode, obj->content.String(), context);
	else if(!strcmp(context.element(), _RECORDING_DELAY_ELEMENT))
		m_recordingDelay = strtoll(obj->content.String(), 0, 10);
	else if(!strcmp(context.element(), _CYCLE_ELEMENT))
		m_cycle = true;
	else if(!strcmp(context.element(), _REF_ELEMENT))
		_read_entry(m_entry, obj->content.String(), context);
	else {
		BString err;
		err << "DormantNodeIO: unexpected element '" << 
			context.element() << "'\n";
		context.reportError(err.String());
	}

	delete child;
}
		
void DormantNodeIO::xmlImportComplete(
	ImportContext&						context) { TOUCH(context); } //nyi; +++++ final checks?


// END -- DormantNodeIO.cpp --

