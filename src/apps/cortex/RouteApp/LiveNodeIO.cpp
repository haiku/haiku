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


// LiveNodeIO.cpp

#include "LiveNodeIO.h"
#include "ImportContext.h"
#include "ExportContext.h"
#include "NodeSetIOContext.h"
#include "StringContent.h"

#include "NodeManager.h"

#include <Debug.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>
#include <MediaRoster.h>
#include <Path.h>

#include "route_app_io.h"

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

LiveNodeIO::~LiveNodeIO() {}

LiveNodeIO::LiveNodeIO() :
	m_kind(0LL),
	m_exportValid(false) {}

LiveNodeIO::LiveNodeIO(
	const NodeManager*			manager,
	const NodeSetIOContext*	context,
	media_node_id						node) :
	m_exportValid(false) {

	status_t err;
	ASSERT(manager);
	ASSERT(context);
	
	err = _get_node_signature(
		manager,
		context,
		node,
		m_key,
		m_name,
		m_kind);
	if(err < B_OK)
		return;
	
	m_exportValid = true;
}

// -------------------------------------------------------- //
// *** import operations
// -------------------------------------------------------- //

// locate the referenced live node
status_t LiveNodeIO::getNode(
	const NodeManager*			manager,
	const NodeSetIOContext*	context,
	media_node_id*					outNode) const {

	ASSERT(manager);
	ASSERT(context);
	status_t err;

	if(hasKey()) {
		// match key against previously imported nodes
		err = context->getNodeFor(key(), outNode);

		if(err < B_OK) {
			// match key against system nodes
			err = _match_system_node_key(key(), manager, outNode);
		
			if(err < B_OK) {
				PRINT((
					"!!! No node found for key '%s'\n",
					key()));
				return B_NAME_NOT_FOUND;
			}
		}
	}
	else {
		err = _match_node_signature(
			name(),
			kind(),
			outNode);
		if(err < B_OK) {
			PRINT((
				"!!! No node found named '%s' with kinds %Ld\n",
				name(),
				kind()));
			return B_NAME_NOT_FOUND;
		}
	}
	
	return B_OK;
}

// -------------------------------------------------------- //
// *** document-type setup
// -------------------------------------------------------- //

/*static*/
void LiveNodeIO::AddTo(
	XML::DocumentType*			docType) {
	
	// map self
	docType->addMapping(new Mapping<LiveNodeIO>(_LIVE_NODE_ELEMENT));
}

// -------------------------------------------------------- //
// *** IPersistent
// -------------------------------------------------------- //

// -------------------------------------------------------- //
// EXPORT:
// -------------------------------------------------------- //

void LiveNodeIO::xmlExportBegin(
	ExportContext&					context) const {

	if(!m_exportValid) {
		context.reportError(
			"LiveNodeIO::xmlExportBegin():\n"
			"*** invalid ***\n");
		return;
	}
	
	context.beginElement(_LIVE_NODE_ELEMENT);
}
	
void LiveNodeIO::xmlExportAttributes(
	ExportContext&					context) const {
	
	if(m_key.Length() > 0)
		context.writeAttr("key", m_key.String());
}

void LiveNodeIO::xmlExportContent(
	ExportContext&					context) const {
	
	if(m_name.Length() > 0) {
		context.beginContent();

		_write_simple(_NAME_ELEMENT, m_name.String(), context);
		_write_node_kinds(m_kind, context);
	}
}

void LiveNodeIO::xmlExportEnd(
	ExportContext&					context) const {
	
	context.endElement();
}

// -------------------------------------------------------- //
// IMPORT:
// -------------------------------------------------------- //

void LiveNodeIO::xmlImportBegin(
	ImportContext&					context) {}

void LiveNodeIO::xmlImportAttribute(
	const char*							key,
	const char*							value,
	ImportContext&					context) {

	if(!strcmp(key, "key")) {
		m_key = value;
	}	
	else {
		BString err;
		err << "LiveNodeIO: unknown attribute '" << key << "'\n";
		context.reportError(err.String());		
	}	
}

void LiveNodeIO::xmlImportContent(
	const char*							data,
	uint32									length,
	ImportContext&					context) {}

void LiveNodeIO::xmlImportChild(
	IPersistent*						child,
	ImportContext&					context) {

	StringContent* obj = dynamic_cast<StringContent*>(child);
	if(!obj) {
		BString err;
		err << "LiveNodeIO: unexpected element '" <<
			context.element() << "'\n";
		context.reportError(err.String());
		return;			
	}
	
	if(!strcmp(context.element(), _NAME_ELEMENT))
		m_name = obj->content;
	else if(!strcmp(context.element(), _KIND_ELEMENT))
		_read_node_kind(m_kind, obj->content.String(), context);
	else {
		BString err;
		err << "LiveNodeIO: unexpected element '" << 
			context.element() << "'\n";
		context.reportError(err.String());
	}

	delete child;
}
		
void LiveNodeIO::xmlImportComplete(
	ImportContext&					context) {}

// END -- LiveNodeIO.cpp --

