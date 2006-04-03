/*
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <OS.h>
#include <Entry.h>
#include <Message.h>
#include <Locker.h>
#include <Autolock.h>
#include <Path.h>
#include <Messenger.h>
#include <MediaDefs.h>
#include <MediaAddOn.h>
#include "debug.h"
#include "NodeManager.h"
#include "DefaultManager.h"
#include "AppManager.h"
#include "MediaMisc.h"

extern AppManager *gAppManager;

const char *get_node_type(node_type t);

NodeManager::NodeManager() :
	fNextAddOnID(1),
	fNextNodeID(1),
	fLocker(new BLocker("node manager locker")),
	fDormantAddonFlavorList(new List<dormant_addon_flavor_info>),
	fAddonPathMap(new Map<media_addon_id, entry_ref>),
	fRegisteredNodeMap(new Map<media_node_id, registered_node>),
	fDefaultManager(new DefaultManager)
{
}


NodeManager::~NodeManager()
{
	delete fLocker;
	delete fDormantAddonFlavorList;
	delete fAddonPathMap;
	delete fRegisteredNodeMap;
	delete fDefaultManager;
}

/**********************************************************************
 * Live node management
 **********************************************************************/

status_t
NodeManager::RegisterNode(media_node_id *nodeid, media_addon_id addon_id, int32 addon_flavor_id, const char *name, uint64 kinds, port_id port, team_id team)
{
	BAutolock lock(fLocker);
	bool b;
	registered_node rn;
	rn.nodeid = fNextNodeID;
	rn.addon_id = addon_id;
	rn.addon_flavor_id = addon_flavor_id;
	strcpy(rn.name, name);
	rn.kinds = kinds;
	rn.port = port;
	rn.team = team;
	rn.creator = -1; // will be set later
	rn.globalrefcount = 1;
	rn.teamrefcount.Insert(team, 1);
	
	b = fRegisteredNodeMap->Insert(fNextNodeID, rn);
	ASSERT(b);
	*nodeid = fNextNodeID;
	fNextNodeID += 1;
	TRACE("NodeManager::RegisterNode: node %ld, addon_id %ld, flavor_id %ld, name \"%s\", kinds %#Lx, port %ld, team %ld\n", *nodeid, addon_id, addon_flavor_id, name, kinds, port, team);
	return B_OK;
}


status_t
NodeManager::UnregisterNode(media_addon_id *addonid, int32 *flavorid, media_node_id nodeid, team_id team)
{
	BAutolock lock(fLocker);
	bool b;
	registered_node *rn;
	TRACE("NodeManager::UnregisterNode enter: node %ld, team %ld\n", nodeid, team);
	b = fRegisteredNodeMap->Get(nodeid, &rn);
	if (!b) {
		ERROR("NodeManager::UnregisterNode: couldn't find node %ld (team %ld)\n", nodeid, team);
		return B_ERROR;
	}
	if (rn->team != team) {
		ERROR("NodeManager::UnregisterNode: team %ld tried to unregister node %ld, but it was instantiated by team %ld\n", team, nodeid, rn->team);
		return B_ERROR;
	}
	if (rn->globalrefcount != 1) {
		ERROR("NodeManager::UnregisterNode: node %ld, team %ld has globalrefcount %ld (should be 1)\n", nodeid, team, rn->globalrefcount);
		//return B_ERROR;
	}
	*addonid = rn->addon_id;
	*flavorid = rn->addon_flavor_id;
	b = fRegisteredNodeMap->Remove(nodeid);
	ASSERT(b);
	TRACE("NodeManager::UnregisterNode leave: node %ld, addon_id %ld, flavor_id %ld team %ld\n", nodeid, *addonid, *flavorid, team);
	return B_OK;
}


status_t
NodeManager::IncrementGlobalRefCount(media_node_id nodeid, team_id team)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	bool b;
	TRACE("NodeManager::IncrementGlobalRefCount enter: node %ld, team %ld\n", nodeid, team);
	b = fRegisteredNodeMap->Get(nodeid, &rn);
	if (!b) {
		ERROR("NodeManager::IncrementGlobalRefCount: node %ld not found\n", nodeid);
		return B_ERROR;
	}
	int32 *count;
	int32 debug_count;
	b = rn->teamrefcount.Get(team, &count);
	if (b) {
		*count += 1;
		debug_count = *count;
	} else {
		b = rn->teamrefcount.Insert(team, 1);
		ASSERT(b);
		debug_count = 1;
	}
	rn->globalrefcount += 1;
	TRACE("NodeManager::IncrementGlobalRefCount leave: node %ld, team %ld, count %ld, globalcount %ld\n", nodeid, team, debug_count, rn->globalrefcount);
	return B_OK;
}


