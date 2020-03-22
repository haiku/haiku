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


// ConnectionIO.h
// * PURPOSE
//   Manage the import and export of a user-instantiated
//   media node description.
//
// * HISTORY
//   e.moon		8dec99		Begun

#ifndef __ConnectionIO_H__
#define __ConnectionIO_H__

#include "NodeRef.h"
#include "Connection.h"
#include "XML.h"

#include <String.h>
#include <Entry.h>

#include <MediaDefs.h>

struct dormant_node_info;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeManager;
class NodeSetIOContext;
class LiveNodeIO;

class ConnectionIO :
	public		IPersistent {

public:											// *** ctor/dtor
	virtual ~ConnectionIO();

	ConnectionIO();

	ConnectionIO(
		const Connection*				con,
		const NodeManager*			manager,
		const NodeSetIOContext*	context);

	bool exportValid() const { return m_exportValid; }

public:											// *** operations

	// call when object imported to create the described
	// connection
	status_t instantiate(
		NodeManager*						manager,
		const NodeSetIOContext*	context,
		Connection*							outCon);

public:											// *** document-type setup
	static void AddTo(
		XML::DocumentType*			docType);

public:											// *** IPersistent

	// EXPORT:

	void xmlExportBegin(
		ExportContext&						context) const;

	void xmlExportAttributes(
		ExportContext&						context) const;

	void xmlExportContent(
		ExportContext&						context) const;

	void xmlExportEnd(
		ExportContext&						context) const;

	// IMPORT:

	virtual void xmlImportBegin(
		ImportContext&						context);

	virtual void xmlImportAttribute(
		const char*								key,
		const char*								value,
		ImportContext&						context);

	virtual void xmlImportContent(
		const char*								data,
		uint32										length,
		ImportContext&						context);

	virtual void xmlImportChild(
		IPersistent*							child,
		ImportContext&						context);

	virtual void xmlImportComplete(
		ImportContext&						context);


	virtual void xmlImportChildBegin(
		const char*								name,
		ImportContext&						context);

	virtual void xmlImportChildComplete(
		const char*								name,
		ImportContext&						context);

private:											// *** implementation

	LiveNodeIO*								m_inputNodeIO;

	BString										m_outputName; // original name if available
	media_format							m_outputFormat;

	LiveNodeIO*								m_outputNodeIO;

	BString										m_inputName; // original name if available
	media_format							m_inputFormat;

	media_format							m_requestedFormat;

	uint32										m_flags;

	bool											m_exportValid;

	// import state
	enum import_state_t {
		IMPORT_NONE,
		IMPORT_OUTPUT,
		IMPORT_INPUT
	};
	import_state_t						m_importState;
};

__END_CORTEX_NAMESPACE
#endif /*__ConnectionIO_H__*/

