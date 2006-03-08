// InfoWindow.h (Cortex/InfoView)
//
// * PURPOSE
//
// * HISTORY
//   c.lenz		25may00		Begun
//

#ifndef __InfoWindow_H__
#define __InfoWindow_H__

// Interface Kit
#include <Window.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class InfoWindow : 
	public		BWindow {

public:						// *** ctor/dtor

							InfoWindow(
								BRect frame);

	virtual					~InfoWindow();

public:						// *** BWindow impl

	// remember that frame was changed manually
	virtual void			FrameResized(
								float width,
								float height);

	// extends BWindow implementation to constrain to screen
	// and remember the initial size
	virtual void			Show();

	// extend basic Zoom() functionality to 'minimize' the
	// window when currently at max size
	virtual void			Zoom(
								BPoint origin,
								float width,
								float height);

private:					// *** internal operations

	// resizes the window to fit in the current screen rect
	void					_constrainToScreen();

private:					// *** data members

	BRect					m_manualSize;

	bool					m_zoomed;

	bool					m_zooming;
};

__END_CORTEX_NAMESPACE
#endif /* __InfoWindow_H__ */