status_t
NodeManager::DecrementGlobalRefCount(media_node_id nodeid, team_id team)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	bool b;
	TRACE("NodeManager::DecrementGlobalRefCount enter: node %ld, team %ld\n", nodeid, team);
	b = fRegisteredNodeMap->Get(nodeid, &rn);
	if (!b) {
		ERROR("NodeManager::DecrementGlobalRefCount: node %ld not found\n", nodeid);
		return B_ERROR;
	}
	int32 *count;
	b = rn->teamrefcount.Get(team, &count);
	if (!b) {
		// Normally it is an error to release a node in another team. But we make one
		// exception. If the node is global, and the creator team tries to release it,
		// we will release it in the the media_addon_server.
		team_id addon_server_team;
		addon_server_team = gAppManager->AddonServerTeam();
		if (rn->creator == team && rn->teamrefcount.Get(addon_server_team, &count)) {
			printf("!!! NodeManager::DecrementGlobalRefCount doing global release!\n");
			rn->creator = -1; //invalidate!
			team = addon_server_team; //redirect!
			// the count variable was already redirected in if() statement above.
		} else {
			ERROR("NodeManager::DecrementGlobalRefCount: node %ld has no team %ld references\n", nodeid, team);
			return B_ERROR;
		}
	}
	*count -= 1;
	#if DEBUG >= 2
		int32 debug_count = *count;
	#endif
	if (*count == 0) {
		b = rn->teamrefcount.Remove(team);
		ASSERT(b);
	}
	rn->globalrefcount -= 1;
	
	if (rn->globalrefcount == 0) {
		printf("NodeManager::DecrementGlobalRefCount: detected released node is now unused, node %ld\n", nodeid);
		FinalReleaseNode(nodeid);
	}
	
	TRACE("NodeManager::DecrementGlobalRefCount leave: node %ld, team %ld, count %ld, globalcount %ld\n", nodeid, team, debug_count, rn->globalrefcount);
	return B_OK;
}

status_t
NodeManager::SetNodeCreator(media_node_id nodeid, team_id creator)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	bool b;

	TRACE("NodeManager::SetNodeCreator node %ld, creator %ld\n", nodeid, creator);

	b = fRegisteredNodeMap->Get(nodeid, &rn);
	if (!b) {
		ERROR("NodeManager::SetNodeCreator: node %ld not found\n", nodeid);
		return B_ERROR;
	}
	
	if (rn->creator != -1) {
		ERROR("NodeManager::SetNodeCreator: node %ld is already assigned creator %ld\n", nodeid, rn->creator);
		return B_ERROR;
	}
	
	rn->creator = creator;
	return B_OK;
}

void
NodeManager::FinalReleaseNode(media_node_id nodeid)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	bool b;
	status_t rv;

	TRACE("NodeManager::FinalReleaseNode enter: node %ld\n", nodeid);
	b = fRegisteredNodeMap->Get(nodeid, &rn);
	if (!b) {
		ERROR("NodeManager::FinalReleaseNode: node %ld not found\n", nodeid);
		return;
	}

	node_final_release_command cmd;
	rv = SendToPort(rn->port, NODE_FINAL_RELEASE, &cmd, sizeof(cmd));
	if (rv != B_OK) {
		ERROR("NodeManager::FinalReleaseNode: can't send command to node %ld\n", nodeid);
		return;
	}
}


status_t
NodeManager::GetCloneForId(media_node *node, media_node_id nodeid, team_id team)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	bool b;
	TRACE("NodeManager::GetCloneForId enter: node %ld team %ld\n", nodeid, team);

	if (B_OK != IncrementGlobalRefCount(nodeid, team)) {
		ERROR("NodeManager::GetCloneForId: couldn't increment ref count, node %ld team %ld\n", nodeid, team);
		return B_ERROR;
	}

	b = fRegisteredNodeMap->Get(nodeid, &rn);
	if (!b) {
		ERROR("NodeManager::GetCloneForId: node %ld not found\n", nodeid);
		DecrementGlobalRefCount(nodeid, team);
		return B_ERROR;
	}

	node->node = rn->nodeid;
	node->port = rn->port;
	node->kind = rn->kinds;

	TRACE("NodeManager::GetCloneForId leave: node %ld team %ld\n", nodeid, team);
	return B_OK;
}


/* This function locates the default "node" for the requested "type" and returnes a clone.
 * If the requested type is AUDIO_OUTPUT_EX, also "input_name" and "input_id" need to be set and returned,
 * as this is required by BMediaRoster::GetAudioOutput(media_node *out_node, int32 *out_input_id, BString *out_input_name)
 */
