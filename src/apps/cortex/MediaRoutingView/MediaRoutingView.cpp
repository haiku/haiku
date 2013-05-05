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


// MediaRoutingView.cpp

#include "MediaRoutingView.h"

// RouteApp
#include "RouteApp.h"
#include "RouteAppNodeManager.h"
#include "RouteWindow.h"
#include "NodeSetIOContext.h"
// DormantNodeView
#include "DormantNodeView.h"
// InfoWindow
#include "InfoWindowManager.h"
// TransportWindow
#include "TransportWindow.h"
// MediaRoutingView
#include "MediaNodePanel.h"
#include "MediaWire.h"
// NodeManager
#include "NodeGroup.h"
#include "NodeRef.h"
#include "Connection.h"
// ParameterWindow
#include "ParameterWindowManager.h"

// Application Kit
#include <Application.h>
// Interface Kit
#include <Alert.h>
#include <Bitmap.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>
// Media Kit
#include <MediaRoster.h>
// Storage Kit
#include <File.h>
#include <NodeInfo.h>
#include <Path.h>
// Translation Kit
#include <BitmapStream.h>
#include <TranslatorRoster.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)
#define D_MOUSE(x) //PRINT (x)
#define D_KEY(x) //PRINT (x)

// ---------------------------------------------------------------- //
// constants
// ---------------------------------------------------------------- //

float	MediaRoutingView::M_CLEANUP_H_GAP			= 25.0;
float	MediaRoutingView::M_CLEANUP_V_GAP			= 10.0;
float	MediaRoutingView::M_CLEANUP_H_MARGIN 		= 15.0;
float	MediaRoutingView::M_CLEANUP_V_MARGIN 		= 10.0;

// ---------------------------------------------------------------- //
// m_inactiveNodeState content
// ---------------------------------------------------------------- //

struct _inactive_node_state_entry {
	_inactive_node_state_entry(
		const char* _name, int32 _kind, const BMessage& _state) :
		name(_name), kind(_kind), state(_state) {}

	BString name;
	uint32 kind;
	BMessage state;
};

// ---------------------------------------------------------------- //
// ctor/dtor
// ---------------------------------------------------------------- //

MediaRoutingView::MediaRoutingView(
	RouteAppNodeManager *nodeManager,
	BRect frame,
	const char *name,
	uint32 resizeMode)
	: DiagramView(frame, "MediaRoutingView", true, B_FOLLOW_ALL_SIDES),
	  manager(nodeManager),
	  m_layout(M_ICON_VIEW),
	  m_nextGroupNumber(1),
	  m_lastDroppedNode(0),
	  m_draggedWire(0)
{
	D_METHOD(("MediaRoutingView::MediaRoutingView()\n"));
	ASSERT(manager);

	setBackgroundColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));
	SetItemAlignment(5.0, 5.0);
	_initLayout();
}

MediaRoutingView::~MediaRoutingView() {
	D_METHOD(("MediaRoutingView::~MediaRoutingView()\n"));

	_emptyInactiveNodeState();
	
	// quit ParameterWindowManager if necessary
	ParameterWindowManager::shutDown();

	// quit InfoWindowManager if necessary
	InfoWindowManager::shutDown();
}

// -------------------------------------------------------- //
// *** derived from DiagramView
// -------------------------------------------------------- //

void MediaRoutingView::connectionAborted(
	DiagramEndPoint *fromWhich)
{
	D_METHOD(("MediaRoutingView::connectionAborted()\n"));

	be_app->SetCursor(B_HAND_CURSOR);
}

void MediaRoutingView::connectionEstablished(
	DiagramEndPoint *fromWhich,
	DiagramEndPoint *toWhich)
{
	D_METHOD(("MediaRoutingView::connectionEstablished()\n"));

	be_app->SetCursor(B_HAND_CURSOR);
}

DiagramWire *MediaRoutingView::createWire(
	DiagramEndPoint *fromWhich,
	DiagramEndPoint *toWhich)
{
	D_METHOD(("MediaRoutingView::createWire()\n"));

	MediaJack *outputJack, *inputJack;
	MediaJack *jack = dynamic_cast<MediaJack *>(fromWhich);
	if (jack && jack->isOutput())
	{
		outputJack = jack;
		inputJack = dynamic_cast<MediaJack *>(toWhich);
		if (!inputJack || !inputJack->isInput())
		{
			return 0;
		}
	}
	else
	{
		inputJack = jack;
		outputJack = dynamic_cast<MediaJack *>(toWhich);
		if (!outputJack || !outputJack->isOutput())
		{
			return 0;
		}
	}
	if (!outputJack->isConnected() && !inputJack->isConnected())
	{
		media_output output;
		media_input input;
		if ((outputJack->getOutput(&output) == B_OK)
		 && (inputJack->getInput(&input) == B_OK))
		{
			status_t error;
			Connection connection;
			error = manager->connect(output, input, &connection);
/*			if (error)
			{
				showErrorMessage("Could not connect", error);
			}
*/		}
	}
	return 0;
}

DiagramWire *MediaRoutingView::createWire(
	DiagramEndPoint *fromWhich)
{
	D_METHOD(("MediaRoutingView::createWire(temp)\n"));

	MediaJack *jack = dynamic_cast<MediaJack *>(fromWhich);
	if (jack)
	{
		if (jack->isOutput()) // this is the start point
		{
			return new MediaWire(jack, true);
		}
		else
		{
			return new MediaWire(jack, false);
		}
	}
	return 0;
}


void
MediaRoutingView::BackgroundMouseDown(BPoint point, uint32 buttons,
	uint32 clicks)
{
	D_MOUSE(("MediaRoutingView::BackgroundMouseDown()\n"));

	if ((buttons == B_SECONDARY_MOUSE_BUTTON)
	 || (modifiers() & B_CONTROL_KEY)) {
		EndRectTracking();
		showContextMenu(point);
	}
}


void
MediaRoutingView::MessageDropped(BPoint point, BMessage *message)
{
	D_METHOD(("MediaRoutingView::MessageDropped()\n"));

	switch (message->what) {
		case DormantNodeView::M_INSTANTIATE_NODE:
		{
			D_MESSAGE(("MediaRoutingView::MessageDropped(DormantNodeView::M_INSTANTIATE_NODE)\n"));
			type_code type;
			int32 count;
			if (message->GetInfo("which", &type, &count) == B_OK) {
				for (int32 n = 0; n < count; n++) {
					dormant_node_info info;
					const void *data;
					ssize_t dataSize;
					if (message->FindData("which", B_RAW_TYPE, &data, &dataSize) == B_OK) {
						memcpy(reinterpret_cast<void *>(&info), data, dataSize);
						NodeRef* droppedNode;
						status_t error;
						error = manager->instantiate(info, &droppedNode);
						if (!error) {
							m_lastDroppedNode = droppedNode->id();
							BPoint dropPoint, dropOffset;
							dropPoint = message->DropPoint(&dropOffset);
							m_lastDropPoint = Align(ConvertFromScreen(dropPoint - dropOffset));
						} else {
							BString s;
							s << "Could not instantiate '" << info.name << "'";
							showErrorMessage(s, error);
						}
					}	
				}
			}
			break;
		}

		case B_SIMPLE_DATA:
		{
			D_MESSAGE(("MediaRoutingView::MessageDropped(B_SIMPLE_DATA)\n"));
			entry_ref fileRef;
			if (message->FindRef("refs", &fileRef) == B_OK)
				_checkDroppedFile(&fileRef, ConvertFromScreen(message->DropPoint()));
			break;
		}

		case B_PASTE:
		{
			D_MESSAGE(("MediaRoutingView::MessageDropped(B_PASTE)\n"));
			ssize_t size;
			const rgb_color *color; // [e.moon 21nov99] fixed const error
			if (message->FindData("RGBColor", B_RGB_COLOR_TYPE,
				reinterpret_cast<const void **>(&color), &size) == B_OK)
				_changeBackground(*color);
			break;
		}

		default:
			DiagramView::MessageDropped(point, message);
	}
}


