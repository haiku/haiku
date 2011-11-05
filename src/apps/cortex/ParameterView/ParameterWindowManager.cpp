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


// ParameterWindowManager.cpp

#include "ParameterWindowManager.h"
#include "ParameterWindow.h"
// NodeManager
#include "NodeRef.h"

// Application Kit
#include <AppDefs.h>
// Media Kit
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

// used to remember the list of ParameterWindows
struct window_map_entry {

public:						// *** ctor/dtor

							window_map_entry(
								const NodeRef *ref,
								BWindow *window)
								: ref(ref),
								  window(window)
							{ }

public:						// *** data members

	const NodeRef		   *ref;

	BWindow				   *window;
};

// used to remember the list of ControlPanels
struct panel_map_entry {

public:						// *** ctor/dtor

							panel_map_entry(
								int32 id,
								const BMessenger &messenger)
								: id(id),
								  messenger(messenger)
							{ }

public:						// *** data members

	int32					id;

	const BMessenger		messenger;
};

// -------------------------------------------------------- //
// static member init
// -------------------------------------------------------- //

const BPoint ParameterWindowManager::M_DEFAULT_OFFSET	= BPoint(20.0, 20.0);
const BPoint ParameterWindowManager::M_INIT_POSITION	= BPoint(90.0, 90.0);

ParameterWindowManager *ParameterWindowManager::s_instance = 0;

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

/* hidden */
ParameterWindowManager::ParameterWindowManager()
	: BLooper("ParameterWindowManager",
			  B_NORMAL_PRIORITY),
	  m_windowList(0),
	  m_panelList(0),
	  m_lastWindowPosition(M_INIT_POSITION - M_DEFAULT_OFFSET) {
	D_ALLOC(("ParameterWindowManager::ParameterWindowManager()\n"));

	m_windowList = new BList();
	m_panelList = new BList();
	Run();
}

ParameterWindowManager::~ParameterWindowManager() {
	D_ALLOC(("ParameterWindowManager::~ParameterWindowManager()\n"));

	while (m_windowList->CountItems() > 0) {
		window_map_entry *entry = static_cast<window_map_entry *>
								  (m_windowList->ItemAt(0));
		if (entry && entry->window) {
			remove_observer(this, entry->ref);
			entry->window->Lock();
			entry->window->Quit();
		}
		m_windowList->RemoveItem(reinterpret_cast<void *>(entry));
		delete entry;
	}	
	delete m_windowList;

	while (m_panelList->CountItems() > 0) {
		panel_map_entry *entry = static_cast<panel_map_entry *>
								 (m_panelList->ItemAt(0));
		if (entry && entry->messenger.IsValid()) {
			entry->messenger.SendMessage(B_QUIT_REQUESTED);
		}
		m_panelList->RemoveItem(reinterpret_cast<void *>(entry));
		delete entry;
	}	
	delete m_panelList;

	s_instance = 0;
}

// -------------------------------------------------------- //
// *** singleton access
// -------------------------------------------------------- //

/*static*/
ParameterWindowManager *ParameterWindowManager::Instance() {
	D_ACCESS(("ParameterWindowManager::Instance()\n"));

	if (!s_instance) {
		D_ACCESS((" -> create instance\n"));
		s_instance = new ParameterWindowManager();
	}		

	return s_instance;
}

/* static */
void ParameterWindowManager::shutDown() {
	D_WINDOW(("ParameterWindowManager::shutDown()\n"));

	if (s_instance) {
		s_instance->Lock();
		s_instance->Quit();
	}
}

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

status_t ParameterWindowManager::openWindowFor(
	const NodeRef *ref) {
	D_WINDOW(("ParameterWindowManager::openWindowFor()\n"));

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

	BMessenger messenger(0, this);
	live_node_info nodeInfo = ref->nodeInfo();
	m_lastWindowPosition += M_DEFAULT_OFFSET;
	window = new ParameterWindow(m_lastWindowPosition,
								 nodeInfo, &messenger);
	if (_addWindowFor(ref, window)) {
		window->Show();
		return B_OK;
	}
	delete window;

	return B_ERROR;
}

status_t ParameterWindowManager::startControlPanelFor(
	const NodeRef *ref) {
	D_WINDOW(("ParameterWindowManager::startControlPanelFor()\n"));

	// make absolutely sure we're locked
	if (!IsLocked()) {
		debugger("The looper must be locked !");
	}

	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (!roster) {
		D_WINDOW((" -> MediaRoster not available\n"));
		return B_ERROR;
	}

	BMessenger messenger;
	if (_findPanelFor(ref->id(), &messenger)) {
		// find out if the messengers target still exists
		if (messenger.IsValid()) {
			return B_OK;
		}
		else {
			_removePanelFor(ref->id());
		}
	}

	status_t error = roster->StartControlPanel(ref->node(),
											   &messenger);
	if (error) {
		D_INTERNAL((" -> StartControlPanel() failed (%s)\n",
					strerror(error)));
		return error;
	}

	_addPanelFor(ref->id(), messenger);
	return B_OK;
}

