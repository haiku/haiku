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


// InfoWindowManager.cpp

#include "InfoWindowManager.h"
// InfoWindow
#include "AppNodeInfoView.h"
#include "ConnectionInfoView.h"
#include "DormantNodeInfoView.h"
#include "EndPointInfoView.h"
#include "FileNodeInfoView.h"
#include "LiveNodeInfoView.h"
#include "InfoWindow.h"
// NodeManager
#include "AddOnHostProtocol.h"
#include "Connection.h"
#include "NodeRef.h"

// Application Kit
#include <Application.h>
#include <AppDefs.h>
#include <Roster.h>
// Media Kit
#include <MediaAddOn.h>
#include <MediaRoster.h>
// Support Kit
#include <List.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ACCESS(x) //PRINT (x)
#define D_ALLOC(x) //PRINT (x)
#define D_INTERNAL(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)
#define D_WINDOW(x) //PRINT (x)

// -------------------------------------------------------- //
// internal types
// -------------------------------------------------------- //

// used to remember window for live nodes
struct live_node_window {

public:						// *** ctor/dtor

							live_node_window(
								const NodeRef *ref,
								BWindow *window)
								: ref(ref),
								  window(window)
							{ }

public:						// *** data members

	const NodeRef		   *ref;

	BWindow				   *window;
};

// used to remember windows for dormant nodes
struct dormant_node_window {

public:						// *** ctor/dtor

							dormant_node_window(
								const dormant_node_info &info,
								BWindow *window)
								: info(info),
								  window(window)
							{ }

public:						// *** data members

	const dormant_node_info	info;

	BWindow				   *window;
};

// used to remember windows for connections
struct connection_window {

public:						// *** ctor/dtor

							connection_window(
								const media_source &source,
								const media_destination &destination,
								BWindow *window)
								: source(source),
								  destination(destination),
								  window(window)
							{ }

public:						// *** data members

	media_source			source;

	media_destination		destination;

	BWindow				   *window;
};

// used to remember windows for media_inputs
struct input_window {

public:						// *** ctor/dtor

							input_window(
								const media_destination &destination,
								BWindow *window)
								: destination(destination),
								  window(window)
							{ }

public:						// *** data members

	media_destination		destination;

	BWindow				   *window;
};

// used to remember windows for media_outputs
struct output_window {

public:						// *** ctor/dtor

							output_window(
								const media_source &source,
								BWindow *window)
								: source(source),
								  window(window)
							{ }

public:						// *** data members

	media_source			source;

	BWindow				   *window;
};

// -------------------------------------------------------- //
// static member init
// -------------------------------------------------------- //

const BPoint InfoWindowManager::M_DEFAULT_OFFSET	= BPoint(20.0, 20.0);
const BPoint InfoWindowManager::M_INIT_POSITION	= BPoint(90.0, 90.0);

InfoWindowManager *InfoWindowManager::s_instance = 0;

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

/* hidden */
InfoWindowManager::InfoWindowManager()
	: BLooper("InfoWindowManager",
			  B_NORMAL_PRIORITY),
	  m_liveNodeWindows(0),
	  m_dormantNodeWindows(0),
	  m_connectionWindows(0),
	  m_inputWindows(0),
	  m_outputWindows(0),
	  m_nextWindowPosition(M_INIT_POSITION) {
	D_ALLOC(("InfoWindowManager::InfoWindowManager()\n"));

	Run();

	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (roster) {
		roster->StartWatching(BMessenger(0, this),
							  B_MEDIA_CONNECTION_BROKEN);
	}
}

