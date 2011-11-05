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


// ImportContext.h
// * PURPOSE
//   Describe the state of a deserialization ('load') operation.
//   The 'save' equivalent is ExportContext.
//
// * HISTORY
//   e.moon		29jun99		Begun

#ifndef __ImportContext_H__
#define __ImportContext_H__

#include <list>
#include <utility>
#include <String.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class IPersistent;

class ImportContext {
	friend class Importer;

public:													// *** types
	enum state_t {
		PARSING,
		COMPLETE,
		ABORT
	};

public:													// *** ctor/dtor
	virtual ~ImportContext();
	ImportContext(
		std::list<BString>&						errors);

public:													// *** accessors

	// fetch the current element (tag)
	// (returns 0 if the stack is empty)
	const char* element() const;
	
	// fetch the current element's parent
	// (returns 0 if the stack is empty or the current element is top-level)
	const char* parentElement() const;

	std::list<BString>& errors() const;
	const state_t state() const;

public:													// *** error-reporting operations

	// register a warning to be returned once the deserialization
	// process is complete.
	void reportWarning(
		const char*									text);
		
	// register a fatal error; halts the deserialization process
	// as soon as possible.
	void reportError(
		const char*									text);

protected:											// *** internal operations

	void reset();

private:												// *** members
	state_t												m_state;
	std::list<BString>&							m_errors;
	
	std::list<BString>								m_elementStack;
	typedef std::pair<const char*, IPersistent*> object_entry;
	std::list<object_entry>						m_objectStack;
	
	void*													m_pParser;
};
__END_CORTEX_NAMESPACE
#endif /*__ImportContext_H__*/
