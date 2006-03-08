// ConnectionInfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   Display connection-specific info in the InfoWindow.
//   This is: source, destination and format.
//
// * HISTORY
//   c.lenz		5nov99		Begun
//

#ifndef __ConnectionInfoView_H__
#define __ConnectionInfoView_H__

#include "InfoView.h"

#include <MediaDefs.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// NodeManager
class Connection;

class ConnectionInfoView : public InfoView
{

public:					// *** ctor/dtor

						ConnectionInfoView(
							const Connection &connection);

	virtual				~ConnectionInfoView();

private:				// *** internal operations

	// adds media_format related fields to the view
	// (wildcard-aware)
	void				_addFormatFields(
							const media_format &format);

public:					// *** BView impl

	// notify InfoWindowManager
	virtual void		DetachedFromWindow();

private:				// *** data members

	media_source		m_source;

	media_destination	m_destination;
};

__END_CORTEX_NAMESPACE
#endif /* __ConnectionInfoView_H__ */
