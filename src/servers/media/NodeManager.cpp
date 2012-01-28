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


#include "NodeManager.h"

#include <Autolock.h>
#include <Entry.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <Path.h>

#include <debug.h>
#include <MediaMisc.h>

#include "AppManager.h"
#include "DefaultManager.h"
#include "media_server.h"


const char*
get_node_type(node_type type)
{
#define CASE(c) case c: return #c;
	switch (type) {
		CASE(VIDEO_INPUT)
		CASE(AUDIO_INPUT)
		CASE(VIDEO_OUTPUT)
		CASE(AUDIO_MIXER)
		CASE(AUDIO_OUTPUT)
		CASE(AUDIO_OUTPUT_EX)
		CASE(TIME_SOURCE)
		CASE(SYSTEM_TIME_SOURCE)

		default:
			return "unknown";
	}
#undef CASE
}


// #pragma mark -


NodeManager::NodeManager()
	:
	BLocker("node manager"),
	fNextAddOnID(1),
	fNextNodeID(1),
	fDefaultManager(new DefaultManager)
{
}


NodeManager::~NodeManager()
{
	delete fDefaultManager;
}


// #pragma mark - Default node management


status_t
NodeManager::SetDefaultNode(node_type type, const media_node* node,
	const dormant_node_info* info, const media_input* input)
{
	BAutolock _(this);

	status_t status = B_BAD_VALUE;
	if (node != NULL)
		status = fDefaultManager->Set(node->node, NULL, 0, type);
	else if (input != NULL) {
		status = fDefaultManager->Set(input->node.node, input->name,
			input->destination.id, type);
	} else if (info != NULL) {
		media_node_id nodeID;
		int32 count = 1;
		status = GetInstances(info->addon, info->flavor_id, &nodeID, &count,
			count);
		if (status == B_OK)
			status = fDefaultManager->Set(nodeID, NULL, 0, type);
	}

	if (status == B_OK && (type == VIDEO_INPUT || type == VIDEO_OUTPUT
			|| type == AUDIO_OUTPUT || type == AUDIO_INPUT)) {
		fDefaultManager->SaveState(this);
		Dump();
	}
	return status;
}


status_t
NodeManager::GetDefaultNode(node_type type, media_node_id* _nodeID,
	char* inputName, int32* _inputID)
{
	BAutolock _(this);
	return fDefaultManager->Get(_nodeID, inputName, _inputID, type);
}


status_t
NodeManager::RescanDefaultNodes()
{
	BAutolock _(this);
	return fDefaultManager->Rescan();
}


// #pragma mark - Live node management


status_t
NodeManager::RegisterNode(media_addon_id addOnID, int32 flavorID,
	const char* name, uint64 kinds, port_id port, team_id team,
	media_node_id* _nodeID)
{
	BAutolock _(this);

	registered_node node;
	node.node_id = fNextNodeID;
	node.add_on_id = addOnID;
	node.flavor_id = flavorID;
	strlcpy(node.name, name, sizeof(node.name));
	node.kinds = kinds;
	node.port = port;
	node.containing_team = team;
	node.creator = -1; // will be set later
	node.ref_count = 1;

	try {
		node.team_ref_count.insert(std::make_pair(team, 1));

		fNodeMap.insert(std::make_pair(fNextNodeID, node));
	} catch (std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}

	*_nodeID = fNextNodeID++;

	TRACE("NodeManager::RegisterNode: node %ld, addon_id %ld, flavor_id %ld, "
		"name \"%s\", kinds %#Lx, port %ld, team %ld\n", *_nodeID, addOnID,
		flavorID, name, kinds, port, team);
	return B_OK;
}


status_t
NodeManager::UnregisterNode(media_node_id id, team_id team,
	media_addon_id* _addOnID, int32* _flavorID)
{
	TRACE("NodeManager::UnregisterNode enter: node %ld, team %ld\n", id, team);

	BAutolock _(this);

	NodeMap::iterator found = fNodeMap.find(id);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::UnregisterNode: couldn't find node %ld (team "
			"%ld)\n", id, team);
		return B_ERROR;
	}

	registered_node& node = found->second;

	if (node.containing_team != team) {
		ERROR("NodeManager::UnregisterNode: team %ld tried to unregister "
			"node %ld, but it was instantiated by team %ld\n", team, id,
			node.containing_team);
		return B_ERROR;
	}
	if (node.ref_count != 1) {
		ERROR("NodeManager::UnregisterNode: node %ld, team %ld has ref count "
			"%ld (should be 1)\n", id, team, node.ref_count);
		//return B_ERROR;
	}

	*_addOnID = node.add_on_id;
	*_flavorID = node.flavor_id;

	fNodeMap.erase(found);

	TRACE("NodeManager::UnregisterNode leave: node %ld, addon_id %ld, "
		"flavor_id %ld team %ld\n", id, *_addOnID, *_flavorID, team);
	return B_OK;
}


