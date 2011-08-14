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


// ConnectionIO.cpp

#include "ConnectionIO.h"
#include "LiveNodeIO.h"
#include "NodeManager.h"
#include "NodeSetIOContext.h"

#include "MediaFormatIO.h"
#include "route_app_io.h"

#include <vector>
#include <Debug.h>

using namespace std;

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

ConnectionIO::~ConnectionIO() {
	if(m_inputNodeIO) delete m_inputNodeIO;
	if(m_outputNodeIO) delete m_outputNodeIO;
}

// initialize for import
ConnectionIO::ConnectionIO() :
	m_inputNodeIO(0),
	m_outputNodeIO(0),
	m_flags(0),
	m_exportValid(false),
	m_importState(IMPORT_NONE) {

	m_outputFormat.type = B_MEDIA_NO_TYPE;	
	m_inputFormat.type = B_MEDIA_NO_TYPE;	
	m_requestedFormat.type = B_MEDIA_NO_TYPE;	
}

// initialize for export
ConnectionIO::ConnectionIO(
	const Connection*				con,
	const NodeManager*			manager,
	const NodeSetIOContext*	context) :

	m_inputNodeIO(0),
	m_outputNodeIO(0),
	m_exportValid(false),
	m_importState(IMPORT_NONE) {

	ASSERT(con);
	ASSERT(manager);
	ASSERT(context);
	
	if(!con->isValid()) {
		PRINT((
			"!!! ConnectionIO(): invalid connection\n"));
		return;
	}

	m_outputNodeIO = new LiveNodeIO(
		manager,
		context,
		con->sourceNode());
	
	// fetch output (connection-point) description
	const char* name;
	if(con->getOutputHint(
		&name,
		&m_outputFormat) == B_OK)
		m_outputName = name;
	else {
		m_outputName = con->outputName();
	}

	m_inputNodeIO = new LiveNodeIO(
		manager,
		context,
		con->destinationNode());

	// fetch input (connection-point) description
	if(con->getInputHint(
		&name,
		&m_inputFormat) == B_OK)
		m_inputName = name;

	else {
		m_inputName = con->inputName();
	}

	m_requestedFormat = con->requestedFormat();
	m_flags = con->flags();
	
	m_exportValid = true;
}

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

// call when object imported to create the described
// connection

// +++++ to do
// smarter input/output matching -- if no name/format provided,
// pick the first available endpoint.  otherwise, make two passes:
// 1) match all nodes w/ given name (pass wildcards through to roster)
// 2) filter by format

status_t ConnectionIO::instantiate(
	NodeManager*						manager,
	const NodeSetIOContext*	context,
	Connection*							outCon) {

	// sanity checks
	ASSERT(manager);
	if(!m_inputNodeIO || !m_outputNodeIO)
		return B_NOT_ALLOWED;

	status_t err;
	media_node_id node;

	// find output node
	NodeRef* outputRef;
	err = m_outputNodeIO->getNode(manager, context, &node);
	if(err < B_OK)
		return err;
	err = manager->getNodeRef(
		node,
		&outputRef);
	if(err < B_OK)
		return err;

	// find output +++++ currently matches by name only
	const int32 outputBufferSize = 16;
	media_output outputs[outputBufferSize];
	int32 count = outputBufferSize;
	
	//vector<media_output> outputs;
//	err = outputRef->getFreeOutputs(
//		outputs/*,
//		m_outputFormat.type*/);

	err = outputRef->getFreeOutputs(
		outputs,
		outputBufferSize,
		&count);
	if(err < B_OK)
		return err;
	
	media_output output;
	bool found = false;
	for(int n = 0; n < count; ++n) {
		if(m_outputName == outputs[n].name) {
			output = outputs[n];
			found = true;
			break;
		}
	}
	if(!found) {
		PRINT(("!!! output '%s' of node '%s' not found\n",
			m_outputName.String(),
			outputRef->name()));
		return B_NAME_NOT_FOUND;
	}
	
	// find input node
	NodeRef* inputRef;
	err = m_inputNodeIO->getNode(manager, context, &node);
	if(err < B_OK)
		return err;
	err = manager->getNodeRef(
		node,
		&inputRef);
	if(err < B_OK)
		return err;

	// find input +++++ currently matches by name only
	vector<media_input> inputs;
	err = inputRef->getFreeInputs(
		inputs /*,
		m_inputFormat.type*/);
	if(err < B_OK)
		return err;
	
	media_input input;
	found = false;
	for(unsigned int n = 0; n < inputs.size(); ++n) {
		if(m_inputName == inputs[n].name) {
			input = inputs[n];
			found = true;
			break;
		}
	}
	if(!found) {
		PRINT(("!!! input '%s' of node '%s' not found\n",
			m_inputName.String(),
			inputRef->name()));
		return B_NAME_NOT_FOUND;
	}
	
	// connect
	Connection con;
	if(m_requestedFormat.type != B_MEDIA_NO_TYPE)
		err = manager->connect(
			output,
			input,
			m_requestedFormat,
			&con);
	else
		err = manager->connect(
			output,
			input,
			&con);
			
	if(err < B_OK)
		return err;
	
	if(outCon)
		*outCon = con;
	return B_OK;
}

