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