status_t
NodeManager::GetClone(media_node *node, char *input_name, int32 *input_id, node_type type, team_id team)
{
	BAutolock lock(fLocker);
	status_t status;
	media_node_id id;

	TRACE("NodeManager::GetClone enter: team %ld, type %d (%s)\n", team, type, get_node_type(type));
	
	status = GetDefaultNode(&id, input_name, input_id, type);
	if (status != B_OK) {
		ERROR("NodeManager::GetClone: couldn't GetDefaultNode, team %ld, type %d (%s)\n", team, type, get_node_type(type));
		*node = media_node::null;
		return status;
	}
	ASSERT(id > 0);

	status = GetCloneForId(node, id, team);
	if (status != B_OK) {
		ERROR("NodeManager::GetClone: couldn't GetCloneForId, id %ld, team %ld, type %d (%s)\n", id, team, type, get_node_type(type));
		*node = media_node::null;
		return status;
	}
	ASSERT(id == node->node);

	TRACE("NodeManager::GetClone leave: node id %ld, node port %ld, node kind %#lx\n", node->node, node->port, node->kind);

	return B_OK;
}


status_t
NodeManager::ReleaseNode(const media_node &node, team_id team)
{
	BAutolock lock(fLocker);
	TRACE("NodeManager::ReleaseNode enter: node %ld team %ld\n", node.node, team);
	if (B_OK != DecrementGlobalRefCount(node.node, team)) {
		ERROR("NodeManager::ReleaseNode: couldn't decrement node %ld team %ld ref count\n", node.node, team);
	}
	TRACE("NodeManager::ReleaseNode leave: node %ld team %ld\n", node.node, team);
	return B_OK;
}


status_t
NodeManager::PublishInputs(const media_node &node, const media_input *inputs, int32 count)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	bool b;
	b = fRegisteredNodeMap->Get(node.node, &rn);
	if (!b) {
		ERROR("NodeManager::PublishInputs: node %ld not found\n", node.node);
		return B_ERROR;
	}
	rn->inputlist.MakeEmpty();
	for (int32 i = 0; i < count; i++)
		rn->inputlist.Insert(inputs[i]);
	return B_OK;
}


status_t
NodeManager::PublishOutputs(const media_node &node, const media_output *outputs, int32 count)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	bool b;
	b = fRegisteredNodeMap->Get(node.node, &rn);
	if (!b) {
		ERROR("NodeManager::PublishOutputs: node %ld not found\n", node.node);
		return B_ERROR;
	}
	rn->outputlist.MakeEmpty();
	for (int32 i = 0; i < count; i++)
		rn->outputlist.Insert(outputs[i]);
	return B_OK;
}


status_t
NodeManager::FindNodeId(media_node_id *nodeid, port_id port)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	for (fRegisteredNodeMap->Rewind(); fRegisteredNodeMap->GetNext(&rn); ) {
		if (rn->port == port) {
			*nodeid = rn->nodeid;
			TRACE("NodeManager::FindNodeId found port %ld, node %ld\n", port, *nodeid);
			return B_OK;
		}
		media_output *output;
		for (rn->outputlist.Rewind(); rn->outputlist.GetNext(&output); ) {
			if (output->source.port == port) {
				*nodeid = rn->nodeid;
				TRACE("NodeManager::FindNodeId found output port %ld, node %ld\n", port, *nodeid);
				return B_OK;
			}
		}
		media_input *input;
		for (rn->inputlist.Rewind(); rn->inputlist.GetNext(&input); ) {
			if (input->destination.port == port) {
				*nodeid = rn->nodeid;
				TRACE("NodeManager::FindNodeId found input port %ld, node %ld\n", port, *nodeid);
				return B_OK;
			}
		}
	}
	ERROR("NodeManager::FindNodeId failed, port %ld\n", port);
	return B_ERROR;
}

status_t
NodeManager::GetDormantNodeInfo(dormant_node_info *node_info, const media_node &node)
{
	BAutolock lock(fLocker);
	// XXX not sure if this is correct
	registered_node *rn;
	for (fRegisteredNodeMap->Rewind(); fRegisteredNodeMap->GetNext(&rn); ) {
		if (rn->nodeid == node.node) {
			if (rn->addon_id == -1 && node.node != NODE_SYSTEM_TIMESOURCE_ID) { // This function must return an error if the node is application owned
				TRACE("NodeManager::GetDormantNodeInfo NODE IS APPLICATION OWNED! node %ld, addon_id %ld, addon_flavor_id %ld, name \"%s\"\n", node.node, rn->addon_id, rn->addon_flavor_id, rn->name);
				return B_ERROR;
			}
			ASSERT(node.port == rn->port);
			ASSERT((node.kind & NODE_KIND_COMPARE_MASK) == (rn->kinds & NODE_KIND_COMPARE_MASK));
			node_info->addon = rn->addon_id;
			node_info->flavor_id = rn->addon_flavor_id;
			strcpy(node_info->name, rn->name);
			TRACE("NodeManager::GetDormantNodeInfo node %ld, addon_id %ld, addon_flavor_id %ld, name \"%s\"\n", node.node, rn->addon_id, rn->addon_flavor_id, rn->name);
			return B_OK;
		}
	}
	ERROR("NodeManager::GetDormantNodeInfo failed, node %ld\n", node.node);
	return B_ERROR;
}