// -------------------------------------------------------- //
// *** document-type setup
// -------------------------------------------------------- //

/*static*/
void ConnectionIO::AddTo(
	XML::DocumentType*			docType) {
	
	// map self
	docType->addMapping(new Mapping<ConnectionIO>(_CONNECTION_ELEMENT));

	// map simple (content-only) elements
	// +++++ should these be added at a higher level, since they're
	//       shared?  no harm is done if one is added more than once,
	//       since they'll always map to StringContent -- but it's way
	//       messy!
	// +++++
	//docType->addMapping(new Mapping<StringContent>(_LIVE_NODE_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_NAME_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_KIND_ELEMENT));
//	docType->addMapping(new Mapping<StringContent>(_FLAG_ELEMENT));
}

// -------------------------------------------------------- //
// *** IPersistent
// -------------------------------------------------------- //

// -------------------------------------------------------- //
// EXPORT:
// -------------------------------------------------------- //


// -------------------------------------------------------- //
void ConnectionIO::xmlExportBegin(
	ExportContext&						context) const {

	if(!m_exportValid) {
		context.reportError(
			"ConnectionIO::xmlExportBegin():\n"
			"*** invalid ***\n");
		return;
	}
	
	context.beginElement(_CONNECTION_ELEMENT);
}
	
void ConnectionIO::xmlExportAttributes(
	ExportContext&						context) const { TOUCH(context); }

void ConnectionIO::xmlExportContent(
	ExportContext&						context) const {

	context.beginContent();

	// write output
	{
		context.beginElement(_OUTPUT_ELEMENT);
		context.beginContent();

		// describe the node
//		LiveNodeIO nodeIO(
//			m_manager,
//			dynamic_cast<NodeSetIOContext*>(&context),
//			m_outputNode);
		context.writeObject(m_outputNodeIO);
		
//		context.beginElement(_LIVE_NODE_ELEMENT);
//		if(m_outputNodeKey.Length()) {
//			context.writeAttr("key", m_outputNodeKey);
//		}
//		else {
//			_write_simple("name", m_outputNodeName.String(), context);
//			_write_node_kinds(m_outputNodeKind, context);
//		}
//		context.endElement(); // _LIVE_NODE_ELEMENT

		// describe the output

		_write_simple("name", m_outputName.String(), context);		

		if(m_outputFormat.type > B_MEDIA_UNKNOWN_TYPE) {
			MediaFormatIO io(m_outputFormat);
			context.writeObject(&io);
		}

		context.endElement(); // _OUTPUT_ELEMENT
	}	

	// write input
	{
		context.beginElement(_INPUT_ELEMENT);
		context.beginContent();

		// describe the node
//		LiveNodeIO nodeIO(
//			m_manager,
//			dynamic_cast<NodeSetIOContext*>(&context),
//			m_inputNode);
		context.writeObject(m_inputNodeIO);

//		context.beginElement(_LIVE_NODE_ELEMENT);
//		if(m_inputNodeKey.Length()) {
//			context.writeAttr("key", m_inputNodeKey);
//		}
//		else {
//			_write_simple("name", m_inputNodeName.String(), context);
//			_write_node_kinds(m_inputNodeKind, context);
//		}
//		context.endElement(); // _LIVE_NODE_ELEMENT

		// describe the input
		
		_write_simple("name", m_inputName.String(), context);		

		if(m_inputFormat.type > B_MEDIA_UNKNOWN_TYPE) {
			MediaFormatIO io(m_inputFormat);
			context.writeObject(&io);
		}

		context.endElement(); // _INPUT_ELEMENT
	}
	
	// write requested format
	if(m_requestedFormat.type > B_MEDIA_UNKNOWN_TYPE) {
		MediaFormatIO io(m_requestedFormat);
		BString comment = "\n";
		comment << context.indentString();
		comment << "<!-- initial requested format -->";
		context.writeString(comment);
		context.writeObject(&io);	
	}
}

