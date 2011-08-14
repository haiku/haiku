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


// ImportContext.cpp
// e.moon 1jul99

#include "ImportContext.h"
#include "expat.h"

using namespace std;

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

ImportContext::~ImportContext() {}
ImportContext::ImportContext(list<BString>& errors) :
	m_state(PARSING),
	m_errors(errors),
	m_pParser(0) {}

// -------------------------------------------------------- //
// accessors
// -------------------------------------------------------- //

// fetch the current element (tag)
// (returns 0 if the stack is empty)
const char* ImportContext::element() const {
	return (m_elementStack.size()) ?
		m_elementStack.back().String() :
		0;
}

const char* ImportContext::parentElement() const {
	if(m_elementStack.size() < 2)
		return 0;
	list<BString>::const_reverse_iterator it = m_elementStack.rbegin();
	++it;
	return (*it).String();
}

list<BString>& ImportContext::errors() const {
	return m_errors;
}
const ImportContext::state_t ImportContext::state() const {
	return m_state;
}

// -------------------------------------------------------- //
// error-reporting operations
// -------------------------------------------------------- //

// register a warning to be returned once the deserialization
// process is complete.
void ImportContext::reportWarning(
	const char*			pText) {

	XML_Parser p = (XML_Parser)m_pParser;
	
	BString err = "Warning: ";
	err << pText;
	if(p) {
		err << "\n         (line " <<
			(uint32)XML_GetCurrentLineNumber(p) << ", column " <<
			(uint32)XML_GetCurrentColumnNumber(p) << ", element '" <<
			(element() ? element() : "(none)") << "')\n";
	} else
		err << "\n";
	m_errors.push_back(err);
}
		
// register a fatal error; halts the deserialization process
// as soon as possible.
void ImportContext::reportError(
	const char*			pText) {

	XML_Parser p = (XML_Parser)m_pParser;

	BString err = "FATAL ERROR: ";
	err << pText;
	if(p) {
		err << "\n             (line " <<
			(uint32)XML_GetCurrentLineNumber(p) << ", column " <<
			(uint32)XML_GetCurrentColumnNumber(p) << ", element '" <<
			(element() ? element() : "(none)") << "')\n";
	} else
		err << "\n";
	m_errors.push_back(err);
	
	m_state = ABORT;
}

// -------------------------------------------------------- //
// internal operations
// -------------------------------------------------------- //

void ImportContext::reset() {
	m_state = PARSING;
	m_elementStack.clear();
	// +++++ potential for memory leaks; reset() is currently
	//       only to be called after an identify cycle, during
	//       which no objects are created anyway, but this still
	//       gives me the shivers...
	m_objectStack.clear();
}

// END -- ImportContext.cpp --