status_t
NodeManager::GetLiveNodeInfo(live_node_info *live_info, const media_node &node)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	for (fRegisteredNodeMap->Rewind(); fRegisteredNodeMap->GetNext(&rn); ) {
		if (rn->nodeid == node.node) {
			ASSERT(node.port == rn->port);
			ASSERT((node.kind & NODE_KIND_COMPARE_MASK) == (rn->kinds & NODE_KIND_COMPARE_MASK));
			live_info->node = node;
			live_info->hint_point = BPoint(0, 0);
			strcpy(live_info->name, rn->name);
			TRACE("NodeManager::GetLiveNodeInfo node %ld, name = \"%s\"\n", node.node, rn->name);
			return B_OK;
		}
	}
	ERROR("NodeManager::GetLiveNodeInfo failed, node %ld\n", node.node);
	return B_ERROR;
}


status_t
NodeManager::GetInstances(media_node_id *node_ids, int32* count, int32 maxcount, media_addon_id addon_id, int32 addon_flavor_id)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	*count = 0;
	for (fRegisteredNodeMap->Rewind(); (maxcount > 0) &&  fRegisteredNodeMap->GetNext(&rn); ) {
		if (rn->addon_id == addon_id && rn->addon_flavor_id == addon_flavor_id) {
			node_ids[*count] = rn->nodeid;
			*count += 1;
			maxcount -= 1;
		}
	}
	TRACE("NodeManager::GetInstances found %ld instances for addon_id %ld, addon_flavor_id %ld\n", *count, addon_id, addon_flavor_id);
	return B_OK;
}


status_t
NodeManager::GetLiveNodes(Stack<live_node_info> *livenodes,	int32 maxcount, const media_format *inputformat /* = NULL */, const media_format *outputformat /* = NULL */, const char* name /* = NULL */, uint64 require_kinds /* = 0 */)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	int namelen;
	
	TRACE("NodeManager::GetLiveNodes: maxcount %ld, in-format %p, out-format %p, name %s, require_kinds 0x%Lx\n",
		  maxcount, inputformat, outputformat, (name ? name : "NULL"), require_kinds);

	// determine the count of byte to compare when checking for a name with(out) wildcard
	if (name) {
		namelen = strlen(name);
		if (name[namelen] == '*')
			namelen--; // compares without the '*'
		else
			namelen++; // also compares the terminating NULL
	} else
		namelen = 0;

	for (fRegisteredNodeMap->Rewind(); (maxcount > 0) && fRegisteredNodeMap->GetNext(&rn); ) {
		if ((rn->kinds & require_kinds) != require_kinds)
			continue;
		if (namelen) {
			if (0 != memcmp(name, rn->name, namelen))
				continue;
		}
		if (inputformat) {
			bool hasit = false;
			media_input *input;
			for (rn->inputlist.Rewind(); rn->inputlist.GetNext(&input); ) {
				if (format_is_compatible(*inputformat, input->format)) {
					hasit = true;
					break;
				}
			}
			if (!hasit)
				continue;
		}
		if (outputformat) {
			bool hasit = false;
			media_output *output;
			for (rn->outputlist.Rewind(); rn->outputlist.GetNext(&output); ) {
				if (format_is_compatible(*outputformat, output->format)) {
					hasit = true;
					break;
				}
			}
			if (!hasit)
				continue;
		}

		live_node_info lni;
		lni.node.node = rn->nodeid;
		lni.node.port = rn->port;
		lni.node.kind = rn->kinds;
		lni.hint_point = BPoint(0, 0);
		strcpy(lni.name, rn->name);
		livenodes->Push(lni);
		maxcount -= 1;
	}

	TRACE("NodeManager::GetLiveNodes found %ld\n", livenodes->CountItems());
	return B_OK;
}

/* Add media_node_id of all live nodes to the message
 * int32 "media_node_id" (multiple items)
 */
status_t
NodeManager::GetLiveNodes(BMessage *msg)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	for (fRegisteredNodeMap->Rewind(); fRegisteredNodeMap->GetNext(&rn); ) {
		msg->AddInt32("media_node_id", rn->nodeid);
	}
	return B_OK;
}

/**********************************************************************
 * Registration of BMediaAddOns
 **********************************************************************/

void 
NodeManager::RegisterAddon(const entry_ref &ref, media_addon_id *newid)
{
	BAutolock lock(fLocker);
	media_addon_id id;
	id = fNextAddOnID;
	fNextAddOnID += 1;
	
	printf("NodeManager::RegisterAddon: ref-name \"%s\", assigning id %ld\n", ref.name, id);
	
	fAddonPathMap->Insert(id, ref);
	*newid = id;
}

