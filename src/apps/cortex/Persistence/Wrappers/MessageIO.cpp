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


// MessageIO.cpp

#include "MessageIO.h"

#include <BeBuild.h>
#include <Debug.h>

#include <cstdlib>
#include <cstring>
#include <cctype>

#include <vector>
#include <utility>

using namespace std;

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

const char* const MessageIO::s_element = "BMessage";

const char* _boolEl			= "bool";
const char* _int8El			= "int8";
const char* _int16El		= "int16";
const char* _int32El		= "int32";
const char* _int64El		= "int64";
const char* _floatEl		= "float";
const char* _doubleEl		= "double";
const char* _stringEl		= "string";
const char* _pointEl		= "point";
const char* _rectEl			= "rect";

// -------------------------------------------------------- //
// *** ctor/dtor/accessor
// -------------------------------------------------------- //

MessageIO::~MessageIO() {
	if(m_ownMessage && m_message)
		delete m_message;
}

MessageIO::MessageIO() :
	m_ownMessage(true),
	m_message(0) {}

// When given a message to export, this object does NOT take
// responsibility for deleting it.  It will, however, handle
// deletion of an imported BMessage.  

MessageIO::MessageIO(
	const BMessage*						message) :
	m_ownMessage(false),
	m_message(const_cast<BMessage*>(message)) {
	
	ASSERT(m_message);
}

void MessageIO::setMessage(
	BMessage*									message) {

	if(m_ownMessage && m_message)
		delete m_message;
	m_ownMessage = false;
	m_message = message;

	ASSERT(m_message);
}

// -------------------------------------------------------- //
// *** static setup method
// -------------------------------------------------------- //
// call this method to install hooks for the tags needed by
// MessageIO into the given document type

/*static*/
void MessageIO::AddTo(
	XML::DocumentType*				docType) {

	docType->addMapping(new Mapping<MessageIO>(s_element));
}

// -------------------------------------------------------- //
// EXPORT:
// -------------------------------------------------------- //

void MessageIO::xmlExportBegin(
	ExportContext&						context) const {

	if(!m_message) {
		context.reportError("No message data to export.\n");
		return;
	}
	context.beginElement(s_element);
}
	
void MessageIO::xmlExportAttributes(
	ExportContext&						context) const {
	
	if(m_message->what)
		context.writeAttr("what", m_message->what);
	if(m_name.Length())
		context.writeAttr("name", m_name.String());
}

void MessageIO::xmlExportContent(
	ExportContext&						context) const {


	ASSERT(m_message);
	status_t err;

	// +++++ the approach:
	// 1) build a list of field names
	// 2) export fields sorted first by index, then name	
	
	typedef vector<BString> field_set;
	field_set fields;
	
#ifdef B_BEOS_VERSION_DANO
	const 
#endif
	char* name;
	type_code type;
	int32 count;
	for(
		int32 n = 0;
		m_message->GetInfo(B_ANY_TYPE, n, &name, &type, &count) == B_OK;
		++n) {	
		fields.push_back(name);	
	}
	
	if(!fields.size())
		return;
		
	context.beginContent();

	bool done = false;
	for(int32 n = 0; !done; ++n) {
	
		done = true;

		for(
			uint32 fieldIndex = 0;
			fieldIndex < fields.size();
			++fieldIndex) {
			
			if(m_message->GetInfo(
				fields[fieldIndex].String(),
				&type,
				&count) < B_OK || n >= count)
				continue;

			// found a field at the current index, so don't give up
			done = false;
			
			err = _exportField(
				context,
				m_message,
				type,
				fields[fieldIndex].String(),
				n);
				
			if(err < B_OK) {
				BString errText;
				errText << "Couldn't export field '" << fields[fieldIndex] <<
					"' index " << n << ": " << strerror(err) << "\n";
				context.reportError(errText.String());
				return;
			}
		}
	}
}

void MessageIO::xmlExportEnd(
	ExportContext&						context) const {
	context.endElement();
}


// -------------------------------------------------------- //
// IMPORT:
// -------------------------------------------------------- //

void MessageIO::xmlImportBegin(
	ImportContext&						context) {

	// create the message
	if(m_message) {
		if(m_ownMessage)
			delete m_message;
	}
	m_message = new BMessage();
	m_name.SetTo("");
}

