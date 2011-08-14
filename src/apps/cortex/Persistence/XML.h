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


// XML.h
// * PURPOSE
//   A central access point for Cortex's XML import/export
//   services.  A completely static class.
//
// * RESPONSIBILITIES
//   - Maintain a set of XML::DocumentType objects, each
//     containing the information needed to import a particular
//     kind of XML document into native objects.
//
//   - Provide a simple API for importing and exporting
//     IPersistent objects from/to streams.
//
// * HISTORY
//   e.moon		4oct99		API changes:
//											- BDataIO used in place of standard streams.
//											- Factory folded into XML::DocumentType
//
//   e.moon		29jun99		Begun

#ifndef __XML_H__
#define __XML_H__

#include "IPersistent.h"
#include "XMLElementMapping.h"

#include <map>
#include <set>

#include <DataIO.h>
#include <Locker.h>
#include <Mime.h>
#include <String.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class Importer;

// -------------------------------------------------------- //
// class XML
// -------------------------------------------------------- //

class XML {
	// internal import helper
	friend class Importer;
	
public:													// *** types
	class DocumentType;

public:													// *** document type operations

	// takes responsibility for the given type object
	static void AddDocumentType(
		XML::DocumentType*					type);
	
public:													// *** import/export operations

	// identify object in stream.
	// returns:
	// - B_OK on success, or
	// - B_BAD_TYPE if no document type matches the root
	//   element of the stream, or 
	// - B_IO_ERROR if the document is malformed, or if a
	//   read error occurs.
	
	static status_t Identify(
		BDataIO*										stream,
		DocumentType**							outType,
		std::list<BString>*						outErrors);

	// create & populate the root object from the given
	// XML stream.
	// returns:
	// - B_OK on success, or
	// - B_IO_ERROR if the document is malformed, or if a
	//   read error occurs, or
	// - B_ERROR

	static status_t Read(
		BDataIO*										stream,
		IPersistent**								outObject,
		std::list<BString>*						outErrors);

	static status_t Read(
		BDataIO*										stream,
		IPersistent**								outObject,
		ImportContext*							context); //nyi
	
	// [e.moon 26nov99]
	// populate the provided root object from the given
	// XML stream.  you need to provide a document type
	// that corresponds to the given object.
	// returns:
	// - B_OK on success, or
	// - B_IO_ERROR if the document is malformed, or if a
	//   read error occurs, or
	// - B_ERROR

	static status_t Read(
		BDataIO*										stream,
		IPersistent*								rootObject,
		XML::DocumentType*					documentType,
		std::list<BString>*						outErrors);

	static status_t Read(
		BDataIO*										stream,
		IPersistent*								rootObject,
		XML::DocumentType*					documentType,
		ImportContext*							context);

	// create an ExportContext and use it to write the given object
	// to the given stream
		
	static status_t Write(
		BDataIO*										stream,
		IPersistent*								object,
		BString*										outError);
			
private:												// static members

	typedef std::map<BString, DocumentType*> doc_type_map;
	
	static doc_type_map						s_docTypeMap;
	static BLocker								s_docTypeLock;
	
private:												// implementation
	static status_t _DoRead(
		BDataIO*										stream,
		Importer&										i,
		std::list<BString>*						outErrors); //nyi
};

// -------------------------------------------------------- //
// class XML::DocumentType
// -------------------------------------------------------- //

class XML::DocumentType {
public:													// *** constant members
	const BString									rootElement;
	const BMimeType								mimeType;
		
	static const BMimeType				s_defaultMimeType;

public:													// *** ctor/dtors
	virtual ~DocumentType();

	DocumentType(
		const char*									_rootElement,
		const char*									_mimeType=0);

public:													// *** 'factory' interface

	// The DocumentType takes ownership of the given mapping
	// object.  If a mapping for the element already exists,
	// the provided object is deleted and the method returns
	// B_NAME_IN_USE.
	virtual status_t addMapping(
		XMLElementMapping*					mapping);

	// returns 0 if no mapping found for the given element
	virtual IPersistent* objectFor(
		const char*									element);

private:												// implementation

	typedef std::set<XMLElementMapping*, _mapping_ptr_less> mapping_set;
	mapping_set										m_mappingSet;
};

__END_CORTEX_NAMESPACE

#endif /*__XML_H__*/
