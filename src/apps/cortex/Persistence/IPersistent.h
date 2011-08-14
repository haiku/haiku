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


// IPersistant.h
// * PURPOSE
//   Interface to be implemented by objects that want to
//   be persistent (loadable/savable) via XML.
//
// * TO DO +++++ IN PROGRESS 8jul99
//   - Make the export API friendlier: classes shouldn't have to
//     know how to format XML, and it should be easy to extend
//     the serialization capabilities in a subclass. [8jul99]
//     * should this stuff be exposed via ExportContext methods?
//
//   - FBC stuffing? [29jun99]
//
// * HISTORY
//   e.moon		29jun99		Reworking; merging with the interface
//                      formerly known as Condenser.
//   e.moon		28jun99		Begun

#ifndef __IPersistent_H__
#define __IPersistent_H__

#include "ImportContext.h"
#include "ExportContext.h"

#include <iostream>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class IPersistent {

public:
	// empty virtual dtor
	virtual ~IPersistent() {}

public: 					// *** REQUIRED INTERFACE

// 8jul99 export rework
//	// EXPORT:
//	// write an XML representation of this object (including
//	// any child objects) to the given stream.  use the
//	// provided context object to format the output nicely.
//
//	virtual void xmlExport(
//		ostream& 					stream,
//		ExportContext&		context) const =0;

	// EXPORT:
	// implement this method to write a start tag via
	// context.startElement().  You can also write comments or
	// processing instructions directly to the stream.
	
	virtual void xmlExportBegin(
		ExportContext& context) const=0;
		
	// EXPORT:
	// implement this method to write all the attributes needed
	// to represent your object, via context.writeAttribute().
	// (If subclassing an IPersistent implementation, don't forget
	// to call up to its version of this method.)
	
	virtual void xmlExportAttributes(
		ExportContext& context) const=0;
		
	// EXPORT: optional
	// implement this method to write any child objects, using
	// context.writeObject().  You can also write text content
	// directly to the stream.
	// (If subclassing an IPersistent implementation, don't forget
	// to call up to its version of this method.)
	
	virtual void xmlExportContent(
		ExportContext& context) const { TOUCH(context); }
	
	// EXPORT:
	// implement this method to write an end tag via
	// context.endElement().
	
	virtual void xmlExportEnd(
		ExportContext& context) const=0;
	
	// IMPORT:
	// called when the start tag of the element corresponding to
	// this object is encountered, immediately after this object
	// is constructed.  no action is required.
	//
	// context.element() will return the element name that produced
	// this object.
	
	virtual void xmlImportBegin(
		ImportContext&		context) =0;
	
	// IMPORT:
	// called for each attribute/value pair found in
	// the element corresponding to this object.
	
	virtual void xmlImportAttribute(
		const char*					key,
		const char*					value,
		ImportContext&		context) =0;
		
	// IMPORT:
	// called when character data is encountered for the
	// element corresponding to this object.  data is not
	// 0-terminated; this method may be called several times
	// (if, for example, the character data is broken up
	//  by child elements.)
	
	virtual void xmlImportContent(
		const char*					data,
		uint32						length,
		ImportContext&		context) =0;
		
	// IMPORT:
	// called when an object has been successfully
	// constructed 'beneath' this one.
	//
	// context.element() will return the element name corresponding
	// to the child object.
	
	virtual void xmlImportChild(
		IPersistent*			child,
		ImportContext&		context) =0;
		
	// IMPORT:
	// called when close tag is encountered for the element
	// corresponding to this object.  a good place to do
	// validation.
	//
	// context.element() will return the element name that produced
	// this object.
	
	virtual void xmlImportComplete(
		ImportContext&		context) =0;

public: 					// *** OPTIONAL CHILD-IMPORT INTERFACE
								//     These methods allow an IPersistent object
								//     to directly import nested elements: handy for
								//     condensing more complicated document structures
								//     into a single object (see MediaFormatIO for an
								//     example.)
	
	virtual void xmlImportChildBegin(
		const char*					name,
		ImportContext&		context) {
		TOUCH(name);
		context.reportWarning("Nested element not supported.");
	}

	virtual void xmlImportChildAttribute(
		const char*					key,
		const char*					value,
		ImportContext&		context) {TOUCH(key); TOUCH(value); TOUCH(context);}
	
	virtual void xmlImportChildContent(
		const char*					data,
		uint32						length,
		ImportContext&		context) {TOUCH(data); TOUCH(length); TOUCH(context);}

	virtual void xmlImportChildComplete(
		const char*					name,
		ImportContext&		context) {TOUCH(name); TOUCH(context);}
};

__END_CORTEX_NAMESPACE

#endif /*__IPersistent_H__*/