void
NodeManager::UnregisterAddon(media_addon_id id)
{
	BAutolock lock(fLocker);

	printf("NodeManager::UnregisterAddon: id %ld\n", id);

	RemoveDormantFlavorInfo(id);
	fAddonPathMap->Remove(id);
}

status_t
NodeManager::GetAddonRef(entry_ref *ref, media_addon_id id)
{
	BAutolock lock(fLocker);
	entry_ref *tempref;

	if (fAddonPathMap->Get(id, &tempref)) {
		*ref = *tempref;
		return B_OK;
	}
	
	return B_ERROR;
}

/**********************************************************************
 * Registration of node flavors, published by BMediaAddOns
 **********************************************************************/

// this function is only called (indirectly) by the media_addon_server
void
NodeManager::AddDormantFlavorInfo(const dormant_flavor_info &dfi)
{
	BAutolock lock(fLocker);
	
	printf("NodeManager::AddDormantFlavorInfo, addon-id %ld, flavor-id %ld, name \"%s\", flavor-name \"%s\", flavor-info \"%s\"\n", dfi.node_info.addon, dfi.node_info.flavor_id, dfi.node_info.name, dfi.name, dfi.info);

	// Try to find the addon-id/flavor-id in the list.
	// If it already exists, update the Info, but don't
	// change the GlobalInstancesCount
	dormant_addon_flavor_info *dafi;
	for (fDormantAddonFlavorList->Rewind(); fDormantAddonFlavorList->GetNext(&dafi); ) {
		if (dafi->AddonID != dfi.node_info.addon || dafi->AddonFlavorID != dfi.node_info.flavor_id)
			continue;
		if (dafi->InfoValid) {
			ERROR("NodeManager::AddDormantFlavorInfo, addon-id %ld, flavor-id %ld does already exist\n", dafi->Info.node_info.addon, dafi->Info.node_info.flavor_id);
		}
		TRACE("NodeManager::AddDormantFlavorInfo, updating addon-id %ld, flavor-id %ld\n", dafi->Info.node_info.addon, dafi->Info.node_info.flavor_id);
		dafi->MaxInstancesCount = dfi.possible_count > 0 ? dfi.possible_count : 0x7fffffff;
		// do NOT modify dafi.GlobalInstancesCount
		dafi->InfoValid = true;
		dafi->Info = dfi;
		return;
	}

	// Insert information into the list
	{
		dormant_addon_flavor_info dafi;
		dafi.AddonID = dfi.node_info.addon;
		dafi.AddonFlavorID = dfi.node_info.flavor_id;
		dafi.MaxInstancesCount = dfi.possible_count > 0 ? dfi.possible_count : 0x7fffffff;
		dafi.GlobalInstancesCount = 0;
		dafi.InfoValid = true;
		dafi.Info = dfi;
		fDormantAddonFlavorList->Insert(dafi);
	}
}

// this function is only called (indirectly) by the media_addon_server
void
NodeManager::InvalidateDormantFlavorInfo(media_addon_id id)
{
	BAutolock lock(fLocker);
	dormant_addon_flavor_info *dafi;
	for (fDormantAddonFlavorList->Rewind(); fDormantAddonFlavorList->GetNext(&dafi); ) {
		if (dafi->AddonID == id && dafi->InfoValid == true) {
			printf("NodeManager::InvalidateDormantFlavorInfo, addon-id %ld, flavor-id %ld, name \"%s\", flavor-name \"%s\", flavor-info \"%s\"\n", dafi->Info.node_info.addon, dafi->Info.node_info.flavor_id, dafi->Info.node_info.name, dafi->Info.name, dafi->Info.info);
			dormant_flavor_info dfi_null;
			dafi->Info = dfi_null;
			dafi->InfoValid = false;
		}
	}
}

// this function is only called by clean up after gone add-ons
void
NodeManager::RemoveDormantFlavorInfo(media_addon_id id)
{
	BAutolock lock(fLocker);
	dormant_addon_flavor_info *dafi;
	for (fDormantAddonFlavorList->Rewind(); fDormantAddonFlavorList->GetNext(&dafi); ) {
		if (dafi->AddonID == id) {
			printf("NodeManager::RemoveDormantFlavorInfo, addon-id %ld, flavor-id %ld, name \"%s\", flavor-name \"%s\", flavor-info \"%s\"\n", dafi->Info.node_info.addon, dafi->Info.node_info.flavor_id, dafi->Info.node_info.name, dafi->Info.name, dafi->Info.info);
			fDormantAddonFlavorList->RemoveCurrent();
		}
	}
}

