/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/* This is a management class for dormant media nodes.
 * It is private to the media kit and only accessed by the BMediaRoster class
 * and the media_addon_server.
 * It handles loading/unloading of dormant nodes.
 *
 * Dormant media nodes can be instantiated on demand. The reside on harddisk in the
 * directories /boot/beos/system/add-ons/media and /boot/home/config/add-ons/media
 * Multiple media nodes can be included in one file, they can be accessed using the 
 * BMediaAddOn that each file implements.
 * The BMediaAddOn allows getting a list of supported flavors. Each flavor represents
 * a media node. 
 * The media_addon_server does the initial scanning of files and getting the list
 * of supported flavors. It uses the flavor_info to do this, and reports the list
 * of flavors to the media_server packed into individual dormant_media_node
 * structures.
 */

#include <stdio.h>
#include <string.h>
#include <Locker.h>
#include <Path.h>
#include <Entry.h>
#include <MediaAddOn.h>
#include <Debug.h>
#include "debug.h"
#include "PortPool.h"
#include "ServerInterface.h"
#include "DataExchange.h"
#include "DormantNodeManager.h"

static BPrivate::media::DormantNodeManager manager;
BPrivate::media::DormantNodeManager *_DormantNodeManager = &manager;

namespace BPrivate {
namespace media {

DormantNodeManager::DormantNodeManager()
{
	fLock = new BLocker();
	fAddonmap = new Map<media_addon_id,loaded_addon_info>;
}


DormantNodeManager::~DormantNodeManager()
{
	delete fLock;

	// force unloading all currently loaded images
	loaded_addon_info *info;
	for (int32 index = 0; fAddonmap->GetPointerAt(index,&info); index++) {
		printf("Forcing unload of add-on id %ld with usecount %ld\n",info->addon->AddonID(), info->usecount);
		UnloadAddon(info->addon, info->image);
	}
	
	delete fAddonmap;
}


BMediaAddOn *
DormantNodeManager::TryGetAddon(media_addon_id id)
{
	loaded_addon_info *info;
	BMediaAddOn *addon;
	
	fLock->Lock();
	if (fAddonmap->GetPointer(id,&info)) {
		info->usecount += 1;
		addon = info->addon;
		ASSERT(id == addon->AddonID());
	} else {
		addon = NULL;
	}
	fLock->Unlock();
	return addon;
}


BMediaAddOn *
DormantNodeManager::GetAddon(media_addon_id id)
{
	BMediaAddOn *addon;
	
	printf("DormantNodeManager::GetAddon, id %ld\n",id);
	
	// first try to use a already loaded add-on
	addon = TryGetAddon(id);
	if (addon)
		return addon;
		
	// Be careful, we avoid locking here!

	// ok, it's not loaded, try to get the path
	BPath path;
	if (B_OK != FindAddonPath(&path, id)) {
		printf("DormantNodeManager::GetAddon: can't find path for add-on %ld\n",id);
		return NULL;
	}
	
	// try to load it
	BMediaAddOn *newaddon;
	image_id image;
	if (B_OK != LoadAddon(&newaddon, &image, path.Path(), id)) {
		printf("DormantNodeManager::GetAddon: can't load add-on %ld from path %s\n",id, path.Path());
		return NULL;
	}
	
	// ok, we successfully loaded it. Now lock and insert it into the map,
	// or unload it if the map already contains one that was loaded by another
	// thread at the same time
	fLock->Lock();
	addon = TryGetAddon(id);
	if (addon) {
		UnloadAddon(newaddon, image);
	} else {
		// we use the loaded one
		addon = newaddon;
		// and save it into the list
		loaded_addon_info info;
		info.addon = newaddon;
		info.image = image;
		info.usecount = 1;
		fAddonmap->Insert(id, info);
	}
	fLock->Unlock();
	ASSERT(addon->Addon() == id);
	return addon;
}


void
DormantNodeManager::PutAddon(media_addon_id id)
{
	loaded_addon_info *info;
	BMediaAddOn *addon = 0; /* avoid compiler warning */
	image_id image = 0; /* avoid compiler warning */
	bool unload;

	printf("DormantNodeManager::PutAddon, id %ld\n",id);
	
	fLock->Lock();
	if (!fAddonmap->GetPointer(id, &info)) {
		printf("DormantNodeManager::PutAddon: failed to find add-on %ld\n",id);
		fLock->Unlock();
		return;
	}
	info->usecount -= 1;
	unload = (info->usecount == 0);
	if (unload) {
		addon	= info->addon;
		image	= info->image;
		fAddonmap->Remove(id);
	}
	fLock->Unlock();

	if (unload)
		UnloadAddon(addon, image);
}


media_addon_id
DormantNodeManager::RegisterAddon(const char *path)
{
	server_register_mediaaddon_request msg;
	server_register_mediaaddon_reply reply;
	port_id port;
	status_t rv;
	int32 code;
	entry_ref tempref;
	
	printf("DormantNodeManager::RegisterAddon, path %s\n",path);
	
	rv = get_ref_for_path(path, &tempref);
	if (rv != B_OK)
		return 0;
	msg.ref = tempref;
	port = find_port("media_server port");
	if (port <= B_OK)
		return 0;
	msg.reply_port = _PortPool->GetPort();
	rv = write_port(port, SERVER_REGISTER_MEDIAADDON, &msg, sizeof(msg));
	if (rv != B_OK) {
		_PortPool->PutPort(msg.reply_port);
		return 0;
	}
	rv = read_port(msg.reply_port, &code, &reply, sizeof(reply));
	_PortPool->PutPort(msg.reply_port);
	if (rv < B_OK)
		return 0;

	printf("DormantNodeManager::RegisterAddon finished with id %ld\n",reply.addonid);

	return reply.addonid;
}


void
DormantNodeManager::UnregisterAddon(media_addon_id id)
{
	ASSERT(id > 0);
	server_unregister_mediaaddon_command msg;

	printf("DormantNodeManager::UnregisterAddon id %ld\n",id);

	port_id port;
	port = find_port("media_server port");
	if (port <= B_OK)
		return;
	msg.addonid = id;
	write_port(port, SERVER_UNREGISTER_MEDIAADDON, &msg, sizeof(msg));
}


status_t
DormantNodeManager::FindAddonPath(BPath *path, media_addon_id id)
{
	server_get_mediaaddon_ref_request msg;
	server_get_mediaaddon_ref_reply reply;
	port_id port;
	entry_ref tempref;
	status_t rv;
	int32 code;
	port = find_port("media_server port");
	if (port <= B_OK)
		return B_ERROR;
	msg.addonid = id;
	msg.reply_port = _PortPool->GetPort();
	rv = write_port(port, SERVER_GET_MEDIAADDON_REF, &msg, sizeof(msg));
	if (rv != B_OK) {
		_PortPool->PutPort(msg.reply_port);
		return B_ERROR;
	}
	rv = read_port(msg.reply_port, &code, &reply, sizeof(reply));
	_PortPool->PutPort(msg.reply_port);
	if (rv < B_OK)
		return B_ERROR;

	tempref = reply.ref;
	return path->SetTo(&tempref);
}


status_t
DormantNodeManager::LoadAddon(BMediaAddOn **newaddon, image_id *newimage, const char *path, media_addon_id id)
{
	BMediaAddOn *(*make_addon)(image_id you);
	BMediaAddOn *addon;
	image_id image;
	status_t rv;
	
	image = load_add_on(path);
	if (image < B_OK) {
		printf("DormantNodeManager::LoadAddon: loading failed %lx %s\n", image, strerror(image));
		return B_ERROR;
	}
	
	rv = get_image_symbol(image, "make_media_addon", B_SYMBOL_TYPE_TEXT, (void**)&make_addon);
	if (rv < B_OK) {
		printf("DormantNodeManager::LoadAddon: loading failed, function not found %lx %s\n", rv, strerror(rv));
		unload_add_on(image);
		return B_ERROR;
	}
	
	addon = make_addon(image);
	if (addon == 0) {
		printf("DormantNodeManager::LoadAddon: creating BMediaAddOn failed\n");
		unload_add_on(image);
		return B_ERROR;
	}
	
	ASSERT(addon->Image() == image); // this should be true for a well behaving add-on

	// everything ok
	*newaddon = addon;
	*newimage = image;
	
	// we are a friend class of BMediaAddOn and initilize these member variables
	addon->fAddon = id;
	addon->fImage = image;

	return B_OK;
}


void
DormantNodeManager::UnloadAddon(BMediaAddOn *addon, image_id image)
{
	ASSERT(addon);
	ASSERT(addon->Image() == image); // if this failes, something bad happened to the add-on
	delete addon;
	unload_add_on(image);
}

}; // namespace media
}; // namespace BPrivate