InfoWindowManager::~InfoWindowManager() {
	D_ALLOC(("InfoWindowManager::~InfoWindowManager()\n"));

	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (roster) {
		roster->StopWatching(BMessenger(0, this),
							  B_MEDIA_CONNECTION_BROKEN);
	}

	if (m_liveNodeWindows) {
		while (m_liveNodeWindows->CountItems() > 0) {
			live_node_window *entry = static_cast<live_node_window *>
									  (m_liveNodeWindows->ItemAt(0));
			if (entry && entry->window) {
				remove_observer(this, entry->ref);
				BMessenger messenger(0, entry->window);
				messenger.SendMessage(B_QUIT_REQUESTED);
			}
			m_liveNodeWindows->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}	
		delete m_liveNodeWindows;
		m_liveNodeWindows = 0;
	}

	if (m_dormantNodeWindows) {
		while (m_dormantNodeWindows->CountItems() > 0) {
			dormant_node_window *entry = static_cast<dormant_node_window *>
										 (m_dormantNodeWindows->ItemAt(0));
			if (entry && entry->window) {
				BMessenger messenger(0, entry->window);
				messenger.SendMessage(B_QUIT_REQUESTED);
			}
			m_dormantNodeWindows->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}	
		delete m_dormantNodeWindows;
		m_dormantNodeWindows = 0;
	}

	if (m_connectionWindows) {
		while (m_connectionWindows->CountItems() > 0) {
			connection_window *entry = static_cast<connection_window *>
									   (m_connectionWindows->ItemAt(0));
			if (entry && entry->window) {
				BMessenger messenger(0, entry->window);
				messenger.SendMessage(B_QUIT_REQUESTED);
			}
			m_connectionWindows->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}	
		delete m_connectionWindows;
		m_connectionWindows = 0;
	}

	if (m_inputWindows) {
		while (m_inputWindows->CountItems() > 0) {
			input_window *entry = static_cast<input_window *>
								  (m_inputWindows->ItemAt(0));
			if (entry && entry->window) {
				BMessenger messenger(0, entry->window);
				messenger.SendMessage(B_QUIT_REQUESTED);
			}
			m_inputWindows->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}	
		delete m_inputWindows;
		m_inputWindows = 0;
	}

	if (m_outputWindows) {
		while (m_outputWindows->CountItems() > 0) {
			output_window *entry = static_cast<output_window *>
								   (m_outputWindows->ItemAt(0));
			if (entry && entry->window) {
				BMessenger messenger(0, entry->window);
				messenger.SendMessage(B_QUIT_REQUESTED);
			}
			m_outputWindows->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}	
		delete m_outputWindows;
		m_outputWindows = 0;
	}
}

// -------------------------------------------------------- //
// *** singleton access
// -------------------------------------------------------- //

/*static*/
InfoWindowManager *InfoWindowManager::Instance() {
	D_ACCESS(("InfoWindowManager::Instance()\n"));

	if (!s_instance) {
		D_ACCESS((" -> create instance\n"));
		s_instance = new InfoWindowManager();
	}		

	return s_instance;
}

/* static */
void InfoWindowManager::shutDown() {
	D_WINDOW(("InfoWindowManager::shutDown()\n"));

	if (s_instance) {
		s_instance->Lock();
		s_instance->Quit();
		s_instance = 0;
	}
}

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

status_t InfoWindowManager::openWindowFor(
	const NodeRef *ref) {
	D_WINDOW(("InfoWindowManager::openWindowFor(live_node)\n"));

	// make absolutely sure we're locked
	if (!IsLocked()) {
		debugger("The looper must be locked !");
	}

	// make sure the ref is valid
	if (!ref) {
		return B_ERROR;
	}

	BWindow *window = 0;
	if (_findWindowFor(ref->id(), &window)) {
		// window for this node already exists, activate it
		window->SetWorkspaces(B_CURRENT_WORKSPACE);
		window->Activate();
		return B_OK;
	}

	BRect frame = InfoView::M_DEFAULT_FRAME;
	frame.OffsetTo(m_nextWindowPosition);
	m_nextWindowPosition += M_DEFAULT_OFFSET;
	window = new InfoWindow(frame);

	if (_addWindowFor(ref, window)) {
		// find the correct InfoView sub-class
		BMediaRoster *roster = BMediaRoster::CurrentRoster();
		dormant_node_info dormantNodeInfo;
		if (ref->kind() & B_FILE_INTERFACE)
		{
			window->AddChild(new FileNodeInfoView(ref));
		}
		else if (roster->GetDormantNodeFor(ref->node(), &dormantNodeInfo) != B_OK) {
			port_info portInfo;
			app_info appInfo;
			if ((get_port_info(ref->node().port, &portInfo) == B_OK)
			 && (be_roster->GetRunningAppInfo(portInfo.team, &appInfo) == B_OK)) {
				app_info thisAppInfo;
				if ((be_app->GetAppInfo(&thisAppInfo) != B_OK)
				 || ((strcmp(appInfo.signature, thisAppInfo.signature) != 0)
				 && (strcmp(appInfo.signature, addon_host::g_appSignature) != 0))) {
					window->AddChild(new AppNodeInfoView(ref));
				}
				else {
					window->AddChild(new LiveNodeInfoView(ref));
				}
			}
			else {
				window->AddChild(new LiveNodeInfoView(ref));
			}
		}
		else {
			window->AddChild(new LiveNodeInfoView(ref));
		}
		// display the window
		window->Show();
		return B_OK;
	}

	// failed
	delete window;
	return B_ERROR;
}

