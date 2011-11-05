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


// TransportWindow.cpp

#include "TransportWindow.h"
#include "RouteWindow.h"
#include "TransportView.h"

#include "NodeGroup.h"

#include <Debug.h>
#include <Message.h>
#include <Messenger.h>
#include <View.h>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** ctors/dtor
// -------------------------------------------------------- //

TransportWindow::~TransportWindow() {}
	
TransportWindow::TransportWindow(
	NodeManager*						manager,
	BWindow*								parent,
	const char*							name) :
	BWindow(
		BRect(200,200,200,200),
		name,
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_WILL_ACCEPT_FIRST_CLICK|B_AVOID_FOCUS|B_NOT_ZOOMABLE|B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS),
	m_view(0),
	m_parent(parent) {
	
	AddToSubset(m_parent);
	
	ASSERT(parent);
	ASSERT(manager);
	
	m_view = new TransportView(manager, "transportView");
	AddChild(m_view);
}

BMessenger TransportWindow::parentWindow() const {
	return BMessenger(m_parent);
}

// -------------------------------------------------------- //
// *** BWindow
// -------------------------------------------------------- //

bool TransportWindow::QuitRequested() {

	// redirect request to parent window
	m_parent->PostMessage(RouteWindow::M_TOGGLE_TRANSPORT_WINDOW);
	return false;
		
//	BMessage m(RouteWindow::M_INSPECTOR_CLOSED);
//	m.AddPointer("inspector", (void*)this);
//	m.AddRect("frame", Frame());
//	BMessenger(m_routeWindow).SendMessage(&m);
//	return true;
}

// -------------------------------------------------------- //
// *** BHandler
// -------------------------------------------------------- //

void TransportWindow::MessageReceived(
	BMessage*								message) {

	switch(message->what) {
		case M_SELECT_GROUP:
			m_view->_handleSelectGroup(message);
			break;
			
		case RouteWindow::M_REFRESH_TRANSPORT_SETTINGS:
			m_view->_refreshTransportSettings();
			break;
			
		default:
			_inherited::MessageReceived(message);
	}
}

// END -- TransportWindow.cpp --
