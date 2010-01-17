/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H


#include <map>
#include <vector>

#include <Locker.h>

#include "TStack.h"
#include "DataExchange.h"


class DefaultManager;
class BufferManager;


typedef std::map<team_id, int32> TeamCountMap;
typedef std::vector<media_input> InputList;
typedef std::vector<media_output> OutputList;
typedef std::vector<live_node_info> LiveNodeList;


struct registered_node {
	media_node_id			node_id;
	media_addon_id			add_on_id;
	int32					flavor_id;
	char					name[B_MEDIA_NAME_LENGTH];
	uint64					kinds;
	port_id					port;
	team_id					creator;	// team that created the node
	team_id					containing_team;
	int32					ref_count;
	TeamCountMap			team_ref_count;
	InputList				input_list;
	OutputList				output_list;
};

struct dormant_add_on_flavor_info {
	media_addon_id			add_on_id;
	int32					flavor_id;

	int32					max_instances_count;
	int32					instances_count;

	TeamCountMap		 	team_instances_count;

	bool					info_valid;
	dormant_flavor_info		info;
};


class NodeManager : BLocker {
public:
								NodeManager();
								~NodeManager();

	// Management of system wide default nodes
			status_t			SetDefaultNode(node_type type,
									const media_node* node,
									const dormant_node_info* info,
									const media_input* input);
			status_t			GetDefaultNode(node_type type,
									media_node_id* _nodeID, char* inputName,
									int32* _inputID);
			status_t			RescanDefaultNodes();

	// Management of live nodes
			status_t			RegisterNode(media_addon_id addOnID,
									int32 flavorID, const char* name,
									uint64 kinds, port_id port, team_id team,
									media_node_id* _nodeID);
			status_t			UnregisterNode(media_node_id nodeID,
									team_id team, media_addon_id* addOnID,
									int32* _flavorID);
			status_t			ReleaseNodeReference(media_node_id id,
									team_id team);
			status_t			ReleaseNodeAll(media_node_id id);
			status_t			GetCloneForID(media_node_id id, team_id team,
									media_node* node);
			status_t			GetClone(node_type type, team_id team,
									media_node* node, char* inputName,
									int32* _id);
			status_t			ReleaseNode(const media_node& node,
									team_id team);
			status_t			PublishInputs(const media_node& node,
									const media_input* inputs, int32 count);
			status_t			PublishOutputs(const media_node& node,
									const media_output* outputs, int32 count);
			status_t			FindNodeID(port_id port, media_node_id* _id);
			status_t			GetLiveNodeInfo(const media_node& node,
									live_node_info* liveInfo);
			status_t			GetInstances(media_addon_id addOnID,
									int32 flavorID, media_node_id* ids,
									int32* _count, int32 maxCount);
			status_t			GetLiveNodes(LiveNodeList& liveNodes,
									int32 maxCount,
									const media_format* inputFormat = NULL,
									const media_format* outputFormat = NULL,
									const char* name = NULL,
									uint64 requireKinds = 0);
			status_t			GetDormantNodeInfo(const media_node& node,
									dormant_node_info* nodeInfo);
			status_t			SetNodeCreator(media_node_id id,
									team_id creator);

	// Add media_node_id of all live nodes to the message
	// int32 "media_node_id" (multiple items)
			status_t			GetLiveNodes(BMessage* message);

			void				RegisterAddOn(const entry_ref& ref,
									media_addon_id* _newID);
			void				UnregisterAddOn(media_addon_id id);

			status_t			AddDormantFlavorInfo(
									const dormant_flavor_info& flavorInfo);
			void				InvalidateDormantFlavorInfo(media_addon_id id);
			void				RemoveDormantFlavorInfo(media_addon_id id);
			void				CleanupDormantFlavorInfos();

			status_t			IncrementFlavorInstancesCount(
									media_addon_id addOnID, int32 flavorID,
									team_id team);
			status_t			DecrementFlavorInstancesCount(
									media_addon_id addOnID, int32 flavorID,
									team_id team);

			status_t			GetAddOnRef(media_addon_id addOnID,
									entry_ref* ref);
			status_t			GetDormantNodes(dormant_node_info* infos,
									int32* _count, const media_format* hasInput,
									const media_format* hasOutput,
									const char* name, uint64 requireKinds,
									uint64 denyKinds);

			status_t			GetDormantFlavorInfoFor(media_addon_id addOnID,
									 int32 flavorID,
									 dormant_flavor_info* flavorInfo);

			void				CleanupTeam(team_id team);

			status_t			LoadState();
			status_t			SaveState();

			void				Dump();

private:
			status_t			_AcquireNodeReference(media_node_id id,
									team_id team);

private:
			typedef std::map<media_addon_id, registered_node> NodeMap;
			typedef std::vector<dormant_add_on_flavor_info> DormantFlavorList;
			typedef std::map<media_addon_id, entry_ref> PathMap;

			media_addon_id		fNextAddOnID;
			media_node_id		fNextNodeID;

			DormantFlavorList	fDormantFlavors;
			PathMap				fPathMap;
			NodeMap				fNodeMap;
			DefaultManager*		fDefaultManager;
};

#endif	// NODE_MANAGER_H