status_t InfoWindowManager::openWindowFor(
	const dormant_node_info &info) {
	D_WINDOW(("InfoWindowManager::openWindowFor(dormant_node)\n"));

	// make absolutely sure we're locked
	if (!IsLocked()) {
		debugger("The looper must be locked !");
	}

	BWindow *window = 0;
	if (_findWindowFor(info, &window)) {
		// window for this node already exists, activate it
		window->SetWorkspaces(B_CURRENT_WORKSPACE);
		window->Activate();
		return B_OK;
	}

	BRect frame = InfoView::M_DEFAULT_FRAME;
	frame.OffsetTo(m_nextWindowPosition);
	m_nextWindowPosition += M_DEFAULT_OFFSET;
	window = new InfoWindow(frame);

	if (_addWindowFor(info, window)) {
		window->AddChild(new DormantNodeInfoView(info));
		window->Show();
		return B_OK;
	}

	// failed
	delete window;
	return B_ERROR;
}

status_t InfoWindowManager::openWindowFor(
	const Connection &connection) {
	D_WINDOW(("InfoWindowManager::openWindowFor(connection)\n"));

	// make absolutely sure we're locked
	if (!IsLocked()) {
		debugger("The looper must be locked !");
	}

	BWindow *window = 0;
	if (_findWindowFor(connection.source(), connection.destination(), &window)) {
		// window for this node already exists, activate it
		window->SetWorkspaces(B_CURRENT_WORKSPACE);
		window->Activate();
		return B_OK;
	}

	BRect frame = InfoView::M_DEFAULT_FRAME;
	frame.OffsetTo(m_nextWindowPosition);
	m_nextWindowPosition += M_DEFAULT_OFFSET;
	window = new InfoWindow(frame);

	if (_addWindowFor(connection, window)) {
		window->AddChild(new ConnectionInfoView(connection));
		window->Show();
		return B_OK;
	}

	// failed
	delete window;
	return B_ERROR;
}

status_t InfoWindowManager::openWindowFor(
	const media_input &input) {
	D_WINDOW(("InfoWindowManager::openWindowFor(input)\n"));

	// make absolutely sure we're locked
	if (!IsLocked()) {
		debugger("The looper must be locked !");
	}

	BWindow *window = 0;
	if (_findWindowFor(input.destination, &window)) {
		// window for this node already exists, activate it
		window->SetWorkspaces(B_CURRENT_WORKSPACE);
		window->Activate();
		return B_OK;
	}

	BRect frame = InfoView::M_DEFAULT_FRAME;
	frame.OffsetTo(m_nextWindowPosition);
	m_nextWindowPosition += M_DEFAULT_OFFSET;
	window = new InfoWindow(frame);

	if (_addWindowFor(input, window)) {
		window->AddChild(new EndPointInfoView(input));
		window->Show();
		return B_OK;
	}

	// failed
	delete window;
	return B_ERROR;
}

status_t InfoWindowManager::openWindowFor(
	const media_output &output) {
	D_WINDOW(("InfoWindowManager::openWindowFor(output)\n"));

	// make absolutely sure we're locked
	if (!IsLocked()) {
		debugger("The looper must be locked !");
	}

	BWindow *window = 0;
	if (_findWindowFor(output.source, &window)) {
		// window for this node already exists, activate it
		window->SetWorkspaces(B_CURRENT_WORKSPACE);
		window->Activate();
		return B_OK;
	}

	BRect frame = InfoView::M_DEFAULT_FRAME;
	frame.OffsetTo(m_nextWindowPosition);
	m_nextWindowPosition += M_DEFAULT_OFFSET;
	window = new BWindow(frame, "", B_DOCUMENT_WINDOW, 0);

	if (_addWindowFor(output, window)) {
		window->AddChild(new EndPointInfoView(output));
		window->Show();
		return B_OK;
	}

	// failed
	delete window;
	return B_ERROR;
}

