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
