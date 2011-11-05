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
