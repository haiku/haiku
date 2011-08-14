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


// ExportContext.cpp
// e.moon 30jun99

#include "ExportContext.h"
#include "IPersistent.h"

#include <DataIO.h>

#include <algorithm>
#include <cstdio>

__USE_CORTEX_NAMESPACE


// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

ExportContext::~ExportContext() {}

ExportContext::ExportContext() :
	
	stream(0),
	m_indentLevel(0),
	m_indentIncrement(4),
	m_attrColumn(30),
	m_state(INIT) {}

ExportContext::ExportContext(
	BDataIO*										_stream) :
	
	stream(_stream),
	m_indentLevel(0),
	m_indentIncrement(2),
	m_attrColumn(30),
	m_state(INIT) {

	ASSERT(_stream);
}



// -------------------------------------------------------- //
// *** XML formatting helpers
// -------------------------------------------------------- //

// writes a start tag.  should only be called from
// IPersistent::xmlExportBegin().
void ExportContext::beginElement(
	const char* 								name) {

	ASSERT(name);

	if(!m_objectStack.size()) {
		reportError("beginElement(): no object being written.\n");
		return;
	}
	if(m_state != WRITE_BEGIN && m_state != WRITE_CONTENT) {
		reportError("beginElement(): not allowed.\n");
		return;
	}
	
	// push tag onto element stack, and link to entry for the current object
	m_elementStack.push_back(element_entry());
	m_elementStack.back().name = name;
	m_objectStack.back().element = m_elementStack.back().name.String();
	
	// write tag
	BString out;
	out << "\n" << indentString() << '<' << name;
	writeString(out);	
	indentMore();
}
	
// writes an end tag corresponding to the current element.
// should only be called from IPersistent::xmlExportEnd() or
// xmlExportContent().

void ExportContext::endElement() {

	if(!m_objectStack.size()) {
		reportError("endElement(): no object being written.\n");
		return;
	}
	ASSERT(m_elementStack.size());
	element_entry& entry = m_elementStack.back();

	if(m_state != WRITE_END && m_state != WRITE_CONTENT) {
		reportError("endElement(): not allowed.\n");
		return;
	}
	
	indentLess();
	
	BString out;

	// write closing tag
	if(!entry.hasContent)
		out << "/>";
	else
		out << "\n" << indentString() << "</" << entry.name.String() << ">";
	
	writeString(out);

	// pop element off stack
	m_elementStack.pop_back();
}

// indicates that content follows (writes the end of the
// current element's start tag.)
void ExportContext::beginContent() {

	if(!m_objectStack.size()) {
		reportError("beginContent(): no object being written.\n");
		return;
	}
	ASSERT(m_elementStack.size());
	element_entry& entry = m_elementStack.back();

	if(m_state != WRITE_CONTENT) {
		reportError("beginContent(): not allowed.\n");
		return;
	}

	BString out = ">";	
	writeString(out);

	entry.hasContent = true;
}

#define _WRITE_ATTR_BODY(VAL_SPEC) \
	if(!m_objectStack.size()) {\
		reportError("writeAttr(): no object being written.\n");\
		return;\
	}\
	ASSERT(m_elementStack.size());\
	if(m_state != WRITE_ATTRIBUTES &&\
		m_state != WRITE_CONTENT) {\
		reportError("writeAttr(): not allowed (state mismatch).\n");\
		return;\
	}\
\
	m_elementStack.back().hasAttributes = true;\
\
	BString out;\
	out << "\n" << indentString() << key;\
	_pad_with_spaces(out, key, *this, m_attrColumn) << " = '" << VAL_SPEC << '\'';\
\
	writeString(out);

#if B_BEOS_VERSION > B_BEOS_VERSION_4_5
void ExportContext::writeAttr(
	const char*									key,
	int8												value) {_WRITE_ATTR_BODY(value)}

void ExportContext::writeAttr(
	const char*									key,
	uint8												value) {_WRITE_ATTR_BODY(uint32(value))}

void ExportContext::writeAttr(
	const char*									key,
	int16												value) {_WRITE_ATTR_BODY(value)}

void ExportContext::writeAttr(
	const char*									key,
	uint16											value) {_WRITE_ATTR_BODY(uint32(value))}
#endif

void ExportContext::writeAttr(
	const char*									key,
	int32												value) {_WRITE_ATTR_BODY(value)}