// -------------------------------------------------------- //
// *** BLooper impl
// -------------------------------------------------------- //

void InfoWindowManager::MessageReceived(
	BMessage *message) {
	D_MESSAGE(("InfoWindowManager::MessageReceived()\n"));

	switch (message->what) {
		case M_LIVE_NODE_WINDOW_CLOSED: {
			D_MESSAGE((" -> M_LIVE_NODE_WINDOW_CLOSED\n"));
			int32 nodeID;
			if (message->FindInt32("nodeID", &nodeID) != B_OK) {
				return;
			}
			_removeWindowFor(nodeID);
			break;
		}
		case M_DORMANT_NODE_WINDOW_CLOSED: {
			D_MESSAGE((" -> M_DORMANT_NODE_WINDOW_CLOSED\n"));
			dormant_node_info info;
			if (message->FindInt32("addOnID", &info.addon) != B_OK) {
				return;
			}
			if (message->FindInt32("flavorID", &info.flavor_id) != B_OK) {
				return;
			}
			_removeWindowFor(info);
			break;
		}
		case M_CONNECTION_WINDOW_CLOSED: {
			D_MESSAGE((" -> M_CONNECTION_WINDOW_CLOSED\n"));
			media_source source;
			if (message->FindInt32("source_port", &source.port) != B_OK) {
				return;
			}
			if (message->FindInt32("source_id", &source.id) != B_OK) {
				return;
			}
			media_destination destination;
			if (message->FindInt32("destination_port", &destination.port) != B_OK) {
				return;
			}
			if (message->FindInt32("destination_id", &destination.id) != B_OK) {
				return;
			}
			_removeWindowFor(source, destination);
			break;
		}
		case M_INPUT_WINDOW_CLOSED: {
			D_MESSAGE((" -> M_INPUT_WINDOW_CLOSED\n"));
			media_destination destination;
			if (message->FindInt32("destination_port", &destination.port) != B_OK) {
				return;
			}
			if (message->FindInt32("destination_id", &destination.id) != B_OK) {
				return;
			}
			_removeWindowFor(destination);
			break;
		}
		case M_OUTPUT_WINDOW_CLOSED: {
			D_MESSAGE((" -> M_OUTPUT_WINDOW_CLOSED\n"));
			media_source source;
			if (message->FindInt32("source_port", &source.port) != B_OK) {
				return;
			}
			if (message->FindInt32("source_id", &source.id) != B_OK) {
				return;
			}
			_removeWindowFor(source);
			break;
		}
		case NodeRef::M_RELEASED: {
			D_MESSAGE((" -> NodeRef::M_RELEASED\n"));
			int32 nodeID;
			if (message->FindInt32("nodeID", &nodeID) != B_OK) {
				return;
			}
			BWindow *window;
			if (_findWindowFor(nodeID, &window)) {
				window->Lock();
				window->Quit();
				_removeWindowFor(nodeID);
			}
			break;
		}
		case B_MEDIA_CONNECTION_BROKEN: {
			D_MESSAGE((" -> B_MEDIA_CONNECTION_BROKEN\n"));
			const void *data;
			ssize_t dataSize;
			if (message->FindData("source", B_RAW_TYPE, &data, &dataSize) != B_OK) {
				return;
			}
			const media_source source = *reinterpret_cast<const media_source *>(data);
			if (message->FindData("destination", B_RAW_TYPE, &data, &dataSize) != B_OK) {
				return;
			}
			const media_destination destination = *reinterpret_cast<const media_destination *>(data);
			BWindow *window;
			if (_findWindowFor(source, destination, &window)) {
				window->Lock();
				window->Quit();
				_removeWindowFor(source, destination);
			}
			break;
		}
		default: {
			BLooper::MessageReceived(message);
		}
	}
}

// -------------------------------------------------------- //
// *** internal operations
// -------------------------------------------------------- //