void
MediaRoutingView::SelectionChanged()
{
	D_METHOD(("MediaRoutingView::selectionChanged()\n"));
	_broadcastSelection();
}

// ---------------------------------------------------------------- //
// *** BView impl.
// ---------------------------------------------------------------- //

void MediaRoutingView::AttachedToWindow()
{
	D_METHOD(("MediaRoutingView::AttachedToWindow()\n"));
	_inherited::AttachedToWindow();
	
	// attach to manager
	ASSERT(manager);
	add_observer(this, manager);

	// add the context-menu shortcuts to the window
	_addShortcuts();

	// populate with existing nodes & connections
	_initContent();

	// [e.moon 29nov99] moved from AllAttached()
	cleanUp();
}

void MediaRoutingView::AllAttached()
{
	D_METHOD(("MediaRoutingView::AllAttached()\n"));
	_inherited::AllAttached();
	
	_adjustScrollBars();

	// grab keyboard events
	MakeFocus();
}	

void MediaRoutingView::DetachedFromWindow()
{
	D_METHOD(("MediaRoutingView::DetachedFromWindow()\n"));
	_inherited::DetachedFromWindow();

	status_t error;
		
	// detach from manager
	if (manager)
	{
		Autolock lock(manager);
		void *cookie = 0;
		NodeRef *ref;
		while (manager->getNextRef(&ref, &cookie) == B_OK)
		{
			remove_observer(this, ref);
		}
		error = remove_observer(this, manager);
		const_cast<RouteAppNodeManager *&>(manager) = 0;
	}
}

void MediaRoutingView::KeyDown(
	const char *bytes,
	int32 numBytes)
{
	D_METHOD(("MediaRoutingView::KeyDown()\n"));

	switch (bytes[0])
	{
		case B_ENTER:
		{
			D_KEY(("MediaRoutingView::KeyDown(B_ENTER)\n"));
			_openParameterWindowsForSelection();
			break;
		}
		case B_DELETE:
		{
			D_KEY(("MediaRoutingView::KeyDown(B_DELETE)\n"));
			_deleteSelection();
			break;
		}
		case B_SPACE:
		{
			// [e.moon 1dec99]
			BWindow* w = Window();
			ASSERT(w);
			BMessenger(w).SendMessage(RouteWindow::M_TOGGLE_GROUP_ROLLING);
			break;
		}
		default:
		{
			DiagramView::KeyDown(bytes, numBytes);
			break;
		}
	}
}

void MediaRoutingView::Pulse()
{
	// do the animation
}

// ---------------------------------------------------------------- //
// BHandler impl
// ---------------------------------------------------------------- //

