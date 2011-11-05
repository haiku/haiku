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


// NodeExportContext.h
// * PURPOSE
//   Extends ExportContext to include a set of nodes
//   to be stored.  This is an abstract class; implement
//   exportUIState() to store any user-interface state
//   info needed for the listed nodes.
//
// TO DO ++++++ GO AWAY!  NodeSetIOContext provides the same
//   functionality for both import and export...
//
// * HISTORY
//   e.moon			7dec99		Begun

#ifndef __NodeExportContext_H__
#define __NodeExportContext_H__

#include <vector>
#include "ExportContext.h"
#include "IStateArchivable.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeExportContext :
	public	ExportContext {
	
public:													// *** dtor/ctor
	virtual ~NodeExportContext() {}

	NodeExportContext(
		BDataIO*										_stream,
		BString&										error) :
		ExportContext(_stream, error) {}

public:													// *** members
	// set of nodes to export; if one or more nodes are
	// not fit to be exported, they may be removed or
	// set to media_node::null.node during the export process.
	typedef vector<media_node_id>	media_node_set;
	media_node_set								nodes;

public:													// *** interface
	virtual void exportUIState(
		BMessage*										archive) =0;
};

__END_CORTEX_NAMESPACE
#endif /*__NodeExportContext_H__*/
