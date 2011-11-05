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


// Connection.cpp
// e.moon 25jun99

#include "Connection.h"
#include "NodeManager.h"
#include "NodeRef.h"

#if CORTEX_XML
	#include "ExportContext.h"
	#include "MediaFormatIO.h"
	#include "xml_export_utils.h"
#endif /*CORTEX_XML*/

#include <Debug.h>

// -------------------------------------------------------- //

__USE_CORTEX_NAMESPACE
	
// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

Connection::~Connection() {

//	PRINT(("~Connection(): '%s'->'%s'\n",
//		outputName(), inputName()));
//	
	// deallocate hints	
	if(m_outputHint) delete m_outputHint;
	if(m_inputHint) delete m_inputHint;
}

Connection::Connection() :
	m_disconnected(true),
	m_id(0),
	m_outputHint(0),
	m_inputHint(0) {}

Connection::Connection(
	uint32										id,
	media_node								srcNode,
	const media_source&				src,
	const char*								outputName,
	media_node								destNode,
	const media_destination&	dest,
	const char*								inputName,
	const media_format&				format,
	uint32										flags) :

	m_disconnected(false),
	m_id(id),
	m_sourceNode(srcNode),
	m_source(src),
	m_outputName(outputName),
	m_outputHint(0),
	m_destinationNode(destNode),
	m_destination(dest),
	m_inputName(inputName),
	m_inputHint(0),
	m_format(format),
	m_flags(flags) {

	ASSERT(id);
	m_requestedFormat.type = B_MEDIA_NO_TYPE;
}

Connection::Connection(
	const Connection&					clone) {
	operator=(clone);
}
	
Connection& Connection::operator=(
	const Connection&					clone) {

	m_disconnected = clone.m_disconnected;
	m_id = clone.m_id;
	m_sourceNode = clone.m_sourceNode;
	m_source = clone.m_source;
	m_outputName = clone.m_outputName;
	m_outputHint = (clone.m_outputHint ?
		new endpoint_hint(
			clone.m_outputHint->name.String(),
			clone.m_outputHint->format) :
		0);
	m_destinationNode = clone.m_destinationNode;
	m_destination = clone.m_destination;
	m_inputName = clone.m_inputName;
	m_inputHint = (clone.m_inputHint ?
		new endpoint_hint(
			clone.m_inputHint->name.String(),
			clone.m_inputHint->format) :
		0);
	m_format = clone.m_format;
	m_flags = clone.m_flags;
	m_requestedFormat = clone.m_requestedFormat;

	return *this;
}

// input/output access [e.moon 14oct99]

status_t Connection::getInput(
	media_input*							outInput) const {
	
	if(!isValid())
		return B_ERROR;

	outInput->node = m_destinationNode;
	strcpy(outInput->name, m_inputName.String());
	outInput->format = format();
	outInput->source = m_source;
	outInput->destination = m_destination;
	return B_OK;
}


status_t Connection::getOutput(
	media_output*							outOutput) const {
	
	if(!isValid())
		return B_ERROR;

	outOutput->node = m_sourceNode;
	strcpy(outOutput->name, m_outputName.String());
	outOutput->format = format();
	outOutput->source = m_source;
	outOutput->destination = m_destination;
	return B_OK;
}

// hint access

status_t Connection::getOutputHint(
	const char**							outName,
	media_format*							outFormat) const {

	if(!m_outputHint)
		return B_NOT_ALLOWED;
	*outName = m_outputHint->name.String();
	*outFormat = m_outputHint->format;
	return B_OK;	
}

status_t Connection::getInputHint(
	const char**							outName,
	media_format*							outFormat) const {

	if(!m_inputHint)
		return B_NOT_ALLOWED;
	*outName = m_inputHint->name.String();
	*outFormat = m_inputHint->format;
	return B_OK;	
}

	
void Connection::setOutputHint(
	const char*									origName,
	const media_format&				origFormat) {

	if(m_outputHint) delete m_outputHint;
	m_outputHint = new endpoint_hint(origName, origFormat);
}

void Connection::setInputHint(
	const char*									origName,
	const media_format&				origFormat) {

	if(m_inputHint) delete m_inputHint;
	m_inputHint = new endpoint_hint(origName, origFormat);
}

// [e.moon 8dec99]
void Connection::setRequestedFormat(
	const media_format&				reqFormat) {
	m_requestedFormat = reqFormat;
}

// END -- Connection.cpp --
