/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _DORMANT_NODE_MANAGER_H
#define _DORMANT_NODE_MANAGER_H

#include "TMap.h"

namespace BPrivate {
namespace media {

class DormantNodeManager
{
public:
	DormantNodeManager();
	~DormantNodeManager();

//	InstantiateDormantNode(const dormant_node_info & in_info, bool global, media_node * out_node);

	// Be careful, GetAddon and PutAddon must be balanced.
	BMediaAddOn *GetAddon(media_addon_id id);
	void PutAddon(media_addon_id id);

	// For use by media_addon_server only
	media_addon_id RegisterAddon(const char *path);
	void UnregisterAddon(media_addon_id id);

private:
	struct loaded_addon_info
	{
		BMediaAddOn *addon;
		image_id image;
		int32 usecount;
	};

	// returns the addon or NULL if it needs to be loaded
	BMediaAddOn *TryGetAddon(media_addon_id id);
	
	// manage loading and unloading add-ons from images
	status_t LoadAddon(BMediaAddOn **newaddon, image_id *newimage, const char *path, media_addon_id id);
	void UnloadAddon(BMediaAddOn *addon, image_id image);

	// query the server for the path
	status_t FindAddonPath(BPath *path, media_addon_id id);
	
private:

	Map<media_addon_id,loaded_addon_info> *fAddonmap;
	BLocker *fLock;
};

}; // namespace media
}; // namespace BPrivate

extern BPrivate::media::DormantNodeManager *_DormantNodeManager;

#endif /* _DORMANT_NODE_MANAGER_H */
