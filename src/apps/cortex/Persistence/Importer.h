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


// Importer.h
//
// * PURPOSE
//   Given a stream of XML parser events, produce the object[s]
//   represented by the markup.
//
// * HISTORY
//   e.moon		30jun99   Moved actual object-building responsibility
//                      to IPersistent; this class is now internal
//                      to Cortex::XML.
//   e.moon		28jun99		Begun.  [was 'Importer']

#ifndef __Importer_H__
#define __Importer_H__

#include <list>
#include <String.h>

#include "ImportContext.h"
#include "XML.h"
#include "expat.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class Importer {

public:													// *** ctor/dtor
	virtual ~Importer();
	
	// constructs a format-guessing Importer (uses the
	// DocumentType registry)
	Importer(
		std::list<BString>&						errors);
	
	// the Importer takes ownership of the given context!
	Importer(
		ImportContext*							context);
	
	// constructs a manual Importer; the given root
	// object is populated
	Importer(
		std::list<BString>&						errors,
		IPersistent*								rootObject,
		XML::DocumentType*					docType);

	// the Importer takes ownership of the given context!
	Importer(
		ImportContext*							context,
		IPersistent*								rootObject,
		XML::DocumentType*					docType);

public:													// *** accessors

	// the import context
	const ImportContext& context() const;
	
	// matched document type
	XML::DocumentType* docType() const;
	
	// completed object (available if
	// context().state() == ImportContext::COMPLETE, or
	// if a root object was provided to the ctor)
	IPersistent* target() const;
		
public:													// *** operations

	// put the importer into 'identify mode'
	// (disengaged once the first element is encountered)
	void setIdentifyMode();

	// prepare to read the document after an identify cycle
	void reset();

	// handle a buffer; return false if an error occurs
	bool parseBuffer(
		const char*									buffer,
		uint32											length,
		bool												last);
	
public:													// *** internal operations

	// create & initialize parser
	void initParser();
	
	// clean up the parser
	void freeParser();
		
public:													// *** XML parser event hooks

	virtual void xmlElementStart(
		const char*									name,
		const char**								attributes);
		
	virtual void xmlElementEnd(
		const char*									name);
		
	virtual void xmlProcessingInstruction(
		const char*									target,
		const char*									data);

	// not 0-terminated
	virtual void xmlCharacterData(
		const char*									data,
		int32												length);
		
	// not 0-terminated
	virtual void xmlDefaultData(
		const char*									data,
		int32												length);
			
private:												// *** implementation

	XML_Parser										m_parser;
	XML::DocumentType*						m_docType;
	
	// if true, the importer is being used to identify the
	// document type -- it should halt as soon as the first
	// element is encountered.
	bool													m_identify;
	
	ImportContext* const					m_context;

	// the constructed object: if no rootObject was provided
	// in the ctor, this is only filled in once the document
	// end tag has been encountered.	
	IPersistent*									m_rootObject;
};

__END_CORTEX_NAMESPACE
#endif /*__Importer_H__*/
