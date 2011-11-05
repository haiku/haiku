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


// XML.cpp
// e.moon 1jul99

#include "XML.h"
#include "Importer.h"

#include <Autolock.h>
#include <Debug.h>

#include "array_delete.h"
#include "set_tools.h"

using namespace std;

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// static members
// -------------------------------------------------------- //

XML::doc_type_map XML::s_docTypeMap;
BLocker XML::s_docTypeLock("XML::s_docTypeLock");

const BMimeType XML::DocumentType::s_defaultMimeType("text/xml");

// -------------------------------------------------------- //
// document type operations
// -------------------------------------------------------- //

// takes responsibility for the given type object

/*static*/
void XML::AddDocumentType(
	XML::DocumentType*					type) {
	
	ASSERT(type);
	BAutolock _l(s_docTypeLock);

//	s_docTypeMap.insert(
//		make_pair(type->rootElement, type));
	s_docTypeMap.insert(
		pair<const BString, XML::DocumentType*>(type->rootElement, type));
}
	
// -------------------------------------------------------- //
// import/export operations
// -------------------------------------------------------- //

// identify object in stream
// returns:
// - B_OK on success, or
// - B_BAD_TYPE if no document type matches the root
//   element of the stream, or 
// - B_IO_ERROR if the document is malformed, or if a
//   read error occurs.

/*static*/
status_t XML::Identify(
	BDataIO*										stream,
	DocumentType**							outType,
	list<BString>*							outErrors) {

	ASSERT(stream);
	
	// prepare the input buffer
	const uint32 bufferSize = 4096;
	char* buffer = new char[bufferSize];
	array_delete<char> _d(buffer);

	// prepare an Importer to figure document type (from first element)
	Importer i(*outErrors);
	i.setIdentifyMode();
		
	while(
		i.context().state() == ImportContext::PARSING) {

		// read chunk (no 0 terminator)
		ssize_t readCount = stream->Read(buffer, bufferSize);
		if(readCount == 0)
			 // done
			break;
		else if(readCount < 0) {
			// error
			BString err = "Read error: '";
			err << strerror(readCount) << "'; ABORTING.";
			outErrors->push_back(err);

			return B_IO_ERROR;
		}
		
		// feed to parser
		if(!i.parseBuffer(
			buffer, readCount, !stream)) {
			break;
		}
	}

	// return found type	
	if(i.docType()) {
		*outType = i.docType();
		return B_OK;
	}
	else return B_BAD_TYPE;
}

// read the root object from the given stream

/*static*/
status_t XML::Read(
	BDataIO*										stream,
	IPersistent**								outObject,
	list<BString>*							outErrors) {

	Importer i(*outErrors);
	status_t err = _DoRead(stream, i, outErrors);
	if(err == B_OK) {
		// return completed object
		ASSERT(i.target());
		*outObject = i.target();
	}
	return err;
}

/*static*/
status_t XML::Read(
	BDataIO*										stream,
	IPersistent**								outObject,
	ImportContext*							context) {

	Importer i(context);
	status_t err = _DoRead(stream, i, &context->errors());
	if(err == B_OK) {
		// return completed object
		ASSERT(i.target());
		*outObject = i.target();
	}
	return err;
}

// [e.moon 26nov99]
// populate the provided root object from the given
// XML stream.  you need to give the expected root
// (document) element name corresponding to the
// item you provide.
// returns:
// - B_OK on success, or
// - B_IO_ERROR if the document is malformed, or if a
//   read error occurs, or
// - B_ERROR

/*static*/
status_t XML::Read(
	BDataIO*										stream,
	IPersistent*								rootObject,
	XML::DocumentType*					documentType,
	list<BString>*							outErrors) {

	Importer i(*outErrors, rootObject, documentType);
	return _DoRead(stream, i, outErrors);
}

/*static*/
status_t XML::Read(
	BDataIO*										stream,
	IPersistent*								rootObject,
	XML::DocumentType*					documentType,
	ImportContext*							context) {

	Importer i(context, rootObject, documentType);
	return _DoRead(stream, i, &context->errors());
}

/*static*/
status_t XML::_DoRead(
	BDataIO*										stream,
	Importer&										i,
	list<BString>*							outErrors) {
	
	// prepare the input buffer
	const uint32 bufferSize = 4096;
	char* buffer = new char[bufferSize];
	array_delete<char> _d(buffer);

	while(
		i.context().state() == ImportContext::PARSING) {

		// read chunk (no 0 terminator)
		ssize_t readCount = stream->Read(buffer, bufferSize);
		if(readCount == 0)
			 // done
			break;
		else if(readCount < 0) {
			// error
			BString err = "Read error: '";
			err << strerror(readCount) << "'; ABORTING.";
			outErrors->push_back(err);
			return B_IO_ERROR;
		}
		
		// feed to parser
		if(!i.parseBuffer(
			buffer, readCount, !stream)) {
			break;
		}
	}

	status_t err = B_ERROR;
	if(i.context().state() == ImportContext::COMPLETE)
		err = B_OK;

	// clean up
	return err;
}

// write the given object to the given stream

/*static*/
status_t XML::Write(
	BDataIO*										stream,
	IPersistent*								object,
	BString*										outError) {

	ASSERT(object);
	
	ExportContext context(stream);
	status_t err = context.writeObject(object);
	if(err < B_OK)
		*outError = context.errorText();
	return err;
}

// -------------------------------------------------------- //
// XML::DocumentType
// -------------------------------------------------------- //

class _NullMapping : public XMLElementMapping {
public:
	_NullMapping(
		const char*									_element) :
		XMLElementMapping(_element) {}
	
	IPersistent* create() const { return 0; }
};

XML::DocumentType::~DocumentType() {
	// clean up
	ptr_set_delete(m_mappingSet.begin(), m_mappingSet.end());
}

XML::DocumentType::DocumentType(
	const char*									_rootElement,
	const char*									_mimeType) :
	rootElement(_rootElement),
	mimeType(_mimeType ? _mimeType : s_defaultMimeType.Type()) {}

// *** 'factory' interface

// The DocumentType takes ownership of the given mapping
// object.  If a mapping for the element already exists,
// the provided object is deleted and the method returns
// B_NAME_IN_USE.
status_t XML::DocumentType::addMapping(
	XMLElementMapping*					mapping) {

	pair<mapping_set::iterator, bool> ret =	m_mappingSet.insert(mapping);
	if(!ret.second) {
		delete mapping;
		return B_NAME_IN_USE;
	} else
		return B_OK;
}

IPersistent* XML::DocumentType::objectFor(
	const char*									element) {

	_NullMapping m(element);
	mapping_set::iterator it = m_mappingSet.find(&m);

	return (it != m_mappingSet.end()) ?
		(*it)->create() : 0;
}

// END -- XML.cpp --
