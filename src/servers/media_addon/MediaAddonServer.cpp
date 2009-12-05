/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

/*
 * Copyright (c) 2002-2004, Marcus Overhagen <marcus@overhagen.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <map>
#include <stdio.h>
#include <vector>

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <debug.h>
#include <DataExchange.h>
#include <DormantNodeManager.h>
#include <MediaMisc.h>
#include <MediaRosterEx.h>
#include <MediaSounds.h>
#include <Notifications.h>
#include <ServerInterface.h>

#include "MediaFilePlayer.h"
#include "SystemTimeSource.h"


//#define USER_ADDON_PATH "../add-ons/media"


typedef std::vector<media_node> NodeVector;


struct AddOnInfo {
	media_addon_id		id;
	bool				wants_autostart;
	int32				flavor_count;

	NodeVector			active_flavors;

	BMediaAddOn*		addon;
		// if != NULL, need to call _DormantNodeManager->PutAddon(id)
};


class MediaAddonServer : BApplication {
public:
								MediaAddonServer(const char* signature);
	virtual						~MediaAddonServer();
	virtual	void				ReadyToRun();
	virtual bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);

private:
			void				_WatchDir(BEntry* dir);
			void				_AddOnAdded(const char* path, ino_t fileNode);
			void				_AddOnRemoved(ino_t fileNode);
			void				_HandleMessage(int32 code, const void* data,
									size_t size);

			void				_PutAddonIfPossible(AddOnInfo& info);
			void				_InstantiatePhysicalInputsAndOutputs(
									AddOnInfo& info);
			void				_InstantiateAutostartFlavors(AddOnInfo& info);
			void				_DestroyInstantiatedFlavors(AddOnInfo& info);

			void				_ScanAddOnFlavors(BMediaAddOn* addOn);

			port_id				_ControlPort() const { return fControlPort; }

	static	status_t			_ControlThread(void* arg);

private:
	typedef std::map<ino_t, media_addon_id> FileMap;
	typedef std::map<media_addon_id, AddOnInfo> InfoMap;

			FileMap				fFileMap;
			InfoMap				fInfoMap;

			BMediaRoster*		fMediaRoster;
			ino_t				fSystemAddOnsNode;
			ino_t				fUserAddOnsNode;
			port_id				fControlPort;
			thread_id			fControlThread;
			bool				fStartup;
			bool				fStartupSound;
};


#if DEBUG >= 2
static void
DumpFlavorInfo(const flavor_info* info)
{
	printf("  name = %s\n", info->name);
	printf("  info = %s\n", info->info);
	printf("  internal_id = %ld\n", info->internal_id);
	printf("  possible_count = %ld\n", info->possible_count);
	printf("  flavor_flags = 0x%lx", info->flavor_flags);
	if (info->flavor_flags & B_FLAVOR_IS_GLOBAL) printf(" B_FLAVOR_IS_GLOBAL");
	if (info->flavor_flags & B_FLAVOR_IS_LOCAL) printf(" B_FLAVOR_IS_LOCAL");
	printf("\n");
	printf("  kinds = 0x%Lx", info->kinds);
	if (info->kinds & B_BUFFER_PRODUCER) printf(" B_BUFFER_PRODUCER");
	if (info->kinds & B_BUFFER_CONSUMER) printf(" B_BUFFER_CONSUMER");
	if (info->kinds & B_TIME_SOURCE) printf(" B_TIME_SOURCE");
	if (info->kinds & B_CONTROLLABLE) printf(" B_CONTROLLABLE");
	if (info->kinds & B_FILE_INTERFACE) printf(" B_FILE_INTERFACE");
	if (info->kinds & B_ENTITY_INTERFACE) printf(" B_ENTITY_INTERFACE");
	if (info->kinds & B_PHYSICAL_INPUT) printf(" B_PHYSICAL_INPUT");
	if (info->kinds & B_PHYSICAL_OUTPUT) printf(" B_PHYSICAL_OUTPUT");
	if (info->kinds & B_SYSTEM_MIXER) printf(" B_SYSTEM_MIXER");
	printf("\n");
	printf("  in_format_count = %ld\n", info->in_format_count);
	printf("  out_format_count = %ld\n", info->out_format_count);
}
#endif


// #pragma mark -


MediaAddonServer::MediaAddonServer(const char* signature)
	:
	BApplication(signature),
	fStartup(true),
	fStartupSound(true)
{
	CALLED();
	fMediaRoster = BMediaRoster::Roster();
	fControlPort = create_port(64, MEDIA_ADDON_SERVER_PORT_NAME);
	fControlThread = spawn_thread(_ControlThread, "media_addon_server control",
		B_NORMAL_PRIORITY + 2, this);
	resume_thread(fControlThread);
}


MediaAddonServer::~MediaAddonServer()
{
	CALLED();

	delete_port(fControlPort);
	wait_for_thread(fControlThread, NULL);

	// unregister all media add-ons
	FileMap::iterator iterator = fFileMap.begin();
	for (; iterator != fFileMap.end(); iterator++)
		_DormantNodeManager->UnregisterAddon(iterator->second);

	// TODO: unregister system time source
}


void
MediaAddonServer::ReadyToRun()
{
	if (!be_roster->IsRunning("application/x-vnd.Be.media-server")) {
		// the media server is not running, let's quit
		fprintf(stderr, "The media_server is not running!\n");
		Quit();
		return;
	}

	// the control thread is already running at this point,
	// so we can talk to the media server and also receive
	// commands for instantiation

	ASSERT(fStartup == true);

	// The very first thing to do is to create the system time source,
	// register it with the server, and make it the default SYSTEM_TIME_SOURCE
	BMediaNode *timeSource = new SystemTimeSource;
	status_t result = fMediaRoster->RegisterNode(timeSource);
	if (result != B_OK) {
		fprintf(stderr, "Can't register system time source : %s\n",
			strerror(result));
		debugger("Can't register system time source");
	}

	if (timeSource->ID() != NODE_SYSTEM_TIMESOURCE_ID)
		debugger("System time source got wrong node ID");
	media_node node = timeSource->Node();
	result = MediaRosterEx(fMediaRoster)->SetNode(SYSTEM_TIME_SOURCE, &node);
	if (result != B_OK)
		debugger("Can't setup system time source as default");

	// During startup, first all add-ons are loaded, then all
	// nodes (flavors) representing physical inputs and outputs
	// are instantiated. Next, all add-ons that need autostart
	// will be autostarted. Finally, add-ons that don't have
	// any active nodes (flavors) will be unloaded.

	// load dormant media nodes
	BPath path;
	find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
	path.Append("media");

	node_ref nref;
	BEntry entry(path.Path());
	entry.GetNodeRef(&nref);
	fSystemAddOnsNode = nref.node;
	_WatchDir(&entry);

#ifdef USER_ADDON_PATH
	entry.SetTo(USER_ADDON_PATH);
#else
	find_directory(B_USER_ADDONS_DIRECTORY, &path);
	path.Append("media");
	entry.SetTo(path.Path());
#endif
	entry.GetNodeRef(&nref);
	fUserAddOnsNode = nref.node;
	_WatchDir(&entry);

	fStartup = false;

	InfoMap::iterator iterator = fInfoMap.begin();
	for (; iterator != fInfoMap.end(); iterator++)
		_InstantiatePhysicalInputsAndOutputs(iterator->second);

	for (iterator = fInfoMap.begin(); iterator != fInfoMap.end(); iterator++)
		_InstantiateAutostartFlavors(iterator->second);

	for (iterator = fInfoMap.begin(); iterator != fInfoMap.end(); iterator++)
		_PutAddonIfPossible(iterator->second);

	server_rescan_defaults_command cmd;
	SendToServer(SERVER_RESCAN_DEFAULTS, &cmd, sizeof(cmd));
}


bool
MediaAddonServer::QuitRequested()
{
	CALLED();

	InfoMap::iterator iterator = fInfoMap.begin();
	for (; iterator != fInfoMap.end(); iterator++) {
		AddOnInfo& info = iterator->second;

		_DestroyInstantiatedFlavors(info);
		_PutAddonIfPossible(info);
	}

	return true;
}


void
MediaAddonServer::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MEDIA_ADDON_SERVER_PLAY_MEDIA:
		{
			const char* name;
			const char* type;
			if (message->FindString(MEDIA_NAME_KEY, &name) != B_OK
				|| message->FindString(MEDIA_TYPE_KEY, &type) != B_OK) {
				message->SendReply(B_ERROR);
				break;
			}

			PlayMediaFile(type, name);
			message->SendReply((uint32)B_OK);
				// TODO: don't know which reply is expected
			return;
		}

		case B_NODE_MONITOR:
		{
			switch (message->FindInt32("opcode")) {
				case B_ENTRY_CREATED:
				{
					const char *name;
					entry_ref ref;
					ino_t node;
					BEntry e;
					BPath p;
					message->FindString("name", &name);
					message->FindInt64("node", &node);
					message->FindInt32("device", &ref.device);
					message->FindInt64("directory", &ref.directory);
					ref.set_name(name);
					e.SetTo(&ref,false);// build a BEntry for the created file/link/dir
					e.GetPath(&p); 		// get the path to the file/link/dir
					e.SetTo(&ref,true); // travese links to see
					if (e.IsFile()) {		// if it's a link to a file, or a file
						if (!message->FindBool("nowait")) {
							// TODO: wait 5 seconds if this is a regular notification
							// because the file creation may not be finshed when the
							// notification arrives (very ugly, how can we fix this?)
							// this will also fail if copying takes longer than 5 seconds
							snooze(5000000);
						}
						_AddOnAdded(p.Path(), node);
					}
					return;
				}
				case B_ENTRY_REMOVED:
				{
					ino_t node;
					message->FindInt64("node", &node);
					_AddOnRemoved(node);
					return;
				}
				case B_ENTRY_MOVED:
				{
					ino_t from;
					ino_t to;
					message->FindInt64("from directory", &from);
					message->FindInt64("to directory", &to);
					if (fSystemAddOnsNode == from || fUserAddOnsNode == from) {
						message->ReplaceInt32("opcode", B_ENTRY_REMOVED);
						message->AddInt64("directory", from);
						MessageReceived(message);
					}
					if (fSystemAddOnsNode == to || fUserAddOnsNode == to) {
						message->ReplaceInt32("opcode", B_ENTRY_CREATED);
						message->AddInt64("directory", to);
						message->AddBool("nowait", true);
						MessageReceived(message);
					}
					return;
				}
			}
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
MediaAddonServer::_HandleMessage(int32 code, const void* data, size_t size)
{
	switch (code) {
		case ADDONSERVER_INSTANTIATE_DORMANT_NODE:
		{
			const addonserver_instantiate_dormant_node_request* request
				= static_cast<
					const addonserver_instantiate_dormant_node_request*>(data);
			addonserver_instantiate_dormant_node_reply reply;

			status_t status
				= MediaRosterEx(fMediaRoster)->InstantiateDormantNode(
					request->addon_id, request->flavor_id,
					request->creator_team, &reply.node);
			request->SendReply(status, &reply, sizeof(reply));
			break;
		}

		case ADDONSERVER_RESCAN_MEDIAADDON_FLAVORS:
		{
			const addonserver_rescan_mediaaddon_flavors_command* command
				= static_cast<
					const addonserver_rescan_mediaaddon_flavors_command*>(data);
			BMediaAddOn* addon
				= _DormantNodeManager->GetAddon(command->addon_id);
			if (addon == NULL) {
				ERROR("rescan flavors: Can't find a addon object for id %d\n",
					(int)command->addon_id);
				break;
			}
			_ScanAddOnFlavors(addon);
			_DormantNodeManager->PutAddon(command->addon_id);
			break;
		}

		case ADDONSERVER_RESCAN_FINISHED_NOTIFY:
			if (fStartupSound) {
				system_beep(MEDIA_SOUNDS_STARTUP);
				fStartupSound = false;
			}
			break;

		default:
			ERROR("media_addon_server: received unknown message code %#08lx\n",
				code);
			break;
	}
}


status_t
MediaAddonServer::_ControlThread(void* _server)
{
	MediaAddonServer* server = (MediaAddonServer*)_server;

	char data[B_MEDIA_MESSAGE_SIZE];
	ssize_t size;
	int32 code;
	while ((size = read_port_etc(server->_ControlPort(), &code, data,
			sizeof(data), 0, 0)) > 0)
		server->_HandleMessage(code, data, size);

	return B_OK;
}


void
MediaAddonServer::_ScanAddOnFlavors(BMediaAddOn* addon)
{
	ASSERT(addon->AddonID() > 0);

	TRACE("MediaAddonServer::_ScanAddOnFlavors: id %ld\n", addon->AddonID());

	port_id port = find_port(MEDIA_SERVER_PORT_NAME);
	if (port <= B_OK) {
		ERROR("couldn't find media_server port\n");
		return;
	}

	// cache the media_addon_id in a local variable to avoid
	// calling BMediaAddOn::AddonID() too often
	media_addon_id addonID = addon->AddonID();

	// update the cached flavor count, get oldflavorcount and newflavorcount

	InfoMap::iterator found = fInfoMap.find(addonID);
	ASSERT(found != fInfoMap.end());

	AddOnInfo& info = found->second;
	int32 oldFlavorCount = info.flavor_count;
	int32 newFlavorCount = addon->CountFlavors();
	info.flavor_count = newFlavorCount;

	TRACE("%ld old flavors, %ld new flavors\n", oldflavorcount, newFlavorCount);

	// during the first update (i == 0), the server removes old dormant_flavor_infos
	for (int i = 0; i < newFlavorCount; i++) {
		const flavor_info* flavorInfo;
		TRACE("flavor %d:\n", i);
		if (addon->GetFlavorAt(i, &flavorInfo) != B_OK) {
			ERROR("MediaAddonServer::_ScanAddOnFlavors GetFlavorAt failed for "
				"index %d!\n", i);
			continue;
		}

#if DEBUG >= 2
		DumpFlavorInfo(flavorInfo);
#endif

		dormant_flavor_info dormantFlavorInfo;
		dormantFlavorInfo = *flavorInfo;
		dormantFlavorInfo.node_info.addon = addonID;
		dormantFlavorInfo.node_info.flavor_id = flavorInfo->internal_id;
		strlcpy(dormantFlavorInfo.node_info.name, flavorInfo->name,
			B_MEDIA_NAME_LENGTH);

		size_t flattenedSize = dormantFlavorInfo.FlattenedSize();
		size_t messageSize = flattenedSize
			+ sizeof(xfer_server_register_dormant_node);
		xfer_server_register_dormant_node* message
			= (xfer_server_register_dormant_node*)malloc(messageSize);
		if (message == NULL)
			break;

		// The server should remove previously registered "dormant_flavor_info"s
		// during the first update, but after  the first iteration, we don't
		// want the server to anymore remove old dormant_flavor_infos
		message->purge_id = i == 0 ? addonID : 0;

		message->type = dormantFlavorInfo.TypeCode();
		message->flattened_size = flattenedSize;
		dormantFlavorInfo.Flatten(message->flattened_data, flattenedSize);

		status_t status = write_port(port, SERVER_REGISTER_DORMANT_NODE,
			message, messageSize);
		if (status != B_OK) {
			ERROR("MediaAddonServer::_ScanAddOnFlavors: couldn't register "
				"dormant node: %s\n", strerror(status));
		}

		free(message);
	}

	// TODO: we currently pretend that all old flavors have been removed, this
	// could probably be done in a smarter way
	BPrivate::media::notifications::FlavorsChanged(addonID, newFlavorCount,
		oldFlavorCount);
}


void
MediaAddonServer::_AddOnAdded(const char* path, ino_t fileNode)
{
	TRACE("\n\nMediaAddonServer::_AddOnAdded: path %s\n", path);

	media_addon_id id = _DormantNodeManager->RegisterAddon(path);
	if (id <= 0) {
		ERROR("MediaAddonServer::_AddOnAdded: failed to register add-on %s\n",
			path);
		return;
	}

	TRACE("MediaAddonServer::_AddOnAdded: loading addon %ld now...\n", id);

	BMediaAddOn* addon = _DormantNodeManager->GetAddon(id);
	if (addon == NULL) {
		ERROR("MediaAddonServer::_AddOnAdded: failed to get add-on %s\n", path);
		_DormantNodeManager->UnregisterAddon(id);
		return;
	}

	TRACE("MediaAddonServer::_AddOnAdded: loading finished, id %ld\n", id);

	try {
		// put file's inode and addon's id into map
		fFileMap.insert(std::make_pair(fileNode, id));

		AddOnInfo info;
		fInfoMap.insert(std::make_pair(id, info));
	} catch (std::bad_alloc& exception) {
		fFileMap.erase(fileNode);
		return;
	}

	InfoMap::iterator found = fInfoMap.find(id);
	AddOnInfo& info = found->second;

	info.id = id;
	info.wants_autostart = false; // temporary default
	info.flavor_count = 0;
	info.addon = addon;

	// scan the flavors
	_ScanAddOnFlavors(addon);

	// need to call BMediaNode::WantsAutoStart()
	// after the flavors have been scanned
	info.wants_autostart = addon->WantsAutoStart();

	if (info.wants_autostart)
		TRACE("add-on %ld WantsAutoStart!\n", id);

	// During startup, first all add-ons are loaded, then all
	// nodes (flavors) representing physical inputs and outputs
	// are instantiated. Next, all add-ons that need autostart
	// will be autostarted. Finally, add-ons that don't have
	// any active nodes (flavors) will be unloaded.

	// After startup is done, we simply do it for each new
	// loaded add-on, too.
	if (!fStartup) {
		_InstantiatePhysicalInputsAndOutputs(info);
		_InstantiateAutostartFlavors(info);
		_PutAddonIfPossible(info);

		// since something might have changed
		server_rescan_defaults_command cmd;
		SendToServer(SERVER_RESCAN_DEFAULTS, &cmd, sizeof(cmd));
	}

	// we do not call _DormantNodeManager->PutAddon(id)
	// since it is done by _PutAddonIfPossible()
}


void
MediaAddonServer::_DestroyInstantiatedFlavors(AddOnInfo& info)
{
	printf("MediaAddonServer::_DestroyInstantiatedFlavors\n");

	NodeVector::iterator iterator = info.active_flavors.begin();
	for (; iterator != info.active_flavors.end(); iterator++) {
		media_node& node = *iterator;
		if ((node.kind & B_TIME_SOURCE) != 0
			&& (fMediaRoster->StopTimeSource(node, 0, true) != B_OK)) {
			printf("MediaAddonServer::_DestroyInstantiatedFlavors couldn't stop "
				"timesource\n");
			continue;
		}

		if (fMediaRoster->StopNode(node, 0, true) != B_OK) {
			printf("MediaAddonServer::_DestroyInstantiatedFlavors couldn't stop "
				"node\n");
			continue;
		}

		if ((node.kind & B_BUFFER_CONSUMER) != 0) {
			media_input inputs[16];
			int32 count = 0;
			if (fMediaRoster->GetConnectedInputsFor(node, inputs, 16, &count)
					!= B_OK) {
				printf("MediaAddonServer::_DestroyInstantiatedFlavors couldn't "
					"get connected inputs\n");
				continue;
			}

			for (int32 i = 0; i < count; i++) {
				media_node_id sourceNode;
				if ((sourceNode = fMediaRoster->NodeIDFor(
						inputs[i].source.port)) < 0) {
					printf("MediaAddonServer::_DestroyInstantiatedFlavors "
						"couldn't get source node id\n");
					continue;
				}

				if (fMediaRoster->Disconnect(sourceNode, inputs[i].source,
						node.node, inputs[i].destination) != B_OK) {
					printf("MediaAddonServer::_DestroyInstantiatedFlavors "
						"couldn't disconnect input\n");
					continue;
				}
			}
		}

		if ((node.kind & B_BUFFER_PRODUCER) != 0) {
			media_output outputs[16];
			int32 count = 0;
			if (fMediaRoster->GetConnectedOutputsFor(node, outputs, 16,
					&count) != B_OK) {
				printf("MediaAddonServer::_DestroyInstantiatedFlavors couldn't "
					"get connected outputs\n");
				continue;
			}

			for (int32 i = 0; i < count; i++) {
				media_node_id destNode;
				if ((destNode = fMediaRoster->NodeIDFor(
						outputs[i].destination.port)) < 0) {
					printf("MediaAddonServer::_DestroyInstantiatedFlavors "
						"couldn't get destination node id\n");
					continue;
				}

				if (fMediaRoster->Disconnect(node.node, outputs[i].source,
						destNode, outputs[i].destination) != B_OK) {
					printf("MediaAddonServer::_DestroyInstantiatedFlavors "
						"couldn't disconnect output\n");
					continue;
				}
			}
		}
	}

	info.active_flavors.clear();
}


void
MediaAddonServer::_PutAddonIfPossible(AddOnInfo& info)
{
	if (info.addon && info.active_flavors.empty()) {
		_DormantNodeManager->PutAddon(info.id);
		info.addon = NULL;
	}
}


void
MediaAddonServer::_InstantiatePhysicalInputsAndOutputs(AddOnInfo& info)
{
	CALLED();
	int32 count = info.addon->CountFlavors();

	for (int32 i = 0; i < count; i++) {
		const flavor_info* flavorinfo;
		if (info.addon->GetFlavorAt(i, &flavorinfo) != B_OK) {
			ERROR("MediaAddonServer::InstantiatePhysialInputsAndOutputs "
				"GetFlavorAt failed for index %ld!\n", i);
			continue;
		}
		if ((flavorinfo->kinds & (B_PHYSICAL_INPUT | B_PHYSICAL_OUTPUT)) != 0) {
			media_node node;

			dormant_node_info dormantNodeInfo;
			dormantNodeInfo.addon = info.id;
			dormantNodeInfo.flavor_id = flavorinfo->internal_id;
			strcpy(dormantNodeInfo.name, flavorinfo->name);

			PRINT("MediaAddonServer::InstantiatePhysialInputsAndOutputs: "
				"\"%s\" is a physical input/output\n", flavorinfo->name);
			status_t status = fMediaRoster->InstantiateDormantNode(
				dormantNodeInfo, &node);
			if (status != B_OK) {
				ERROR("MediaAddonServer::InstantiatePhysialInputsAndOutputs "
					"Couldn't instantiate node flavor, internal_id %ld, "
					"name %s\n", flavorinfo->internal_id, flavorinfo->name);
			} else {
				PRINT("Node created!\n");
				info.active_flavors.push_back(node);
			}
		}
	}
}


void
MediaAddonServer::_InstantiateAutostartFlavors(AddOnInfo& info)
{
	if (!info.wants_autostart)
		return;

	for (int32 index = 0;; index++) {
		PRINT("trying autostart of node %ld, index %ld\n", info.id, index);

		BMediaNode* node;
		int32 internalID;
		bool hasMore;
		status_t status = info.addon->AutoStart(index, &node, &internalID,
			&hasMore);
		if (status == B_OK) {
			printf("started node %ld\n", index);

			// TODO: IncrementAddonFlavorInstancesCount

			status = MediaRosterEx(fMediaRoster)->RegisterNode(node, info.id,
				internalID);
			if (status != B_OK) {
				ERROR("failed to register node %ld\n", index);
				// TODO: DecrementAddonFlavorInstancesCount
			}

			info.active_flavors.push_back(node->Node());

			if (!hasMore)
				return;
		} else if (status == B_MEDIA_ADDON_FAILED && hasMore) {
			continue;
		} else {
			break;
		}
	}
}


void
MediaAddonServer::_AddOnRemoved(ino_t fileNode)
{
	// TODO: locking?

	FileMap::iterator foundFile = fFileMap.find(fileNode);
	if (foundFile == fFileMap.end()) {
		ERROR("MediaAddonServer::_AddOnRemoved: inode %Ld removed, but no "
			"media add-on found\n", fileNode);
		return;
	}

	media_addon_id id = foundFile->second;
	fFileMap.erase(foundFile);

	int32 oldFlavorCount;
	InfoMap::iterator foundInfo = fInfoMap.find(id);

	if (foundInfo == fInfoMap.end()) {
		ERROR("MediaAddonServer::_AddOnRemoved: couldn't get addon info for "
			"add-on %ld\n", id);
		oldFlavorCount = 1000;
	} else {
		AddOnInfo& info = foundInfo->second;
		oldFlavorCount = info.flavor_count;

		_DestroyInstantiatedFlavors(info);
		_PutAddonIfPossible(info);

		if (info.addon) {
			ERROR("MediaAddonServer::_AddOnRemoved: couldn't unload addon "
				"%ld since flavors are in use\n", id);
		}

		fInfoMap.erase(foundInfo);
	}

	_DormantNodeManager->UnregisterAddon(id);

	BPrivate::media::notifications::FlavorsChanged(id, 0, oldFlavorCount);
}


void
MediaAddonServer::_WatchDir(BEntry* dir)
{
	// send fake notices to trigger add-on loading
	BDirectory directory(dir);
	BEntry entry;
	while (directory.GetNextEntry(&entry, false) == B_OK) {
		node_ref nodeRef;
		entry_ref ref;
		if (entry.GetRef(&ref) != B_OK || entry.GetNodeRef(&nodeRef) != B_OK)
			continue;

		BMessage msg(B_NODE_MONITOR);
		msg.AddInt32("opcode", B_ENTRY_CREATED);
		msg.AddInt32("device", ref.device);
		msg.AddInt64("directory", ref.directory);
		msg.AddInt64("node", nodeRef.node);
		msg.AddString("name", ref.name);
		msg.AddBool("nowait", true);
		MessageReceived(&msg);
	}

	node_ref nodeRef;
	if (dir->GetNodeRef(&nodeRef) == B_OK)
		watch_node(&nodeRef, B_WATCH_DIRECTORY, be_app_messenger);
}


//	#pragma mark -


int
main()
{
	new MediaAddonServer(B_MEDIA_ADDON_SERVER_SIGNATURE);
	be_app->Run();
	delete be_app;
	return 0;
}

