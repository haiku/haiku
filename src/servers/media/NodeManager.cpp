/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
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

NodeManager::NodeManager() :
	nextaddonid(1),
	nextnodeid(1)
{
	fLocker = new BLocker("node manager locker");
	fDormantFlavorList = new List<dormant_flavor_info>;
	fAddonPathMap = new Map<media_addon_id, entry_ref>;
	fRegisteredNodeMap = new Map<media_node_id, registered_node>;
}


NodeManager::~NodeManager()
{
	delete fLocker;
	delete fDormantFlavorList;
	delete fAddonPathMap;
	delete fRegisteredNodeMap;
}


status_t
NodeManager::RegisterNode(media_node_id *nodeid, media_addon_id addon_id, int32 addon_flavor_id, const char *name, uint64 kinds, port_id port, team_id team)
{
	BAutolock lock(fLocker);
	bool b;
	registered_node rn;
	rn.nodeid = nextnodeid;
	rn.addon_id = addon_id;
	rn.addon_flavor_id = addon_flavor_id;
	strcpy(rn.name, name);
	rn.kinds = kinds;
	rn.port = port;
	rn.team = team;
	rn.globalrefcount = 1;
	rn.teamrefcount.Insert(team, 1);
	
	b = fRegisteredNodeMap->Insert(nextnodeid, rn);
	ASSERT(b);
	*nodeid = nextnodeid;
	nextnodeid += 1;
	TRACE("NodeManager::RegisterNode: node %ld, addon_id %ld, flavor_id %ld, name \"%s\", kinds %#Lx, port %ld, team %ld\n", *nodeid, addon_id, addon_flavor_id, name, kinds, port, team);
	return B_OK;
}


status_t
NodeManager::UnregisterNode(media_addon_id *addon_id, media_node_id nodeid, team_id team)
{
	BAutolock lock(fLocker);
	bool b;
	registered_node *rn;
	TRACE("NodeManager::UnregisterNode enter: node %ld, team %ld\n", nodeid, team);
	b = fRegisteredNodeMap->Get(nodeid, &rn);
	if (!b) {
		FATAL("!!! NodeManager::UnregisterNode: Error: couldn't finde node %ld (team %ld)\n", nodeid, team);
		return B_ERROR;
	}
	if (rn->team != team) {
		FATAL("!!! NodeManager::UnregisterNode: Error: team %ld tried to unregister node %ld, but it was instantiated by team %ld\n", team, nodeid, rn->team);
		return B_ERROR;
	}
	if (rn->globalrefcount != 1) {
		FATAL("!!! NodeManager::UnregisterNode: Error: node %ld, team %ld, globalrefcount %ld\n", nodeid, team, rn->globalrefcount);
		//return B_ERROR;
	}
	*addon_id = rn->addon_id;
	b = fRegisteredNodeMap->Remove(nodeid);
	ASSERT(b);
	TRACE("NodeManager::UnregisterNode leave: node %ld, addon_id %ld, team %ld\n", nodeid, *addon_id, team);
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
		FATAL("!!! NodeManager::IncrementGlobalRefCount: Error: node %ld not found\n", nodeid);
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
		FATAL("!!! NodeManager::DecrementGlobalRefCount: Error: node %ld not found\n", nodeid);
		return B_ERROR;
	}
	int32 *count;
	b = rn->teamrefcount.Get(team, &count);
	if (!b) {
		FATAL("!!! NodeManager::DecrementGlobalRefCount: Error: node %ld has no team %ld references\n", nodeid, team);
		return B_ERROR;
	}
	*count -= 1;
	int32 debug_count = *count;
	if (*count == 0) {
		b = rn->teamrefcount.Remove(team);
		ASSERT(b);
	}
	rn->globalrefcount -= 1;
	TRACE("NodeManager::DecrementGlobalRefCount leave: node %ld, team %ld, count %ld, globalcount %ld\n", nodeid, team, debug_count, rn->globalrefcount);
	return B_OK;
}