void ExportContext::writeAttr(
	const char*									key,
	uint32											value) {_WRITE_ATTR_BODY(value)}

void ExportContext::writeAttr(
	const char*									key,
	int64												value) {_WRITE_ATTR_BODY(value)}

void ExportContext::writeAttr(
	const char*									key,
	uint64											value) {_WRITE_ATTR_BODY(value)}

void ExportContext::writeAttr(
	const char*									key,
	const char*									value) {_WRITE_ATTR_BODY(value)}

void ExportContext::writeAttr(
	const char*									key,
	const BString&							value) {_WRITE_ATTR_BODY(value)}

void ExportContext::writeAttr(
	const char*									key,
	float												value) {_WRITE_ATTR_BODY(value)}

// writes a child object.
// should only be called from IPersistent::xmlExportContent().
// returns B_OK on success, or B_ERROR if an error occurred.
status_t ExportContext::writeObject(
	IPersistent* object) {

	// * SETUP	
	ASSERT(object);
	if(m_state == ABORT)
		return B_ERROR;
	state_t origState = m_state;

	//   write entry to object stack
	m_objectStack.push_back(object_entry());
	object_entry& entry = m_objectStack.back();
	entry.object = object;
	
	// * START TAG
	int elements = m_elementStack.size();
	m_state = WRITE_BEGIN;
	object->xmlExportBegin(*this);
	
	if(m_state == ABORT)
		return B_ERROR;
	
	if(!entry.element)
		reportError("writeObject(): no start tag for object.\n");
	else if(m_elementStack.size() - elements > 1)
		reportError("writeObject(): object wrote more than one start tag.\n");

	if(m_state == ABORT)
		return B_ERROR;
	
	// * ATTRIBUTES
	m_state = WRITE_ATTRIBUTES;
	object->xmlExportAttributes(*this);

	if(m_state == ABORT)
		return B_ERROR;

	// * CONTENT
	m_state = WRITE_CONTENT;
	object->xmlExportContent(*this);
	
	if(m_state == ABORT)
		return B_ERROR;
		
	// * END
	m_state = WRITE_END;
	object->xmlExportEnd(*this);

	m_state = origState;
		
	//   pop object entry
	m_objectStack.pop_back();
	
	return (m_state == ABORT) ? B_ERROR : B_OK;
}

// writes an arbitrary string to the stream (calls reportError()
// on failure.)

status_t ExportContext::writeString(
	const BString&							string) {

	return writeString(string.String(), string.Length());
}

status_t ExportContext::writeString(
	const char*									data,
	ssize_t											length) {

	ssize_t written = stream->Write(data, length);
	if(written < 0) {
		BString err = "Write error: '";
		err << strerror(written) << "'.\n";
		reportError(err.String());
		return written;
	}
	else if(written < length) {
		BString err = "Write incomplete: '";
		err << written << " of " << length << " bytes written.\n";
		reportError(err.String());
		return B_IO_ERROR;
	}
	return B_OK;
}
		

// -------------------------------------------------------- //
// *** indentation helpers
// -------------------------------------------------------- //

const char* ExportContext::indentString() const {
	return m_indentString.String();
}

uint16 ExportContext::indentLevel() const {
	return m_indentLevel;
}
	
void ExportContext::indentLess() {
	m_indentLevel = (m_indentLevel > m_indentIncrement) ?
		m_indentLevel - m_indentIncrement : 0;
	m_indentString.SetTo(' ', m_indentLevel);
}

void ExportContext::indentMore() {
	m_indentLevel += m_indentIncrement;
	m_indentString.SetTo(' ', m_indentLevel);
}

// -------------------------------------------------------- //
// *** error-reporting operations
// -------------------------------------------------------- //

class dump_element { public:
	BString& _s;
	
	dump_element(BString& s) : _s(s) {}
	void operator()(const ExportContext::element_entry& entry) {
		_s << "  " << entry.name << '\n';
	}
};

// register a fatal error; halts the write process
// as soon as possible.
void ExportContext::reportError(
	const char*			text) {
	
	m_error << "FATAL ERROR: ";
	m_error << text << "\n";
	if(m_elementStack.size()) {
		_dumpElementStack(m_error);
	}
	
	m_state = ABORT;
}

void ExportContext::_dumpElementStack(
	BString&										out) {
	out << "Element stack:\n";
		for_each(m_elementStack.begin(), m_elementStack.end(), dump_element(out));
}

// END -- ExportContext.cpp --

