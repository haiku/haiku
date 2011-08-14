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


// NodeSetIOContext.h
// * PURPOSE
//   Store state info for import & export of a set
//   of media node descriptions.  Provide hooks for
//   import/export of associated UI state info.
//   To be used as a mix-in w/ derived ImportContext
//   and ExportContext classes.
//
// * HISTORY
//   e.moon			7dec99		Begun

#ifndef __NodeSetIOContext_H__
#define __NodeSetIOContext_H__

#include "NodeKey.h"

#include <MediaDefs.h>
#include <vector>
#include <utility>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE


class NodeSetIOContext {
public:														// *** dtor/ctor
	virtual ~NodeSetIOContext();
	NodeSetIOContext();

public:														// *** hooks
	virtual void importUIState(
		const BMessage*							archive) {}

	virtual void exportUIState(
		BMessage*										archive) {}

public:														// *** operations
	// The node must be valid (!= media_node::null.node).
	// If no key is given, a new one will be generated.
	status_t addNode(
		media_node_id									node,
		const char*										key=0);
	
	status_t removeNode(
		media_node_id									node);

	status_t removeNodeAt(
		uint32												index);

	uint32 countNodes() const;

	// returns 0 if out of range
	media_node_id nodeAt(
		uint32												index) const;

	const char* keyAt(
		uint32												index) const;

	status_t getKeyFor(
		media_node_id									node,
		const char**									outKey) const;
		
	status_t getNodeFor(
		const char*										key,
		media_node_id*								outNode) const;

private:													// implementation

	// the node/key set
	typedef std::pair<BString,media_node_id> node_entry;
	typedef std::vector<node_entry>		node_set;
	node_set												m_nodes;
	
	// next node key value
	int16														m_nodeKeyIndex;
};

__END_CORTEX_NAMESPACE
#endif /*__NodeSetIOContext_H__*/