status_t
NodeManager::GetCloneForId(media_node *node, media_node_id nodeid, team_id team)
{
	BAutolock lock(fLocker);
	registered_node *rn;
	bool b;
	TRACE("NodeManager::GetCloneForId enter: node %ld team %ld\n", nodeid, team);

	if (B_OK != IncrementGlobalRefCount(nodeid, team)) {
		FATAL("!!! NodeManager::GetCloneForId: Error: couldn't increment ref count, node %ld team %ld\n", nodeid, team);
		return B_ERROR;
	}

	b = fRegisteredNodeMap->Get(nodeid, &rn);
	if (!b) {
		FATAL("!!! NodeManager::GetCloneForId: Error: node %ld not found\n", nodeid);
		DecrementGlobalRefCount(nodeid, team);
		return B_ERROR;
	}

	node->node = rn->nodeid;
	node->port = rn->port;
	node->kind = rn->kinds;

	TRACE("NodeManager::GetCloneForId leave: node %ld team %ld\n", nodeid, team);
	return B_OK;
}


status_t
NodeManager::GetClone(media_node *node, char *input_name, int32 *input_id, node_type type, team_id team)
{
	BAutolock lock(fLocker);
	FATAL("!!! NodeManager::GetClone not implemented\n");
	*node = media_node::null;
	return B_ERROR;
}


status_t
NodeManager::ReleaseNode(const media_node &node, team_id team)
{
	BAutolock lock(fLocker);
	TRACE("NodeManager::ReleaseNode enter: node %ld team %ld\n", node.node, team);
	if (B_OK != DecrementGlobalRefCount(node.node, team)) {
		FATAL("!!! NodeManager::ReleaseNode: Error: couldn't decrement node %ld team %ld ref count\n", node.node, team);
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
		FATAL("!!! NodeManager::PublishInputs: Error: node %ld not found\n", node.node);
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
		FATAL("!!! NodeManager::PublishOutputs: Error: node %ld not found\n", node.node);
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
			if (output->source.port == port || output->destination.port == port) {
				*nodeid = rn->nodeid;
				TRACE("NodeManager::FindNodeId found output port %ld, node %ld\n", port, *nodeid);
				return B_OK;
			}
		}
		media_input *input;
		for (rn->inputlist.Rewind(); rn->inputlist.GetNext(&input); ) {
			if (input->source.port == port || input->destination.port == port) {
				*nodeid = rn->nodeid;
				TRACE("NodeManager::FindNodeId found input port %ld, node %ld\n", port, *nodeid);
				return B_OK;
			}
		}
	}
	FATAL("!!! NodeManager::FindNodeId failed, port %ld\n", port);
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
			ASSERT(node.kind == rn->kinds);
			live_info->node = node;
			live_info->hint_point = BPoint(0, 0);
			strcpy(live_info->name, rn->name);
			TRACE("NodeManager::GetLiveNodeInfo node %ld, name = \"%s\"\n", node.node, rn->name);
			return B_OK;
		}
	}
	FATAL("!!! NodeManager::GetLiveNodeInfo failed, node %ld\n", node.node);
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