void MessageIO::xmlImportAttribute(
	const char*								key,
	const char*								value,
	ImportContext&						context) {
	
	ASSERT(m_message);
	
	if(!strcmp(key, "what"))
		m_message->what = atol(value);
	else if(!strcmp(key, "name"))
		m_name.SetTo(value);
}

void MessageIO::xmlImportContent(
	const char*								data,
	uint32										length,
	ImportContext&						context) {}

void MessageIO::xmlImportChild(
	IPersistent*							child,
	ImportContext&						context) {
	
	ASSERT(m_message);
	
	if(strcmp(context.element(), s_element) != 0) {
		context.reportError("Unexpected child element.\n");
		return;
	}

	MessageIO* childMessageIO = dynamic_cast<MessageIO*>(child);
	ASSERT(childMessageIO);
	
	m_message->AddMessage(
		childMessageIO->m_name.String(),
		childMessageIO->m_message);
}
		
void MessageIO::xmlImportComplete(
	ImportContext&						context) {}
	
void MessageIO::xmlImportChildBegin(
	const char*								name,
	ImportContext&						context) {

	// sanity checks
	
	ASSERT(m_message);
	
	if(strcmp(context.parentElement(), s_element) != 0) {
		context.reportError("Unexpected parent element.\n");
		return;
	}
	
	if(!_isValidMessageElement(context.element())) {
		context.reportError("Invalid message field element.\n");
		return;
	}
	
	m_fieldData.SetTo("");
}

void MessageIO::xmlImportChildAttribute(
	const char*								key,
	const char*								value,
	ImportContext&						context) {
	
	if(!strcmp(key, "name"))
		m_fieldName.SetTo(value);
	if(!strcmp(key, "value"))
		m_fieldData.SetTo(value);
}

void MessageIO::xmlImportChildContent(
	const char*								data,
	uint32										length,
	ImportContext&						context) {

	m_fieldData.Append(data, length);
}

void MessageIO::xmlImportChildComplete(
	const char*								name,
	ImportContext&						context) {

	ASSERT(m_message);
	
	status_t err = _importField(
		m_message,
		name,
		m_fieldName.String(),
		m_fieldData.String());	
	if(err < B_OK) {
		context.reportWarning("Invalid field data.\n");
	}
}

// -------------------------------------------------------- //
// implementation
// -------------------------------------------------------- //

bool MessageIO::_isValidMessageElement(
	const char*								element) const {

	if(!strcmp(element, _boolEl)) return true;
	if(!strcmp(element, _int8El)) return true;
	if(!strcmp(element, _int16El)) return true;
	if(!strcmp(element, _int32El)) return true;
	if(!strcmp(element, _int64El)) return true;
	if(!strcmp(element, _floatEl)) return true;
	if(!strcmp(element, _doubleEl)) return true;
	if(!strcmp(element, _stringEl)) return true;
	if(!strcmp(element, _pointEl)) return true;
	if(!strcmp(element, _rectEl)) return true;

	return false;	
}

