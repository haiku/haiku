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


// xml_export_utils.h
// * PURPOSE
//   helper functions for writing XML representations of
//   C++ objects.
//
// * HISTORY
//   e.moon		5jul99		Begun

#ifndef __xml_export_utils_H__
#define __xml_export_utils_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// Writes the correct number of spaces to the given stream,
// so that text written after this call will start at the given
// column.
// Assumes that the given text string has already been written
// (with the level of indentation currently stored in the given
// context.)

inline ostream& pad_with_spaces(
	ostream&						str,
	const char*						text,
	ExportContext&			context,
	uint16							column=30) {

	int16 spaces = column - (strlen(text) + context.indentLevel());
	if(spaces < 0) spaces = 0;
	while(spaces--) str << ' ';
	return str;
}


// Writes the given key/value as an XML attribute (a newline
// is written first, to facilitate compact representation of
// elements with no attributes.)

template <class T>
void write_attr(
	const char* key,
	T value,
	ostream& stream,
	ExportContext& context) {
	
	stream << endl << context.indentString() << key;
	pad_with_spaces(stream, key, context) << " = '" << value << '\'';
}

__END_CORTEX_NAMESPACE

#endif /*__xml_export_utils_H__*/
