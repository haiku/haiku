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


// Importer.cpp
// e.moon 28jun99

#include "Importer.h"
#include <stdexcept>

#include <Autolock.h>
#include <Debug.h>

using namespace std;

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// expat hooks
// -------------------------------------------------------- //

void _oc_handle_start(
	void* pUser,
	const XML_Char* pName,
	const XML_Char** ppAtts) {
	((Importer*)pUser)->xmlElementStart(pName, ppAtts);	
}

void _oc_handle_end(
	void* pUser,
	const XML_Char* pName) {
	((Importer*)pUser)->xmlElementEnd(pName);
}

void _oc_handle_pi(
	void* pUser,
	const XML_Char* pTarget,
	const XML_Char* pData) {
	((Importer*)pUser)->xmlProcessingInstruction(pTarget, pData);
}

void _oc_handle_char(
	void* pUser,
	const XML_Char* pData,
	int length) {
	((Importer*)pUser)->xmlCharacterData(pData, length);
}

void _oc_handle_default(
	void* pUser,
	const XML_Char* pData,
	int length) {
	((Importer*)pUser)->xmlDefaultData(pData, length);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

Importer::~Importer() {
	// clean up
	freeParser();

	delete m_context;
}


Importer::Importer(
	list<BString>&							errors) :

	m_parser(0),
	m_docType(0),
	m_identify(false),
	m_context(new ImportContext(errors)),
	m_rootObject(0) {
	
	initParser();
}

Importer::Importer(
	ImportContext*							context) :

	m_parser(0),
	m_docType(0),
	m_identify(false),
	m_context(context),
	m_rootObject(0) {
	
	ASSERT(m_context);
	
	initParser();
}

Importer::Importer(
	list<BString>&							errors,
	IPersistent*								rootObject,
	XML::DocumentType*					docType) :

	m_parser(0),
	m_docType(docType),
	m_identify(false),
	m_context(new ImportContext(errors)),
	m_rootObject(rootObject) {

	ASSERT(rootObject);
	ASSERT(docType);

	initParser();
}

Importer::Importer(
	ImportContext*							context,
	IPersistent*								rootObject,
	XML::DocumentType*					docType) :

	m_parser(0),
	m_docType(docType),
	m_identify(false),
	m_context(context),
	m_rootObject(rootObject) {

	ASSERT(m_context);
	ASSERT(rootObject);
	ASSERT(docType);

	initParser();
}
		
// -------------------------------------------------------- //
// accessors
// -------------------------------------------------------- //

// the import context
const ImportContext& Importer::context() const {
	return *m_context;
}

// matched (or provided) document type
XML::DocumentType* Importer::docType() const {
	return m_docType;
}

// completed object (available if
// context().state() == ImportContext::COMPLETE, or
// if a root object was provided to the ctor)
IPersistent* Importer::target() const {
	return m_rootObject;
}
		
// -------------------------------------------------------- //
// operations
// -------------------------------------------------------- //

// put the importer into 'identify mode'
// (disengaged once the first element is encountered)
void Importer::setIdentifyMode() {
	reset();
	m_docType = 0;
	m_identify = true;
}

void Importer::reset() {
	// doesn't forget document type from identify cycle!
		
	m_identify = false;
	m_context->reset();
	m_rootObject = 0;
}

// handle a buffer; return false if an error occurs
bool Importer::parseBuffer(
	const char* pBuffer,
	uint32 length,
	bool last) {

	ASSERT(m_parser);

	int err = XML_Parse(m_parser, pBuffer, length, last);
		
	if(!err) {
		BString str = "Parse Error: ";
		str << XML_ErrorString(XML_GetErrorCode(m_parser));
		m_context->reportError(str.String());
		return false;

	} else
		return true;
}
	
// -------------------------------------------------------- //
// internal operations
// -------------------------------------------------------- //

// create & initialize parser
void Importer::initParser() {
	ASSERT(!m_parser);
	m_parser = XML_ParserCreate(0);
	m_context->m_pParser = m_parser;
	
	XML_SetElementHandler(
		m_parser,
		&_oc_handle_start,
		&_oc_handle_end);
	
	XML_SetProcessingInstructionHandler(
		m_parser,
		&_oc_handle_pi);

	XML_SetCharacterDataHandler(
		m_parser,
		&_oc_handle_char);
		
	XML_SetDefaultHandlerExpand(
		m_parser,
		&_oc_handle_default);
		
	XML_SetUserData(
		m_parser,
		(void*)this);	
}
	
// clean up the parser
void Importer::freeParser() {
	ASSERT(m_parser);
	XML_ParserFree(m_parser);
	m_parser = 0;
}

// -------------------------------------------------------- //
// XML parser event hooks
// -------------------------------------------------------- //

void Importer::xmlElementStart(
	const char*			pName,
	const char**		ppAttributes) {

	if(m_context->m_state != ImportContext::PARSING)
		return;

	IPersistent* target = 0;

	if(!m_context->m_elementStack.size()) {
		// this is the first element; identify or verify document type
		
		if(m_rootObject) {
			// test against expected document type
			ASSERT(m_docType);
			if(m_docType->rootElement != pName) {
				BString err("Unexpected document element (should be <");
				err << m_docType->rootElement << "/>";
				m_context->reportError(err.String());
				return;
			}
			
			// target the provided root object
			target = m_rootObject;
		}
		else {
			// look up doc type
			BAutolock _l(XML::s_docTypeLock);
			XML::doc_type_map::iterator it = XML::s_docTypeMap.find(
				BString(pName));
		
			if(it != XML::s_docTypeMap.end())
				m_docType = (*it).second;
			else {
				// whoops, don't know how to handle this element:
				BString err("No document type registered for element '");
				err << pName << "'.";
				m_context->reportError(err.String());
				return;
			}
				
			if(m_identify) {
				// end of identify cycle
				m_context->m_state = ImportContext::COMPLETE;
				return;
			}
		}
	}
	// at this point, there'd better be a valid document type
	ASSERT(m_docType);	
	
	// push element name onto the stack
	m_context->m_elementStack.push_back(pName);

	// try to create an object for this element if necessary
	if(!target)
		target = m_docType->objectFor(pName);	
	
	if(target) {
		// call 'begin import' hook
		m_context->m_objectStack.push_back(
			make_pair(m_context->element(), target));
		target->xmlImportBegin(*m_context);
		
		// error? bail
		if(m_context->state() != ImportContext::PARSING)
			return;
	
		// walk attributes
		while(*ppAttributes) {
			target->xmlImportAttribute(	
				ppAttributes[0],
				ppAttributes[1],
				*m_context);

			// error? bail
			if(m_context->state() != ImportContext::PARSING)
				return;
			
			ppAttributes += 2;
		}
	} else {
		// no object directly maps to this element; hand to
		// the current focus object

		ASSERT(m_context->m_objectStack.size());
		IPersistent* curObject = m_context->m_objectStack.back().second;
		ASSERT(curObject);
		
		curObject->xmlImportChildBegin(
			pName,
			*m_context);

		// error? bail
		if(m_context->state() != ImportContext::PARSING)
			return;

		// walk attributes
		while(*ppAttributes) {
			curObject->xmlImportChildAttribute(	
				ppAttributes[0],
				ppAttributes[1],
				*m_context);

			// error? bail
			if(m_context->state() != ImportContext::PARSING)
				return;
			
			ppAttributes += 2;
		}
	}
}
		
void Importer::xmlElementEnd(
	const char*			pName) {

	if(m_context->m_state != ImportContext::PARSING)
		return;
	ASSERT(m_docType);

//	PRINT(("Importer::xmlElementEnd(): %s\n", pName));

	// compare name to element on top of stack
	if(!m_context->m_elementStack.size() ||
		m_context->m_elementStack.back() != pName) {
		m_context->reportError("Mismatched end tag.");
		return;
	}

	// see if it matches the topmost object
	IPersistent* pObject = 0;
	if(!m_context->m_objectStack.size()) {
		m_context->reportError("No object being constructed.");
		return;
	}
	if(m_context->m_objectStack.back().first == m_context->element()) {
		// matched; pop it
		pObject = m_context->m_objectStack.back().second;
		m_context->m_objectStack.pop_back();
	}

	if(pObject) {
		// notify object that import is complete
		pObject->xmlImportComplete(
			*m_context);
		
		// error? bail
		if(m_context->state() != ImportContext::PARSING)
			return;

		if(m_context->m_objectStack.size()) {
			// hand the newly-constructed child to its parent
			m_context->m_objectStack.back().second->xmlImportChild(
				pObject,
				*m_context);
		} else {
			// done
			ASSERT(m_context->m_elementStack.size() == 1);
			m_context->m_state = ImportContext::COMPLETE;
			if(m_rootObject) {
				ASSERT(m_rootObject == pObject);
			} else
				m_rootObject = pObject;
		}
	}
	else {
		// notify current topmost object
		ASSERT(m_context->m_objectStack.size());
		IPersistent* curObject = m_context->m_objectStack.back().second;
		ASSERT(curObject);
		
		curObject->xmlImportChildComplete(
			pName,
			*m_context);
	}
	
	// remove entry from element stack
	m_context->m_elementStack.pop_back();
	ASSERT(m_context->m_objectStack.size() <= m_context->m_elementStack.size());
}
	
void Importer::xmlProcessingInstruction(
	const char*			pTarget,
	const char*			pData) {

	if(m_context->m_state != ImportContext::PARSING)
		return;
//	PRINT(("Importer::xmlProcessingInstruction(): %s, %s\n",
//		pTarget, pData));
	
}

// not 0-terminated
void Importer::xmlCharacterData(
	const char*			pData,
	int32					length) {

	if(m_context->m_state != ImportContext::PARSING)
		return;

	// see if the current element matches the topmost object
	IPersistent* pObject = 0;
	if(!m_context->m_objectStack.size()) {
		m_context->reportError("No object being constructed.");
		return;
	}

	pObject = m_context->m_objectStack.back().second;
	if(m_context->m_objectStack.back().first == m_context->element()) {
		
		pObject->xmlImportContent(
			pData,
			length,
			*m_context);
	}
	else {
		pObject->xmlImportChildContent(
			pData,
			length,
			*m_context);
	}
}
		
// not 0-terminated
void Importer::xmlDefaultData(
	const char*			pData,
	int32					length) {
	
	if(m_context->m_state != ImportContext::PARSING)
		return;
//	PRINT(("Importer::xmlDefaultData()\n"));
}
		
// END -- Importer.cpp --