// -------------------------------------------------------- //
// *** BLooper impl
// -------------------------------------------------------- //

void ParameterWindowManager::MessageReceived(
	BMessage *message) {
	D_MESSAGE(("ParameterWindowManager::MessageReceived()\n"));

	switch (message->what) {
		case ParameterWindow::M_CLOSED: {
			D_MESSAGE((" -> ParameterWindow::M_CLOSED\n"));
			int32 nodeID;
			if (message->FindInt32("nodeID", &nodeID) != B_OK) {
				return;
			}
			_removeWindowFor(nodeID);
			break;
		}
		case ParameterWindow::M_CONTROL_PANEL_STARTED: {
			D_MESSAGE((" -> ParameterWindow::M_CONTROL_PANEL_STARTED\n"));
			int32 nodeID;
			if (message->FindInt32("nodeID", &nodeID) != B_OK) {
				return;
			}
			BMessenger messenger;
			if (message->FindMessenger("messenger", &messenger) != B_OK) {
				return;
			}
			_addPanelFor(nodeID, messenger);
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
		default: {
			BLooper::MessageReceived(message);
		}
	}
}

// -------------------------------------------------------- //
// *** internal operations
// -------------------------------------------------------- //

bool ParameterWindowManager::_addWindowFor(
	const NodeRef *ref,
	BWindow *window) {
	D_INTERNAL(("ParameterWindowManager::_addWindowFor()\n"));

	window_map_entry *entry = new window_map_entry(ref,
												   window);
	if (m_windowList->AddItem(reinterpret_cast<void *>(entry))) {
		add_observer(this, entry->ref);
		return true;
	}

	return false;
}

bool ParameterWindowManager::_findWindowFor(
	int32 id,
	BWindow **outWindow) {
	D_INTERNAL(("ParameterWindowManager::_findWindowFor()\n"));

	for (int32 i = 0; i < m_windowList->CountItems(); i++) {
		window_map_entry *entry = static_cast<window_map_entry *>
								  (m_windowList->ItemAt(i));
		if (entry->ref->id() == id) {
			*outWindow = entry->window;
			return true;
		}
	}

	return false;
}

void ParameterWindowManager::_removeWindowFor(
	int32 id) {
	D_INTERNAL(("ParameterWindowManager::_removeWindowFor()\n"));

	for (int32 i = 0; i < m_windowList->CountItems(); i++) {
		window_map_entry *entry = static_cast<window_map_entry *>
								  (m_windowList->ItemAt(i));
		if (entry->ref->id() == id) {
			m_windowList->RemoveItem(reinterpret_cast<void *>(entry));
			remove_observer(this, entry->ref);
			delete entry;
		}
	}
	
	// try to shutdown
	if (m_windowList->CountItems() == 0) {
		int32 i = 0;
		while (true) {
			// take all invalid messengers out of the panel list
			panel_map_entry *entry = static_cast<panel_map_entry *>
									 (m_panelList->ItemAt(i));
			if (!entry) {
				// end of list
				break;
			}
			if (!entry->messenger.IsValid()) {
				// this control panel doesn't exist anymore
				m_panelList->RemoveItem(entry);
				continue;
			}
		}
		if (m_panelList->CountItems() == 0) {
			// neither windows nor panels to manage, go to sleep
			PostMessage(B_QUIT_REQUESTED);
		}
	}
}

bool ParameterWindowManager::_addPanelFor(
	int32 id,
	const BMessenger &messenger) {
	D_INTERNAL(("ParameterWindowManager::_addPanelFor()\n"));

	panel_map_entry *entry = new panel_map_entry(id,
												 messenger);
	if (m_panelList->AddItem(reinterpret_cast<void *>(entry))) {
		return true;
	}

	return false;
}

bool ParameterWindowManager::_findPanelFor(
	int32 id,
	BMessenger *outMessenger) {
	D_INTERNAL(("ParameterWindowManager::_findPanelFor()\n"));

	for (int32 i = 0; i < m_panelList->CountItems(); i++) {
		panel_map_entry *entry = static_cast<panel_map_entry *>
								 (m_panelList->ItemAt(i));
		if (entry->id == id) {
			*outMessenger = entry->messenger;
			return true;
		}
	}

	return false;
}

void ParameterWindowManager::_removePanelFor(
	int32 id) {
	D_INTERNAL(("ParameterWindowManager::_removeWindowFor()\n"));

	for (int32 i = 0; i < m_panelList->CountItems(); i++) {
		panel_map_entry *entry = static_cast<panel_map_entry *>
								 (m_panelList->ItemAt(i));
		if (entry->id == id) {
			m_panelList->RemoveItem(reinterpret_cast<void *>(entry));
			delete entry;
		}
	}
}

// END -- ParameterWindowManager.cpp --
