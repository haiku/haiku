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

/* to comply with the license above, do not remove the following line */
char __dont_remove_copyright_from_binary[] = "Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>";

#define BUILDING_MEDIA_ADDON 1
#include <Application.h>
#include <Alert.h>
#include <MediaRoster.h>
#include <NodeMonitor.h>
#include <MediaAddOn.h>
#include <String.h>
#include <stdio.h>
#include <Entry.h>
#include <Path.h>
#include <Directory.h>
#include <Looper.h>
#include "debug.h"
#include "TMap.h"
#include "ServerInterface.h"
#include "DataExchange.h"
#include "DormantNodeManager.h"
#include "Notifications.h"
#include "MediaMisc.h"
#include "MediaRosterEx.h"
#include "SystemTimeSource.h"

//#define USER_ADDON_PATH "../add-ons/media"

void DumpFlavorInfo(const flavor_info *info);

struct AddOnInfo
{
	media_addon_id id;
	bool wants_autostart;
	int32 flavor_count;
	
	List<media_node> active_flavors;

	BMediaAddOn *addon; // if != NULL, need to call _DormantNodeManager->PutAddon(id)
};

class MediaAddonServer : BApplication
{
public:
	MediaAddonServer(const char *sig);
	~MediaAddonServer();
	void ReadyToRun();
	bool QuitRequested();
	void MessageReceived(BMessage *msg);
	
	void WatchDir(BEntry *dir);
	void AddOnAdded(const char *path, ino_t file_node);
	void AddOnRemoved(ino_t file_node);
	void HandleMessage(int32 code, const void *data, size_t size);
	static int32 controlthread(void *arg);

	void PutAddonIfPossible(AddOnInfo *info);
	void InstantiatePhysicalInputsAndOutputs(AddOnInfo *info);
	void InstantiateAutostartFlavors(AddOnInfo *info);
	void DestroyInstantiatedFlavors(AddOnInfo *info);
	
	void ScanAddOnFlavors(BMediaAddOn *addon);

	Map<ino_t, media_addon_id> *filemap;
	Map<media_addon_id, AddOnInfo> *infomap;

	BMediaRoster *mediaroster;
	ino_t		DirNodeSystem;
	ino_t		DirNodeUser;
	port_id		control_port;
	thread_id	control_thread;
	bool		fStartup;
	
	typedef BApplication inherited;
};

MediaAddonServer::MediaAddonServer(const char *sig) : 
	BApplication(sig),
	fStartup(true)
{
	CALLED();
	mediaroster = BMediaRoster::Roster();
	filemap = new Map<ino_t, media_addon_id>;
	infomap = new Map<media_addon_id, AddOnInfo>;
	control_port = create_port(64, MEDIA_ADDON_SERVER_PORT_NAME);
	control_thread = spawn_thread(controlthread, "media_addon_server control", 12, this);
	resume_thread(control_thread);
}

MediaAddonServer::~MediaAddonServer()
{
	CALLED();
	
	delete_port(control_port);
	status_t err;
	wait_for_thread(control_thread,&err);

	// unregister all media add-ons
	media_addon_id *id;
	for (filemap->Rewind(); filemap->GetNext(&id); )
		_DormantNodeManager->UnregisterAddon(*id);
	
	
	// XXX unregister system time source
	
	delete filemap;
	delete infomap;
}

