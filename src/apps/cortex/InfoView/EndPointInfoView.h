// EndPointInfoView.h (Cortex/InfoView)
//
// * PURPOSE
//   Display input/output related info in the InfoWindow.
//   Very similar to ConnectionInfoView, only that this
//   views is prepared to handle wildcard values in the
//   media_format (a connection should never have to!)
//
// * HISTORY
//   c.lenz		14nov99		Begun
//

#ifndef __EndPointInfoView_H__
#define __EndPointInfoView_H__

#include "InfoView.h"

// Media Kit
#include <MediaDefs.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class EndPointInfoView : 
	public InfoView {

public:					// *** ctor/dtor

	// creates an InfoView for a media_input. adds source
	// and destination fields, then calls _addFormatFields
						EndPointInfoView(
							const media_input &input);

	// creates an InfoView for a media_output. adds source
	// and destination fields, then calls _addFormatFields
						EndPointInfoView(
							const media_output &output);

	virtual				~EndPointInfoView();
	
public:					// *** BView impl

	// notify InfoWindowManager
	virtual void		DetachedFromWindow();

private:				// *** internal operations

	// adds media_format related fields to the view
	// (wildcard-aware)
	void				_addFormatFields(
							const media_format &format);

private:				// *** data members

	// true if media_output
	bool				m_output;

	int32				m_port;

	int32				m_id;
};

__END_CORTEX_NAMESPACE
#endif /* __EndPointInfoView_H__ */
