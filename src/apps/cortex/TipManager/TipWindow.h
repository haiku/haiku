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


// TipWindow.h
// * PURPOSE
//   A floating window used to display floating tips
//   (aka 'ToolTips'.)  May be subclassed to provide
//   a custom tip view or to extend the window's
//   behavior.
//
// * HISTORY
//   e.moon		17oct99		Begun (based on TipFloater.)

#ifndef __TipWindow_H__
#define __TipWindow_H__

#include <String.h>
#include <Window.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class TipView;

class TipWindow :
	public	BWindow {
	typedef	BWindow _inherited;
	
public:											// *** dtor/ctors
	virtual ~TipWindow();
	TipWindow(
		const char*							text=0);

public:											// *** operations (LOCK REQUIRED)

	const char* text() const;
	virtual void setText(
		const char*							text);

protected:									// *** hooks
	// override to substitute your own view class
	virtual TipView* createTipView();

public:											// *** BWindow

	// initializes the tip view
	virtual void Show();

	// remove tip view? +++++
	virtual void Hide();

	// hides the window when the user switches workspaces
	// +++++ should it be restored when the user switches back?
	virtual void WorkspaceActivated(
		int32										workspace,
		bool										active);

private:										// implementation
	TipView*									m_tipView;
	BString										m_text;
	
	void _createTipView();
	void _destroyTipView();
};

__END_CORTEX_NAMESPACE
#endif /*__TipWindow_H__*/
