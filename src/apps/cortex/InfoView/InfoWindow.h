/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
