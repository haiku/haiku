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


// StringContent.h
// * PURPOSE
//   Implements IPersistent to store element content in
//   a BString and automatically strip leading/trailing
//   whitespace.  Doesn't handle child elements; export
//   is not implemented (triggers an error).
//
// * HISTORY
//   e.moon			7dec99		Begun

#ifndef __StringContent_H__
#define __StringContent_H__

#include <MediaDefs.h>

#include "XML.h"
#include "cortex_defs.h"

__BEGIN_CORTEX_NAMESPACE

class StringContent :
	public	IPersistent {
	
public:														// content access
	BString													content;

public:														// *** dtor, default ctors
	virtual ~StringContent() {}
	StringContent() {}
	StringContent(
		const char*										c) : content(c) {}

public:														// *** IPersistent

	// EXPORT
	
	virtual void xmlExportBegin(
		ExportContext&								context) const;
		
	virtual void xmlExportAttributes(
		ExportContext&								context) const;

	virtual void xmlExportContent(
		ExportContext&								context) const;
	
	virtual void xmlExportEnd(
		ExportContext&								context) const;

	// IMPORT	
	
	virtual void xmlImportBegin(
		ImportContext&								context);
	
	virtual void xmlImportAttribute(
		const char*										key,
		const char*										value,
		ImportContext&								context);
			
	virtual void xmlImportContent(
		const char*										data,
		uint32												length,
		ImportContext&								context);
	
	virtual void xmlImportChild(
		IPersistent*									child,
		ImportContext&								context);
		
	virtual void xmlImportComplete(
		ImportContext&								context);
};

__END_CORTEX_NAMESPACE
#endif /*__StringContent_H__ */