status_t
NodeManager::ReleaseNodeReference(media_node_id id, team_id team)
{
	TRACE("NodeManager::ReleaseNodeReference enter: node %ld, team %ld\n", id,
		team);

	BAutolock _(this);

	NodeMap::iterator found = fNodeMap.find(id);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::ReleaseNodeReference: node %ld not found\n", id);
		return B_ERROR;
	}

	registered_node& node = found->second;

	TeamCountMap::iterator teamRef = node.team_ref_count.find(team);
	if (teamRef == node.team_ref_count.end()) {
		// Normally it is an error to release a node in another team. But we
		// make one exception: if the node is global, and the creator team
		// tries to release it, we will release it in the the
		// media_addon_server.
		team_id addOnServer = gAppManager->AddOnServerTeam();
		teamRef = node.team_ref_count.find(addOnServer);

		if (node.creator == team && teamRef != node.team_ref_count.end()) {
			PRINT(1, "!!! NodeManager::ReleaseNodeReference doing global "
				"release!\n");
			node.creator = -1; // invalidate!
			team = addOnServer;
		} else {
			ERROR("NodeManager::ReleaseNodeReference: node %ld has no team "
				"%ld references\n", id, team);
			return B_ERROR;
		}
	}

#if DEBUG
	int32 teamCount = teamRef->second - 1;
	(void)teamCount;
#endif

	if (--teamRef->second == 0)
		node.team_ref_count.erase(teamRef);

	if (--node.ref_count == 0) {
		PRINT(1, "NodeManager::ReleaseNodeReference: detected released node is "
			"now unused, node %ld\n", id);

		// TODO: remove!
		node_final_release_command command;
		status_t status = SendToPort(node.port, NODE_FINAL_RELEASE, &command,
			sizeof(command));
		if (status != B_OK) {
			ERROR("NodeManager::ReleaseNodeReference: can't send command to "
				"node %ld\n", id);
			// ignore error
		}
	}

	TRACE("NodeManager::ReleaseNodeReference leave: node %ld, team %ld, "
		"ref %ld, team ref %ld\n", id, team, node.ref_count, teamCount);
	return B_OK;
}


status_t
NodeManager::ReleaseNodeAll(media_node_id id)
{
	TRACE("NodeManager::ReleaseNodeAll enter: node %ld\n", id);

	BAutolock _(this);
	
	NodeMap::iterator found = fNodeMap.find(id);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::ReleaseNodeAll: node %ld not found\n", id);
		return B_ERROR;
	}
	
	registered_node& node = found->second;
	node.team_ref_count.clear();
	node.ref_count = 0;
	
	node_final_release_command command;
	status_t status = SendToPort(node.port, NODE_FINAL_RELEASE, &command,
		sizeof(command));
	if (status != B_OK) {
		ERROR("NodeManager::ReleaseNodeAll: can't send command to "
			"node %ld\n", id);
		// ignore error
	}

	TRACE("NodeManager::ReleaseNodeAll leave: node %ld\n", id);
	return B_OK;
}


status_t
NodeManager::SetNodeCreator(media_node_id id, team_id creator)
{
	TRACE("NodeManager::SetNodeCreator node %ld, creator %ld\n", id, creator);

	BAutolock _(this);

	NodeMap::iterator found = fNodeMap.find(id);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::SetNodeCreator: node %ld not found\n", id);
		return B_ERROR;
	}

	registered_node& node = found->second;

	if (node.creator != -1) {
		ERROR("NodeManager::SetNodeCreator: node %ld is already assigned "
			"creator %ld\n", id, node.creator);
		return B_ERROR;
	}

	node.creator = creator;
	return B_OK;
}


status_t
NodeManager::GetCloneForID(media_node_id id, team_id team, media_node* node)
{
	TRACE("NodeManager::GetCloneForID enter: node %ld team %ld\n", id, team);

	BAutolock _(this);

	status_t status = _AcquireNodeReference(id, team);
	if (status != B_OK) {
		ERROR("NodeManager::GetCloneForID: couldn't increment ref count, "
			"node %ld team %ld\n", id, team);
		return status;
	}

	NodeMap::iterator found = fNodeMap.find(id);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::GetCloneForID: node %ld not found\n", id);
		return B_ERROR;
	}

	registered_node& registeredNode = found->second;

	node->node = registeredNode.node_id;
	node->port = registeredNode.port;
	node->kind = registeredNode.kinds;

	TRACE("NodeManager::GetCloneForID leave: node %ld team %ld\n", id, team);
	return B_OK;
}


