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


// TipWindow.cpp

#include "TipWindow.h"
#include "TipView.h"

#include <Debug.h>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** dtor/ctors
// -------------------------------------------------------- //

TipWindow::~TipWindow() {}
TipWindow::TipWindow(
	const char*							text) :
	BWindow(
		BRect(0,0,0,0),
		"TipWindow",
		B_NO_BORDER_WINDOW_LOOK,
		B_FLOATING_ALL_WINDOW_FEEL,
		B_NOT_MOVABLE|B_AVOID_FOCUS/*,
		B_ALL_WORKSPACES*/),
	// the TipView is created on demand
	m_tipView(0) {

	if(text)
		m_text = text;
}

		
// -------------------------------------------------------- //
// *** operations (LOCK REQUIRED)
// -------------------------------------------------------- //

const char* TipWindow::text() const {
	return m_text.Length() ?
		m_text.String() :
		0;
}

void TipWindow::setText(
	const char*							text) {

	if(!m_tipView)
		_createTipView();

	m_text = text;	
	m_tipView->setText(text);
	
	// size to fit view
	float width, height;
	m_tipView->GetPreferredSize(&width, &height);
	m_tipView->ResizeTo(width, height);
	ResizeTo(width, height);
	
	m_tipView->Invalidate();
}

// -------------------------------------------------------- //
// *** hooks
// -------------------------------------------------------- //

// override to substitute your own view class
TipView* TipWindow::createTipView() {
	return new TipView;
}

// -------------------------------------------------------- //
// *** BWindow
// -------------------------------------------------------- //

// initializes the tip view
void TipWindow::Show() {

	// initialize the tip view if necessary
	if(!m_tipView)
		_createTipView();
	
	_inherited::Show();
}

// remove tip view? +++++
void TipWindow::Hide() {
	_inherited::Hide();
}


// hides the window when the user switches workspaces
// +++++ should it be restored when the user switches back?
void TipWindow::WorkspaceActivated(
	int32										workspace,
	bool										active) {

	// don't confuse the user
	if(!IsHidden())
		Hide();	

	_inherited::WorkspaceActivated(workspace, active);
}

// -------------------------------------------------------- //
// implementation
// -------------------------------------------------------- //

void TipWindow::_createTipView() {
	if(m_tipView)
		_destroyTipView();
	m_tipView = createTipView();
	ASSERT(m_tipView);
	
	AddChild(m_tipView);
	
	if(m_text.Length())
		m_tipView->setText(m_text.String());
	else
		m_tipView->setText("(no info)");
}

void TipWindow::_destroyTipView() {
	if(!m_tipView)
		return;
	RemoveChild(m_tipView);
	delete m_tipView;
	m_tipView = 0;
}

// END -- TipWindow.cpp --