void MediaRoutingView::MessageReceived(
	BMessage* message)
{
	D_METHOD(("MediaRoutingView::MessageReceived()\n"));

	switch (message->what)
	{
		case B_MEDIA_NODE_CREATED:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(B_MEDIA_NODE_CREATED)\n"));
			type_code type;
			int32 count;
			if (message->GetInfo("media_node_id", &type, &count) == B_OK)
			{
				for(int32 n = 0; n < count; n++) 
				{
					int32 id;
					if (message->FindInt32("media_node_id", n, &id) == B_OK)
					{
						// [e.moon 8dec99] allow for existing panel
						MediaNodePanel* panel;
						if(_findPanelFor(id, &panel) < B_OK)
							_addPanelFor(id, BPoint(5.0, 5.0));
					}
				}
			}
			break;
		}
		case B_MEDIA_NODE_DELETED:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(B_MEDIA_NODE_DELETED)\n"));
			type_code type;
			int32 count;
			if (message->GetInfo("media_node_id", &type, &count) == B_OK)
			{
				for (int32 n = 0; n < count; n++) 
				{
					int32 id;
					if (message->FindInt32("media_node_id", n, &id) == B_OK)
					{
						_removePanelFor(id);
					}
				}
			}			
			break;
		}
		case B_MEDIA_CONNECTION_MADE:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(B_MEDIA_CONNECTION_MADE)\n"));
			type_code type;
			int32 count;
			if (message->GetInfo("output", &type, &count) == B_OK)
			{
				for (int32 n = 0; n < count; n++)
				{
					media_output output;
					const void *data;
					ssize_t dataSize;
					if (message->FindData("output", B_RAW_TYPE, n, &data, &dataSize) == B_OK)
					{
						output = *reinterpret_cast<const media_output *>(data);
						Connection connection;
						if (manager->findConnection(output.node.node, output.source, &connection) == B_OK)
						{
							_addWireFor(connection);
						}
					}
				}
			}
			break;
		}
		case B_MEDIA_CONNECTION_BROKEN:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(B_MEDIA_CONNECTION_BROKEN)\n"));
			type_code type;
			int32 count;
			if (message->GetInfo("__connection_id", &type, &count) == B_OK)
			{
				for (int32 n = 0; n < count; n++)
				{
					int32 id;
					if (message->FindInt32("__connection_id", n, &id) == B_OK)
					{
						_removeWireFor(id);
					}	
				}
			}
			break;
		}
		case B_MEDIA_FORMAT_CHANGED:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(B_MEDIA_FORMAT_CHANGED)\n"));
			
			media_node_id nodeID;
			if(message->FindInt32("__source_node_id", &nodeID) < B_OK)
				break;

			uint32 connectionID;
			if(message->FindInt32("__connection_id", (int32*)&connectionID) < B_OK)
				break;

			media_source* source;
			ssize_t dataSize;
			if(message->FindData("be:source", B_RAW_TYPE, (const void**)&source, &dataSize) < B_OK)
				break;
				
			MediaWire* wire;
			if(_findWireFor(connectionID, &wire) == B_OK) {
				// copy new connection data
				manager->findConnection(nodeID, *source, &wire->connection);
			}
			break;
		}
		case M_CLEANUP_REQUESTED:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(M_M_CLEANUP_REQUESTED)\n"));
			cleanUp();
			break;
		}
		case M_SELECT_ALL:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(M_SELECT_ALL)\n"));
			SelectAll(DiagramItem::M_BOX);
			break;
		}
		case M_DELETE_SELECTION:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(M_DELETE_SELECTION)\n"));
			_deleteSelection();
			break;
		}
		case M_NODE_CHANGE_CYCLING:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(M_NODE_CYCLING_CHANGED)\n"));
			bool cycle;
			if (message->FindBool("cycle", &cycle) == B_OK)
			{
				_changeCyclingForSelection(cycle);
			}
			break;
		}
		case M_NODE_CHANGE_RUN_MODE:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(M_NODE_RUNMODE_CHANGED)\n"));
			int32 mode;
			if (message->FindInt32("run_mode", &mode) == B_OK)
			{
				_changeRunModeForSelection(static_cast<uint32>(mode));
			}
			break;
		}
		case M_LAYOUT_CHANGED:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(M_LAYOUT_CHANGED)\n"));
			layout_t layout;
			if (message->FindInt32("layout", (int32*)&layout) == B_OK)
			{
				if (layout != m_layout)
				{
					layoutChanged(layout);
					updateDataRect();
					Invalidate();
				}
			}
			break;
		}
		case M_NODE_START_TIME_SOURCE:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(M_NODE_START_TIME_SOURCE)\n"));
			int32 id;
			if(message->FindInt32("nodeID", &id) < B_OK)
				break;
			NodeRef* ref;
			if(manager->getNodeRef(id, &ref) < B_OK)
				break;
			
			bigtime_t when = system_time();
			status_t err = manager->roster->StartTimeSource(ref->node(), when);
			if(err < B_OK) {
				PRINT((
					"! StartTimeSource(%ld): '%s'\n",
					ref->id(), strerror(err)));
			}
			break;
		}
		case M_NODE_STOP_TIME_SOURCE:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(M_NODE_STOP_TIME_SOURCE)\n"));
			int32 id;
			if(message->FindInt32("nodeID", &id) < B_OK)
				break;
			NodeRef* ref;
			if(manager->getNodeRef(id, &ref) < B_OK)
				break;
			
			bigtime_t when = system_time();
			status_t err = manager->roster->StopTimeSource(ref->node(), when);
			if(err < B_OK) {
				PRINT((
					"! StopTimeSource(%ld): '%s'\n",
					ref->id(), strerror(err)));
			}
			break;
		}
		case M_NODE_TWEAK_PARAMETERS: {
			D_MESSAGE((" -> M_NODE_TWEAK_PARAMETERS\n"));
			_openParameterWindowsForSelection();
			break;
		}
		case M_NODE_START_CONTROL_PANEL: {
			D_MESSAGE((" -> M_NODE_START_CONTROL_PANEL\n"));
			_startControlPanelsForSelection();
			break;
		}
		case M_GROUP_SET_LOCKED:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(M_GROUP_SET_LOCKED)\n"));
			int32 groupID;
			if(message->FindInt32("groupID", &groupID) < B_OK)
				break;
			bool locked;
			if(message->FindBool("locked", &locked) < B_OK)
				break;
			NodeGroup* group;
			if(manager->findGroup(groupID, &group) < B_OK)
				break;
			uint32 f = locked ?
				group->groupFlags() | NodeGroup::GROUP_LOCKED :
				group->groupFlags() & ~NodeGroup::GROUP_LOCKED;
			group->setGroupFlags(f);
			break;
		}
		case M_BROADCAST_SELECTION: {
			D_MESSAGE((" -> M_BROADCAST_SELECTION\n"));
			_broadcastSelection();
			break;
		}
		case InfoWindowManager::M_INFO_WINDOW_REQUESTED:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(InfoView::M_INFO_WINDOW_REQUESTED)\n"));
			type_code type;
			int32 count;
			if (message->GetInfo("input", &type, &count) == B_OK)
			{
				for (int32 i = 0; i < count; i++)
				{
					media_input input;
					const void *data;
					ssize_t dataSize;
					if (message->FindData("input", B_RAW_TYPE, i, &data, &dataSize) == B_OK)
					{
						input = *reinterpret_cast<const media_input *>(data);
						InfoWindowManager *manager = InfoWindowManager::Instance();
						if (manager && manager->Lock()) {
							manager->openWindowFor(input);
							manager->Unlock();
						}
					}
				}
			}
			else if (message->GetInfo("output", &type, &count) == B_OK)
			{
				for (int32 i = 0; i < count; i++)
				{
					media_output output;
					const void *data;
					ssize_t dataSize;
					if (message->FindData("output", B_RAW_TYPE, i, &data, &dataSize) == B_OK)
					{
						output = *reinterpret_cast<const media_output *>(data);
						InfoWindowManager *manager = InfoWindowManager::Instance();
						if (manager && manager->Lock()) {
							manager->openWindowFor(output);
							manager->Unlock();
						}
					}
				}
			}
			else
			{
				_openInfoWindowsForSelection();
			}
			break;
		}
		case NodeManager::M_RELEASED:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(NodeManager::M_RELEASED)\n"));
			remove_observer(this, manager);
			const_cast<RouteAppNodeManager*&>(manager) = 0;	
			// +++++ disable view!
			break;
		}			
		case NodeRef::M_RELEASED:
		{
			D_MESSAGE(("MediaRoutingView::MessageReceived(NodeRef::M_RELEASED)\n"));
			// only relevant on shutdown; do nothing
			break;
		}
		default:
		{
			DiagramView::MessageReceived(message);
		}
	}
}

// ---------------------------------------------------------------- //
// *** operations (public)
// ---------------------------------------------------------------- //

BPoint MediaRoutingView::findFreePositionFor(
	const MediaNodePanel* panel) const
{
	D_METHOD(("MediaRoutingView::_findFreeSpotFor()\n"));
	
	BPoint p(M_CLEANUP_H_MARGIN, M_CLEANUP_V_MARGIN);
	if (panel)
	{
		switch (m_layout)
		{
			case M_ICON_VIEW:
			{
				// find the target column by node_kind
				p.x += M_CLEANUP_H_GAP + MediaNodePanel::M_DEFAULT_WIDTH;
				if (panel->ref->kind() & B_BUFFER_PRODUCER)
				{
					p.x -= M_CLEANUP_H_GAP + MediaNodePanel::M_DEFAULT_WIDTH;
				}
				if (panel->ref->kind() & B_BUFFER_CONSUMER)
				{
					p.x += M_CLEANUP_H_GAP + MediaNodePanel::M_DEFAULT_WIDTH;
				}
				// find the bottom item in the column
				float bottom = 0.0;
				for (uint32 i = 0; i < CountItems(DiagramItem::M_BOX); i++)
				{
					BRect r = ItemAt(i, DiagramItem::M_BOX)->Frame();
					if ((r.left >= p.x)
					 && (r.left <= p.x + MediaNodePanel::M_DEFAULT_WIDTH))
					{
						bottom = (r.bottom > bottom) ? r.bottom : bottom;
					}
				}
				if (bottom >= p.y)
				{
					p.y = bottom + M_CLEANUP_V_GAP;
				}
				break;
			}
			case M_MINI_ICON_VIEW:
			{
				// find the target row by node_kind
				p.y += M_CLEANUP_V_GAP + MediaNodePanel::M_DEFAULT_HEIGHT;
				if (panel->ref->kind() & B_BUFFER_PRODUCER)
				{
					p.y -= M_CLEANUP_V_GAP + MediaNodePanel::M_DEFAULT_HEIGHT;
				}
				if (panel->ref->kind() & B_BUFFER_CONSUMER)
				{
					p.y += M_CLEANUP_V_GAP + MediaNodePanel::M_DEFAULT_HEIGHT;
				}
				// find the right-most item in the row
				float right = 0.0;
				for (uint32 i = 0; i < CountItems(DiagramItem::M_BOX); i++)
				{
					BRect r = ItemAt(i, DiagramItem::M_BOX)->Frame();
					if ((r.top >= p.y)
					 && (r.top <= p.y + MediaNodePanel::M_DEFAULT_HEIGHT))
					{
						right = (r.right > right) ? r.right : right;
					}
				}
				if (right >= p.x)
				{
					p.x = right + M_CLEANUP_H_GAP;
				}
				break;				
			}
		}
	}
	return p;
}