status_t
NodeManager::IncrementAddonFlavorInstancesCount(media_addon_id addonid, int32 flavorid, team_id team)
{
	BAutolock lock(fLocker);

	dormant_addon_flavor_info *dafi;
	for (fDormantAddonFlavorList->Rewind(); fDormantAddonFlavorList->GetNext(&dafi); ) {
		if (dafi->AddonID != addonid || dafi->AddonFlavorID != flavorid)
			continue;

		if (dafi->GlobalInstancesCount >= dafi->MaxInstancesCount) {
			ERROR("NodeManager::IncrementAddonFlavorInstancesCount addonid %ld, flavorid %ld maximum (or more) instances already exist\n", addonid, flavorid);
			return B_ERROR; // maximum (or more) instances already exist
		}
			
		bool b;		
		int32 *count;
		b = dafi->TeamInstancesCount.Get(team, &count);
		if (b) {
			*count += 1;
		} else {
			b = dafi->TeamInstancesCount.Insert(team, 1);
			ASSERT(b);
		}
		dafi->GlobalInstancesCount += 1;
		return B_OK;
	}
	ERROR("NodeManager::IncrementAddonFlavorInstancesCount addonid %ld, flavorid %ld not found\n", addonid, flavorid);
	return B_ERROR;
}

status_t
NodeManager::DecrementAddonFlavorInstancesCount(media_addon_id addonid, int32 flavorid, team_id team)
{
	BAutolock lock(fLocker);

	dormant_addon_flavor_info *dafi;
	for (fDormantAddonFlavorList->Rewind(); fDormantAddonFlavorList->GetNext(&dafi); ) {
		if (dafi->AddonID != addonid || dafi->AddonFlavorID != flavorid)
			continue;

		bool b;	
		int32 *count;
		b = dafi->TeamInstancesCount.Get(team, &count);
		if (!b) {
			ERROR("NodeManager::DecrementAddonFlavorInstancesCount addonid %ld, flavorid %ld team %ld has no references\n", addonid, flavorid, team);
			return B_ERROR;
		}
		*count -= 1;
		if (*count == 0) {
			b = dafi->TeamInstancesCount.Remove(team);
			ASSERT(b);
		}
		if (dafi->GlobalInstancesCount > 0)		// avoid underflow
			dafi->GlobalInstancesCount -= 1;
		return B_OK;
	}
	ERROR("NodeManager::DecrementAddonFlavorInstancesCount addonid %ld, flavorid %ld not found\n", addonid, flavorid);
	return B_ERROR;
}

// this function is called when the media_addon_server has crashed
void
NodeManager::CleanupDormantFlavorInfos()
{
	BAutolock lock(fLocker);
	printf("NodeManager::CleanupDormantFlavorInfos\n");
	fDormantAddonFlavorList->MakeEmpty();
	printf("NodeManager::CleanupDormantFlavorInfos done\n");
	// XXX FlavorsChanged(media_addon_id addonid, int32 newcount, int32 gonecount)
}

status_t 
NodeManager::GetDormantNodes(dormant_node_info * out_info,
							  int32 * io_count,
							  const media_format * has_input /* = NULL */,
							  const media_format * has_output /* = NULL */,
							  const char * name /* = NULL */,
							  uint64 require_kinds /* = NULL */,
							  uint64 deny_kinds /* = NULL */)
{
	BAutolock lock(fLocker);
	dormant_addon_flavor_info *dafi;
	int32 maxcount;
	int namelen;

	// determine the count of byte to compare when checking for a name with(out) wildcard
	if (name) {
		namelen = strlen(name);
		if (name[namelen] == '*')
			namelen--; // compares without the '*'
		else
			namelen++; // also compares the terminating NULL
	} else
		namelen = 0;

	maxcount = *io_count;	
	*io_count = 0;
	for (fDormantAddonFlavorList->Rewind(); (*io_count < maxcount) && fDormantAddonFlavorList->GetNext(&dafi); ) {
		if (!dafi->InfoValid)
			continue;

		dormant_flavor_info *dfi;
		dfi = &dafi->Info;

		if ((dfi->kinds & require_kinds) != require_kinds)
			continue;
		if ((dfi->kinds & deny_kinds) != 0)
			continue;
		if (namelen) {
			if (0 != memcmp(name, dfi->name, namelen))
				continue;
		}
		if (has_input) {
			bool hasit = false;
			for (int32 i = 0; i < dfi->in_format_count; i++)
				if (format_is_compatible(*has_input, dfi->in_formats[i])) {
					hasit = true;
					break;
				}
			if (!hasit)
				continue;
		}
		if (has_output) {
			bool hasit = false;
			for (int32 i = 0; i < dfi->out_format_count; i++)
				if (format_is_compatible(*has_output, dfi->out_formats[i])) {
					hasit = true;
					break;
				}
			if (!hasit)
				continue;
		}
		
		out_info[*io_count] = dfi->node_info;
		*io_count += 1;
	}

	return B_OK;
}

