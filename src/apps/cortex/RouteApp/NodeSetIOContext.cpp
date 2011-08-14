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


// NodeSetIOContext.cpp

#include "NodeSetIOContext.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>

#include <Debug.h>
#include <MediaNode.h>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** dtor/ctor
// -------------------------------------------------------- //

NodeSetIOContext::~NodeSetIOContext() {}

NodeSetIOContext::NodeSetIOContext() :
	m_nodeKeyIndex(1) {}

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

status_t NodeSetIOContext::addNode(
	media_node_id									node,
	const char*										key) {

	if(node == media_node::null.node)
		return B_BAD_VALUE;

	for(node_set::const_iterator it = m_nodes.begin();
		it != m_nodes.end(); ++it) {

		if((*it).second == node) {
			// already in set; does key match?		
			if(key &&
				(*it).first.Length() &&
				strcmp(key, (*it).first.String()) != 0) {
				PRINT((
					"!!! NodeSetIOContext::addNode(%ld, '%s'):\n"
					"    found matching node with key '%s'!\n",
					node, key, (*it).first.String()));
			}
			return B_NOT_ALLOWED;
		}
	}

	if(key)
		m_nodes.push_back(node_entry(key, node));
	else {
		char buffer[16];
		sprintf(buffer, "N_%03d", m_nodeKeyIndex++);
		m_nodes.push_back(node_entry(buffer, node));
	}

	return B_OK;
}

status_t NodeSetIOContext::removeNode(
	media_node_id									node) {

	for(node_set::iterator it = m_nodes.begin();
		it != m_nodes.end(); ++it) {
		if((*it).second == node) {
			m_nodes.erase(it);
			return B_OK;
		}
	}
	return B_BAD_VALUE;
}

status_t NodeSetIOContext::removeNodeAt(
	uint32												index) {

	if(index < 0 || index >= m_nodes.size())
		return B_BAD_INDEX;
	
	m_nodes.erase(m_nodes.begin() + index);	
	return B_OK;
}

uint32 NodeSetIOContext::countNodes() const {
	return m_nodes.size();
}

media_node_id NodeSetIOContext::nodeAt(
	uint32												index) const {

	if(index < 0 || index >= m_nodes.size())
		return media_node::null.node;	

	return m_nodes[index].second;
}

const char* NodeSetIOContext::keyAt(
	uint32												index) const {

	if(index < 0 || index >= m_nodes.size())
		return 0;	

	return m_nodes[index].first.String();
}

status_t NodeSetIOContext::getKeyFor(
	media_node_id									node,
	const char**									outKey) const {

	if(node == media_node::null.node)
		return B_BAD_VALUE;
	
	for(node_set::const_iterator it = m_nodes.begin();
		it != m_nodes.end(); ++it) {

		if((*it).second == node) {
			*outKey = (*it).first.String();
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}

status_t NodeSetIOContext::getNodeFor(
	const char*										key,
	media_node_id*								outNode) const {

	if(!key || !*key)
		return B_BAD_VALUE;
			
	for(node_set::const_iterator it = m_nodes.begin();
		it != m_nodes.end(); ++it) {
		if(!strcmp((*it).first.String(), key)) {
			*outNode = (*it).second;
			return B_OK;
		}
	}
	
	return B_NAME_NOT_FOUND;
}

// -------------------------------------------------------- //

// END -- NodeSetIOContext.cpp --