// ---------------------------------------------------------------- //
// *** operations (protected)
// ---------------------------------------------------------------- //

void MediaRoutingView::layoutChanged(
	layout_t layout)
{
	D_METHOD(("MediaRoutingView::layoutChanged()\n"));
	
	switch (layout)
	{
		case M_ICON_VIEW:
		{
			float temp;

			// swap the cleanup defaults
			temp = M_CLEANUP_H_GAP;
			M_CLEANUP_H_GAP = M_CLEANUP_V_GAP;
			M_CLEANUP_V_GAP = temp;
			temp = M_CLEANUP_H_MARGIN;
			M_CLEANUP_H_MARGIN = M_CLEANUP_V_MARGIN;
			M_CLEANUP_V_MARGIN = temp;

			// swap the default dimensions for MediaJacks
			temp = MediaJack::M_DEFAULT_WIDTH;
			MediaJack::M_DEFAULT_WIDTH = MediaJack::M_DEFAULT_HEIGHT;
			MediaJack::M_DEFAULT_HEIGHT = temp;

			// Add space for the 3-letter i/o-abbreviation
			BFont font(be_plain_font);
			font.SetSize(font.Size() - 2.0);
			for (int i = 0; i < MediaJack::M_MAX_ABBR_LENGTH; i++)
			{
				MediaJack::M_DEFAULT_WIDTH += font.StringWidth("M");
			}
			MediaJack::M_DEFAULT_WIDTH += 2.0; // add some padding

			// Adjust the default size for MediaNodePanels
			float labelWidth, bodyWidth;
			float labelHeight, bodyHeight;
			font_height fh;
			be_plain_font->GetHeight(&fh);
			labelWidth = 4 * MediaNodePanel::M_LABEL_H_MARGIN
						 + be_plain_font->StringWidth(" Be Audio Mixer ");
			bodyWidth = 2 * MediaNodePanel::M_BODY_H_MARGIN + B_LARGE_ICON
						+ 2 * MediaJack::M_DEFAULT_WIDTH;
			labelHeight = 2 * MediaNodePanel::M_LABEL_V_MARGIN
						  + fh.ascent + fh.descent + fh.leading + 1.0;
			bodyHeight = 2 * MediaNodePanel::M_BODY_V_MARGIN + B_LARGE_ICON;
			MediaNodePanel::M_DEFAULT_WIDTH = labelWidth > bodyWidth ? labelWidth : bodyWidth;
			MediaNodePanel::M_DEFAULT_HEIGHT = labelHeight + bodyHeight;
			Align(&MediaNodePanel::M_DEFAULT_WIDTH, &MediaNodePanel::M_DEFAULT_HEIGHT);
			break;
		}
		case M_MINI_ICON_VIEW:
		{
			float temp;

			// Swap the cleanup defaults
			temp = M_CLEANUP_H_GAP;
			M_CLEANUP_H_GAP = M_CLEANUP_V_GAP;
			M_CLEANUP_V_GAP = temp;
			temp = M_CLEANUP_H_MARGIN;
			M_CLEANUP_H_MARGIN = M_CLEANUP_V_MARGIN;
			M_CLEANUP_V_MARGIN = temp;

			// Subtract space for the 3-letter i/o-abbreviation
			BFont font(be_plain_font);
			font.SetSize(font.Size() - 2.0);
			for (int i = 0; i < MediaJack::M_MAX_ABBR_LENGTH; i++)
			{
				MediaJack::M_DEFAULT_WIDTH -= font.StringWidth("M");
			}
			MediaJack::M_DEFAULT_WIDTH -= 2.0; // substract the padding

			// Swap the default dimensions for MediaJacks
			temp = MediaJack::M_DEFAULT_WIDTH;
			MediaJack::M_DEFAULT_WIDTH = MediaJack::M_DEFAULT_HEIGHT;
			MediaJack::M_DEFAULT_HEIGHT = temp;

			// Adjust the default size for MediaNodePanels
			float labelWidth, bodyWidth;
			float labelHeight, bodyHeight;
			font_height fh;
			be_plain_font->GetHeight(&fh);
			labelWidth = 4 * MediaNodePanel::M_LABEL_H_MARGIN
						 + be_plain_font->StringWidth(" Be Audio Mixer ");
			bodyWidth = 2 * MediaNodePanel::M_BODY_H_MARGIN + B_MINI_ICON;
			labelHeight = 3 * MediaNodePanel::M_LABEL_V_MARGIN
						  + fh.ascent + fh.descent + fh.leading
						  + 2 * MediaJack::M_DEFAULT_HEIGHT;
			bodyHeight = 2 * MediaNodePanel::M_BODY_V_MARGIN + B_MINI_ICON;
			MediaNodePanel::M_DEFAULT_WIDTH = labelWidth + bodyWidth;
			MediaNodePanel::M_DEFAULT_HEIGHT = labelHeight > bodyHeight ? labelHeight : bodyHeight;
			Align(&MediaNodePanel::M_DEFAULT_WIDTH, &MediaNodePanel::M_DEFAULT_HEIGHT);
			break;
		}
	}
	m_layout = layout;
	for (uint32 i = 0; i < CountItems(DiagramItem::M_BOX); i++)
	{
		MediaNodePanel *panel = dynamic_cast<MediaNodePanel *>(ItemAt(i, DiagramItem::M_BOX));
		if (panel)
		{
			panel->layoutChanged(layout);
		}
	}

	_adjustScrollBars();
}

void MediaRoutingView::cleanUp()
{
	D_METHOD(("MediaRoutingView::cleanUp()\n"));

	SortItems(DiagramItem::M_BOX, compareID);

	// move all the panels offscreen
	for (uint32 i = 0; i < CountItems(DiagramItem::M_BOX); i++)
	{
		ItemAt(i, DiagramItem::M_BOX)->moveTo(BPoint(-200.0, -200.0));
	}

	// move all panels to their 'ideal' position
	for (uint32 i = 0; i < CountItems(DiagramItem::M_BOX); i++)
	{
		MediaNodePanel *panel;
		panel = dynamic_cast<MediaNodePanel *>(ItemAt(i, DiagramItem::M_BOX));
		BPoint p = findFreePositionFor(panel);
		panel->moveTo(p);
	}

	SortItems(DiagramItem::M_BOX, compareSelectionTime);
	Invalidate();
	updateDataRect();
}

void MediaRoutingView::showContextMenu(
	BPoint point)
{
	D_METHOD(("MediaRoutingView::showContextMenu()\n"));

	BPopUpMenu *menu = new BPopUpMenu("MediaRoutingView PopUp", false, false, B_ITEMS_IN_COLUMN);
	menu->SetFont(be_plain_font);

	// add layout options
	BMenuItem *item;
	BMessage *message = new BMessage(M_LAYOUT_CHANGED);
	message->AddInt32("layout", M_ICON_VIEW);
	menu->AddItem(item = new BMenuItem("Icon view", message));
	if (m_layout == M_ICON_VIEW)
	{
		item->SetMarked(true);
	}
	message = new BMessage(M_LAYOUT_CHANGED);
	message->AddInt32("layout", M_MINI_ICON_VIEW);
	menu->AddItem(item = new BMenuItem("Mini icon view", message));
	if (m_layout == M_MINI_ICON_VIEW)
	{
		item->SetMarked(true);
	}
	menu->AddSeparatorItem();

	// add 'CleanUp' command
	menu->AddItem(new BMenuItem("Clean up", new BMessage(M_CLEANUP_REQUESTED), 'K'));

	// add 'Select All' command
	menu->AddItem(new BMenuItem("Select all", new BMessage(M_SELECT_ALL), 'A'));

	menu->SetTargetForItems(this);
	ConvertToScreen(&point);
	point -= BPoint(1.0, 1.0);
	menu->Go(point, true, true, true);
}

