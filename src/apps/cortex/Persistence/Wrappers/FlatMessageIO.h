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


// FlatMessageIO.h
// * PURPOSE
//   Efficient export/import of BMessages to and from
//   XML using the Cortex persistence library.
//   Messages are stored in flattened form.
// * HISTORY
//   e.moon		6jul99		Begun.
//
// Example:
//
// <flat-BMessage
//   encoding = 'base64'>
//   nKJNFBlkn3lknbxlkfnbLKN/lknlknlDSLKn3lkn3l2k35234ljk234
//   lkdsg23823nlknsdlkbNDSLKBNlkn3lk23n4kl23n423lknLKENL+==
// </flat-BMessage>

#ifndef __FlatMessageIO_H__
#define __FlatMessageIO_H__

#include <Message.h>
#include <String.h>
#include "XML.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class FlatMessageIO :
	public		IPersistent {

public:					// *** ctor/dtor/accessor
	virtual ~FlatMessageIO();

	FlatMessageIO();
	
	// When given a message to export, this object does NOT take
	// responsibility for deleting it.  It will, however, handle
	// deletion of an imported BMessage.  
	
	FlatMessageIO(const BMessage* message);
	void setMessage(BMessage* message);	
	
	// Returns 0 if no message has been set, and if no message has
	// been imported.
	
	const BMessage* message() const { return m_message; }
	
	// Returns true if the message will be automatically deleted.
	
	bool ownsMessage() const { return m_ownMessage; }

public:					// *** static setup method
	// call this method to install hooks for the tags needed by
	// FlatMessageIO into the given document type
	static void AddTo(XML::DocumentType* pDocType);
	
public:					// *** XML formatting
	static const char* const			s_element;
	static const uint16					s_encodeToMax			= 72;
	static const uint16					s_encodeToMin			= 24;

public:					// *** IPersistent impl.

//	virtual void xmlExport(
//		ostream& 					stream,
//		ExportContext&		context) const;

	// EXPORT:
	
	void xmlExportBegin(
		ExportContext& context) const;
		
	void xmlExportAttributes(
		ExportContext& context) const;
	
	void xmlExportContent(
		ExportContext& context) const;
	
	void xmlExportEnd(
		ExportContext& context) const;


	// IMPORT:

	virtual void xmlImportBegin(
		ImportContext&		context);

	virtual void xmlImportAttribute(
		const char*					key,
		const char*					value,
		ImportContext&		context);
	
	virtual void xmlImportContent(
		const char*					data,
		uint32						length,
		ImportContext&		context);
	
	virtual void xmlImportChild(
		IPersistent*			child,
		ImportContext&		context);
			
	virtual void xmlImportComplete(
		ImportContext&		context);
		
private:					// *** members
	bool						m_ownMessage;
	BMessage*			m_message;

	// encoded data is cached here during the import process	
	BString				m_base64Data;
};

__END_CORTEX_NAMESPACE

#endif /*__FlatMessageIO_H__*/
