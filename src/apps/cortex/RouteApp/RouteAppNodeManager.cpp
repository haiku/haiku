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


// RouteAppNodeManager.cpp

#include "RouteAppNodeManager.h"

#include "MediaIcon.h"
#include "NodeGroup.h"
#include "NodeRef.h"
#include "Connection.h"

#include "route_app_io.h"
#include "ConnectionIO.h"
#include "DormantNodeIO.h"
#include "LiveNodeIO.h"
#include "MediaFormatIO.h"
#include "MessageIO.h"
#include "NodeSetIOContext.h"
#include "StringContent.h"
#include "MediaString.h"

#include <Autolock.h>
#include <Debug.h>
#include <Entry.h>
#include <Path.h>

#include <TimeSource.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <typeinfo>

#include "set_tools.h"

using namespace std;

__USE_CORTEX_NAMESPACE

#define D_METHOD(x) //PRINT (x)
#define D_HOOK(x) //PRINT (x)
#define D_SETTINGS(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

RouteAppNodeManager::~RouteAppNodeManager() {

	_freeIcons();
}

RouteAppNodeManager::RouteAppNodeManager(
	bool													useAddOnHost) :
	NodeManager(useAddOnHost),
	m_nextGroupNumber(1) {
	
	// pre-cache icons? +++++
	
}

// -------------------------------------------------------- //
// *** group management
// -------------------------------------------------------- //

// -------------------------------------------------------- //
// *** icon management
// -------------------------------------------------------- //

// fetch cached icon for the given live node; the MediaIcon
// instance is guaranteed to last as long as this object.
// Returns 0 if the node doesn't exist.

const MediaIcon* RouteAppNodeManager::mediaIconFor(
	media_node_id									nodeID,
	icon_size											iconSize) {
	
	BAutolock _l(this);

	uint64 key = _makeIconKey(nodeID, iconSize);

	icon_map::const_iterator it = m_iconMap.find(key);
	if(it != m_iconMap.end()) {
		// already cached
		return (*it).second;
	}

	// look up live_node_info
	NodeRef* ref;
	status_t err = getNodeRef(nodeID, &ref);
	if(err < B_OK)
		return 0;	

	return mediaIconFor(ref->nodeInfo(), iconSize);	
}

const MediaIcon* RouteAppNodeManager::mediaIconFor(
	live_node_info								nodeInfo,
	icon_size											iconSize) {
	
	uint64 key = _makeIconKey(nodeInfo.node.node, iconSize);

	icon_map::const_iterator it = m_iconMap.find(key);
	if(it != m_iconMap.end()) {
		// already cached
		return (*it).second;
	}

	// create & cache icon
	MediaIcon* icon = new MediaIcon(
		nodeInfo, iconSize);

	m_iconMap.insert(
		icon_map::value_type(key, icon));

	return icon;
}

// -------------------------------------------------------- //
// *** error handling
// -------------------------------------------------------- //

status_t RouteAppNodeManager::setLogTarget(
	const BMessenger&							target) {
	
	BAutolock _l(this);
	
	if(!target.IsValid())
		return B_BAD_VALUE;
	
	m_logTarget = target;
	return B_OK;
}

// -------------------------------------------------------- //
// NodeManager hook implementations
// -------------------------------------------------------- //

void RouteAppNodeManager::nodeCreated(
	NodeRef*											ref) {

	// prepare the log message
	BMessage logMsg(M_LOG);
	BString title = "Node '";
	title << ref->name() << "' created";
	logMsg.AddString("title", title);

	// create a default group for the node
	// [em 8feb00]
	NodeGroup* g = createGroup(ref->name());

	if(ref->kind() & B_TIME_SOURCE) {
		// notify observers
		BMessage m(M_TIME_SOURCE_CREATED);
		m.AddInt32("nodeID", ref->id());
		notify(&m);
	}

	// adopt node's time source if it's not the system clock (the default)
	// [em 20mar00]
	media_node systemClock;
	status_t err = roster->GetSystemTimeSource(&systemClock);
	if(err == B_OK)
	{
		BTimeSource* ts = roster->MakeTimeSourceFor(ref->node());
		if(ts->Node() != systemClock)
		{
			g->setTimeSource(ts->Node());
			logMsg.AddString("line", "Synced to system clock");
		}
		ts->Release();
	}

	g->addNode(ref);

	m_logTarget.SendMessage(&logMsg);
}

void RouteAppNodeManager::nodeDeleted(
	const NodeRef*								ref) {

	// prepare the log message
	BMessage logMsg(M_LOG);
	BString title = "Node '";
	title << ref->name() << "' released";
	logMsg.AddString("title", title);

	if(ref->kind() & B_TIME_SOURCE) {
		// notify observers
		BMessage m(M_TIME_SOURCE_DELETED);
		m.AddInt32("nodeID", ref->id());
		notify(&m);
	}

	m_logTarget.SendMessage(&logMsg);
}

