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


// RouteAppNodeManager.h
// * PURPOSE
//   Extends NodeManager to provide services to a graphical
//   routing interface:
//   - automatic group management (groups are formed/merged
//     as nodes are connected, and split when nodes are
//     disconnected)
//   - error logging via BMessage +++++ nyi
//   - icon management: generation and caching of MediaIcons
//
//   - persistence support: drives export and import of user-created
//     nodes, groups, and connections to & from XML streams.
//
//   EXPORT PROCESS
//
//     1) Assign each node to be saved an ID string.  Reject nodes
//        that the user didn't create.
//     2) Export each node.
//     3) Export each connection.
//     4) Export each group.
//     5) Export UI state data via export-context hook
//
//   IMPORT PROCESS
//
//     1) Import each node's description and [try to] instantiate the
//        node.  Build a map of ID -> media_node_id.
//        (?) how much failure tolerance is too much?
//     2) Import and attempt to recreate each connection.  Note: once
//        notification of the connection comes, the default behavior
//        will be to automatically group the node; this needs to be
//        bypassed!
//     3) Import each group.
//     4) Import UI state data via import-context hook
//
// * HISTORY
//	 c.lenz		28may00		Begun notification/error logging support
//   e.moon		7dec99		Persistence support
//   e.moon		7nov99		Begun

#ifndef __RouteAppNodeManager_H__
#define __RouteAppNodeManager_H__

#include <Mime.h> // defines icon_size -- weird.

#include <map>
#include <set>

#include "NodeManager.h"
#include "XML.h"
#include "ImportContext.h"
#include "ExportContext.h"

#include "NodeKey.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class MediaIcon;

class RouteAppNodeManager :
	public	NodeManager,
	public	IPersistent {
	
public:														// *** constants
	enum message_t {
		// outbound: sent to the designated log-message target
		// 'text' (string) +++++ not yet implemented
		M_LOG													= RouteAppNodeManager_message_base,
		
		// outbound: sent to observers when a time source is added/removed
		// 'nodeID' (int32)
		M_TIME_SOURCE_CREATED,
		M_TIME_SOURCE_DELETED
	};

//	static const char* const				s_rootElement;
//	static const char* const				s_uiStateElement;

public:														// *** ctor/dtor
	virtual ~RouteAppNodeManager();
	RouteAppNodeManager(
		bool													useAddOnHost=false);

public:														// *** group management

public:														// *** icon management

	// fetch cached icon for the given live node; the MediaIcon
	// instance is guaranteed to last as long as this object.
	// Returns 0 if the node doesn't exist.

	const MediaIcon* mediaIconFor(
		media_node_id									nodeID,
		icon_size											iconSize);

	const MediaIcon* mediaIconFor(
		live_node_info								nodeInfo,
		icon_size											iconSize);

public:														// *** notification & error handling

	status_t setNotifyTarget(
		const BMessenger&							target);

	status_t setLogTarget(
		const BMessenger&							target);
		
public:														// NodeManager hook implementations

	virtual void nodeCreated(
		NodeRef*									ref);
	
	virtual void nodeDeleted(
		const NodeRef*								ref);

	virtual void connectionMade(
		Connection*									connection);

	virtual void connectionBroken(
		const Connection*							connection);

	virtual void connectionFailed(
		const media_output &						output,
		const media_input &							input,
		const media_format &						format,
		status_t									error);

public:														// *** IPersistent

	// EXPORT
	
	virtual void xmlExportBegin(
		ExportContext&								context) const;
		
	virtual void xmlExportAttributes(
		ExportContext&								context) const;

	virtual void xmlExportContent(
		ExportContext&								context) const; //nyi
	
	virtual void xmlExportEnd(
		ExportContext&								context) const;

	// IMPORT	
	
	virtual void xmlImportBegin(
		ImportContext&								context); //nyi
	
	virtual void xmlImportAttribute(
		const char*										key,
		const char*										value,
		ImportContext&								context); //nyi
			
	virtual void xmlImportContent(
		const char*										data,
		uint32												length,
		ImportContext&								context); //nyi
	
	virtual void xmlImportChild(
		IPersistent*									child,
		ImportContext&								context); //nyi
		
	virtual void xmlImportComplete(
		ImportContext&								context); //nyi

public:												// *** static setup method

	// call this method to install element hooks in the
	// given document type
	static void AddTo(
		XML::DocumentType*				docType);

private:													// implementation

	// current new-group-name index
	uint32													m_nextGroupNumber;

	// app message handler: group selection, etc
	BMessenger											m_notifyTarget;

	// log-message handler
	BMessenger											m_logTarget;

	// cached MediaIcon instances
	// KEY:
	// high 32 bits: media_node_id
	// low 32 bits: icon_size
	typedef std::map<uint64, MediaIcon*> icon_map;
	icon_map												m_iconMap;

//	class import_state*								m_importState;

private:	

	uint64 _makeIconKey(media_node_id, icon_size);
	void _readIconKey(uint64, media_node_id&, icon_size&);
	void _freeIcons();

	bool _canGroup(NodeRef* ref) const;
	
//	void _exportNode(NodeRef* ref, const char* key, ExportContext& context) const;
//	void _exportConnection(Connection* connection, ExportContext& context) const; //nyi
//	void _exportGroup(NodeGroup* group, ExportContext& context) const; //nyi
//	
//	void _importDormantNode(
//		class _dormant_node_import_state* state,
//		ImportContext& context); //nyi
};

__END_CORTEX_NAMESPACE
#endif /*__RouteAppNodeManager_H__*/
