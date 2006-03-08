// ParameterContainerView.h (Cortex/ParameterWindow)
//
// * PURPOSE
//
// * TODO
//
// * HISTORY
//   c.lenz		16feb2000		Begun
//

#ifndef __ParameterContainerView_H__
#define __ParameterContainerView_H__

// Interface Kit
#include <View.h>
// Support Kit
#include <String.h>

#include "cortex_defs.h"

class BScrollBar;

__BEGIN_CORTEX_NAMESPACE

class ParameterContainerView :
	public		BView {
	
public:					// *** ctor/dtor

						ParameterContainerView(
							BRect dataRect,
							BView *target);

	virtual				~ParameterContainerView();

public:					// *** BScrollView impl.

	virtual void		FrameResized(
							float width,
							float height);

private:				// *** internal operations

	void				_updateScrollBars();

private:				// *** data members

	BView*      m_target;
	BRect				m_dataRect;
	BRect				m_boundsRect;
	BScrollBar* m_hScroll;
	BScrollBar* m_vScroll;
};

__END_CORTEX_NAMESPACE
#endif /* __ParameterContainerView_H__ */