bool InfoWindowManager::_addWindowFor(
	const NodeRef *ref,
	BWindow *window) {
	D_INTERNAL(("InfoWindowManager::_addWindowFor(live_node)\n"));

	if (!m_liveNodeWindows) {
		m_liveNodeWindows = new BList();
	}

	live_node_window *entry = new live_node_window(ref, window);
	if (m_liveNodeWindows->AddItem(reinterpret_cast<void *>(entry))) {
		add_observer(this, entry->ref);
		return true;
	}

	return false;
}

bool InfoWindowManager::_findWindowFor(
	int32 nodeID,
	BWindow **outWindow) {
	D_INTERNAL(("InfoWindowManager::_findWindowFor(live_node)\n"));

	if (!m_liveNodeWindows) {
		return false;
	}

	for (int32 i = 0; i < m_liveNodeWindows->CountItems(); i++) {
		live_node_window *entry = static_cast<live_node_window *>
								  (m_liveNodeWindows->ItemAt(i));
		if (entry->ref->id() == nodeID) {
			*outWindow = entry->window;
			return true;
		}
	}

	return false;
}

void InfoWindowManager::_removeWindowFor(
	int32 nodeID) {
	D_INTERNAL(("InfoWindowManager::_removeWindowFor(live_node)\n"));

	if (!m_liveNodeWindows) {
		return;
	}

	for (int32 i = 0; i < m_liveNodeWindows->CountItems(); i++) {
		live_node_window *entry = static_cast<live_node_window *>
								  (m_liveNodeWindows->ItemAt(i));
		if (entry->ref->id() == nodeID) {
			m_liveNodeWindows->RemoveItem(reinterpret_cast<void *>(entry));
			remove_observer(this, entry->ref);
			delete entry;
		}
	}

	if (m_liveNodeWindows->CountItems() <= 0) {
		delete m_liveNodeWindows;
		m_liveNodeWindows = 0;
	}
}

bool InfoWindowManager::_addWindowFor(
	const dormant_node_info &info,
	BWindow *window) {
	D_INTERNAL(("InfoWindowManager::_addWindowFor(dormant_node)\n"));

	if (!m_dormantNodeWindows) {
		m_dormantNodeWindows = new BList();
	}

	dormant_node_window *entry = new dormant_node_window(info, window);
	return m_dormantNodeWindows->AddItem(reinterpret_cast<void *>(entry));
}

bool InfoWindowManager::_findWindowFor(
	const dormant_node_info &info,
	BWindow **outWindow) {
	D_INTERNAL(("InfoWindowManager::_findWindowFor(dormant_node)\n"));

	if (!m_dormantNodeWindows) {
		return false;
	}

	for (int32 i = 0; i < m_dormantNodeWindows->CountItems(); i++) {
		dormant_node_window *entry = static_cast<dormant_node_window *>
									 (m_dormantNodeWindows->ItemAt(i));
		if ((entry->info.addon == info.addon) 
		 && (entry->info.flavor_id == info.flavor_id)) {
			*outWindow = entry->window;
			return true;
		}
	}

	return false;
}

void InfoWindowManager::_removeWindowFor(
	const dormant_node_info &info) {
	D_INTERNAL(("InfoWindowManager::_removeWindowFor(dormant_node)\n"));

	if (!m_dormantNodeWindows) {
		return;
	}

	for (int32 i = 0; i < m_dormantNodeWindows->CountItems(); i++) {
		dormant_node_window *entry = static_cast<dormant_node_window *>
									 (m_dormantNodeWindows->ItemAt(i));
		if ((entry->info.addon == info.addon) 
		 && (entry->info.flavor_id == info.flavor_id)) {
			m_dormantNodeWindows->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}
	}

	if (m_dormantNodeWindows->CountItems() >= 0) {
		delete m_dormantNodeWindows;
		m_dormantNodeWindows = 0;
	}
}

bool InfoWindowManager::_addWindowFor(
	const Connection &connection,
	BWindow *window) {
	D_INTERNAL(("InfoWindowManager::_addWindowFor(connection)\n"));

	if (!m_connectionWindows) {
		m_connectionWindows = new BList();
	}

	connection_window *entry = new connection_window(connection.source(),
													 connection.destination(),
													 window);
	return m_connectionWindows->AddItem(reinterpret_cast<void *>(entry));
}

