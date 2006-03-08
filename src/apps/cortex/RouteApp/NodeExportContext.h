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