void RouteAppNodeManager::connectionMade(
	Connection*										connection) {

	D_HOOK((
		"@ RouteAppNodeManager::connectionMade()\n"));
		
	status_t err;

	// prepare the log message
	BMessage logMsg(M_LOG);
	BString title = "Connection ";
	if (strcmp(connection->outputName(), connection->inputName()) == 0) {
		title << "'" << connection->outputName() << "' ";
	}
	title << "made";
	logMsg.AddString("title", title);

	if(!(connection->flags() & Connection::INTERNAL))
		// don't react to connection Cortex didn't make
		return;
	
	// create or merge groups
	NodeRef *producer, *consumer;
	err = getNodeRef(connection->sourceNode(), &producer);
	if(err < B_OK) {
		D_HOOK((
			"!!! RouteAppNodeManager::connectionMade():\n"
			"    sourceNode (%ld) not found\n",
			connection->sourceNode()));
		return;	
	}
	err = getNodeRef(connection->destinationNode(), &consumer);
	if(err < B_OK) {
		D_HOOK((
			"!!! RouteAppNodeManager::connectionMade():\n"
			"    destinationNode (%ld) not found\n",
			connection->destinationNode()));
		return;	
	}

	// add node names to log messages
	BString line = "Between:";
	logMsg.AddString("line", line);
	line = "    ";
	line << producer->name() << " and ";
	line << consumer->name();
	logMsg.AddString("line", line);

	// add format to log message
	line = "Negotiated format:";
	logMsg.AddString("line", line);
	line = "    ";
	line << MediaString::getStringFor(connection->format(), false);
	logMsg.AddString("line", line);

	NodeGroup *group = 0;
	BString groupName = "Untitled group ";
	if(_canGroup(producer) && _canGroup(consumer))
	{
		if (producer->group() && consumer->group() &&
			!(producer->group()->groupFlags() & NodeGroup::GROUP_LOCKED) &&
			!(consumer->group()->groupFlags() & NodeGroup::GROUP_LOCKED))
		{
			// merge into consumers group
			group = consumer->group();
			mergeGroups(producer->group(), group);
		}
		else if (producer->group() &&
			!(producer->group()->groupFlags() & NodeGroup::GROUP_LOCKED))
		{ // add consumer to producers group
			group = producer->group();
			group->addNode(consumer);
		}
		else if (consumer->group() &&
			!(consumer->group()->groupFlags() & NodeGroup::GROUP_LOCKED))
		{ // add producer to consumers group
			group = consumer->group();
			group->addNode(producer);
		}
		else
		{ // make new group for both
			groupName << m_nextGroupNumber++;
			group = createGroup(groupName.String());
			group->addNode(producer);
			group->addNode(consumer);
		}
	}
	else if(_canGroup(producer) && !producer->group())
	{ // make new group for producer
		groupName << m_nextGroupNumber++;
		group = createGroup(groupName.String());
		group->addNode(producer);
	}
	else if(_canGroup(consumer) && !consumer->group())
	{ // make new group for consumer
		groupName << m_nextGroupNumber++;
		group = createGroup(groupName.String());
		group->addNode(consumer);			
	}

	m_logTarget.SendMessage(&logMsg);
}

void RouteAppNodeManager::connectionBroken(
	const Connection*									connection) {

	D_HOOK((
		"@ RouteAppNodeManager::connectionBroken()\n"));
		
	// prepare the log message
	BMessage logMsg(M_LOG);
	BString title = "Connection ";
	if (strcmp(connection->outputName(), connection->inputName()) == 0) {
		title << "'" << connection->outputName() << "' ";
	}
	title << "broken";
	logMsg.AddString("title", title);

	if(!(connection->flags() & Connection::INTERNAL))
		// don't react to connection Cortex didn't make
		return;

	status_t err;
	
	// if the source and destination nodes belong to the same group,
	// and if no direct or indirect connection remains between the
	// source and destination nodes, split groups

	NodeRef *producer, *consumer;
	err = getNodeRef(connection->sourceNode(), &producer);
	if(err < B_OK) {
		D_HOOK((
			"!!! RouteAppNodeManager::connectionMade():\n"
			"    sourceNode (%ld) not found\n",
			connection->sourceNode()));
		return;	
	}
	err = getNodeRef(connection->destinationNode(), &consumer);
	if(err < B_OK) {
		D_HOOK((
			"!!! RouteAppNodeManager::connectionMade():\n"
			"    destinationNode (%ld) not found\n",
			connection->destinationNode()));
		return;	
	}

	// add node names to log messages
	BString line = "Between:";
	logMsg.AddString("line", line);
	line = "    ";
	line << producer->name() << " and ";
	line << consumer->name();
	logMsg.AddString("line", line);
	
	if(
		producer->group() &&
		producer->group() == consumer->group() &&
		!findRoute(producer->id(), consumer->id())) {

		NodeGroup *newGroup;
		splitGroup(producer, consumer, &newGroup);
	}

	m_logTarget.SendMessage(&logMsg);
}