status_t MessageIO::_importField(
	BMessage*									message,
	const char*								element,
	const char*								name,
	const char*								data) {

	// skip leading whitespace
	while(*data && isspace(*data)) ++data;
	
	if(!strcmp(element, _boolEl)) {
		bool v;
		if(!strcmp(data, "true") || !strcmp(data, "1"))
			v = true;
		else if(!strcmp(data, "false") || !strcmp(data, "0"))
			v = false;
		else
			return B_BAD_VALUE;
		return message->AddBool(name, v);
	}
	
	if(!strcmp(element, _int8El)) {
		int8 v = atoi(data);
		return message->AddInt8(name, v);
	}
	if(!strcmp(element, _int16El)) {
		int16 v = atoi(data);
		return message->AddInt16(name, v);
	}
	if(!strcmp(element, _int32El)) {
		int32 v = atol(data);
		return message->AddInt32(name, v);
	}
	if(!strcmp(element, _int64El)) {
//		int64 v = atoll(data);
		int64 v = strtoll(data, 0, 10);
		return message->AddInt64(name, v);
	}
	if(!strcmp(element, _floatEl)) {
		float v = (float)atof(data);
		return message->AddFloat(name, v);
	}
	if(!strcmp(element, _doubleEl)) {
		double v = atof(data);
		return message->AddDouble(name, v);
	}
	
	if(!strcmp(element, _stringEl)) {
		// +++++ chomp leading/trailing whitespace?
		
		return message->AddString(name, data);
	}
	
	if(!strcmp(element, _pointEl)) {
		BPoint p;
		const char* ystart = strchr(data, ',');
		if(!ystart)
			return B_BAD_VALUE;
		++ystart;
		if(!*ystart)
			return B_BAD_VALUE;
		p.x = (float)atof(data);
		p.y = (float)atof(ystart);
		
		return message->AddPoint(name, p);
	}
	
	if(!strcmp(element, _rectEl)) {
		BRect r;
		const char* topstart = strchr(data, ',');
		if(!topstart)
			return B_BAD_VALUE;
		++topstart;
		if(!*topstart)
			return B_BAD_VALUE;
		
		const char* rightstart = strchr(topstart, ',');
		if(!rightstart)
			return B_BAD_VALUE;
		++rightstart;
		if(!*rightstart)
			return B_BAD_VALUE;
		
		const char* bottomstart = strchr(rightstart, ',');
		if(!bottomstart)
			return B_BAD_VALUE;
		++bottomstart;
		if(!*bottomstart)
			return B_BAD_VALUE;
		
		r.left = (float)atof(data);
		r.top = (float)atof(topstart);
		r.right = (float)atof(rightstart);
		r.bottom = (float)atof(bottomstart);
		
		return message->AddRect(name, r);
	}

	return B_BAD_INDEX;
}

status_t MessageIO::_exportField(
	ExportContext&						context,
	BMessage*									message,
	type_code									type,
	const char*								name,
	int32											index) const {
	
	status_t err;
	BString elementName;
	BString content;
	
	switch(type) {
		case B_BOOL_TYPE: {
			bool v;
			err = message->FindBool(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _boolEl;
			content = (v ? "true" : "false");
			break;
		}
		
		case B_INT8_TYPE: {
			int8 v;
			err = message->FindInt8(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _int8El;
			content << (int32)v;
			break;
		}
			
		case B_INT16_TYPE: {
			int16 v;
			err = message->FindInt16(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _int16El;
			content << (int32)v;
			break;
		}
			
		case B_INT32_TYPE: {
			int32 v;
			err = message->FindInt32(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _int32El;
			content << v;
			break;
		}
			
		case B_INT64_TYPE: {
			int64 v;
			err = message->FindInt64(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _int64El;
			content << v;
			break;
		}
			
		case B_FLOAT_TYPE: {
			float v;
			err = message->FindFloat(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _floatEl;
			content << v; // +++++ need adjustable precision!
			break;
		}
			
		case B_DOUBLE_TYPE: {
			double v;
			err = message->FindDouble(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _doubleEl;
			content << (float)v; // +++++ need adjustable precision!
			break;
		}
		
		case B_STRING_TYPE: {
			const char* v;
			err = message->FindString(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _stringEl;
			content = v;
			break;
		}

		case B_POINT_TYPE: {
			BPoint v;
			err = message->FindPoint(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _pointEl;
			content << v.x << ", " << v.y;
			break;
		}

		case B_RECT_TYPE: {
			BRect v;
			err = message->FindRect(name, index, &v);
			if(err < B_OK)
				return err;
			elementName = _rectEl;
			content << v.left << ", " << v.top << ", " <<
				v.right << ", " << v.bottom;
			break;
		}
		
		case B_MESSAGE_TYPE: {
			BMessage m;
			err = message->FindMessage(name, index, &m);
			if(err < B_OK)
				return err;
			
			// write child message
			MessageIO io(&m);
			io.m_name = name;
			return context.writeObject(&io);
		}

		default:
			return B_BAD_TYPE;
	}
	
	// spew the element
	context.beginElement(elementName.String());
	context.writeAttr("name", name);
	context.writeAttr("value", content.String());
//	context.beginContent();
//	context.writeString(content);
	context.endElement();
	
	return B_OK;
}
// END -- MessageIO.cpp --
