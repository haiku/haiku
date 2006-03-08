// AppNodeInfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   In addition to the fields defined by its base class
//   LiveNodeInfoView, this class defines fields about
//   the application in which the node 'lives'
//
// * HISTORY
//   c.lenz		3dec99		Begun
//

#ifndef __AppNodeInfoView_H__
#define __AppNodeInfoView_H__

#include "LiveNodeInfoView.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeRef;

class AppNodeInfoView : public LiveNodeInfoView
{

public:					// *** ctor/dtor

	// add app related fields to the view by looking at
	// the nodes port and querying the application roster
						AppNodeInfoView(
							const NodeRef *ref);

	virtual				~AppNodeInfoView();
};

__END_CORTEX_NAMESPACE
#endif /* __AppNodeInfoView_H__ */
