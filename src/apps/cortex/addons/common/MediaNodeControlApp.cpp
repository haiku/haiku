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


// MediaNodeControlApp.cpp
// e.moon 8jun99

#include "MediaNodeControlApp.h"
#include <Window.h>
#include <View.h>
#include <MediaRoster.h>
#include <MediaTheme.h>
#include <ParameterWeb.h>
#include <String.h>
#include <Alert.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

class PanelWindow :
	public		BWindow {
	typedef	BWindow _inherited;
public:
	PanelWindow() :
		BWindow(BRect(50, 50, 100, 100), "MediaNodeControlApp",
			B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS |
			B_WILL_ACCEPT_FIRST_CLICK) {}
	
	bool QuitRequested() {
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
};

MediaNodeControlApp::~MediaNodeControlApp() {
	BMediaRoster* r = BMediaRoster::Roster();
	r->ReleaseNode(m_node);
}

MediaNodeControlApp::MediaNodeControlApp(
	const char* pAppSignature,
	media_node_id nodeID) :
	BApplication(pAppSignature) {

	BMediaRoster* r = BMediaRoster::Roster();
	
	// get the node	
	status_t err = r->GetNodeFor(nodeID, &m_node);
	if(err < B_OK) {
		char buffer[512];
		sprintf(buffer,
			"MediaNodeControlApp: couldn't find node (%ld):\n%s\n",
			nodeID, strerror(err));
		(new BAlert("error", buffer, "OK"))->Go();
		return;
	}
	
	// fetch info (name)
	live_node_info nInfo;
	err = r->GetLiveNodeInfo(m_node, &nInfo);
	if(err < B_OK) {
		char buffer[512];
		sprintf(buffer,
			"MediaNodeControlApp: couldn't get node info (%ld):\n%s\n",
			nodeID, strerror(err));
		(new BAlert("error", buffer, "OK"))->Go();
		return;
	}
	
	BString windowTitle;
	windowTitle << nInfo.name << '(' << nodeID << ") controls";		
	
	// get parameter web
	BParameterWeb* pWeb;
	err = r->GetParameterWebFor(m_node, &pWeb);
	if(err < B_OK) {
		char buffer[512];
		sprintf(buffer,
			"MediaNodeControlApp: no parameters for node (%ld):\n%s\n",
			nodeID, strerror(err));
		(new BAlert("error", buffer, "OK"))->Go();
		return;
	}
	
	// build & show control window
	BView* pView = BMediaTheme::ViewFor(pWeb);
	BWindow* pWnd = new PanelWindow();
	pWnd->AddChild(pView);
	pWnd->ResizeTo(pView->Bounds().Width(), pView->Bounds().Height());
	pWnd->SetTitle(windowTitle.String());
	pWnd->Show();
	
	// release the node
	//r->ReleaseNode(m_node);	
}

// END -- MediaNodeControlApp.cpp --
