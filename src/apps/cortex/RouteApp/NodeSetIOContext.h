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