void MediaRoutingView::showErrorMessage(
	BString text,
	status_t error)
{
	D_METHOD(("MediaRoutingView::showErrorMessage()\n"));

	if (error) {
		text << " (" << strerror(error) << ")";
	}

	BMessage message(M_SHOW_ERROR_MESSAGE);
	message.AddString("text", text.String());
	if (error) {
		message.AddBool("error", true);
	}
	BMessenger messenger(0, Window());
	if (!messenger.IsValid()
	 || (messenger.SendMessage(&message) != B_OK)) {
		BAlert *alert = new BAlert("Error", text.String(), "OK", 0, 0,
								   B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}
}

// -------------------------------------------------------- //
// *** IStateArchivable
// -------------------------------------------------------- //

status_t MediaRoutingView::importState(
	const BMessage*						archive) {

	status_t err;

	_emptyInactiveNodeState();

	layout_t layout;
	err = archive->FindInt32("layout", (int32*)&layout);
	if(err == B_OK && layout != m_layout) {
		layoutChanged(layout);
	}
	
	const char* path;
	err = archive->FindString("bgBitmap", &path);
	if(err == B_OK) {
		BEntry entry(path);
		entry_ref ref;
		err = entry.GetRef(&ref);
		if(err == B_OK)
			_changeBackground(&ref);
	}
	else {
		rgb_color color;
		color.alpha = 255;
		if(
			archive->FindInt8("bgRed", (int8*)&color.red) == B_OK &&
			archive->FindInt8("bgGreen", (int8*)&color.green) == B_OK &&
			archive->FindInt8("bgBlue", (int8*)&color.blue) == B_OK)
				_changeBackground(color);
	}	
	
	for(int32 n = 0; ; ++n) {

		// find panel state info; stop when exhausted
		BMessage m;
		err = archive->FindMessage("panel", n, &m);
		if(err < B_OK)
			break;
		
		const char* nodeName;
		err = archive->FindString("nodeName", n, &nodeName);
		if(err < B_OK)
			break;
			
		uint32 nodeKind;
		err = archive->FindInt32("nodeKind", n, (int32*)&nodeKind);
		if(err < B_OK)
			break;

		// look up matching panel +++++ SLOW +++++
		uint32 panelIndex;
		uint32 items = CountItems(DiagramItem::M_BOX);
		for(
			panelIndex = 0;
			panelIndex < items;
			++panelIndex) {
		
			MediaNodePanel* panel = dynamic_cast<MediaNodePanel*>(
				ItemAt(panelIndex, DiagramItem::M_BOX));
				
			if(panel &&
				!strcmp(panel->ref->name(), nodeName) &&
				panel->ref->kind() == nodeKind) {
				
				// found match; hand message to panel
				panel->importState(&m);
				break;
			}
		}
		if(panelIndex == items) {
			// no panel found
			// if a "system node" hang onto (and re-export) state info
			bool sysOwned;
			if(m.FindBool("sysOwned", &sysOwned) == B_OK && sysOwned) {
				m_inactiveNodeState.AddItem(
					new _inactive_node_state_entry(
						nodeName, nodeKind, m));
			}
		}
	}

	updateDataRect();
	
	return B_OK;
}

// +++++ export state info for currently inactive system nodes +++++
status_t MediaRoutingView::exportState(
	BMessage*									archive) const {

	// store layout mode
	archive->AddInt32("layout", m_layout);

	// store background settings
	if(m_backgroundBitmapEntry.InitCheck() == B_OK) {
		BPath path;
		m_backgroundBitmapEntry.GetPath(&path);
		archive->AddString("bgBitmap", path.Path());
	} else {
		rgb_color c = backgroundColor();
		archive->AddInt8("bgRed", c.red);
		archive->AddInt8("bgGreen", c.green);
		archive->AddInt8("bgBlue", c.blue);
	}

	// store panel positions w/ node names & signatures
	for(uint32 n = 0; n < CountItems(DiagramItem::M_BOX); ++n) {
		MediaNodePanel* panel = dynamic_cast<MediaNodePanel*>(
			ItemAt(n, DiagramItem::M_BOX));
		if(!panel)
			continue;
		
		if(panel->ref->isInternal())
			// skip internal nodes
			continue;

		BMessage m;
		panel->exportState(&m);
		archive->AddString("nodeName", panel->ref->name());
		archive->AddInt32("nodeKind", panel->ref->kind());
		archive->AddMessage("panel", &m);		
	}
	
	// copy inactive node state info
	for(int32 n = 0; n < m_inactiveNodeState.CountItems(); ++n) {
		_inactive_node_state_entry* e = reinterpret_cast<_inactive_node_state_entry*>(
			m_inactiveNodeState.ItemAt(n));

		archive->AddString("nodeName", e->name.String());
		archive->AddInt32("nodeKind", e->kind);
		archive->AddMessage("panel", &e->state);
	}

	return B_OK;	
}

// [e.moon 8dec99] subset support

status_t MediaRoutingView::importStateFor(
	const NodeSetIOContext*		context,
	const BMessage*						archive) {

	status_t err;

	for(int32 archiveIndex = 0;; ++archiveIndex) {

		// fetch archived key & panel data
		const char* key;
		err = archive->FindString("nodeKey", archiveIndex, &key);
		if(err < B_OK)
			break;
		
		BMessage m;
		err = archive->FindMessage("panel", archiveIndex, &m);
		if(err < B_OK) {
			PRINT((
				"!!! MediaRoutingView::importStateFor(): missing panel %ld\n",
				archiveIndex));
			continue;
		}
		
		// find corresponding node
		media_node_id id;
		err = context->getNodeFor(key, &id);
		if(err < B_OK) {
			PRINT((
				"!!! MediaRoutingView::importStateFor(): missing node '%s'\n",
				key));
			continue;
		}
		
		// look for panel, create it if necessary
		MediaNodePanel* panel;
		err = _findPanelFor(id,	&panel);
		if(err < B_OK) {
			// create it
			err = _addPanelFor(
				id,
				BPoint(5.0, 5.0));
			if(err < B_OK) {
				PRINT((
					"!!! MediaRoutingView::importStateFor(): _addPanelFor():\n"
					"    %s\n", strerror(err)));
				continue;
			}

			err = _findPanelFor(id,	&panel);
			if(err < B_OK) {
				PRINT((
					"!!! MediaRoutingView::importStateFor(): _findPanelFor():\n"
					"    %s\n", strerror(err)));
				continue;
			}
		}

		// pass state data along
		panel->importState(&m);
		
		// select the panel
		SelectItem(panel, false);
	}
	
	return B_OK;
}

status_t MediaRoutingView::exportStateFor(
	const NodeSetIOContext*		context,
	BMessage*									archive) const {

	status_t err;
	
	for(uint32 n = 0; n < context->countNodes(); ++n) {
		MediaNodePanel* panel;
		err = _findPanelFor(
			context->nodeAt(n),
			&panel);
		if(err < B_OK) {
			PRINT((
				"!!! MediaRoutingView::exportStateFor():\n"
				"    no panel for node %ld\n",
				context->nodeAt(n)));
			return B_BAD_VALUE;
		}
			
		const char* key = context->keyAt(n);
		
		archive->AddString("nodeKey", key);
		BMessage m;
		panel->exportState(&m);
		archive->AddMessage("panel", &m);
	}
	
	return B_OK;
}

// -------------------------------------------------------- //
// *** children management
// -------------------------------------------------------- //

status_t MediaRoutingView::_addPanelFor(
	media_node_id id,
	BPoint atPoint)
{
	D_METHOD(("MediaRoutingView::_addPanelFor()\n"));

	manager->lock();
	NodeRef *ref;
	status_t error = manager->getNodeRef(id, &ref);
	manager->unlock();
	if (!error)
	{
		add_observer(this, ref);
		MediaNodePanel *panel = 0;
		if (id == m_lastDroppedNode) // this was instantiated thru drag & drop
		{
			AddItem(panel = new MediaNodePanel(m_lastDropPoint, ref));
			SelectItem(panel, true);
			m_lastDroppedNode = 0;
		}
		else // this was an externally created node, must find a nice position first
		{
			panel = new MediaNodePanel(BPoint(0.0, 0.0), ref);
			AddItem(panel);
			BMessage state;
			if(_fetchInactiveNodeState(panel, &state) == B_OK)
				panel->importState(&state);
			else {
				BPoint p = findFreePositionFor(panel);
				panel->moveTo(p);
			}
			Invalidate(panel->Frame());
		}
	}
	updateDataRect();
	return error;
}

status_t MediaRoutingView::_findPanelFor(
	media_node_id id,
	MediaNodePanel **outPanel) const
{
	D_METHOD(("MediaRoutingView::_findPanelFor()\n"));

	for (uint32 i = 0; i < CountItems(DiagramItem::M_BOX); i++)
	{
		MediaNodePanel *panel = dynamic_cast<MediaNodePanel *>(ItemAt(i, DiagramItem::M_BOX));
		if (panel)
		{
			if (panel->ref->id() == id)
			{
				*outPanel = panel;
				return B_OK;
			}
		}
	}
	return B_ERROR;
}

status_t MediaRoutingView::_removePanelFor(
	media_node_id id)
{
	D_METHOD(("MediaRoutingView::_removePanelFor()\n"));

	MediaNodePanel *panel;
	if (_findPanelFor(id, &panel) == B_OK)
	{
		if (RemoveItem(panel))
		{
			remove_observer(this, panel->ref);
			Invalidate(panel->Frame());
			delete panel;
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t MediaRoutingView::_addWireFor(
	Connection& connection)
{
	D_METHOD(("MediaRoutingView::_addWireFor()\n"));

	MediaNodePanel *source, *destination;
	if ((_findPanelFor(connection.sourceNode(), &source) == B_OK)
	 && (_findPanelFor(connection.destinationNode(), &destination) == B_OK))
	{
		status_t error;
		
		media_output output;
		error = connection.getOutput(&output);
		if (error)
		{
			return error;
		}
		MediaJack *outputJack = new MediaJack(output);
		source->AddItem(outputJack);

		media_input input;
		error = connection.getInput(&input);
		if (error)
		{
			return error;
		}
		MediaJack *inputJack =  new MediaJack(input);
		destination->AddItem(inputJack);

		MediaWire *wire = new MediaWire(connection, outputJack, inputJack);
		AddItem(wire);
		source->updateIOJacks();
		source->arrangeIOJacks();
		destination->updateIOJacks();
		destination->arrangeIOJacks();
		updateDataRect();

		// [e.moon 21nov99] group creation/merging now performed by
		// RouteAppNodeManager

		Invalidate(source->Frame());
		Invalidate(destination->Frame());
		Invalidate(wire->Frame());
		return B_OK;
	}
	else
	{
		return B_ERROR;
	}
}

status_t MediaRoutingView::_findWireFor(
	uint32 connectionID,
	MediaWire **outWire) const
{
	D_METHOD(("MediaRoutingView::_findWireFor()\n"));

	for (uint32 i = 0; i < CountItems(DiagramItem::M_WIRE); i++)
	{
		MediaWire *wire = dynamic_cast<MediaWire *>(ItemAt(i, DiagramItem::M_WIRE));
		if (wire && wire->connection.id() == connectionID)
		{
			*outWire = wire;
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t MediaRoutingView::_removeWireFor(
	uint32 connectionID)
{
	D_METHOD(("MediaRoutingView::_removeWireFor()\n"));
	
	MediaWire *wire;
	if (_findWireFor(connectionID, &wire) == B_OK)
	{
		MediaNodePanel *source, *destination;
		_findPanelFor(wire->connection.sourceNode(), &source);
		_findPanelFor(wire->connection.destinationNode(), &destination);
		RemoveItem(wire);
		Invalidate(wire->Frame());
		delete wire;
		if (source)
		{
			source->updateIOJacks();
			source->arrangeIOJacks();
			Invalidate(source->Frame());
		}
		if (destination)
		{
			destination->updateIOJacks();
			destination->arrangeIOJacks();
			Invalidate(destination->Frame());
		}	
		
		// [e.moon 21nov99] group split/remove now performed by
		// RouteAppNodeManager

		updateDataRect();
		return B_OK;
	}
	return B_ERROR;
}

// -------------------------------------------------------- //
// *** internal methods
// -------------------------------------------------------- //

void MediaRoutingView::_addShortcuts()
{
	Window()->AddShortcut('A', B_COMMAND_KEY,
						  new BMessage(M_SELECT_ALL), this);
	Window()->AddShortcut('K', B_COMMAND_KEY,
						  new BMessage(M_CLEANUP_REQUESTED), this);
	Window()->AddShortcut('T', B_COMMAND_KEY,
						  new BMessage(M_DELETE_SELECTION), this);
	Window()->AddShortcut('P', B_COMMAND_KEY,
						  new BMessage(M_NODE_TWEAK_PARAMETERS), this);
	Window()->AddShortcut('P', B_COMMAND_KEY | B_SHIFT_KEY,
						  new BMessage(M_NODE_START_CONTROL_PANEL), this);
	Window()->AddShortcut('I', B_COMMAND_KEY,
						  new BMessage(InfoWindowManager::M_INFO_WINDOW_REQUESTED), this);
}

void MediaRoutingView::_initLayout()
{
	D_METHOD(("MediaRoutingView::_initLayout()\n"));

	switch (m_layout)
	{
		case M_ICON_VIEW:
		{
			// Adjust the jack width for displaying the abbreviated
			// input/output name
			BFont font(be_plain_font);
			font.SetSize(font.Size() - 2.0);
			for (int i = 0; i < MediaJack::M_MAX_ABBR_LENGTH; i++)
			{
				MediaJack::M_DEFAULT_WIDTH += font.StringWidth("M");
			}
			MediaJack::M_DEFAULT_WIDTH += 2.0; // add some padding

			// Adjust the default size for MediaNodePanels to fit the
			// size of be_plain_font
			float labelWidth, bodyWidth;
			float labelHeight, bodyHeight;
			font_height fh;
			be_plain_font->GetHeight(&fh);
			labelWidth = 4 * MediaNodePanel::M_LABEL_H_MARGIN
						 + be_plain_font->StringWidth(" Be Audio Mixer ");
			bodyWidth = 2 * MediaNodePanel::M_BODY_H_MARGIN + B_LARGE_ICON
						+ 2 * MediaJack::M_DEFAULT_WIDTH;
			labelHeight = 2 * MediaNodePanel::M_LABEL_V_MARGIN
						  + fh.ascent + fh.descent + fh.leading + 1.0;
			bodyHeight = 2 * MediaNodePanel::M_BODY_V_MARGIN + B_LARGE_ICON;
			MediaNodePanel::M_DEFAULT_WIDTH = labelWidth > bodyWidth ? labelWidth : bodyWidth;
			MediaNodePanel::M_DEFAULT_HEIGHT = labelHeight + bodyHeight;
			Align(&MediaNodePanel::M_DEFAULT_WIDTH, &MediaNodePanel::M_DEFAULT_HEIGHT);

			// Adjust the cleanup settings
			M_CLEANUP_H_GAP += MediaNodePanel::M_DEFAULT_WIDTH;
			break;
		}
		case M_MINI_ICON_VIEW:
		{
			// Adjust the default size for MediaNodePanels to fit the
			// size of be_plain_font
			float labelWidth, bodyWidth;
			float labelHeight, bodyHeight;
			font_height fh;
			be_plain_font->GetHeight(&fh);
			labelWidth = 4 * MediaNodePanel::M_LABEL_H_MARGIN
						 + be_plain_font->StringWidth(" Be Audio Mixer ");
			bodyWidth = 2 * MediaNodePanel::M_BODY_H_MARGIN + B_MINI_ICON;
			labelHeight = 3 * MediaNodePanel::M_LABEL_V_MARGIN
						  + fh.ascent + fh.descent + fh.leading
						  + 2 * MediaJack::M_DEFAULT_HEIGHT;
			bodyHeight = 2 * MediaNodePanel::M_BODY_V_MARGIN + B_MINI_ICON;
			MediaNodePanel::M_DEFAULT_WIDTH = labelWidth + bodyWidth;
			MediaNodePanel::M_DEFAULT_HEIGHT = labelHeight > bodyHeight ? labelHeight : bodyHeight;
			Align(&MediaNodePanel::M_DEFAULT_WIDTH, &MediaNodePanel::M_DEFAULT_HEIGHT);

			// Adjust the cleanup settings
			M_CLEANUP_V_GAP += MediaNodePanel::M_DEFAULT_HEIGHT;
			break;
		}
	}
}

void MediaRoutingView::_initContent()
{
	D_METHOD(("MediaRoutingView::_initContent()\n"));

	Autolock lock(manager);

	void *cookie = 0;
	NodeRef *ref;
	while (manager->getNextRef(&ref, &cookie) == B_OK)
	{
		// add self as observer
		add_observer(this, ref);
		// create & place node view (+++++ defer until observer status confirmed!)
		_addPanelFor(ref->id(), BPoint(M_CLEANUP_H_MARGIN, M_CLEANUP_V_MARGIN));
	}
	cookie = 0;
	Connection connection;
	while (manager->getNextConnection(&connection, &cookie) == B_OK)
	{
		_addWireFor(connection);
	}

	// create default groups
	NodeGroup* group;
	NodeRef* videoIn = manager->videoInputNode();
	if (videoIn)
	{
		group = manager->createGroup("Video input");
		group->setRunMode(BMediaNode::B_RECORDING);
		group->addNode(videoIn);
	}
	NodeRef* audioIn = manager->audioInputNode();
	if (audioIn)
	{
		group = manager->createGroup("Audio input");
		group->setRunMode(BMediaNode::B_RECORDING);
		group->addNode(audioIn);
	}
	NodeRef* videoOut = manager->videoOutputNode();
	if (videoOut)
	{
		group = manager->createGroup("Video output");
		group->addNode(videoOut);
	}
}

void MediaRoutingView::_changeCyclingForSelection(
	bool cycle)
{
	D_METHOD(("MediaRoutingView::_changeCyclingForSelection()\n"));

	if (SelectedType() == DiagramItem::M_BOX)
	{
		manager->lock();
		for (uint32 i = 0; i < CountSelectedItems(); i++)
		{
			MediaNodePanel *panel = dynamic_cast<MediaNodePanel *>(SelectedItemAt(i));
			if (panel && (panel->ref->isCycling() != cycle))
			{
				panel->ref->setCycling(cycle);
			}			
		}
		manager->unlock();
	}
}

void MediaRoutingView::_changeRunModeForSelection(
	uint32 mode)
{
	D_METHOD(("MediaRoutingView::_changeRunModeForSelection()\n"));

	if (SelectedType() == DiagramItem::M_BOX)
	{
		manager->lock();
		for (uint32 i = 0; i < CountSelectedItems(); i++)
		{
			MediaNodePanel *panel = dynamic_cast<MediaNodePanel *>(SelectedItemAt(i));
			if (panel && (panel->ref->runMode() != mode))
			{
				panel->ref->setRunMode(mode);
			}
		}
		manager->unlock();
	}
}

void MediaRoutingView::_openInfoWindowsForSelection() {
	D_METHOD(("MediaRoutingView::_openInfoWindowsForSelection()\n"));

	InfoWindowManager *manager = InfoWindowManager::Instance();
	if (!manager) {
		return;
	}

	if (SelectedType() == DiagramItem::M_BOX) {
		for (uint32 i = 0; i < CountSelectedItems(); i++) {
			MediaNodePanel *panel = dynamic_cast<MediaNodePanel *>(SelectedItemAt(i));
			if (panel && manager->Lock()) {
				manager->openWindowFor(panel->ref);
				manager->Unlock();
			}
		}
	}
	else if (SelectedType() == DiagramItem::M_WIRE) {
		for (uint32 i = 0; i < CountSelectedItems(); i++) {
			MediaWire *wire = dynamic_cast<MediaWire *>(SelectedItemAt(i));
			if (wire && manager->Lock()) {
				manager->openWindowFor(wire->connection);
				manager->Unlock();
			}
		}
	}
}

void MediaRoutingView::_openParameterWindowsForSelection() {
	D_METHOD(("MediaRoutingView::_openParameterWindowsForSelection()\n"));

	if (SelectedType() != DiagramItem::M_BOX) {
		// can only open parameter window for nodes
		return;
	}

	for (uint32 i = 0; i < CountSelectedItems(); i++) {
		MediaNodePanel *panel = dynamic_cast<MediaNodePanel *>(SelectedItemAt(i));
		if (panel && (panel->ref->kind() & B_CONTROLLABLE)) {
			ParameterWindowManager *paramMgr= ParameterWindowManager::Instance();
			if (paramMgr && paramMgr->Lock()) {
				paramMgr->openWindowFor(panel->ref);
				paramMgr->Unlock();
			}
		}
	}
}

void MediaRoutingView::_startControlPanelsForSelection() {
	D_METHOD(("MediaRoutingView::_startControlPanelsForSelection()\n"));

	if (SelectedType() != DiagramItem::M_BOX) {
		// can only start control panel for nodes
		return;
	}

	for (uint32 i = 0; i < CountSelectedItems(); i++) {
		MediaNodePanel *panel = dynamic_cast<MediaNodePanel *>(SelectedItemAt(i));
		if (panel && (panel->ref->kind() & B_CONTROLLABLE)) {
			ParameterWindowManager *paramMgr= ParameterWindowManager::Instance();
			if (paramMgr && paramMgr->Lock()) {
				paramMgr->startControlPanelFor(panel->ref);
				paramMgr->Unlock();
			}
		}
	}
}

void MediaRoutingView::_deleteSelection()
{
	D_METHOD(("MediaRoutingView::_deleteSelection()\n"));
	if (SelectedType() == DiagramItem::M_BOX)
	{
		for (uint32 i = 0; i < CountSelectedItems(); i++)
		{
			MediaNodePanel *panel = dynamic_cast<MediaNodePanel *>(SelectedItemAt(i));
			if (panel && panel->ref->isInternal())
			{
				status_t error = panel->ref->releaseNode();
				if (error)
				{
					BString s;
					s << "Could not release '" << panel->ref->name() << "'";
					showErrorMessage(s, error);
				}
			}			
		}
	}
	else if (SelectedType() == DiagramItem::M_WIRE)
	{
		for (uint32 i = 0; i < CountSelectedItems(); i++)
		{
			MediaWire *wire = dynamic_cast<MediaWire *>(SelectedItemAt(i));
			if (wire && !(wire->connection.flags() & Connection::LOCKED))
			{
				status_t error = manager->disconnect(wire->connection);
				if (error)
				{
					showErrorMessage("Could not disconnect", error);
				}
			}
		}
	}
	// make sure none of the deleted items is still displaying its mouse cursor !
	be_app->SetCursor(B_HAND_CURSOR);
}

void MediaRoutingView::_checkDroppedFile(
	entry_ref *ref,
	BPoint dropPoint)
{
	D_METHOD(("MediaRoutingView::_checkDroppedFile()\n"));

	// [cell 26apr00] traverse links
	BEntry entry(ref, true);
	entry.GetRef(ref);

	BNode node(ref);
	if (node.InitCheck() == B_OK)
	{
		BNodeInfo nodeInfo(&node);
		if (nodeInfo.InitCheck() == B_OK)
		{
			char mimeString[B_MIME_TYPE_LENGTH];
			if (nodeInfo.GetType(mimeString) == B_OK)
			{
				BMimeType mimeType(mimeString);
				BMimeType superType;
				
				// [e.moon 22dec99] handle dropped node-set files
				if(mimeType == RouteApp::s_nodeSetType) {
					BMessage m(B_REFS_RECEIVED);
					m.AddRef("refs", ref);
					be_app_messenger.SendMessage(&m);
				}
				else if (mimeType.GetSupertype(&superType) == B_OK)
				{
					if (superType == "image")
					{
						_changeBackground(ref);
					}
					else if ((superType == "audio") || (superType == "video"))
					{
						NodeRef* droppedNode;
						status_t error;
						error = manager->instantiate(*ref, B_BUFFER_PRODUCER, &droppedNode);
						if (!error)
						{
							media_output encVideoOutput;
							if (droppedNode->findFreeOutput(&encVideoOutput, B_MEDIA_ENCODED_VIDEO) == B_OK)
							{
								droppedNode->setFlags(droppedNode->flags() | NodeRef::NO_POSITION_REPORTING);
							}
							m_lastDroppedNode = droppedNode->id();
							m_lastDropPoint = Align(dropPoint);
						}
						else
						{
							char fileName[B_FILE_NAME_LENGTH]; 
							BEntry entry(ref);
							entry.GetName(fileName);
							BString s;
							s << "Could not load '" << fileName << "'";
							showErrorMessage(s, error);
						}
					}
				}
			}
		}
	}
}

void MediaRoutingView::_changeBackground(
	entry_ref *ref)
{
	D_METHOD(("MediaRoutingView::_changeBackground()\n"));

	status_t error;
	BBitmap *background = 0; 
	BFile file(ref, B_READ_ONLY); 
	error = file.InitCheck();
	if (!error)
	{
		BTranslatorRoster *roster = BTranslatorRoster::Default(); 
		BBitmapStream stream; 
		error = roster->Translate(&file, NULL, NULL, &stream, B_TRANSLATOR_BITMAP);
		if (!error)
		{
			stream.DetachBitmap(&background); 
			setBackgroundBitmap(background);
			Invalidate();

			// [e.moon 1dec99] persistence, yay
			m_backgroundBitmapEntry.SetTo(ref);
		}
	}
	delete background;
}

void MediaRoutingView::_changeBackground(
	rgb_color color)
{
	D_METHOD(("MediaRoutingView::_changeBackground()\n"));
	setBackgroundColor(color);
	Invalidate();
	
	// [e.moon 1dec99] persistence, yay
	m_backgroundBitmapEntry.Unset();
}

void
MediaRoutingView::_adjustScrollBars()
{
	D_METHOD(("MediaRoutingView::_adjustScrollBars()\n"));

	BScrollBar *scrollBar;

	// adjust horizontal scroll bar
	scrollBar = ScrollBar(B_HORIZONTAL);
	if (scrollBar) {
		float bigStep = floor(MediaNodePanel::M_DEFAULT_WIDTH + M_CLEANUP_H_GAP);
		scrollBar->SetSteps(floor(bigStep / 10.0), bigStep);
	}

	// adjust vertical scroll bar
	scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar) {
		float bigStep = floor(MediaNodePanel::M_DEFAULT_HEIGHT + M_CLEANUP_V_GAP);
		scrollBar->SetSteps(floor(bigStep / 10.0), bigStep);
	}
}

void 
MediaRoutingView::_broadcastSelection() const
{
	int32 selectedGroup = 0;

	if (SelectedType() == DiagramItem::M_BOX) {
		// iterate thru the list of selected node panels and make the
		// first group we find the selected group
		for (uint32 i = 0; i < CountSelectedItems(); i++) {
			MediaNodePanel *panel = dynamic_cast<MediaNodePanel *>(SelectedItemAt(i));
			if (panel && panel->ref->group()) {
				selectedGroup = panel->ref->group()->id();
				BMessenger messenger(Window());
				BMessage groupMsg(M_GROUP_SELECTED);
				groupMsg.AddInt32("groupID", selectedGroup);
				messenger.SendMessage(&groupMsg);
				return;
			}
		}
	}

	// currently no group is selected
	BMessenger messenger(Window());
	BMessage groupMsg(M_GROUP_SELECTED);
	groupMsg.AddInt32("groupID", selectedGroup);
	messenger.SendMessage(&groupMsg);
}

status_t 
MediaRoutingView::_fetchInactiveNodeState(MediaNodePanel *forPanel, BMessage *outMessage)
{
	// copy inactive node state info
	int32 c = m_inactiveNodeState.CountItems();
	for(int32 n = 0; n < c; n++) {
		_inactive_node_state_entry* e = reinterpret_cast<_inactive_node_state_entry*>(
			m_inactiveNodeState.ItemAt(n));
		ASSERT(e);
		if(e->name != forPanel->ref->name())
			continue;
			
		if(e->kind != forPanel->ref->kind())
			continue;

		// found match; extract message & remove entry
		*outMessage = e->state;
		m_inactiveNodeState.RemoveItem(n);
		return B_OK;
	}

	return B_BAD_VALUE;		
}

void 
MediaRoutingView::_emptyInactiveNodeState()
{
	int32 c = m_inactiveNodeState.CountItems();
	for(int32 n = 0; n < c; n++) {
		_inactive_node_state_entry* e = reinterpret_cast<_inactive_node_state_entry*>(
			m_inactiveNodeState.ItemAt(n));
		ASSERT(e);
		delete e;
	}
	m_inactiveNodeState.MakeEmpty();
}


// END -- MediaRoutingView.cpp --