/*!	This function locates the default "node" for the requested "type" and
	returns a clone.
	If the requested type is AUDIO_OUTPUT_EX, also "input_name" and "input_id"
	need to be set and returned, as this is required by
	BMediaRoster::GetAudioOutput(media_node *out_node, int32 *out_input_id,
		BString *out_input_name).
*/
status_t
NodeManager::GetClone(node_type type, team_id team, media_node* node,
	char* inputName, int32* _inputID)
{
	BAutolock _(this);

	TRACE("NodeManager::GetClone enter: team %ld, type %d (%s)\n", team, type, get_node_type(type));

	media_node_id id;
	status_t status = GetDefaultNode(type, &id, inputName, _inputID);
	if (status != B_OK) {
		ERROR("NodeManager::GetClone: couldn't GetDefaultNode, team %ld, "
			"type %d (%s)\n", team, type, get_node_type(type));
		*node = media_node::null;
		return status;
	}
	ASSERT(id > 0);

	status = GetCloneForID(id, team, node);
	if (status != B_OK) {
		ERROR("NodeManager::GetClone: couldn't GetCloneForID, id %ld, team "
			"%ld, type %d (%s)\n", id, team, type, get_node_type(type));
		*node = media_node::null;
		return status;
	}
	ASSERT(id == node->node);

	TRACE("NodeManager::GetClone leave: node id %ld, node port %ld, node "
		"kind %#lx\n", node->node, node->port, node->kind);
	return B_OK;
}


status_t
NodeManager::ReleaseNode(const media_node& node, team_id team)
{
	TRACE("NodeManager::ReleaseNode enter: node %ld team %ld\n", node.node,
		team);

	if (ReleaseNodeReference(node.node, team) != B_OK) {
		ERROR("NodeManager::ReleaseNode: couldn't decrement node %ld team %ld "
			"ref count\n", node.node, team);
	}

	return B_OK;
}


status_t
NodeManager::PublishInputs(const media_node& node, const media_input* inputs,
	int32 count)
{
	BAutolock _(this);

	NodeMap::iterator found = fNodeMap.find(node.node);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::PublishInputs: node %ld not found\n", node.node);
		return B_ERROR;
	}

	registered_node& registeredNode = found->second;

	registeredNode.input_list.clear();

	try {
		for (int32 i = 0; i < count; i++)
			registeredNode.input_list.push_back(inputs[i]);
	} catch (std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
NodeManager::PublishOutputs(const media_node &node, const media_output* outputs,
	int32 count)
{
	BAutolock _(this);

	NodeMap::iterator found = fNodeMap.find(node.node);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::PublishOutputs: node %ld not found\n", node.node);
		return B_ERROR;
	}

	registered_node& registeredNode = found->second;

	registeredNode.output_list.clear();

	try {
		for (int32 i = 0; i < count; i++)
			registeredNode.output_list.push_back(outputs[i]);
	} catch (std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
NodeManager::FindNodeID(port_id port, media_node_id* _id)
{
	BAutolock _(this);

	NodeMap::iterator iterator = fNodeMap.begin();
	for (; iterator != fNodeMap.end(); iterator++) {
		registered_node& node = iterator->second;

		if (node.port == port) {
			*_id = node.node_id;
			TRACE("NodeManager::FindNodeID found port %ld, node %ld\n", port,
				node.node_id);
			return B_OK;
		}

		OutputList::iterator outIterator = node.output_list.begin();
		for (; outIterator != node.output_list.end(); outIterator++) {
			if (outIterator->source.port == port) {
				*_id = node.node_id;
				TRACE("NodeManager::FindNodeID found output port %ld, node "
					"%ld\n", port, node.node_id);
				return B_OK;
			}
		}

		InputList::iterator inIterator = node.input_list.begin();
		for (; inIterator != node.input_list.end(); inIterator++) {
			if (inIterator->destination.port == port) {
				*_id = node.node_id;
				TRACE("NodeManager::FindNodeID found input port %ld, node "
					"%ld\n", port, node.node_id);
				return B_OK;
			}
		}
	}

	ERROR("NodeManager::FindNodeID failed, port %ld\n", port);
	return B_ERROR;
}


status_t
NodeManager::GetDormantNodeInfo(const media_node& node,
	dormant_node_info* nodeInfo)
{
	// TODO: not sure if this is correct
	BAutolock _(this);

	NodeMap::iterator found = fNodeMap.find(node.node);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::GetDormantNodeInfo: node %ld not found\n",
			node.node);
		return B_ERROR;
	}

	registered_node& registeredNode = found->second;

	if (registeredNode.add_on_id == -1
		&& node.node != NODE_SYSTEM_TIMESOURCE_ID) {
		// This function must return an error if the node is application owned
		TRACE("NodeManager::GetDormantNodeInfo NODE IS APPLICATION OWNED! "
			"node %ld, add_on_id %ld, flavor_id %ld, name \"%s\"\n", node.node,
			registeredNode.add_on_id, registeredNode.flavor_id,
			registeredNode.name);
		return B_ERROR;
	}

	ASSERT(node.port == registeredNode.port);
	ASSERT((node.kind & NODE_KIND_COMPARE_MASK)
		== (registeredNode.kinds & NODE_KIND_COMPARE_MASK));

	nodeInfo->addon = registeredNode.add_on_id;
	nodeInfo->flavor_id = registeredNode.flavor_id;
	strlcpy(nodeInfo->name, registeredNode.name, sizeof(nodeInfo->name));

	TRACE("NodeManager::GetDormantNodeInfo node %ld, add_on_id %ld, "
		"flavor_id %ld, name \"%s\"\n", node.node, registeredNode.add_on_id,
		registeredNode.flavor_id, registeredNode.name);
	return B_OK;
}


