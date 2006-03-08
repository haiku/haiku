// DormantNodeInfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   An InfoView variant for dormant MediaNodes. Accepts
//   a dormant_node_info struct in the constructor and 
//   tries to aquire the corresponding dormant_flavor_info
//   from the BMediaRoster.
//
// * HISTORY
//   c.lenz		5nov99		Begun
//

#ifndef __DormantNodeInfoView_H__
#define __DormantNodeInfoView_H__

#include "InfoView.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeRef;

class DormantNodeInfoView :
	public InfoView {

public:					// *** ctor/dtor

	// add fields relevant for dormant MediaNodes (like 
	// AddOn-ID, input/output formats etc.)
						DormantNodeInfoView(
							const dormant_node_info &info);

	virtual				~DormantNodeInfoView();

public:					// *** BView impl

	// notify InfoWindowManager
	virtual void		DetachedFromWindow();

private:				// *** data members

	int32				m_addOnID;

	int32				m_flavorID;
};

__END_CORTEX_NAMESPACE
#endif /* __DormantNodeInfoView_H__ */
