/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DORMANT_NODE_MANAGER_H
#define _DORMANT_NODE_MANAGER_H


#include <map>

#include <Locker.h>
#include <MediaAddOn.h>


class BPath;


namespace BPrivate {
namespace media {

// To instantiate a dormant node in the current address space, we need to
// either load the add-on from file and create a new BMediaAddOn class and
// cache the BMediaAddOn after that for later reuse, or reuse the cached
// BMediaAddOn if we already have one.
// This is handled by the DormantNodeManager.
//
// GetAddon() will provide a ready to use BMediaAddOn, while
// PutAddonDelayed() will unload it when it's no longer needed,
// but will delay the unloading slightly, because it is called
// from a node destructor of the loaded add-on.

class DormantNodeManager {
public:
								DormantNodeManager();
								~DormantNodeManager();

	// Be careful, GetAddon and PutAddon[Delayed] must be balanced.
			BMediaAddOn*		GetAddOn(media_addon_id id);
			void				PutAddOn(media_addon_id id);
			void				PutAddOnDelayed(media_addon_id id);

	// For use by media_addon_server only
			media_addon_id		RegisterAddOn(const char* path);
			void				UnregisterAddOn(media_addon_id id);

	// query the server for the path
			status_t			FindAddOnPath(BPath* path, media_addon_id id);

private:
			struct loaded_add_on_info {
				BMediaAddOn*	add_on;
				image_id		image;
				int32			use_count;
			};

	// returns the addon or NULL if it needs to be loaded
			BMediaAddOn*		_LookupAddOn(media_addon_id id);

	// manage loading and unloading add-ons from images
			status_t			_LoadAddOn(const char* path, media_addon_id id,
									BMediaAddOn** _newAddOn,
									image_id* _newImage);
			void				_UnloadAddOn(BMediaAddOn* addOn,
									image_id image);

private:
			typedef std::map<media_addon_id, loaded_add_on_info> AddOnMap;

			BLocker				fLock;
			AddOnMap			fAddOnMap;
};


extern DormantNodeManager* gDormantNodeManager;


}	// namespace media
}	// namespace BPrivate


using BPrivate::media::gDormantNodeManager;


#endif /* _DORMANT_NODE_MANAGER_H */