status_t
NodeManager::GetLiveNodeInfo(const media_node& node, live_node_info* liveInfo)
{
	BAutolock _(this);

	NodeMap::iterator found = fNodeMap.find(node.node);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::GetLiveNodeInfo: node %ld not found\n",
			node.node);
		return B_ERROR;
	}

	registered_node& registeredNode = found->second;

	ASSERT(node.port == registeredNode.port);
	ASSERT((node.kind & NODE_KIND_COMPARE_MASK)
		== (registeredNode.kinds & NODE_KIND_COMPARE_MASK));

	liveInfo->node = node;
	liveInfo->hint_point = BPoint(0, 0);
	strlcpy(liveInfo->name, registeredNode.name, sizeof(liveInfo->name));

	TRACE("NodeManager::GetLiveNodeInfo node %ld, name = \"%s\"\n", node.node,
		registeredNode.name);
	return B_OK;
}


status_t
NodeManager::GetInstances(media_addon_id addOnID, int32 flavorID,
	media_node_id* ids, int32* _count, int32 maxCount)
{
	BAutolock _(this);

	NodeMap::iterator iterator = fNodeMap.begin();
	int32 count = 0;
	for (; iterator != fNodeMap.end() && count < maxCount; iterator++) {
		registered_node& node = iterator->second;

		if (node.add_on_id == addOnID && node.flavor_id == flavorID)
			ids[count++] = node.node_id;
	}

	TRACE("NodeManager::GetInstances found %ld instances for addon_id %ld, "
		"flavor_id %ld\n", count, addOnID, flavorID);
	*_count = count;
	return B_OK;
}


status_t
NodeManager::GetLiveNodes(LiveNodeList& liveNodes, int32 maxCount,
	const media_format* inputFormat, const media_format* outputFormat,
	const char* name, uint64 requireKinds)
{
	TRACE("NodeManager::GetLiveNodes: maxCount %ld, in-format %p, out-format "
		"%p, name %s, require kinds 0x%Lx\n", maxCount, inputFormat,
		outputFormat, name != NULL ? name : "NULL", requireKinds);

	BAutolock _(this);

	// Determine the count of byte to compare when checking for a name with
	// or without wildcard
	size_t nameLength = 0;
	if (name != NULL) {
		nameLength = strlen(name);
		if (nameLength > 0 && name[nameLength - 1] == '*')
			nameLength--;
	}

	NodeMap::iterator iterator = fNodeMap.begin();
	int32 count = 0;
	for (; iterator != fNodeMap.end() && count < maxCount; iterator++) {
		registered_node& node = iterator->second;

		if ((node.kinds & requireKinds) != requireKinds)
			continue;

		if (nameLength != 0) {
			if (strncmp(name, node.name, nameLength) != 0)
				continue;
		}

		if (inputFormat != NULL) {
			bool found = false;

			for (InputList::iterator inIterator = node.input_list.begin();
					inIterator != node.input_list.end(); inIterator++) {
				media_input& input = *inIterator;

				if (format_is_compatible(*inputFormat, input.format)) {
					found = true;
					break;
				}
			}

			if (!found)
				continue;
		}

		if (outputFormat != NULL) {
			bool found = false;

			for (OutputList::iterator outIterator = node.output_list.begin();
					outIterator != node.output_list.end(); outIterator++) {
				media_output& output = *outIterator;

				if (format_is_compatible(*outputFormat, output.format)) {
					found = true;
					break;
				}
			}

			if (!found)
				continue;
		}

		live_node_info info;
		info.node.node = node.node_id;
		info.node.port = node.port;
		info.node.kind = node.kinds;
		info.hint_point = BPoint(0, 0);
		strlcpy(info.name, node.name, sizeof(info.name));

		try {
			liveNodes.push_back(info);
		} catch (std::bad_alloc& exception) {
			return B_NO_MEMORY;
		}

		count++;
	}

	TRACE("NodeManager::GetLiveNodes found %ld\n", count);
	return B_OK;
}


