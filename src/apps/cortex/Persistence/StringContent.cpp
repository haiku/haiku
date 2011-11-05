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


// StringContent.cpp

#include "StringContent.h"
#include "ImportContext.h"

#include <cctype>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// EXPORT [not implemented]
// -------------------------------------------------------- //

void StringContent::xmlExportBegin(
	ExportContext&								context) const {
	context.reportError("StringContent: no export");
}	
void StringContent::xmlExportAttributes(
	ExportContext&								context) const {
	context.reportError("StringContent: no export");
}
void StringContent::xmlExportContent(
	ExportContext&								context) const {
	context.reportError("StringContent: no export");
}
void StringContent::xmlExportEnd(
	ExportContext&								context) const {
	context.reportError("StringContent: no export");
}

// -------------------------------------------------------- //
// IMPORT	
// -------------------------------------------------------- //

void StringContent::xmlImportBegin(
	ImportContext&								context) {}

void StringContent::xmlImportAttribute(
	const char*										key,
	const char*										value,
	ImportContext&								context) {}
		
void StringContent::xmlImportContent(
	const char*										data,
	uint32												length,
	ImportContext&								context) {
	content.Append(data, length);
}

void StringContent::xmlImportChild(
	IPersistent*									child,
	ImportContext&								context) {
	context.reportError("StringContent: child not expected");
}
	
void StringContent::xmlImportComplete(
	ImportContext&								context) {

	// chomp leading/trailing whitespace
	if(content.Length() == 0)
		return;

	int32 last = 0;
	for(; last < content.Length() && isspace(content[last]); ++last) {}
	if(last > 0)
		content.Remove(0, last);

	last = content.Length() - 1;
	int32 from = last;
	for(; from > 0 && isspace(content[from]); --from) {}
	if(from < last)
		content.Remove(from+1, last-from);
}

// END -- StringContent.cpp --