void RouteAppNodeManager::connectionFailed(
	const media_output &							output,
	const media_input &								input,
	const media_format &							format,
	status_t										error) {
	D_HOOK((
		"@ RouteAppNodeManager::connectionFailed()\n"));

	status_t err;
		
	// prepare the log message
	BMessage logMsg(M_LOG);
	BString title = "Connection failed";
	logMsg.AddString("title", title);
	logMsg.AddInt32("error", error);

	NodeRef *producer, *consumer;
	err = getNodeRef(output.node.node, &producer);
	if(err < B_OK) {
		return;	
	}
	err = getNodeRef(input.node.node, &consumer);
	if(err < B_OK) {
		return;	
	}

	// add node names to log messages
	BString line = "Between:";
	logMsg.AddString("line", line);
	line = "    ";
	line << producer->name() << " and " << consumer->name();
	logMsg.AddString("line", line);

	// add format to log message
	line = "Tried format:";
	logMsg.AddString("line", line);
	line = "    ";
	line << MediaString::getStringFor(format, true);
	logMsg.AddString("line", line);

	// and send it
	m_logTarget.SendMessage(&logMsg);
}

// -------------------------------------------------------- //
// *** IPersistent
// -------------------------------------------------------- //

void RouteAppNodeManager::xmlExportBegin(
	ExportContext&								context) const {

	status_t err;
	
	try {
		NodeSetIOContext& set = dynamic_cast<NodeSetIOContext&>(context);
		context.beginElement(_NODE_SET_ELEMENT);
		
		// validate the node set
		for(int n = set.countNodes()-1; n >= 0; --n) {
			media_node_id id = set.nodeAt(n);
			ASSERT(id != media_node::null.node);
			
			// fetch node
			NodeRef* ref;
			err = getNodeRef(id, &ref);
			if(err < B_OK) {
				D_SETTINGS((
					"! RVNM::xmlExportBegin(): node %ld doesn't exist\n", id));

				set.removeNodeAt(n);
				continue;
			}
			// skip unless internal
			if(!ref->isInternal()) {
				D_SETTINGS((
					"! RVNM::xmlExportBegin(): node %ld not internal; skipping.\n", id));

				set.removeNodeAt(n);
				continue;
			}
		}
	}
	catch(bad_cast& e) {
		context.reportError("RouteAppNodeManager: expected a NodeSetIOContext\n");
	}
}
	
void RouteAppNodeManager::xmlExportAttributes(
	ExportContext&								context) const {}

void RouteAppNodeManager::xmlExportContent(
	ExportContext&								context) const {

	status_t err;

	try {
		NodeSetIOContext& nodeSet = dynamic_cast<NodeSetIOContext&>(context);
		context.beginContent();
	
		// write nodes; enumerate connections
		typedef map<uint32,Connection> connection_map;
		connection_map connections;
		int count = nodeSet.countNodes();
		for(int n = 0; n < count; ++n) {
			media_node_id id = nodeSet.nodeAt(n);
			ASSERT(id != media_node::null.node);
			
			// fetch node
			NodeRef* ref;
			err = getNodeRef(id, &ref);
			if(err < B_OK) {
				D_SETTINGS((
					"! RouteAppNodeManager::xmlExportContent():\n"
					"  getNodeRef(%ld) failed: '%s'\n",
					id, strerror(err)));
				continue;
			}

			// fetch connections
			vector<Connection> conSet;
			ref->getInputConnections(conSet);
			ref->getOutputConnections(conSet);
			for(uint32 c = 0; c < conSet.size(); ++c)
				// non-unique connections will be rejected:
				connections.insert(
					connection_map::value_type(conSet[c].id(), conSet[c]));

			// create an IO object for the node & write it
			DormantNodeIO io(ref, nodeSet.keyAt(n));
			if(context.writeObject(&io) < B_OK)
				// abort
				return;
		}		

		// write connections
		for(connection_map::const_iterator it = connections.begin();
			it != connections.end(); ++it) {
		
			ConnectionIO io(
				&(*it).second, 
				this,
				&nodeSet);
			if(context.writeObject(&io) < B_OK)
				// abort
				return;
				
		}
	
		// +++++ write groups
	
		// write UI state
		{
			BMessage m;
			nodeSet.exportUIState(&m);
			context.beginElement(_UI_STATE_ELEMENT);
			context.beginContent();
			MessageIO io(&m);
			context.writeObject(&io);
			context.endElement();
		}
	}
	catch(bad_cast& e) {
		context.reportError("RouteAppNodeManager: expected a NodeSetIOContext\n");
	}	
}