/*!	Add media_node_id of all live nodes to the message
	int32 "media_node_id" (multiple items)
*/
status_t
NodeManager::GetLiveNodes(BMessage* message)
{
	BAutolock _(this);

	NodeMap::iterator iterator = fNodeMap.begin();
	for (; iterator != fNodeMap.end(); iterator++) {
		registered_node& node = iterator->second;

		if (message->AddInt32("media_node_id", node.node_id) != B_OK)
			return B_NO_MEMORY;
	}

	return B_OK;
}


// #pragma mark - Registration of BMediaAddOns


void
NodeManager::RegisterAddOn(const entry_ref& ref, media_addon_id* _newID)
{
	BAutolock _(this);

	media_addon_id id = fNextAddOnID++;

//	printf("NodeManager::RegisterAddOn: ref-name \"%s\", assigning id %ld\n",
//		ref.name, id);

	try {
		fPathMap.insert(std::make_pair(id, ref));
		*_newID = id;
	} catch (std::bad_alloc& exception) {
		*_newID = -1;
	}
}


void
NodeManager::UnregisterAddOn(media_addon_id addOnID)
{
	PRINT(1, "NodeManager::UnregisterAddOn: id %ld\n", addOnID);

	BAutolock _(this);

	RemoveDormantFlavorInfo(addOnID);
	fPathMap.erase(addOnID);
}


status_t
NodeManager::GetAddOnRef(media_addon_id addOnID, entry_ref* ref)
{
	BAutolock _(this);

	PathMap::iterator found = fPathMap.find(addOnID);
	if (found == fPathMap.end())
		return B_ERROR;

	*ref = found->second;
	return B_OK;
}


// #pragma mark - Registration of node flavors, published by BMediaAddOns


