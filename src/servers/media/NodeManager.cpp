#include <OS.h>
#include <Message.h>
#include <Messenger.h>
#include <MediaDefs.h>
#include <MediaAddOn.h>
#include "NodeManager.h"

// XXX locking is missing

NodeManager::NodeManager() :
	nextaddonid(1)
{
	fDormantFlavorList = new List<dormant_flavor_info>;
}

NodeManager::~NodeManager()
{
	delete fDormantFlavorList;
}

void 
NodeManager::RegisterAddon(media_addon_id *newid)
{
	*newid = nextaddonid++;
}

void
NodeManager::UnregisterAddon(media_addon_id id)
{
	RemoveDormantFlavorInfo(id);
	// unload the image once it's no longer used (refcounting!)
}

void
NodeManager::AddDormantFlavorInfo(const dormant_flavor_info &dfi)
{
	fDormantFlavorList->Insert(dfi);
}

void
NodeManager::RemoveDormantFlavorInfo(media_addon_id id)
{
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
	int32 maxcount;
	int32 index;
	dormant_flavor_info dfi;
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
	for (index = 0; (*io_count < maxcount) && fDormantFlavorList->GetAt(index, &dfi); index++) {
		if ((dfi.kinds & require_kinds) != require_kinds)
			continue;
		if ((dfi.kinds & deny_kinds) != 0)
			continue;
		if (namelen) {
			if (0 != memcmp(name, dfi.name, namelen))
				continue;
		}
		if (has_input) {
			bool hasit = false;
			for (int32 i = 0; i < dfi.in_format_count; i++)
				if (format_is_compatible(*has_input, dfi.in_formats[i])) {
					hasit = true;
					break;
				}
			if (!hasit)
				continue;
		}
		if (has_output) {
			bool hasit = false;
			for (int32 i = 0; i < dfi.out_format_count; i++)
				if (format_is_compatible(*has_output, dfi.out_formats[i])) {
					hasit = true;
					break;
				}
			if (!hasit)
				continue;
		}
		
		out_info[*io_count] = dfi.node_info;
		*io_count += 1;
	}

	return B_OK;
}

status_t 
NodeManager::GetDormantFlavorInfoFor(media_addon_id addon,
									 int32 flavor_id,
									 dormant_flavor_info *outFlavor)
{
	for (int32 index = 0; fDormantFlavorList->GetAt(index, outFlavor); index++) {
		if (outFlavor->node_info.addon == addon && outFlavor->node_info.flavor_id == flavor_id)
			return B_OK;
	}
	return B_ERROR;
}