void ConnectionIO::xmlExportEnd(
	ExportContext&						context) const {

	context.endElement(); // _CONNECTION_ELEMENT	
}

// -------------------------------------------------------- //
// IMPORT:
// -------------------------------------------------------- //

void ConnectionIO::xmlImportBegin(
	ImportContext&						context) { TOUCH(context); }

void ConnectionIO::xmlImportAttribute(
	const char*								key,
	const char*								value,
	ImportContext&						context) { TOUCH(key); TOUCH(value); TOUCH(context); }

void ConnectionIO::xmlImportContent(
	const char*								data,
	uint32										length,
	ImportContext&						context) { TOUCH(data); TOUCH(length); TOUCH(context); }

void ConnectionIO::xmlImportChild(
	IPersistent*							child,
	ImportContext&						context) {

	status_t err;

	if(!strcmp(context.element(), _LIVE_NODE_ELEMENT)) {
		LiveNodeIO* nodeIO = dynamic_cast<LiveNodeIO*>(child);
		ASSERT(nodeIO);

		// store the LiveNodeIO for now; it will be used in
		// instantiate()
		
		switch(m_importState) {
			case IMPORT_OUTPUT:
				m_outputNodeIO = nodeIO;
				child = 0; // don't delete child object
				break;
				
			case IMPORT_INPUT:
				m_inputNodeIO = nodeIO;
				child = 0; // don't delete child object
				break;

			case IMPORT_NONE:
				context.reportError("Unexpected node description.\n");
				delete child;
				return;
		}
	}
	else if(!strcmp(context.element(), _NAME_ELEMENT)) {
		StringContent* c = dynamic_cast<StringContent*>(child);
		ASSERT(c);		

		switch(m_importState) {
			case IMPORT_OUTPUT:
				m_outputName = c->content;
				break;
				
			case IMPORT_INPUT:
				m_inputName = c->content;
				break;

			case IMPORT_NONE:
				context.reportError("Unexpected node name.\n");
				delete child;
				return;
		}
	}
	else {
		MediaFormatIO* io = dynamic_cast<MediaFormatIO*>(child);
		if(!io) {
			context.reportError("Unexpected element.\n");
			delete child;
			return;
		}
		
		media_format f;
		err = io->getFormat(f);
		if(err < B_OK) {
			context.reportError("Malformed format.\n");
			delete child;
			return;
		}
		
		switch(m_importState) {
			case IMPORT_OUTPUT:
				m_outputFormat = f;
				break;
				
			case IMPORT_INPUT:
				m_inputFormat = f;
				break;

			case IMPORT_NONE:
				m_requestedFormat = f;
				break;
		}		
	}
	
	if(child)
		delete child;	
}
		
void ConnectionIO::xmlImportComplete(
	ImportContext&						context) {

	// +++++
}

void ConnectionIO::xmlImportChildBegin(
	const char*								name,
	ImportContext&						context) {

	if(!strcmp(name, "input")) {
		if(m_importState != IMPORT_NONE) {
			context.reportError("ConnectionIO: unexpected nested child element\n");
			return;
		}
		m_importState = IMPORT_INPUT;
	}
	else if(!strcmp(name, "output")) {
		if(m_importState != IMPORT_NONE) {
			context.reportError("ConnectionIO: unexpected nested child element\n");
			return;
		}
		m_importState = IMPORT_OUTPUT;
	}
	else
		context.reportError("ConnectionIO: unexpected child element\n");
}

void ConnectionIO::xmlImportChildComplete(
	const char*								name,
	ImportContext&						context) {
	TOUCH(name); TOUCH(context);

	m_importState = IMPORT_NONE;
}

// END -- ConnectionIO.cpp --