bool InfoWindowManager::_findWindowFor(
	const media_source &source,
	const media_destination &destination,
	BWindow **outWindow) {
	D_INTERNAL(("InfoWindowManager::_findWindowFor(connection)\n"));

	if (!m_connectionWindows) {
		return false;
	}

	for (int32 i = 0; i < m_connectionWindows->CountItems(); i++) {
		connection_window *entry = static_cast<connection_window *>
								   (m_connectionWindows->ItemAt(i));
		if ((entry->source == source) 
		 && (entry->destination == destination)) {
			*outWindow = entry->window;
			return true;
		}
	}

	return false;
}

void InfoWindowManager::_removeWindowFor(
	const media_source &source,
	const media_destination &destination) {
	D_INTERNAL(("InfoWindowManager::_removeWindowFor(connection)\n"));

	if (!m_connectionWindows) {
		return;
	}

	for (int32 i = 0; i < m_connectionWindows->CountItems(); i++) {
		connection_window *entry = static_cast<connection_window *>
								   (m_connectionWindows->ItemAt(i));
		if ((entry->source == source) 
		 && (entry->destination == destination)) {
			m_connectionWindows->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}
	}

	if (m_connectionWindows->CountItems() >= 0) {
		delete m_connectionWindows;
		m_connectionWindows = 0;
	}
}

bool InfoWindowManager::_addWindowFor(
	const media_input &input,
	BWindow *window) {
	D_INTERNAL(("InfoWindowManager::_addWindowFor(input)\n"));

	if (!m_inputWindows) {
		m_inputWindows = new BList();
	}

	input_window *entry = new input_window(input.destination, window);
	return m_inputWindows->AddItem(reinterpret_cast<void *>(entry));
}

bool InfoWindowManager::_findWindowFor(
	const media_destination &destination,
	BWindow **outWindow) {
	D_INTERNAL(("InfoWindowManager::_findWindowFor(input)\n"));

	if (!m_inputWindows) {
		return false;
	}

	for (int32 i = 0; i < m_inputWindows->CountItems(); i++) {
		input_window *entry = static_cast<input_window *>
							  (m_inputWindows->ItemAt(i));
		if (entry->destination == destination) {
			*outWindow = entry->window;
			return true;
		}
	}

	return false;
}

void InfoWindowManager::_removeWindowFor(
	const media_destination &destination) {
	D_INTERNAL(("InfoWindowManager::_removeWindowFor(input)\n"));

	if (!m_inputWindows) {
		return;
	}

	for (int32 i = 0; i < m_inputWindows->CountItems(); i++) {
		input_window *entry = static_cast<input_window *>
							  (m_inputWindows->ItemAt(i));
		if (entry->destination == destination) {
			m_inputWindows->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}
	}

	if (m_inputWindows->CountItems() >= 0) {
		delete m_inputWindows;
		m_inputWindows = 0;
	}
}

bool InfoWindowManager::_addWindowFor(
	const media_output &output,
	BWindow *window) {
	D_INTERNAL(("InfoWindowManager::_addWindowFor(output)\n"));

	if (!m_outputWindows) {
		m_outputWindows = new BList();
	}

	output_window *entry = new output_window(output.source, window);
	return m_outputWindows->AddItem(reinterpret_cast<void *>(entry));
}

bool InfoWindowManager::_findWindowFor(
	const media_source &source,
	BWindow **outWindow) {
	D_INTERNAL(("InfoWindowManager::_findWindowFor(output)\n"));

	if (!m_outputWindows) {
		return false;
	}

	for (int32 i = 0; i < m_outputWindows->CountItems(); i++) {
		output_window *entry = static_cast<output_window *>
							  (m_outputWindows->ItemAt(i));
		if (entry->source == source) {
			*outWindow = entry->window;
			return true;
		}
	}

	return false;
}

void InfoWindowManager::_removeWindowFor(
	const media_source &source) {
	D_INTERNAL(("InfoWindowManager::_removeWindowFor(output)\n"));

	if (!m_outputWindows) {
		return;
	}

	for (int32 i = 0; i < m_outputWindows->CountItems(); i++) {
		output_window *entry = static_cast<output_window *>
							  (m_outputWindows->ItemAt(i));
		if (entry->source == source) {
			m_outputWindows->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}
	}

	if (m_outputWindows->CountItems() >= 0) {
		delete m_outputWindows;
		m_outputWindows = 0;
	}
}

// END -- InfoWindowManager.cpp --
