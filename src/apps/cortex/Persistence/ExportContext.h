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


// ExportContext.h
// * PURPOSE
//   Describe the state of a serialization ('save') operation.
//   The 'load' equivalent is ImportContext.
//
// * HISTORY
//   e.moon		28jun99		Begun

#ifndef __ExportContext_H__
#define __ExportContext_H__

#include <Debug.h>

#include <String.h>
#include <iostream>

#include <list>
#include <utility>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class BDataIO;
class IPersistent;
class ExportContext;

// writeAttr() helper
inline BString& _pad_with_spaces(BString& out, const char* text,
	ExportContext& context, uint16 column);


class ExportContext {
public:
	ExportContext();
	ExportContext(BDataIO* stream);
	virtual ~ExportContext();

	// the output stream
	BDataIO* stream;

	// the element stack
	struct element_entry {
		element_entry() : hasAttributes(false), hasContent(false) {}

		BString	name;
		bool	hasAttributes;
		bool	hasContent;
	};

	typedef std::list<element_entry> element_list;
	element_list m_elementStack;

public:													// *** XML formatting helpers

	// writes a start tag.  should be called from
	// IPersistent::xmlExportBegin()
	// (or xmlExportContent(), if you're writing nested elements)

	void beginElement(const char* name);
	
	// writes an end tag corresponding to the current element.
	// should only be called from IPersistent::xmlExportEnd() or
	// xmlExportContent().
	void endElement();
	
	// indicates that content follows (writes the end of the
	// current element's start tag.)
	void beginContent();
	
//	// writes an attribute.
//	// should only be called from IPersistent::xmlExportAttributes().
//	template <class T>
//	void writeAttr(
//		const char*									key,
//		T														value) {
//
//		if(!m_objectStack.size()) {
//			reportError("writeAttr(): no object being written.\n");
//			return;
//		}
//		ASSERT(m_elementStack.size());
//		if(m_state != WRITE_ATTRIBUTES &&
//			m_state != WRITE_CONTENT) {
//			reportError("writeAttr(): not allowed (state mismatch).\n");
//			return;
//		}
//	
//		m_elementStack.back().hasAttributes = true;
//
//		BString out;
//		out << "\n" << indentString() << key;
//		_pad_with_spaces(out, key, *this, m_attrColumn) << " = '" << value << '\'';
//		
//		writeString(out);
//	}
	
	// [e.moon 22dec99]
	// non-template forms of writeAttr()

#if B_BEOS_VERSION > B_BEOS_VERSION_4_5
	void writeAttr(
		const char*									key,
		int8												value);

	void writeAttr(
		const char*									key,
		uint8												value);

	void writeAttr(
		const char*									key,
		int16												value);

	void writeAttr(
		const char*									key,
		uint16											value);
#endif

	void writeAttr(
		const char*									key,
		int32												value);

	void writeAttr(
		const char*									key,
		uint32											value);

	void writeAttr(
		const char*									key,
		int64												value);

	void writeAttr(
		const char*									key,
		uint64											value);

	void writeAttr(
		const char*									key,
		const char*									value);

	void writeAttr(
		const char*									key,
		const BString&							value);

	void writeAttr(
		const char*									key,
		float												value);

	// writes a child object.
	// should only be called from IPersistent::xmlExportContent().
	// returns B_OK on success, or B_ERROR if an error occurred.
	status_t writeObject(
		IPersistent*								object);
	
	// writes an arbitrary string to the stream (calls reportError()
	// on failure.)
	status_t writeString(
		const BString&							string);
	
	status_t writeString(
		const char*									data,
		ssize_t											length);
		
public:													// *** indentation helpers

	// return a string padded with spaces to the current
	// indent level
	const char* indentString() const;
	
	// return the current indent level
	uint16 indentLevel() const;
	
	// decrease the indent level
	void indentLess();
	
	// increase the indent level
	void indentMore();
	
	// +++++ extra formatting controls needed [e.moon 1dec99]
	// * attrColumn access
	// * single vs. multi-line element formatting
	

public:													// *** error operations

	// register a fatal error; halts the write process
	// as soon as possible.
	void reportError(
		const char*									text);
	
	// fetch error text
	const char* errorText() const { return m_error.String(); }

private:				// members

	// * Indentation/formatting
	
	uint16												m_indentLevel;
	uint16												m_indentIncrement;
	
	uint16												m_attrColumn;
	
	BString												m_indentString;

	// * State
	
	enum state_t {
		INIT,
		WRITE_BEGIN,
		WRITE_ATTRIBUTES,
		WRITE_CONTENT,
		WRITE_END,
		ABORT
	};

	state_t												m_state;
	BString												m_error;

	// object stack

	struct object_entry {
		object_entry() : element(0), object(0) {}
			
		const char*		element;
		IPersistent*	object;
	};
	
	typedef std::list<object_entry> object_list;
	object_list										m_objectStack;
	
private:
	void _dumpElementStack(
		BString&										out);
};

// ExportContext::writeAttr() helper
inline BString& _pad_with_spaces(
	BString&											out,
	const char*										text,
	ExportContext&								context,
	uint16												column) {

	int16 spaces = column - (strlen(text) + context.indentLevel());
	if(spaces < 0) spaces = 0;
	while(spaces--) out << ' ';
	return out;
}

__END_CORTEX_NAMESPACE

#endif /*__ExportContext_H__*/