status_t 
NodeManager::GetDormantFlavorInfoFor(media_addon_id addon,
									 int32 flavor_id,
									 dormant_flavor_info *outFlavor)
{
	BAutolock lock(fLocker);
	dormant_addon_flavor_info *dafi;
	for (fDormantAddonFlavorList->Rewind(); fDormantAddonFlavorList->GetNext(&dafi); ) {
		if (dafi->AddonID == addon && dafi->AddonFlavorID == flavor_id && dafi->InfoValid == true) {
			*outFlavor = dafi->Info;
			return B_OK;
		}	
	}
	return B_ERROR;
}

/**********************************************************************
 * Default node management
 **********************************************************************/

status_t
NodeManager::SetDefaultNode(node_type type, const media_node *node, const dormant_node_info *info, const media_input *input)
{
	BAutolock lock(fLocker);
	status_t err = B_BAD_VALUE;
	if (node)
		err = fDefaultManager->Set(node->node, NULL, 0, type);
	else if (input)
		err = fDefaultManager->Set(input->node.node, input->name, input->destination.id, type);
	else if (info) {
		media_node_id node_id;
		int32 count = 1;
		if ((err=GetInstances(&node_id, &count, count, info->addon, info->flavor_id))!=B_OK)
			return err;
		err = fDefaultManager->Set(node_id, NULL, 0, type);
	}
	if(err==B_OK && (type == VIDEO_INPUT || type == VIDEO_OUTPUT || type == AUDIO_OUTPUT || type == AUDIO_INPUT)) {
		fDefaultManager->SaveState(this);
		Dump();
	}
	return err;
}

status_t
NodeManager::GetDefaultNode(media_node_id *nodeid, char *input_name, int32 *input_id, node_type type)
{
	BAutolock lock(fLocker);
	return fDefaultManager->Get(nodeid, input_name, input_id, type);
}

status_t
NodeManager::RescanDefaultNodes()
{
	BAutolock lock(fLocker);
	return fDefaultManager->Rescan();
}

/**********************************************************************
 * Cleanup of dead teams
 **********************************************************************/

void
NodeManager::CleanupTeam(team_id team)
{
	BAutolock lock(fLocker);

	fDefaultManager->CleanupTeam(team);

	PRINT(1, "NodeManager::CleanupTeam: team %ld\n", team);

	// XXX send notifications after removing nodes
	
	// Cleanup node references
	
	registered_node *rn;
	for (fRegisteredNodeMap->Rewind(); fRegisteredNodeMap->GetNext(&rn); ) {
		// if the gone team was the creator of some global dormant node instance, we now invalidate that
		// we may want to remove that global node, but I'm not sure
		if (rn->creator == team) {
			rn->creator = -1;
			// fall through
		}
		// if the team hosting this node is gone, remove node from database
		if (rn->team == team) {
			PRINT(1, "NodeManager::CleanupTeam: removing node id %ld, team %ld\n", rn->nodeid, team);
			fRegisteredNodeMap->RemoveCurrent();
			continue;
		}
		// check the list of teams that have references to this node, and remove the team
		team_id *pteam;
		int32 *prefcount;
		for (rn->teamrefcount.Rewind(); rn->teamrefcount.GetNext(&prefcount); ) {
			rn->teamrefcount.GetCurrentKey(&pteam);
			if (*pteam == team) {
				PRINT(1, "NodeManager::CleanupTeam: removing %ld refs from node id %ld, team %ld\n", *prefcount, rn->nodeid, team);
				rn->teamrefcount.RemoveCurrent();
				break;
			}
		}
		// if the team refcount is now empty, also remove the node
		if (rn->teamrefcount.IsEmpty()) {
			PRINT(1, "NodeManager::CleanupTeam: removing node id %ld that has no teams\n", rn->nodeid);
			fRegisteredNodeMap->RemoveCurrent();
		}
	}
	
	// Cleanup addon references
	dormant_addon_flavor_info *dafi;
	for (fDormantAddonFlavorList->Rewind(); fDormantAddonFlavorList->GetNext(&dafi); ) {
		bool b;	
		int32 *count;
		b = dafi->TeamInstancesCount.Get(team, &count);
		if (b) {
			PRINT(1, "NodeManager::CleanupTeam: removing %ld instances from addon %ld, flavor %ld\n", *count, dafi->AddonID, dafi->AddonFlavorID);
			dafi->GlobalInstancesCount -= *count;
			if (dafi->GlobalInstancesCount < 0)		// avoid underflow
				dafi->GlobalInstancesCount = 0;
			b = dafi->TeamInstancesCount.Remove(team);
			ASSERT(b);
		}
	}
}

/**********************************************************************
 * State saving/loading
 **********************************************************************/

status_t
NodeManager::LoadState()
{
	BAutolock lock(fLocker);
	return fDefaultManager->LoadState();
}