//!	This function is only used (indirectly) by the media_addon_server.
status_t
NodeManager::AddDormantFlavorInfo(const dormant_flavor_info& flavorInfo)
{
	PRINT(1, "NodeManager::AddDormantFlavorInfo, addon-id %ld, flavor-id %ld, "
		"name \"%s\", flavor-name \"%s\", flavor-info \"%s\"\n",
		flavorInfo.node_info.addon, flavorInfo.node_info.flavor_id,
		flavorInfo.node_info.name, flavorInfo.name, flavorInfo.info);

	BAutolock _(this);

	// Try to find the addon-id/flavor-id in the list.
	// If it already exists, update the info, but don't change its instance
	// count.

	for (DormantFlavorList::iterator iterator = fDormantFlavors.begin();
			iterator != fDormantFlavors.end(); iterator++) {
		dormant_add_on_flavor_info& info = *iterator;

		if (info.add_on_id != flavorInfo.node_info.addon
			|| info.flavor_id != flavorInfo.node_info.flavor_id)
			continue;

		if (info.info_valid) {
			ERROR("NodeManager::AddDormantFlavorInfo, addon-id %ld, "
				"flavor-id %ld does already exist\n", info.info.node_info.addon,
				info.info.node_info.flavor_id);
		}

		TRACE("NodeManager::AddDormantFlavorInfo, updating addon-id %ld, "
			"flavor-id %ld\n", info.info.node_info.addon,
			info.info.node_info.flavor_id);

		info.max_instances_count = flavorInfo.possible_count > 0
			? flavorInfo.possible_count : INT32_MAX;
		info.info_valid = true;
		info.info = flavorInfo;
		return B_OK;
	}

	// Insert information into the list

	dormant_add_on_flavor_info info;
	info.add_on_id = flavorInfo.node_info.addon;
	info.flavor_id = flavorInfo.node_info.flavor_id;
	info.max_instances_count = flavorInfo.possible_count > 0
		? flavorInfo.possible_count : INT32_MAX;
	info.instances_count = 0;
	info.info_valid = true;
	info.info = flavorInfo;

	try {
		fDormantFlavors.push_back(info);
	} catch (std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


//!	This function is only used (indirectly) by the media_addon_server
void
NodeManager::InvalidateDormantFlavorInfo(media_addon_id addOnID)
{
	BAutolock _(this);

	for (DormantFlavorList::iterator iterator = fDormantFlavors.begin();
			iterator != fDormantFlavors.end(); iterator++) {
		dormant_add_on_flavor_info& info = *iterator;

		if (info.add_on_id == addOnID && info.info_valid) {
			PRINT(1, "NodeManager::InvalidateDormantFlavorInfo, addon-id %ld, "
				"flavor-id %ld, name \"%s\", flavor-name \"%s\", flavor-info "
				"\"%s\"\n", info.info.node_info.addon,
				info.info.node_info.flavor_id, info.info.node_info.name,
				info.info.name, info.info.info);

			info.info_valid = false;
		}
	}
}


//!	This function is only used (indirectly) by the media_addon_server
void
NodeManager::RemoveDormantFlavorInfo(media_addon_id addOnID)
{
	BAutolock _(this);

	for (size_t index = 0; index < fDormantFlavors.size(); index++) {
		dormant_add_on_flavor_info& info = fDormantFlavors[index];

		if (info.add_on_id == addOnID) {
			PRINT(1, "NodeManager::RemoveDormantFlavorInfo, addon-id %ld, "
				"flavor-id %ld, name \"%s\", flavor-name \"%s\", flavor-info "
				"\"%s\"\n", info.info.node_info.addon,
				info.info.node_info.flavor_id, info.info.node_info.name,
				info.info.name, info.info.info);
			fDormantFlavors.erase(fDormantFlavors.begin() + index--);
		}
	}
}


status_t
NodeManager::IncrementFlavorInstancesCount(media_addon_id addOnID,
	int32 flavorID, team_id team)
{
	BAutolock _(this);

	for (DormantFlavorList::iterator iterator = fDormantFlavors.begin();
			iterator != fDormantFlavors.end(); iterator++) {
		dormant_add_on_flavor_info& info = *iterator;

		if (info.add_on_id != addOnID || info.flavor_id != flavorID)
			continue;

		if (info.instances_count >= info.max_instances_count) {
			// maximum (or more) instances already exist
			ERROR("NodeManager::IncrementFlavorInstancesCount addon-id %ld, "
				"flavor-id %ld maximum (or more) instances already exist\n",
				addOnID, flavorID);
			return B_ERROR;
		}

		TeamCountMap::iterator teamInstance
			= info.team_instances_count.find(team);
		if (teamInstance == info.team_instances_count.end()) {
			// This is the team's first instance
			try {
				info.team_instances_count.insert(std::make_pair(team, 1));
			} catch (std::bad_alloc& exception) {
				return B_NO_MEMORY;
			}
		} else {
			// Just increase its ref count
			teamInstance->second++;
		}

		info.instances_count++;
		return B_OK;
	}

	ERROR("NodeManager::IncrementFlavorInstancesCount addon-id %ld, "
		"flavor-id %ld not found\n", addOnID, flavorID);
	return B_ERROR;
}


status_t
NodeManager::DecrementFlavorInstancesCount(media_addon_id addOnID,
	int32 flavorID, team_id team)
{
	BAutolock _(this);

	for (DormantFlavorList::iterator iterator = fDormantFlavors.begin();
			iterator != fDormantFlavors.end(); iterator++) {
		dormant_add_on_flavor_info& info = *iterator;

		if (info.add_on_id != addOnID || info.flavor_id != flavorID)
			continue;

		TeamCountMap::iterator teamInstance
			= info.team_instances_count.find(team);
		if (teamInstance == info.team_instances_count.end()) {
			ERROR("NodeManager::DecrementFlavorInstancesCount addon-id %ld, "
				"flavor-id %ld team %ld has no references\n", addOnID, flavorID,
				team);
			return B_ERROR;
		}
		if (--teamInstance->second == 0)
			info.team_instances_count.erase(teamInstance);

		info.instances_count--;
		return B_OK;
	}

	ERROR("NodeManager::DecrementFlavorInstancesCount addon-id %ld, "
		"flavor-id %ld not found\n", addOnID, flavorID);
	return B_ERROR;
}


//!	This function is called when the media_addon_server has crashed
void
NodeManager::CleanupDormantFlavorInfos()
{
	PRINT(1, "NodeManager::CleanupDormantFlavorInfos\n");

	BAutolock _(this);
	fDormantFlavors.clear();
	// TODO: FlavorsChanged() notification

	PRINT(1, "NodeManager::CleanupDormantFlavorInfos done\n");
}


status_t
NodeManager::GetDormantNodes(dormant_node_info* infos, int32* _count,
	const media_format* input, const media_format* output, const char* name,
	uint64 requireKinds, uint64 denyKinds)
{
	BAutolock _(this);

	// Determine the count of byte to compare when checking for a name with
	// or without wildcard
	size_t nameLength = 0;
	if (name != NULL) {
		nameLength = strlen(name);
		if (nameLength > 0 && name[nameLength - 1] == '*')
			nameLength--;
	}

	int32 maxCount = *_count;
	int32 count = 0;

	for (DormantFlavorList::iterator iterator = fDormantFlavors.begin();
			iterator != fDormantFlavors.end() && count < maxCount; iterator++) {
		dormant_add_on_flavor_info& info = *iterator;

		if (!info.info_valid)
			continue;

		if ((info.info.kinds & requireKinds) != requireKinds
			|| (info.info.kinds & denyKinds) != 0)
			continue;

		if (nameLength != 0) {
			if (strncmp(name, info.info.name, nameLength) != 0)
				continue;
		}

		if (input != NULL) {
			bool found = false;

			for (int32 i = 0; i < info.info.in_format_count; i++) {
				if (format_is_compatible(*input, info.info.in_formats[i])) {
					found = true;
					break;
				}
			}

			if (!found)
				continue;
		}

		if (output != NULL) {
			bool found = false;

			for (int32 i = 0; i < info.info.out_format_count; i++) {
				if (format_is_compatible(*output, info.info.out_formats[i])) {
					found = true;
					break;
				}
			}

			if (!found)
				continue;
		}

		infos[count++] = info.info.node_info;
	}

	*_count = count;
	return B_OK;
}


status_t
NodeManager::GetDormantFlavorInfoFor(media_addon_id addOnID, int32 flavorID,
	dormant_flavor_info* flavorInfo)
{
	BAutolock _(this);

	for (DormantFlavorList::iterator iterator = fDormantFlavors.begin();
			iterator != fDormantFlavors.end(); iterator++) {
		dormant_add_on_flavor_info& info = *iterator;

		if (info.add_on_id == addOnID && info.flavor_id == flavorID
			&& info.info_valid) {
			*flavorInfo = info.info;
			return B_OK;
		}
	}

	return B_ERROR;
}


// #pragma mark - Misc.


void
NodeManager::CleanupTeam(team_id team)
{
	BAutolock _(this);

	fDefaultManager->CleanupTeam(team);

	PRINT(1, "NodeManager::CleanupTeam: team %ld\n", team);

	// TODO: send notifications after removing nodes

	// Cleanup node references

	for (NodeMap::iterator iterator = fNodeMap.begin();
			iterator != fNodeMap.end();) {
		registered_node& node = iterator->second;
		NodeMap::iterator remove = iterator++;

		// If the gone team was the creator of some global dormant node
		// instance, we now invalidate that we may want to remove that
		// global node, but I'm not sure
		if (node.creator == team) {
			node.creator = -1;
			// fall through
		}

		// If the team hosting this node is gone, remove node from database
		if (node.containing_team == team) {
			PRINT(1, "NodeManager::CleanupTeam: removing node id %ld, team "
				"%ld\n", node.node_id, team);
			fNodeMap.erase(remove);
			continue;
		}

		// Check the list of teams that have references to this node, and
		// remove the team
		TeamCountMap::iterator teamRef = node.team_ref_count.find(team);
		if (teamRef != node.team_ref_count.end()) {
			PRINT(1, "NodeManager::CleanupTeam: removing %ld refs from node "
				"id %ld, team %ld\n", teamRef->second, node.node_id, team);
			node.ref_count -= teamRef->second;
			if (node.ref_count == 0) {
				PRINT(1, "NodeManager::CleanupTeam: removing node id %ld that "
					"has no teams\n", node.node_id);

				fNodeMap.erase(remove);
			} else
				node.team_ref_count.erase(teamRef);
		}
	}

	// Cleanup add-on references

	for (size_t index = 0; index < fDormantFlavors.size(); index++) {
		dormant_add_on_flavor_info& flavorInfo = fDormantFlavors[index];

		TeamCountMap::iterator instanceCount
			= flavorInfo.team_instances_count.find(team);
		if (instanceCount != flavorInfo.team_instances_count.end()) {
			PRINT(1, "NodeManager::CleanupTeam: removing %ld instances from "
				"addon %ld, flavor %ld\n", instanceCount->second,
				flavorInfo.add_on_id, flavorInfo.flavor_id);

			flavorInfo.instances_count -= instanceCount->second;
			if (flavorInfo.instances_count <= 0)
				fDormantFlavors.erase(fDormantFlavors.begin() + index--);
			else
				flavorInfo.team_instances_count.erase(team);
		}
	}
}


status_t
NodeManager::LoadState()
{
	BAutolock _(this);
	return fDefaultManager->LoadState();
}

status_t
NodeManager::SaveState()
{
	BAutolock _(this);
	return fDefaultManager->SaveState(this);
}


void
NodeManager::Dump()
{
	BAutolock _(this);

	// for each addon-id, the add-on path map contains an entry_ref

	printf("\nNodeManager: addon path map follows:\n");

	for (PathMap::iterator iterator = fPathMap.begin();
			iterator != fPathMap.end(); iterator++) {
		BPath path(&iterator->second);
		printf(" addon-id %ld, path \"%s\"\n", iterator->first,
			path.InitCheck() == B_OK ? path.Path() : "INVALID");
	}

	printf("NodeManager: list end\n\n");

	// for each node-id, the registered node map contians information about
	// source of the node, users, etc.

	printf("NodeManager: registered nodes map follows:\n");
	for (NodeMap::iterator iterator = fNodeMap.begin();
			iterator != fNodeMap.end(); iterator++) {
		registered_node& node = iterator->second;

		printf("  node-id %ld, addon-id %ld, addon-flavor-id %ld, port %ld, "
			"creator %ld, team %ld, kinds %#08Lx, name \"%s\", ref_count %ld\n",
			node.node_id, node.add_on_id, node.flavor_id, node.port, 
			node.creator, node.containing_team, node.kinds, node.name,
			node.ref_count);

		printf("    teams (refcount): ");
		for (TeamCountMap::iterator refsIterator = node.team_ref_count.begin();
				refsIterator != node.team_ref_count.end(); refsIterator++) {
			printf("%ld (%ld), ", refsIterator->first, refsIterator->second);
		}
		printf("\n");

		for (InputList::iterator inIterator = node.input_list.begin();
				inIterator != node.input_list.end(); inIterator++) {
			media_input& input = *inIterator;
			printf("    media_input: node-id %ld, node-port %ld, source-port "
				"%ld, source-id  %ld, dest-port %ld, dest-id %ld, name "
				"\"%s\"\n", input.node.node, input.node.port, input.source.port,
				input.source.id, input.destination.port, input.destination.id,
				input.name);
		}
		if (node.input_list.empty())
			printf("    media_input: none\n");

		for (OutputList::iterator outIterator = node.output_list.begin();
				outIterator != node.output_list.end(); outIterator++) {
			media_output& output = *outIterator;
			printf("    media_output: node-id %ld, node-port %ld, source-port "
				"%ld, source-id  %ld, dest-port %ld, dest-id %ld, name "
				"\"%s\"\n", output.node.node, output.node.port,
				output.source.port, output.source.id, output.destination.port,
				output.destination.id, output.name);
		}
		if (node.output_list.empty())
			printf("    media_output: none\n");
	}

	printf("NodeManager: list end\n");
	printf("\n");

	// Dormant add-on flavors

	printf("NodeManager: dormant flavor list follows:\n");

	for (DormantFlavorList::iterator iterator = fDormantFlavors.begin();
			iterator != fDormantFlavors.end(); iterator++) {
		dormant_add_on_flavor_info& flavorInfo = *iterator;

		printf("  addon-id %ld, flavor-id %ld, max instances count %ld, "
			"instances count %ld, info valid %s\n",
			flavorInfo.add_on_id, flavorInfo.flavor_id,
			flavorInfo.max_instances_count, flavorInfo.instances_count,
			flavorInfo.info_valid ? "yes" : "no");
		printf("    teams (instances): ");
		for (TeamCountMap::iterator countIterator
					= flavorInfo.team_instances_count.begin();
				countIterator != flavorInfo.team_instances_count.end();
				countIterator++) {
			printf("%ld (%ld), ", countIterator->first, countIterator->second);
		}
		printf("\n");
		if (!flavorInfo.info_valid)
			continue;

		printf("    addon-id %ld, addon-flavor-id %ld, addon-name \"%s\"\n",
			flavorInfo.info.node_info.addon,
			flavorInfo.info.node_info.flavor_id,
			flavorInfo.info.node_info.name);
		printf("    flavor-kinds %#08Lx, flavor_flags %#08lx, internal_id %ld, "
			"possible_count %ld, in_format_count %ld, out_format_count %ld\n",
			flavorInfo.info.kinds, flavorInfo.info.flavor_flags,
			flavorInfo.info.internal_id, flavorInfo.info.possible_count,
			flavorInfo.info.in_format_count, flavorInfo.info.out_format_count);
		printf("    flavor-name \"%s\"\n", flavorInfo.info.name);
		printf("    flavor-info \"%s\"\n", flavorInfo.info.info);
	}
	printf("NodeManager: list end\n");

	fDefaultManager->Dump();
}


// #pragma mark - private methods


status_t
NodeManager::_AcquireNodeReference(media_node_id id, team_id team)
{
	TRACE("NodeManager::_AcquireNodeReference enter: node %ld, team %ld\n", id,
		team);

	BAutolock _(this);

	NodeMap::iterator found = fNodeMap.find(id);
	if (found == fNodeMap.end()) {
		ERROR("NodeManager::_AcquireNodeReference: node %ld not found\n", id);
		return B_ERROR;
	}

	registered_node& node = found->second;

	TeamCountMap::iterator teamRef = node.team_ref_count.find(team);
	if (teamRef == node.team_ref_count.end()) {
		// This is the team's first reference
		try {
			node.team_ref_count.insert(std::make_pair(team, 1));
		} catch (std::bad_alloc& exception) {
			return B_NO_MEMORY;
		}
	} else {
		// Just increase its ref count
		teamRef->second++;
	}

	node.ref_count++;

	TRACE("NodeManager::_AcquireNodeReference leave: node %ld, team %ld, "
		"ref %ld, team ref %ld\n", id, team, node.ref_count,
		node.team_ref_count.find(team)->second);
	return B_OK;
}
