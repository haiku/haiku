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


/*!	This is a management class for dormant media nodes.
	It is private to the media kit and only accessed by the BMediaRoster class
	and the media_addon_server.
	It handles loading/unloading of dormant nodes.

	Dormant media nodes can be instantiated on demand. The reside on harddisk in
	the directories /boot/beos/system/add-ons/media
	and /boot/home/config/add-ons/media.
	Multiple media nodes can be included in one file, they can be accessed using
	the BMediaAddOn that each file implements.
	The BMediaAddOn allows getting a list of supported flavors. Each flavor
	represents a media node.
	The media_addon_server does the initial scanning of files and getting the
	list of supported flavors. It uses the flavor_info to do this, and reports
	the list of flavors to the media_server packed into individual
	dormant_media_node structures.
*/


#include "DormantNodeManager.h"

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Entry.h>
#include <Path.h>

#include <MediaDebug.h>
#include <MediaMisc.h>
#include <ServerInterface.h>
#include <DataExchange.h>


namespace BPrivate {
namespace media {


DormantNodeManager* gDormantNodeManager;
	// initialized by BMediaRoster.


DormantNodeManager::DormantNodeManager()
	:
	fLock("dormant node manager locker")
{
}


DormantNodeManager::~DormantNodeManager()
{
	// force unloading all currently loaded images

	AddOnMap::iterator iterator = fAddOnMap.begin();
	for (; iterator != fAddOnMap.end(); iterator++) {
		loaded_add_on_info& info = iterator->second;

		ERROR("Forcing unload of add-on id %" B_PRId32 " with usecount %"
			B_PRId32 "\n", info.add_on->AddonID(), info.use_count);
		_UnloadAddOn(info.add_on, info.image);
	}
}


BMediaAddOn*
DormantNodeManager::GetAddOn(media_addon_id id)
{
	TRACE("DormantNodeManager::GetAddon, id %" B_PRId32 "\n", id);

	// first try to use a already loaded add-on
	BMediaAddOn* addOn = _LookupAddOn(id);
	if (addOn != NULL)
		return addOn;

	// Be careful, we avoid locking here!

	// ok, it's not loaded, try to get the path
	BPath path;
	if (FindAddOnPath(&path, id) != B_OK) {
		ERROR("DormantNodeManager::GetAddon: can't find path for add-on %"
			B_PRId32 "\n", id);
		return NULL;
	}

	// try to load it
	BMediaAddOn* newAddOn;
	image_id image;
	if (_LoadAddOn(path.Path(), id, &newAddOn, &image) != B_OK) {
		ERROR("DormantNodeManager::GetAddon: can't load add-on %" B_PRId32
			" from path %s\n",id, path.Path());
		return NULL;
	}

	// ok, we successfully loaded it. Now lock and insert it into the map,
	// or unload it if the map already contains one that was loaded by another
	// thread at the same time

	BAutolock _(fLock);

	addOn = _LookupAddOn(id);
	if (addOn == NULL) {
		// we use the loaded one
		addOn = newAddOn;

		// and save it into the list
		loaded_add_on_info info;
		info.add_on = newAddOn;
		info.image = image;
		info.use_count = 1;
		try {
			fAddOnMap.insert(std::make_pair(id, info));
		} catch (std::bad_alloc& exception) {
			_UnloadAddOn(newAddOn, image);
			return NULL;
		}
	} else
		_UnloadAddOn(newAddOn, image);

	ASSERT(addOn->AddonID() == id);
	return addOn;
}


void
DormantNodeManager::PutAddOn(media_addon_id id)
{
	TRACE("DormantNodeManager::PutAddon, id %" B_PRId32 "\n", id);

	BAutolock locker(fLock);

	AddOnMap::iterator found = fAddOnMap.find(id);
	if (found == fAddOnMap.end()) {
		ERROR("DormantNodeManager::PutAddon: failed to find add-on %" B_PRId32
			"\n", id);
		return;
	}

	loaded_add_on_info& info = found->second;

	if (--info.use_count == 0) {
		// unload add-on

		BMediaAddOn* addOn = info.add_on;
		image_id image = info.image;
		fAddOnMap.erase(found);

		locker.Unlock();
		_UnloadAddOn(addOn, image);
	}
}


void
DormantNodeManager::PutAddOnDelayed(media_addon_id id)
{
	// Called from a node destructor of the loaded media-add-on.
	// We must make sure that the media-add-on stays in memory
	// a couple of seconds longer.

	UNIMPLEMENTED();
}


//!	For use by media_addon_server only
media_addon_id
DormantNodeManager::RegisterAddOn(const char* path)
{
	TRACE("DormantNodeManager::RegisterAddon, path %s\n", path);

	entry_ref ref;
	status_t status = get_ref_for_path(path, &ref);
	if (status != B_OK) {
		ERROR("DormantNodeManager::RegisterAddon failed, couldn't get ref "
			"for path %s: %s\n", path, strerror(status));
		return 0;
	}

	server_register_add_on_request request;
	request.ref = ref;

	server_register_add_on_reply reply;
	status = QueryServer(SERVER_REGISTER_ADD_ON, &request, sizeof(request),
		&reply, sizeof(reply));
	if (status != B_OK) {
		ERROR("DormantNodeManager::RegisterAddon failed, couldn't talk to "
			"media server: %s\n", strerror(status));
		return 0;
	}

	TRACE("DormantNodeManager::RegisterAddon finished with id %" B_PRId32 "\n",
		reply.add_on_id);

	return reply.add_on_id;
}


//!	For use by media_addon_server only
void
DormantNodeManager::UnregisterAddOn(media_addon_id id)
{
	TRACE("DormantNodeManager::UnregisterAddon id %" B_PRId32 "\n", id);
	ASSERT(id > 0);

	port_id port = find_port(MEDIA_SERVER_PORT_NAME);
	if (port < 0)
		return;

	server_unregister_add_on_command msg;
	msg.add_on_id = id;
	write_port(port, SERVER_UNREGISTER_ADD_ON, &msg, sizeof(msg));
}


status_t
DormantNodeManager::FindAddOnPath(BPath* path, media_addon_id id)
{
	server_get_add_on_ref_request request;
	request.add_on_id = id;

	server_get_add_on_ref_reply reply;
	status_t status = QueryServer(SERVER_GET_ADD_ON_REF, &request,
		sizeof(request), &reply, sizeof(reply));
	if (status != B_OK)
		return status;

	entry_ref ref = reply.ref;
	return path->SetTo(&ref);
}


BMediaAddOn*
DormantNodeManager::_LookupAddOn(media_addon_id id)
{
	BAutolock _(fLock);

	AddOnMap::iterator found = fAddOnMap.find(id);
	if (found == fAddOnMap.end())
		return NULL;

	loaded_add_on_info& info = found->second;

	ASSERT(id == info.add_on->AddonID());
	info.use_count++;

	return info.add_on;
}


status_t
DormantNodeManager::_LoadAddOn(const char* path, media_addon_id id,
	BMediaAddOn** _newAddOn, image_id* _newImage)
{
	image_id image = load_add_on(path);
	if (image < 0) {
		ERROR("DormantNodeManager::LoadAddon: loading \"%s\" failed: %s\n",
			path, strerror(image));
		return image;
	}

	BMediaAddOn* (*makeAddOn)(image_id);
	status_t status = get_image_symbol(image, "make_media_addon",
		B_SYMBOL_TYPE_TEXT, (void**)&makeAddOn);
	if (status != B_OK) {
		ERROR("DormantNodeManager::LoadAddon: loading failed, function not "
			"found: %s\n", strerror(status));
		unload_add_on(image);
		return status;
	}

	BMediaAddOn* addOn = makeAddOn(image);
	if (addOn == NULL) {
		ERROR("DormantNodeManager::LoadAddon: creating BMediaAddOn failed\n");
		unload_add_on(image);
		return B_ERROR;
	}

	ASSERT(addOn->ImageID() == image);
		// this should be true for a well behaving add-ons

	// We are a friend class of BMediaAddOn and initialize these member
	// variables
	addOn->fAddon = id;
	addOn->fImage = image;

	// everything ok
	*_newAddOn = addOn;
	*_newImage = image;

	return B_OK;
}


void
DormantNodeManager::_UnloadAddOn(BMediaAddOn* addOn, image_id image)
{
	ASSERT(addOn != NULL);
	ASSERT(addOn->ImageID() == image);
		// if this fails, something bad happened to the add-on

	delete addOn;
	unload_add_on(image);
}


}	// namespace media
}	// namespace BPrivate
