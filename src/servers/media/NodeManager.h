/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "TList.h"
#include "TMap.h"
#include "TStack.h"
#include "DataExchange.h"

struct registered_node
{
	media_node_id nodeid;
	media_addon_id addon_id;
	int32 addon_flavor_id;
	char name[B_MEDIA_NAME_LENGTH];
	uint64 kinds;
	port_id port;
	team_id team;
	int32 globalrefcount;
	Map<team_id, int32> teamrefcount;
	List<media_input> inputlist;
	List<media_output> outputlist;
};

class BufferManager;

class NodeManager
{
public:
	NodeManager();
	~NodeManager();
	status_t RegisterNode(BMessenger, media_node &, char const *, long *, char const *, long, char const *, long, media_type, media_type);
	status_t UnregisterNode(long);
	status_t GetNodes(BMessage &, char const *);
	status_t GetLiveNode(BMessage &, char const *, long);
	status_t GetLiveNodes(BMessage &, char const *, media_format const *, media_format const *, char const *, unsigned long);
	status_t FindNode(long, media_node &);
	status_t FindSaveInfo(long, char const **, long *, long *, char const **);
	status_t FindDormantNodeFor(long, dormant_node_info *);
	status_t FindNodesFor(long, long, BMessage &, char const *);
	status_t FindNodesForPort(long, BMessage &, char const *);
	status_t UnregisterTeamNodes(long, BMessage &, char const *, long *, BufferManager *);
	status_t DumpGlobalReferences(BMessage &, char const *);
	status_t BroadcastMessage(long, void *, long, long long);
	status_t LoadState();
	status_t SaveState();


	status_t RegisterNode(media_node_id *nodeid, media_addon_id addon_id, int32 addon_flavor_id, const char *name, uint64 kinds, port_id port, team_id team);
	status_t UnregisterNode(media_addon_id *addon_id, media_node_id nodeid, team_id team);
	status_t GetCloneForId(media_node *node, media_node_id nodeid, team_id team);
	status_t GetClone(media_node *node, char *input_name, int32 *input_id, node_type type, team_id team);
	status_t ReleaseNode(const media_node &node, team_id team);
	status_t PublishInputs(const media_node &node, const media_input *inputs, int32 count);
	status_t PublishOutputs(const media_node &node, const media_output *outputs, int32 count);
	status_t FindNodeId(media_node_id *nodeid, port_id port);
	status_t GetLiveNodeInfo(live_node_info *live_info, const media_node &node);
	status_t GetInstances(media_node_id *node_ids, int32* count, int32 maxcount, media_addon_id addon_id, int32 addon_flavor_id);
	status_t GetLiveNodes(Stack<live_node_info> *livenodes,	int32 maxcount, const media_format *inputformat = NULL, const media_format *outputformat = NULL, const char* name = NULL, uint64 require_kinds = 0);
	status_t GetDormantNodeInfo(dormant_node_info *node_info, const media_node &node);
	status_t IncrementGlobalRefCount(media_node_id nodeid, team_id team);
	status_t DecrementGlobalRefCount(media_node_id nodeid, team_id team);

	/* Add media_node_id of all live nodes to the message
	 * int32 "media_node_id" (multiple items)
	 */
	status_t GetLiveNodes(BMessage *msg);
	
	void AddDormantFlavorInfo(const dormant_flavor_info &dfi);	
	void RemoveDormantFlavorInfo(media_addon_id id);	
	void RegisterAddon(const entry_ref &ref, media_addon_id *newid);
	void UnregisterAddon(media_addon_id id);
	status_t GetAddonRef(entry_ref *ref, media_addon_id id);
	status_t GetDormantNodes(dormant_node_info * out_info,
							  int32 * io_count,
							  const media_format * has_input /* = NULL */,
							  const media_format * has_output /* = NULL */,
							  const char * name /* = NULL */,
							  uint64 require_kinds /* = NULL */,
							  uint64 deny_kinds /* = NULL */);

	status_t GetDormantFlavorInfoFor(media_addon_id addon,
									 int32 flavor_id,
									 dormant_flavor_info *outFlavor);

	void CleanupTeam(team_id team);
	
private:
	media_addon_id nextaddonid;
	media_node_id nextnodeid;
	
	BLocker *fLocker;
	List<dormant_flavor_info> *fDormantFlavorList;
	Map<media_addon_id,entry_ref> *fAddonPathMap;
	Map<media_node_id,registered_node> *fRegisteredNodeMap;
};
