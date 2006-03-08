// FileNodeInfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   In addition to the fields defined by its base class
//   LiveNodeInfoView, this class defines media-file specific
//   fields, line number of tracks, codecs, formats, etc.
//
// * HISTORY
//   c.lenz		13nov99		Begun
//

#ifndef __FileNodeInfoView_H__
#define __FileNodeInfoView_H__

#include "LiveNodeInfoView.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class NodeRef;

class FileNodeInfoView : public LiveNodeInfoView
{

public:					// *** ctor/dtor

	// add file related fields to the view by creating
	// and querying an instance of BMediaFile/BMediaTrack
						FileNodeInfoView(
							const NodeRef *ref);

	virtual				~FileNodeInfoView();
};

__END_CORTEX_NAMESPACE
#endif /* __FileNodeInfoView_H__ */
