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
		(new BAlert("error", buffer, "Ok"))->Go();
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
		(new BAlert("error", buffer, "Ok"))->Go();
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
		(new BAlert("error", buffer, "Ok"))->Go();
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
