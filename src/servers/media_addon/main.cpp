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

//#define USER_ADDON_PATH "../add-ons/media"

void DumpFlavorInfo(const flavor_info *info);

namespace BPrivate { namespace media {
	extern team_id team;
} } // BPrivate::media

class MediaAddonServer : BApplication
{
public:
	MediaAddonServer(const char *sig);
	~MediaAddonServer();
	void ReadyToRun();
	void MessageReceived(BMessage *msg);
	
	void WatchDir(BEntry *dir);
	void AddOnAdded(const char *path, ino_t file_node);
	void AddOnRemoved(ino_t file_node);
	void HandleMessage(int32 code, const void *data, size_t size);
	static int32 controlthread(void *arg);
	
	void ScanAddOnFlavors(BMediaAddOn *addon);

	Map<ino_t, media_addon_id> *filemap;
	Map<media_addon_id, int32> *flavorcountmap;

	BMediaRoster *mediaroster;
	ino_t		DirNodeSystem;
	ino_t		DirNodeUser;
	port_id		control_port;
	thread_id	control_thread;
};

MediaAddonServer::MediaAddonServer(const char *sig) : 
	BApplication(sig)
{
	mediaroster = BMediaRoster::Roster();
	filemap = new Map<ino_t, media_addon_id>;
	flavorcountmap = new Map<media_addon_id, int32>;
	control_port = create_port(64,"media_addon_server port");
	control_thread = spawn_thread(controlthread,"media_addon_server control",12,this);
	resume_thread(control_thread);
}

MediaAddonServer::~MediaAddonServer()
{
	delete_port(control_port);
	status_t err;
	wait_for_thread(control_thread,&err);

	// unregister all media add-ons
	media_addon_id id;
	for (int32 index = 0; filemap->GetAt(index,&id); index++)
		_DormantNodeManager->UnregisterAddon(id);
	
	delete filemap;
	delete flavorcountmap;
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
			rv = mediaroster->InstantiateDormantNode(request->info, &reply.node);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case ADDONSERVER_RESCAN_MEDIAADDON_FLAVORS:
		{
			const addonserver_rescan_mediaaddon_flavors_command *command = static_cast<const addonserver_rescan_mediaaddon_flavors_command *>(data);
			BMediaAddOn *addon;
			addon = _DormantNodeManager->GetAddon(command->addonid);
			if (!addon) {
				FATAL("rescan flavors: Can't find a addon object for id %d\n",(int)command->addonid);
				break;
			}
			ScanAddOnFlavors(addon);
			_DormantNodeManager->PutAddon(command->addonid);
			break;
		}

		default:
			FATAL("media_addon_server: received unknown message code %#08lx\n",code);
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
	// register with media_server
	server_register_addonserver_request request;
	server_register_addonserver_reply reply;
	status_t result;
	request.team = BPrivate::media::team;
	result = QueryServer(SERVER_REGISTER_ADDONSERVER, &request, sizeof(request), &reply, sizeof(reply));
	if (result != B_OK) {
		FATAL("Communication with server failed. Terminating.\n");
		PostMessage(B_QUIT_REQUESTED);
		return;
	}

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
}

void 
MediaAddonServer::ScanAddOnFlavors(BMediaAddOn *addon)
{
	int32 *flavorcount;
	int32 oldflavorcount;
	int32 newflavorcount;
	media_addon_id addon_id;
	port_id port;
	status_t rv;
	bool b;
	
	ASSERT(addon);
	ASSERT(addon->AddonID() > 0);
	
	TRACE("MediaAddonServer::ScanAddOnFlavors: id %ld\n",addon->AddonID());
	
	port = find_port("media_server port");
	if (port <= B_OK) {
		FATAL("couldn't find media_server port\n");
		return;
	}
	
	// cache the media_addon_id in a local variable to avoid
	// calling BMediaAddOn::AddonID() too often
	addon_id = addon->AddonID();
	
	// update the cached flavor count, get oldflavorcount and newflavorcount
	b = flavorcountmap->GetPointer(addon->AddonID(), &flavorcount);
	ASSERT(b);
	oldflavorcount = *flavorcount;
	newflavorcount = addon->CountFlavors();
	*flavorcount = newflavorcount;
	
	TRACE("%ld old flavors, %ld new flavors\n", oldflavorcount, newflavorcount);

	// during the first update (i == 0), the server removes old dormant_flavor_infos
	for (int i = 0; i < newflavorcount; i++) {
		const flavor_info *info;
		TRACE("flavor %d:\n",i);
		if (B_OK != addon->GetFlavorAt(i, &info)) {
			FATAL("MediaAddonServer::ScanAddOnFlavors GetFlavorAt failed for index %d!\n", i);
			continue;
		}
		
		#if DEBUG >= 3
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
			FATAL("MediaAddonServer::ScanAddOnFlavors: couldn't register dormant node\n");
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
		FATAL("MediaAddonServer::AddOnAdded: failed to register add-on %s\n", path);
		return;
	}
	
	addon = _DormantNodeManager->GetAddon(id);
	if (addon == NULL) {
		FATAL("MediaAddonServer::AddOnAdded: failed to get add-on %s\n", path);
		_DormantNodeManager->UnregisterAddon(id);
		return;
	}

	TRACE("MediaAddonServer::AddOnAdded: loading finished, id %d\n",(int)id);

	filemap->Insert(file_node, id);
	flavorcountmap->Insert(id, 0);

	ScanAddOnFlavors(addon);
	
	if (addon->WantsAutoStart())
		TRACE("#### WantsAutoStart!\n");

/*
 * the mixer (which we can't load because of unresolved symbols)
 * is the only node that uses autostart (which is not implemented yet)
 */

/*
loading BMediaAddOn from /boot/beos/system/add-ons/media/mixer.media_addon
adding media add-on -1
1 flavors:
flavor 0:
  internal_id = 0
  name = Be Audio Mixer
  info = The system-wide sound mixer of the future.
  flavor_flags = 0x0
  kinds = 0x40003 B_BUFFER_PRODUCER B_BUFFER_CONSUMER B_SYSTEM_MIXER
  in_format_count = 1
  out_format_count = 1
#### WantsAutoStart!
*/

/*
	if (addon->WantsAutoStart()) {
		for (int32 index = 0; ;index++) {
			BMediaNode *outNode;
			int32 outInternalID;
			bool outHasMore;
			rv = addon->AutoStart(index, &outNode, &outInternalID, &outHasMore);
			if (rv == B_OK) {
				printf("started node %ld\n",index);
				rv = mediaroster->RegisterNode(outNode);
				if (rv != B_OK)
					printf("failed to register node %ld\n",index);
				if (!outHasMore)
//					break;
					return;
			} else if (rv == B_MEDIA_ADDON_FAILED && outHasMore) {
				continue;
			} else {
				break;
			}
		}
		return;
	}
*/
	_DormantNodeManager->PutAddon(id);
}


void
MediaAddonServer::AddOnRemoved(ino_t file_node)
{	
	media_addon_id id;
	if (!filemap->Get(file_node,&id)) {
		FATAL("MediaAddonServer::AddOnRemoved: inode %Ld removed, but no media add-on found\n", file_node);
		return;
	}
	filemap->Remove(file_node);
	flavorcountmap->Remove(id);
	_DormantNodeManager->UnregisterAddon(id);
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
	}
	printf("MediaAddonServer: Unhandled message:\n");
	msg->PrintToStream();
}

int main()
{
	new MediaAddonServer("application/x-vnd.OpenBeOS-media-addon-server");
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
