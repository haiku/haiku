// LiveNodeInfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   Defines fields to be displayed for all live MediaNodes,
//   which can be added to for special nodes like file-readers
//   (see FileNodeInfoView) etc.
//
// * HISTORY
//   c.lenz		5nov99		Begun
//

#ifndef __LiveNodeInfoView_H__
#define __LiveNodeInfoView_H__

#include "InfoView.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeRef;

class LiveNodeInfoView :
	public InfoView {

public:					// *** ctor/dtor

	// adds the live-node relevant fields the the
	// InfoView
						LiveNodeInfoView(
							const NodeRef *ref);

	virtual				~LiveNodeInfoView();

public:					// *** BView impl

	// notify InfoWindowManager
	virtual void		DetachedFromWindow();

private:				// *** data members

	int32				m_nodeID;
};

__END_CORTEX_NAMESPACE
#endif /* __LiveNodeInfoView_H__ */