status_t
NodeManager::GetDormantNodeInfo(dormant_node_info *node_info, const media_node &node)
{
	BAutolock lock(fLocker);
	// XXX not sure if this is correct
	registered_node *rn;
	for (fRegisteredNodeMap->Rewind(); fRegisteredNodeMap->GetNext(&rn); ) {
		if (rn->nodeid == node.node) {
			ASSERT(node.port == rn->port);
			ASSERT(node.kind == rn->kinds);
			node_info->addon = rn->addon_id;
			node_info->flavor_id = rn->addon_flavor_id;
			strcpy(node_info->name, rn->name);
			TRACE("NodeManager::GetDormantNodeInfo node %ld, addon_id %ld, addon_flavor_id %ld, name \"%s\"\n", node.node, rn->addon_id, rn->addon_flavor_id, rn->name);
			return B_OK;
		}
	}
	FATAL("!!! NodeManager::GetDormantNodeInfo failed, node %ld\n", node.node);
	return B_ERROR;
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

void 
NodeManager::RegisterAddon(const entry_ref &ref, media_addon_id *newid)
{
	BAutolock lock(fLocker);
	media_addon_id id;
	id = nextaddonid;
	nextaddonid += 1;
	
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

// this function is only called (indirectly) by the media_addon_server
void
NodeManager::AddDormantFlavorInfo(const dormant_flavor_info &dfi)
{
	BAutolock lock(fLocker);
	
	printf("NodeManager::AddDormantFlavorInfo, addon-id %ld, flavor-id %ld, name \"%s\", flavor-name \"%s\", flavor-info \"%s\"\n", dfi.node_info.addon, dfi.node_info.flavor_id, dfi.node_info.name, dfi.name, dfi.info);

	fDormantFlavorList->Insert(dfi);
}

// this function is only called (indirectly) by the media_addon_server
void
NodeManager::RemoveDormantFlavorInfo(media_addon_id id)
{
	BAutolock lock(fLocker);
	dormant_flavor_info *flavor;
	for (fDormantFlavorList->Rewind(); fDormantFlavorList->GetNext(&flavor); ) {
		if (flavor->node_info.addon == id) {
			printf("NodeManager::RemoveDormantFlavorInfo, addon-id %ld, flavor-id %ld, name \"%s\", flavor-name \"%s\", flavor-info \"%s\"\n", flavor->node_info.addon, flavor->node_info.flavor_id, flavor->node_info.name, flavor->name, flavor->info);
			fDormantFlavorList->RemoveCurrent();
		}
	}
}

// this function is called when the media_addon_server has crashed
void
NodeManager::CleanupDormantFlavorInfos()
{
	BAutolock lock(fLocker);
	printf("NodeManager::CleanupDormantFlavorInfos\n");
	fDormantFlavorList->MakeEmpty();
	printf("NodeManager::CleanupDormantFlavorInfos done\n");
	// XXX FlavorsChanged(media_addon_id addonid, int32 newcount, int32 gonecount)
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
	int32 maxcount;
	dormant_flavor_info *dfi;
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
	for (fDormantFlavorList->Rewind(); (*io_count < maxcount) && fDormantFlavorList->GetNext(&dfi); ) {
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
	dormant_flavor_info *flavor;
	for (fDormantFlavorList->Rewind(); fDormantFlavorList->GetNext(&flavor); ) {
		if (flavor->node_info.addon == addon && flavor->node_info.flavor_id == flavor_id) {
			*outFlavor = *flavor;
			return B_OK;
		}	
	}
	return B_ERROR;
}

void
NodeManager::CleanupTeam(team_id team)
{
	BAutolock lock(fLocker);
	FATAL("NodeManager::CleanupTeam: should cleanup team %ld\n", team);
}

void
NodeManager::Dump()
{
	BAutolock lock(fLocker);
	printf("\n");
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
	printf("NodeManager: registered nodes map follows:\n");
	registered_node *rn;
	for (fRegisteredNodeMap->Rewind(); fRegisteredNodeMap->GetNext(&rn); ) {
		printf("  node-id %ld, addon-id %ld, addon-flavor-id %ld, port %ld, team %ld, kinds %#08x, name \"%s\"\n",
			rn->nodeid, rn->addon_id, rn->addon_flavor_id, rn->port, rn->team, rn->kinds, rn->name);
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
		media_output *output;
		for (rn->outputlist.Rewind(); rn->outputlist.GetNext(&output); ) {
			printf("    media_output: node-id %ld, node-port %ld, source-port %ld, source-id  %ld, dest-port %ld, dest-id %ld, name \"%s\"\n",
				output->node.node, output->node.port, output->source.port, output->source.id, output->destination.port, output->destination.id, output->name);
		}
	}
	printf("NodeManager: list end\n");
	printf("\n");
	printf("NodeManager: dormant flavor list follows:\n");
	dormant_flavor_info *dfi;
	for (fDormantFlavorList->Rewind(); fDormantFlavorList->GetNext(&dfi); ) {
		printf("  addon-id %ld, addon-flavor-id %ld, addon-name \"%s\"\n",
			dfi->node_info.addon, dfi->node_info.flavor_id, dfi->node_info.name);
		printf("    flavor-kinds %#08x, flavor_flags %#08x, internal_id %ld, possible_count %ld, in_format_count %ld, out_format_count %ld\n",
			 dfi->kinds, dfi->flavor_flags, dfi->internal_id, dfi->possible_count, dfi->in_format_count, dfi->out_format_count);
		printf("    flavor-name \"%s\"\n", dfi->name);
		printf("    flavor-info \"%s\"\n", dfi->info);
	}
	printf("NodeManager: list end\n");
}