void RouteAppNodeManager::xmlExportEnd(
	ExportContext&								context) const {

	context.endElement();
}

// -------------------------------------------------------- //
// IMPORT	
// -------------------------------------------------------- //

void RouteAppNodeManager::xmlImportBegin(
	ImportContext&								context) {

}

void RouteAppNodeManager::xmlImportAttribute(
	const char*										key,
	const char*										value,
	ImportContext&								context) {}
		
void RouteAppNodeManager::xmlImportContent(
	const char*										data,
	uint32												length,
	ImportContext&								context) {}

void RouteAppNodeManager::xmlImportChild(
	IPersistent*									child,
	ImportContext&								context) {

	status_t err;
	
	if(!strcmp(context.element(), _DORMANT_NODE_ELEMENT)) {
		DormantNodeIO* io = dynamic_cast<DormantNodeIO*>(child);
		ASSERT(io);
		
		NodeRef* newRef;
		err = io->instantiate(this, &newRef);
		if(err == B_OK) {
			// created node; add an entry to the set stored in the
			// ImportContext for later reference
			try {
				NodeSetIOContext& set = dynamic_cast<NodeSetIOContext&>(context);
				set.addNode(newRef->id(), io->nodeKey());
			}
			catch(bad_cast& e) {
				context.reportError("RouteAppNodeManager: expected a NodeSetIOContext\n");
			}
		}
		else {
			D_SETTINGS((
				"!!! RouteAppNodeManager::xmlImportChild():\n"
				"    DormantNodeIO::instantiate() failed:\n"
				"    '%s'\n",
				strerror(err)));
		}
	}
	else if(!strcmp(context.element(), _CONNECTION_ELEMENT)) {
		ConnectionIO* io = dynamic_cast<ConnectionIO*>(child);
		ASSERT(io);
		
		// instantiate the connection
		Connection con;
		err = io->instantiate(
			this,
			dynamic_cast<NodeSetIOContext*>(&context),
			&con);
		if(err < B_OK) {
			D_SETTINGS((
				"!!! ConnectionIO::instantiate() failed:\n"
				"    '%s'\n", strerror(err)));
		}
		
		// +++++ group magic?

	}
	else if(!strcmp(context.element(), _NODE_GROUP_ELEMENT)) {
		// +++++
	}
	else if(
		context.parentElement() &&
		!strcmp(context.parentElement(), _UI_STATE_ELEMENT)) {
	
		// expect a nested message
		MessageIO* io = dynamic_cast<MessageIO*>(child);
		if(!io) {
			BString err;
			err <<
				"RouteAppNodeManager: unexpected child '" <<
				context.element() << "'\n";
			context.reportError(err.String());
		}
		
		// hand it off via the extended context object
		try {
			NodeSetIOContext& set = dynamic_cast<NodeSetIOContext&>(context);
			set.importUIState(io->message());
		}
		catch(bad_cast& e) {
			context.reportError("RouteAppNodeManager: expected a NodeSetIOContext\n");
		}
	}
	
}
	
void RouteAppNodeManager::xmlImportComplete(
	ImportContext&								context) {

}

// -------------------------------------------------------- //

/*static*/
void RouteAppNodeManager::AddTo(
	XML::DocumentType*				docType) {

	// set up the document type

	MessageIO::AddTo(docType);
	MediaFormatIO::AddTo(docType);
	ConnectionIO::AddTo(docType);
	DormantNodeIO::AddTo(docType);
	LiveNodeIO::AddTo(docType);
	_add_string_elements(docType);
}

// -------------------------------------------------------- //
// implementation
// -------------------------------------------------------- //

uint64 RouteAppNodeManager::_makeIconKey(
	media_node_id nodeID, icon_size iconSize) {

	return ((uint64)nodeID) << 32 | iconSize;
}

void RouteAppNodeManager::_readIconKey(
	uint64 key, media_node_id& nodeID, icon_size& iconSize) {
	
	nodeID = key >> 32;
	iconSize = icon_size(key & 0xffffffff);
}

void RouteAppNodeManager::_freeIcons() {

	ptr_map_delete(
		m_iconMap.begin(),
		m_iconMap.end());
}

bool RouteAppNodeManager::_canGroup(NodeRef* ref) const {

	// sanity check & easy cases
	ASSERT(ref);
	if(ref->isInternal())
		return true;

	// bar 'touchy' system nodes
	if(ref == audioMixerNode() || ref == audioOutputNode())
		return false;
		
	return true;
}

// END -- RouteAppNodeManager.cpp --