void 
MediaAddonServer::HandleMessage(int32 code, const void *data, size_t size)
{
	switch (code) {
		case ADDONSERVER_INSTANTIATE_DORMANT_NODE:
		{
			const addonserver_instantiate_dormant_node_request *request = static_cast<const addonserver_instantiate_dormant_node_request *>(data);
			addonserver_instantiate_dormant_node_reply reply;
			status_t rv;
			rv = MediaRosterEx(mediaroster)->InstantiateDormantNode(request->addonid, request->flavorid, request->creator_team, &reply.node);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case ADDONSERVER_RESCAN_MEDIAADDON_FLAVORS:
		{
			const addonserver_rescan_mediaaddon_flavors_command *command = static_cast<const addonserver_rescan_mediaaddon_flavors_command *>(data);
			BMediaAddOn *addon;
			addon = _DormantNodeManager->GetAddon(command->addonid);
			if (!addon) {
				ERROR("rescan flavors: Can't find a addon object for id %d\n",(int)command->addonid);
				break;
			}
			ScanAddOnFlavors(addon);
			_DormantNodeManager->PutAddon(command->addonid);
			break;
		}

		default:
			ERROR("media_addon_server: received unknown message code %#08lx\n",code);
	}
}

int32
MediaAddonServer::controlthread(void *arg)
{
	char data[B_MEDIA_MESSAGE_SIZE];
	MediaAddonServer *app;
	ssize_t size;
	int32 code;
	
	app = (MediaAddonServer *)arg;
	while ((size = read_port_etc(app->control_port, &code, data, sizeof(data), 0, 0)) > 0)
		app->HandleMessage(code, data, size);

	return 0;
}

void 
MediaAddonServer::ReadyToRun()
{
	// the control thread is already running at this point,
	// so we can talk to the media server and also receive
	// commands for instantiation

	// register with media_server
	server_register_addonserver_request request;
	server_register_addonserver_reply reply;
	status_t result;
	request.team = BPrivate::media::team;
	result = QueryServer(SERVER_REGISTER_ADDONSERVER, &request, sizeof(request), &reply, sizeof(reply));
	if (result != B_OK) {
		ERROR("Communication with server failed. Terminating.\n");
		PostMessage(B_QUIT_REQUESTED);
		return;
	}

	ASSERT(fStartup == true);
	
	// The very first thing to do is to create the system time source,
	// register it with the server, and make it the default SYSTEM_TIME_SOURCE
	BMediaNode *ts = new SystemTimeSource;
	result = mediaroster->RegisterNode(ts);
	if (result != B_OK)
		debugger("Can't register system time source");
	if (ts->ID() != NODE_SYSTEM_TIMESOURCE_ID)
		debugger("System time source got wrong node ID");
	media_node node = ts->Node();
	result = MediaRosterEx(mediaroster)->SetNode(SYSTEM_TIME_SOURCE, &node);
	if (result != B_OK)
		debugger("Can't setup system time source as default");

	// During startup, first all add-ons are loaded, then all
	// nodes (flavors) representing physical inputs and outputs
	// are instantiated. Next, all add-ons that need autostart
	// will be autostarted. Finally, add-ons that don't have
	// any active nodes (flavors) will be unloaded.

	// load dormant media nodes
	node_ref nref;
	BEntry e("/boot/beos/system/add-ons/media");
	e.GetNodeRef(&nref);
	DirNodeSystem = nref.node;
	WatchDir(&e);

#ifdef USER_ADDON_PATH
	BEntry e2(USER_ADDON_PATH);
#else
	BEntry e2("/boot/home/config/add-ons/media");
#endif
	e2.GetNodeRef(&nref);
	DirNodeUser = nref.node;
	WatchDir(&e2);
	
	fStartup = false;
	
	AddOnInfo *info;
	
	infomap->Rewind();
	while (infomap->GetNext(&info))
		InstantiatePhysicalInputsAndOutputs(info);

	infomap->Rewind();
	while (infomap->GetNext(&info))
		InstantiateAutostartFlavors(info);

	infomap->Rewind();
	while (infomap->GetNext(&info))
		PutAddonIfPossible(info);
	
	server_rescan_defaults_command cmd;
	SendToServer(SERVER_RESCAN_DEFAULTS, &cmd, sizeof(cmd));
}


bool
MediaAddonServer::QuitRequested()
{
	CALLED();
	
	AddOnInfo *info;
	infomap->Rewind();
	while(infomap->GetNext(&info)) {
		DestroyInstantiatedFlavors(info);
		PutAddonIfPossible(info);
	}
	
	return true;
}


void 
MediaAddonServer::ScanAddOnFlavors(BMediaAddOn *addon)
{
	AddOnInfo *info;
	int32 oldflavorcount;
	int32 newflavorcount;
	media_addon_id addon_id;
	port_id port;
	status_t rv;
	bool b;
	
	ASSERT(addon);
	ASSERT(addon->AddonID() > 0);
	
	TRACE("MediaAddonServer::ScanAddOnFlavors: id %ld\n",addon->AddonID());
	
	port = find_port(MEDIA_SERVER_PORT_NAME);
	if (port <= B_OK) {
		ERROR("couldn't find media_server port\n");
		return;
	}
	
	// cache the media_addon_id in a local variable to avoid
	// calling BMediaAddOn::AddonID() too often
	addon_id = addon->AddonID();
	
	// update the cached flavor count, get oldflavorcount and newflavorcount
	b = infomap->Get(addon_id, &info);
	ASSERT(b);
	oldflavorcount = info->flavor_count;
	newflavorcount = addon->CountFlavors();
	info->flavor_count = newflavorcount;
	
	TRACE("%ld old flavors, %ld new flavors\n", oldflavorcount, newflavorcount);

	// during the first update (i == 0), the server removes old dormant_flavor_infos
	for (int i = 0; i < newflavorcount; i++) {
		const flavor_info *info;
		TRACE("flavor %d:\n",i);
		if (B_OK != addon->GetFlavorAt(i, &info)) {
			ERROR("MediaAddonServer::ScanAddOnFlavors GetFlavorAt failed for index %d!\n", i);
			continue;
		}
		
		#if DEBUG >= 2
		  DumpFlavorInfo(info);
		#endif
		
		dormant_flavor_info dfi;
		dfi = *info;
		dfi.node_info.addon = addon_id;
		dfi.node_info.flavor_id = info->internal_id;
		strncpy(dfi.node_info.name, info->name, B_MEDIA_NAME_LENGTH - 1);
		dfi.node_info.name[B_MEDIA_NAME_LENGTH - 1] = 0;
		
		xfer_server_register_dormant_node *msg;
		size_t flattensize;
		size_t msgsize;
		
		flattensize = dfi.FlattenedSize();
		msgsize = flattensize + sizeof(xfer_server_register_dormant_node);
		msg = (xfer_server_register_dormant_node *) malloc(msgsize);
		
		// the server should remove previously registered "dormant_flavor_info"s
		// during the first update, but after  the first iteration, we don't want
		// the server to anymore remove old dormant_flavor_infos;
		msg->purge_id = (i == 0) ? addon_id : 0;

		msg->dfi_type = dfi.TypeCode();
		msg->dfi_size = flattensize;
		dfi.Flatten(&(msg->dfi),flattensize);

		rv = write_port(port, SERVER_REGISTER_DORMANT_NODE, msg, msgsize);
		if (rv != B_OK) {
			ERROR("MediaAddonServer::ScanAddOnFlavors: couldn't register dormant node\n");
		}

		free(msg);
	}

	// XXX parameter list is (media_addon_id addonid, int32 newcount, int32 gonecount)
	// XXX we currently pretend that all old flavors have been removed, this could
	// XXX probably be done in a smarter way
	BPrivate::media::notifications::FlavorsChanged(addon_id, newflavorcount, oldflavorcount);
}

void 
MediaAddonServer::AddOnAdded(const char *path, ino_t file_node)
{
	TRACE("\n\nMediaAddonServer::AddOnAdded: path %s\n",path);

	BMediaAddOn *addon;
	media_addon_id id;

	id = _DormantNodeManager->RegisterAddon(path);
	if (id <= 0) {
		ERROR("MediaAddonServer::AddOnAdded: failed to register add-on %s\n", path);
		return;
	}
	
	TRACE("MediaAddonServer::AddOnAdded: loading addon %ld now...\n", id);

	addon = _DormantNodeManager->GetAddon(id);
	if (addon == NULL) {
		ERROR("MediaAddonServer::AddOnAdded: failed to get add-on %s\n", path);
		_DormantNodeManager->UnregisterAddon(id);
		return;
	}

	TRACE("MediaAddonServer::AddOnAdded: loading finished, id %ld\n", id);

	// put file's inode and addon's id into map
	filemap->Insert(file_node, id);
	
	// also create AddOnInfo struct and get a pointer so
	// we can modify it
	AddOnInfo tempinfo;
	infomap->Insert(id, tempinfo);
	AddOnInfo *info;
	infomap->Get(id, &info);

	// setup
	info->id = id;
	info->wants_autostart = false; // temporary default
	info->flavor_count = 0;
	info->addon = addon; 

	// scan the flavors
	ScanAddOnFlavors(addon);

	// need to call BMediaNode::WantsAutoStart()
	// after the flavors have been scanned
	info->wants_autostart = addon->WantsAutoStart();
	
	if (info->wants_autostart)
		TRACE("add-on %ld WantsAutoStart!\n", id);

	// During startup, first all add-ons are loaded, then all
	// nodes (flavors) representing physical inputs and outputs
	// are instantiated. Next, all add-ons that need autostart
	// will be autostarted. Finally, add-ons that don't have
	// any active nodes (flavors) will be unloaded.

	// After startup is done, we simply do it for each new
	// loaded add-on, too.
	if (!fStartup) {
		InstantiatePhysicalInputsAndOutputs(info);
		InstantiateAutostartFlavors(info);
		PutAddonIfPossible(info);

		// since something might have changed
		server_rescan_defaults_command cmd;
		SendToServer(SERVER_RESCAN_DEFAULTS, &cmd, sizeof(cmd));	
	}

	// we do not call _DormantNodeManager->PutAddon(id)
	// since it is done by PutAddonIfPossible()
}

void
MediaAddonServer::DestroyInstantiatedFlavors(AddOnInfo *info)
{
	printf("MediaAddonServer::DestroyInstantiatedFlavors\n");
	media_node *node;
	while (info->active_flavors.GetNext(&node)) {
		if ((node->kind & B_TIME_SOURCE) 
			&& (mediaroster->StopTimeSource(*node, 0, true)!=B_OK)) {
			printf("MediaAddonServer::DestroyInstantiatedFlavors couldn't stop timesource\n");
			continue;
		}	
	
		if(mediaroster->StopNode(*node, 0, true)!=B_OK) {
			printf("MediaAddonServer::DestroyInstantiatedFlavors couldn't stop node\n");
			continue;
		}
		
		if (node->kind & B_BUFFER_CONSUMER) { 
			media_input inputs[16];
			int32 count = 0;
			if(mediaroster->GetConnectedInputsFor(*node, inputs, 16, &count)!=B_OK) {
				printf("MediaAddonServer::DestroyInstantiatedFlavors couldn't get connected inputs\n");
				continue;
			}
			
			for(int32 i=0; i<count; i++) {
				media_node_id sourceNode;
				if((sourceNode = mediaroster->NodeIDFor(inputs[i].source.port)) < 0) {
					printf("MediaAddonServer::DestroyInstantiatedFlavors couldn't get source node id\n");
					continue;
				}
				
				if(mediaroster->Disconnect(sourceNode, inputs[i].source, node->node, inputs[i].destination)!=B_OK) {
					printf("MediaAddonServer::DestroyInstantiatedFlavors couldn't disconnect input\n");
					continue;
				}
			}
		}
		
		if (node->kind & B_BUFFER_PRODUCER) { 
			media_output outputs[16];
			int32 count = 0;
			if(mediaroster->GetConnectedOutputsFor(*node, outputs, 16, &count)!=B_OK) {
				printf("MediaAddonServer::DestroyInstantiatedFlavors couldn't get connected outputs\n");
				continue;
			}
			
			for(int32 i=0; i<count; i++) {
				media_node_id destNode;
				if((destNode = mediaroster->NodeIDFor(outputs[i].destination.port)) < 0) {
					printf("MediaAddonServer::DestroyInstantiatedFlavors couldn't get destination node id\n");
					continue;
				}
				
				if(mediaroster->Disconnect(node->node, outputs[i].source, destNode, outputs[i].destination)!=B_OK) {
					printf("MediaAddonServer::DestroyInstantiatedFlavors couldn't disconnect output\n");
					continue;
				}
			}
		}
	
		info->active_flavors.RemoveCurrent();
	}
}

void
MediaAddonServer::PutAddonIfPossible(AddOnInfo *info)
{
	if (info->addon && info->active_flavors.IsEmpty()) {
		_DormantNodeManager->PutAddon(info->id);
		info->addon = NULL;
	}
}

void
MediaAddonServer::InstantiatePhysicalInputsAndOutputs(AddOnInfo *info)
{
	CALLED();
	int count = info->addon->CountFlavors();
	for (int i = 0; i < count; i++) {
		const flavor_info *flavorinfo;
		if (B_OK != info->addon->GetFlavorAt(i, &flavorinfo)) {
			ERROR("MediaAddonServer::InstantiatePhysialInputsAndOutputs GetFlavorAt failed for index %d!\n", i);
			continue;
		}
		if (flavorinfo->kinds & (B_PHYSICAL_INPUT | B_PHYSICAL_OUTPUT)) {
			media_node node;
			status_t rv;

			dormant_node_info dni;
			dni.addon = info->id;
			dni.flavor_id = flavorinfo->internal_id;
			strcpy(dni.name, flavorinfo->name);
			
			printf("MediaAddonServer::InstantiatePhysialInputsAndOutputs: \"%s\" is a physical input/output\n", flavorinfo->name);
			rv = mediaroster->InstantiateDormantNode(dni, &node);
			if (rv != B_OK) {
				ERROR("MediaAddonServer::InstantiatePhysialInputsAndOutputs Couldn't instantiate node flavor, internal_id %ld, name %s\n", flavorinfo->internal_id, flavorinfo->name);
			} else {
				printf("Node created!\n");
				info->active_flavors.Insert(node);
			}
		}
	}
}

void
MediaAddonServer::InstantiateAutostartFlavors(AddOnInfo *info)
{
	if (!info->wants_autostart)
		return;
		
	for (int32 index = 0; ;index++) {
		BMediaNode *outNode;
		int32 outInternalID;
		bool outHasMore;
		status_t rv;
		printf("trying autostart of node %ld, index %ld\n", info->id, index);
		rv = info->addon->AutoStart(index, &outNode, &outInternalID, &outHasMore);
		if (rv == B_OK) {
			printf("started node %ld\n",index);

			// XXX IncrementAddonFlavorInstancesCount

			rv = MediaRosterEx(mediaroster)->RegisterNode(outNode, info->id, outInternalID);
			if (rv != B_OK) {
				printf("failed to register node %ld\n",index);
				// XXX DecrementAddonFlavorInstancesCount
			}
			
			info->active_flavors.Insert(outNode->Node());
			
			if (!outHasMore)
				return;
		} else if (rv == B_MEDIA_ADDON_FAILED && outHasMore) {
			continue;
		} else {
			break;
		}
	}
}

void
MediaAddonServer::AddOnRemoved(ino_t file_node)
{	
	media_addon_id *tempid;
	media_addon_id id;
	AddOnInfo *info;
	int32 oldflavorcount;
	// XXX locking?
	
	if (!filemap->Get(file_node, &tempid)) {
		ERROR("MediaAddonServer::AddOnRemoved: inode %Ld removed, but no media add-on found\n", file_node);
		return;
	}
	id = *tempid; // tempid pointer is invalid after Removing() it from the map
	filemap->Remove(file_node);
	
	if (!infomap->Get(id, &info)) {
		ERROR("MediaAddonServer::AddOnRemoved: couldn't get addon info for add-on %ld\n", id);
		oldflavorcount = 1000;
	} else {
		oldflavorcount = info->flavor_count; //same reason as above
	
		DestroyInstantiatedFlavors(info);
		PutAddonIfPossible(info);
	
		if (info->addon) {
			ERROR("MediaAddonServer::AddOnRemoved: couldn't unload addon %ld since flavors are in use\n", id);
		}
	}

		
	infomap->Remove(id);
	_DormantNodeManager->UnregisterAddon(id);
	
	BPrivate::media::notifications::FlavorsChanged(id, 0, oldflavorcount);
}

void 
MediaAddonServer::WatchDir(BEntry *dir)
{
	BEntry e;
	BDirectory d(dir);
	node_ref nref;
	entry_ref ref;
	while (B_OK == d.GetNextEntry(&e, false)) {
		e.GetRef(&ref);
		e.GetNodeRef(&nref);
		BMessage msg(B_NODE_MONITOR);
		msg.AddInt32("opcode",B_ENTRY_CREATED);
		msg.AddInt32("device",ref.device);
		msg.AddInt64("directory",ref.directory);
		msg.AddInt64("node",nref.node);
		msg.AddString("name",ref.name);
		msg.AddBool("nowait",true);
		MessageReceived(&msg);
	}

	dir->GetNodeRef(&nref); 
	watch_node(&nref,B_WATCH_DIRECTORY,be_app_messenger);
}

void 
MediaAddonServer::MessageReceived(BMessage *msg)
{
	switch (msg->what) 
	{
		case B_NODE_MONITOR:
		{
			switch (msg->FindInt32("opcode")) 
			{
				case B_ENTRY_CREATED:
				{
					const char *name;
					entry_ref ref;
					ino_t node;
					BEntry e;
					BPath p;
					msg->FindString("name", &name);
					msg->FindInt64("node", &node);
					msg->FindInt32("device", &ref.device);
					msg->FindInt64("directory", &ref.directory);
					ref.set_name(name);
					e.SetTo(&ref,false);// build a BEntry for the created file/link/dir
					e.GetPath(&p); 		// get the path to the file/link/dir
					e.SetTo(&ref,true); // travese links to see
					if (e.IsFile()) {		// if it's a link to a file, or a file
						if (false == msg->FindBool("nowait")) {
							// XXX wait 5 seconds if this is a regular notification
							// because the file creation may not be finshed when the
							// notification arrives (very ugly, how can we fix this?)
							// this will also fail if copying takes longer than 5 seconds
							snooze(5000000);
						}
						AddOnAdded(p.Path(),node);
					}
					return;
				}
				case B_ENTRY_REMOVED:
				{
					ino_t node;
					msg->FindInt64("node",&node);
					AddOnRemoved(node);
					return;
				}
				case B_ENTRY_MOVED:
				{
					ino_t from;
					ino_t to;
					msg->FindInt64("from directory", &from);
					msg->FindInt64("to directory", &to);
					if (DirNodeSystem == from || DirNodeUser == from) {
						msg->ReplaceInt32("opcode",B_ENTRY_REMOVED);
						msg->AddInt64("directory",from);
						MessageReceived(msg);
					}
					if (DirNodeSystem == to || DirNodeUser == to) {
						msg->ReplaceInt32("opcode",B_ENTRY_CREATED);
						msg->AddInt64("directory",to);
						msg->AddBool("nowait",true);
						MessageReceived(msg);
					}
					return;
				}
			}
			break;
		}
		default: inherited::MessageReceived(msg); break;
	}
	printf("MediaAddonServer: Unhandled message:\n");
	msg->PrintToStream();
}

int main()
{
	new MediaAddonServer("application/x-vnd.OpenBeOS-addon-host");
	be_app->Run();
	return 0;
}

void DumpFlavorInfo(const flavor_info *info)
{
	printf("  name = %s\n",info->name);
	printf("  info = %s\n",info->info);
	printf("  internal_id = %ld\n",info->internal_id);
	printf("  possible_count = %ld\n",info->possible_count);
	printf("  flavor_flags = 0x%lx",info->flavor_flags);
	if (info->flavor_flags & B_FLAVOR_IS_GLOBAL) printf(" B_FLAVOR_IS_GLOBAL");
	if (info->flavor_flags & B_FLAVOR_IS_LOCAL) printf(" B_FLAVOR_IS_LOCAL");
	printf("\n");
	printf("  kinds = 0x%Lx",info->kinds);
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
	printf("  in_format_count = %ld\n",info->in_format_count);
	printf("  out_format_count = %ld\n",info->out_format_count);
}