status_t
NodeManager::SaveState()
{
	BAutolock lock(fLocker);
	return fDefaultManager->SaveState(this);
}

/**********************************************************************
 * Debugging
 **********************************************************************/

void
NodeManager::Dump()
{
	BAutolock lock(fLocker);
	printf("\n");
	
	/* for each addon-id, the addon path map contains an entry_ref
	 */
	printf("NodeManager: addon path map follows:\n");
	entry_ref *ref;
	media_addon_id *id;
	for (fAddonPathMap->Rewind(); fAddonPathMap->GetNext(&ref); ) {
		fAddonPathMap->GetCurrentKey(&id);
		BPath path(ref);
		printf(" addon-id %ld, ref-name \"%s\", path \"%s\"\n", *id, ref->name, (path.InitCheck() == B_OK) ? path.Path() : "INVALID");
	}
	printf("NodeManager: list end\n");
	printf("\n");

	/* for each node-id, the registered node map contians information about source of the node, users, etc.
	 */
	printf("NodeManager: registered nodes map follows:\n");
	registered_node *rn;
	for (fRegisteredNodeMap->Rewind(); fRegisteredNodeMap->GetNext(&rn); ) {
		printf("  node-id %ld, addon-id %ld, addon-flavor-id %ld, port %ld, creator %ld, team %ld, kinds %#08Lx, name \"%s\"\n",
			rn->nodeid, rn->addon_id, rn->addon_flavor_id, rn->port, rn->creator, rn->team, rn->kinds, rn->name);
		printf("    teams (refcount): ");
		team_id *team;
		int32 *refcount;
		for (rn->teamrefcount.Rewind(); rn->teamrefcount.GetNext(&refcount); ) {
			rn->teamrefcount.GetCurrentKey(&team);
			printf("%ld (%ld), ", *team, *refcount);
		}
		printf("\n");
		media_input *input;
		for (rn->inputlist.Rewind(); rn->inputlist.GetNext(&input); ) {
			printf("    media_input: node-id %ld, node-port %ld, source-port %ld, source-id  %ld, dest-port %ld, dest-id %ld, name \"%s\"\n",
				input->node.node, input->node.port, input->source.port, input->source.id, input->destination.port, input->destination.id, input->name);
		}
		if (rn->inputlist.IsEmpty())
			printf("    media_input: none\n");
		media_output *output;
		for (rn->outputlist.Rewind(); rn->outputlist.GetNext(&output); ) {
			printf("    media_output: node-id %ld, node-port %ld, source-port %ld, source-id  %ld, dest-port %ld, dest-id %ld, name \"%s\"\n",
				output->node.node, output->node.port, output->source.port, output->source.id, output->destination.port, output->destination.id, output->name);
		}
		if (rn->outputlist.IsEmpty())
			printf("    media_output: none\n");
	}
	printf("NodeManager: list end\n");
	printf("\n");

	/* 
	 */
	printf("NodeManager: dormant flavor list follows:\n");
	dormant_addon_flavor_info *dafi;
	for (fDormantAddonFlavorList->Rewind(); fDormantAddonFlavorList->GetNext(&dafi); ) {
		printf("  AddonID %ld, AddonFlavorID %ld, MaxInstancesCount %ld, GlobalInstancesCount %ld, InfoValid %s\n",
			dafi->AddonID, dafi->AddonFlavorID, dafi->MaxInstancesCount, dafi->GlobalInstancesCount, dafi->InfoValid ? "yes" : "no");
		if (!dafi->InfoValid)
			continue;
		printf("    addon-id %ld, addon-flavor-id %ld, addon-name \"%s\"\n",
			dafi->Info.node_info.addon, dafi->Info.node_info.flavor_id, dafi->Info.node_info.name);
		printf("    flavor-kinds %#08Lx, flavor_flags %#08lx, internal_id %ld, possible_count %ld, in_format_count %ld, out_format_count %ld\n",
			 dafi->Info.kinds, dafi->Info.flavor_flags, dafi->Info.internal_id, dafi->Info.possible_count, dafi->Info.in_format_count, dafi->Info.out_format_count);
		printf("    flavor-name \"%s\"\n", dafi->Info.name);
		printf("    flavor-info \"%s\"\n", dafi->Info.info);
	}
	printf("NodeManager: list end\n");
	fDefaultManager->Dump();
}

const char *
get_node_type(node_type t)
{
	switch (t) {
		#define CASE(c) case c: return #c;
		CASE(VIDEO_INPUT)
		CASE(AUDIO_INPUT)
		CASE(VIDEO_OUTPUT)
		CASE(AUDIO_MIXER)
		CASE(AUDIO_OUTPUT)
		CASE(AUDIO_OUTPUT_EX)
		CASE(TIME_SOURCE)
		CASE(SYSTEM_TIME_SOURCE)
		default: return "unknown";
	}
};

