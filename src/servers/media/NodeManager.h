#include "TList.h"

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
	status_t IncrementGlobalRefCount(long);
	status_t DumpGlobalReferences(BMessage &, char const *);
	status_t DecrementGlobalRefCount(long, BMessage *);
	status_t BroadcastMessage(long, void *, long, long long);
	status_t LoadState();
	status_t SaveState();

	void AddDormantFlavorInfo(const dormant_flavor_info &dfi);	
	void RemoveDormantFlavorInfo(media_addon_id id);	
	void RegisterAddon(media_addon_id *newid);
	void UnregisterAddon(media_addon_id id);
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

private:
	media_addon_id nextaddonid;
	
	List<dormant_flavor_info> *fDormantFlavorList;
};
