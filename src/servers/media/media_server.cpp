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

#include <Application.h>
#include <stdio.h>
#include <Messenger.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Autolock.h>
#include <string.h>
#include "MMediaFilesManager.h"
#include "NotificationManager.h"
#include "ServerInterface.h"
#include "DataExchange.h"
#include "BufferManager.h"
#include "NodeManager.h"
#include "AddOnManager.h"
#include "AppManager.h"
#include "FormatManager.h"
#include "MediaMisc.h"
#include "media_server.h"
#include "debug.h"

/*
 *
 * An implementation of a new media_server for the OpenBeOS MediaKit
 * Started by Marcus Overhagen <marcus@overhagen.de> on 2001-10-25
 *
 */

AddOnManager *			gAddOnManager;
AppManager *			gAppManager;
BufferManager *			gBufferManager;
FormatManager *			gFormatManager;
MMediaFilesManager *	gMMediaFilesManager;
NodeManager *			gNodeManager;
NotificationManager *	gNotificationManager;

namespace BPrivate { namespace media {
	extern team_id team;
} } // BPrivate::media


#define REPLY_TIMEOUT ((bigtime_t)500000)

class ServerApp : BApplication
{
public:
	ServerApp();
	~ServerApp();

	bool QuitRequested();
	void HandleMessage(int32 code, void *data, size_t size);
	void ArgvReceived(int32 argc, char **argv);
	static int32 controlthread(void *arg);
//	void StartSystemTimeSource();

/* functionality not yet implemented
00014a00 T _ServerApp::_ServerApp(void)
00014e1c T _ServerApp::~_ServerApp(void)
00014ff4 T _ServerApp::MessageReceived(BMessage *);
00015840 T _ServerApp::QuitRequested(void)
00015b50 T _ServerApp::_DoNotify(command_data *)
00015d18 T _ServerApp::_UnregisterApp(long, bool)
00018e90 T _ServerApp::AddOnHost(void)
00019530 T _ServerApp::AboutRequested(void)
00019d04 T _ServerApp::AddPurgableBufferGroup(long, long, long, void *)
00019db8 T _ServerApp::CancelPurgableBufferGroupCleanup(long)
00019e50 T _ServerApp::DirtyWork(void)
0001a4bc T _ServerApp::ArgvReceived(long, char **)
0001a508 T _ServerApp::CleanupPurgedBufferGroup(_ServerApp::purgable_buffer_group const &, bool)
0001a5dc T _ServerApp::DirtyWorkLaunch(void *)
0001a634 T _ServerApp::SetQuitMode(bool)
0001a648 T _ServerApp::IsQuitMode(void) const
0001a658 T _ServerApp::BroadcastCurrentStateTo(BMessenger &)
0001adcc T _ServerApp::ReadyToRun(void)
*/

private:
	port_id		control_port;
	thread_id	control_thread;

	BLocker *fLocker;
	
	virtual void MessageReceived(BMessage *msg);
	virtual void ReadyToRun();
	typedef BApplication inherited;
};

ServerApp::ServerApp()
 	: BApplication(B_MEDIA_SERVER_SIGNATURE),
	fLocker(new BLocker("media server locker"))
{
 	gNotificationManager = new NotificationManager;
 	gBufferManager = new BufferManager;
	gAppManager = new AppManager;
	gNodeManager = new NodeManager;
	gMMediaFilesManager = new MMediaFilesManager;
	gFormatManager = new FormatManager;
	gAddOnManager = new AddOnManager;

	control_port = create_port(64, MEDIA_SERVER_PORT_NAME);
	control_thread = spawn_thread(controlthread, "media_server control", 105, this);
	resume_thread(control_thread);

//	StartSystemTimeSource();
	gNodeManager->LoadState();
	gFormatManager->LoadState();
	

}

void ServerApp::ReadyToRun()
{
	gAppManager->StartAddonServer();
	gAddOnManager->LoadState();
}

ServerApp::~ServerApp()
{
	TRACE("ServerApp::~ServerApp()\n");
	delete gAddOnManager;
	delete gNotificationManager;
	delete gBufferManager;
	delete gAppManager;
	delete gNodeManager;
	delete gMMediaFilesManager;
	delete gFormatManager;
	delete fLocker;
	delete_port(control_port);
	status_t err;
	wait_for_thread(control_thread,&err);
}

void ServerApp::ArgvReceived(int32 argc, char **argv)
{
	for (int arg = 1; arg < argc; arg++) {
		if (strstr(argv[arg], "dump")) {
			gAppManager->Dump();
			gNodeManager->Dump();
			gBufferManager->Dump();
			gNotificationManager->Dump();
			gMMediaFilesManager->Dump();
		}
		if (strstr(argv[arg], "buffer")) {
			gBufferManager->Dump();
		}
		if (strstr(argv[arg], "node")) {
			gNodeManager->Dump();
		}
		if (strstr(argv[arg], "quit")) {
			PostMessage(B_QUIT_REQUESTED);
		}
	}
}

bool
ServerApp::QuitRequested()
{
	TRACE("ServerApp::QuitRequested()\n");
	gMMediaFilesManager->SaveState();
	gNodeManager->SaveState();
	gFormatManager->SaveState();
	gAddOnManager->SaveState();
	gAppManager->TerminateAddonServer();
	return true;
}

/*
void
ServerApp::StartSystemTimeSource()
{
	TRACE("StartSystemTimeSource enter\n");
	status_t rv;

	TRACE("StartSystemTimeSource creating object\n");

	// register a dummy node 
	media_node node;
	rv = gNodeManager->RegisterNode(&node.node, -1, 0, "System Clock", B_TIME_SOURCE, SYSTEM_TIMESOURCE_CONTROL_PORT, BPrivate::media::team);
	ASSERT(rv == B_OK);
	
	ASSERT(node.node == NODE_SYSTEM_TIMESOURCE_ID);

	TRACE("StartSystemTimeSource setting as default\n");
	
	rv = gNodeManager->SetDefaultNode(SYSTEM_TIME_SOURCE, &node, NULL, NULL);
	ASSERT(rv == B_OK);
	
	TRACE("StartSystemTimeSource leave\n");
}
*/

void 
ServerApp::HandleMessage(int32 code, void *data, size_t size)
{
	status_t rv;
	TRACE("ServerApp::HandleMessage %#lx enter\n", code);
	switch (code) {
		case SERVER_CHANGE_ADDON_FLAVOR_INSTANCES_COUNT:
		{
			const server_change_addon_flavor_instances_count_request *request = reinterpret_cast<const server_change_addon_flavor_instances_count_request *>(data);
			server_change_addon_flavor_instances_count_reply reply;
			ASSERT(request->delta == 1 || request->delta == -1);
			if (request->delta == 1)
				rv = gNodeManager->IncrementAddonFlavorInstancesCount(request->addonid,	request->flavorid, request->team);
			else
				rv = gNodeManager->DecrementAddonFlavorInstancesCount(request->addonid,	request->flavorid, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_RESCAN_DEFAULTS:
		{
			gNodeManager->RescanDefaultNodes();
			break;
		}
	
		case SERVER_REGISTER_ADDONSERVER:
		{
			const server_register_addonserver_request *request = reinterpret_cast<const server_register_addonserver_request *>(data);
			server_register_addonserver_reply reply;
			rv = gAppManager->RegisterAddonServer(request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_REGISTER_APP:
		{
			const server_register_app_request *request = reinterpret_cast<const server_register_app_request *>(data);
			server_register_app_reply reply;
			rv = gAppManager->RegisterTeam(request->team, request->messenger);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_APP:
		{
			const server_unregister_app_request *request = reinterpret_cast<const server_unregister_app_request *>(data);
			server_unregister_app_reply reply;
			rv = gAppManager->UnregisterTeam(request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
	
		case SERVER_GET_MEDIAADDON_REF:
		{
			server_get_mediaaddon_ref_request *msg = (server_get_mediaaddon_ref_request *)data;
			server_get_mediaaddon_ref_reply reply;
			entry_ref tempref;
			reply.result = gNodeManager->GetAddonRef(&tempref, msg->addonid);
			reply.ref = tempref;
			write_port(msg->reply_port, 0, &reply, sizeof(reply));
			break;
		}

		case SERVER_NODE_ID_FOR:
		{
			const server_node_id_for_request *request = reinterpret_cast<const server_node_id_for_request *>(data);
			server_node_id_for_reply reply;
			rv = gNodeManager->FindNodeId(&reply.nodeid, request->port);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_GET_LIVE_NODE_INFO:
		{
			const server_get_live_node_info_request *request = reinterpret_cast<const server_get_live_node_info_request *>(data);
			server_get_live_node_info_reply reply;
			rv = gNodeManager->GetLiveNodeInfo(&reply.live_info, request->node);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_GET_LIVE_NODES:
		{
			const server_get_live_nodes_request *request = reinterpret_cast<const server_get_live_nodes_request *>(data);
			server_get_live_nodes_reply reply;
			Stack<live_node_info> livenodes;
			rv = gNodeManager->GetLiveNodes(
					&livenodes,
					request->maxcount,
					request->has_input ? &request->inputformat : NULL,
					request->has_output ? &request->outputformat : NULL,
					request->has_name ? request->name : NULL,
					request->require_kinds);
			reply.count = livenodes.CountItems();
			if (reply.count <= MAX_LIVE_INFO) {
				for (int32 index = 0; index < reply.count; index++)
					livenodes.Pop(&reply.live_info[index]);
				reply.area = -1;
			} else {
				// we create an area here, and pass it to the library, where it will be deleted.
				live_node_info *start_addr;
				size_t size;
				size = ((reply.count * sizeof(live_node_info)) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
				reply.area = create_area("get live nodes", reinterpret_cast<void **>(&start_addr), B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
				if (reply.area < B_OK) {
					ERROR("SERVER_GET_LIVE_NODES: failed to create area, error %#lx\n", reply.area);
					reply.count = 0;
					rv = B_ERROR;
				} else {
					for (int32 index = 0; index < reply.count; index++)
						livenodes.Pop(&start_addr[index]);
				}
			}
			rv = request->SendReply(rv, &reply, sizeof(reply));
			if (rv != B_OK)
				delete_area(reply.area); // if we couldn't send the message, delete the area
			break;
		}
		
		case SERVER_GET_NODE_FOR:
		{
			const server_get_node_for_request *request = reinterpret_cast<const server_get_node_for_request *>(data);
			server_get_node_for_reply reply;
			rv = gNodeManager->GetCloneForId(&reply.clone, request->nodeid, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_RELEASE_NODE:
		{
			const server_release_node_request *request = reinterpret_cast<const server_release_node_request *>(data);
			server_release_node_reply reply;
			rv = gNodeManager->ReleaseNode(request->node, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_REGISTER_NODE:
		{
			const server_register_node_request *request = reinterpret_cast<const server_register_node_request *>(data);
			server_register_node_reply reply;
			rv = gNodeManager->RegisterNode(&reply.nodeid, request->addon_id, request->addon_flavor_id, request->name, request->kinds, request->port, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_UNREGISTER_NODE:
		{
			const server_unregister_node_request *request = reinterpret_cast<const server_unregister_node_request *>(data);
			server_unregister_node_reply reply;
			rv = gNodeManager->UnregisterNode(&reply.addonid, &reply.flavorid, request->nodeid, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_PUBLISH_INPUTS:
		{
			const server_publish_inputs_request *request = reinterpret_cast<const server_publish_inputs_request *>(data);
			server_publish_inputs_reply reply;
			if (request->count <= MAX_INPUTS) {
				rv = gNodeManager->PublishInputs(request->node, request->inputs, request->count);
			} else {
				media_input *inputs;
				area_id clone;
				clone = clone_area("media_inputs clone", reinterpret_cast<void **>(&inputs), B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, request->area);
				if (clone < B_OK) {
					ERROR("SERVER_PUBLISH_INPUTS: failed to clone area, error %#lx\n", clone);
					rv = B_ERROR;
				} else {
					rv = gNodeManager->PublishInputs(request->node, inputs, request->count);
					delete_area(clone);
				}
			}
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_PUBLISH_OUTPUTS:
		{
			const server_publish_outputs_request *request = reinterpret_cast<const server_publish_outputs_request *>(data);
			server_publish_outputs_reply reply;
			if (request->count <= MAX_OUTPUTS) {
				rv = gNodeManager->PublishOutputs(request->node, request->outputs, request->count);
			} else {
				media_output *outputs;
				area_id clone;
				clone = clone_area("media_outputs clone", reinterpret_cast<void **>(&outputs), B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, request->area);
				if (clone < B_OK) {
					ERROR("SERVER_PUBLISH_OUTPUTS: failed to clone area, error %#lx\n", clone);
					rv = B_ERROR;
				} else {
					rv = gNodeManager->PublishOutputs(request->node, outputs, request->count);
					delete_area(clone);
				}
			}
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_NODE:
		{
			const server_get_node_request *request = reinterpret_cast<const server_get_node_request *>(data);
			server_get_node_reply reply;
			rv = gNodeManager->GetClone(&reply.node, reply.input_name, &reply.input_id, request->type, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_SET_NODE:
		{
			const server_set_node_request *request = reinterpret_cast<const server_set_node_request *>(data);
			server_set_node_reply reply;
			rv = gNodeManager->SetDefaultNode(request->type, request->use_node ? &request->node : NULL, request->use_dni ? &request->dni : NULL, request->use_input ?  &request->input : NULL);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_GET_DORMANT_NODE_FOR:
		{
			const server_get_dormant_node_for_request *request = reinterpret_cast<const server_get_dormant_node_for_request *>(data);
			server_get_dormant_node_for_reply reply;
			rv = gNodeManager->GetDormantNodeInfo(&reply.node_info, request->node);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_INSTANCES_FOR:
		{
			const server_get_instances_for_request *request = reinterpret_cast<const server_get_instances_for_request *>(data);
			server_get_instances_for_reply reply;
			rv = gNodeManager->GetInstances(reply.node_id, &reply.count, min_c(request->maxcount, MAX_NODE_ID), request->addon_id, request->addon_flavor_id);
			if (reply.count == MAX_NODE_ID && request->maxcount > MAX_NODE_ID) { // XXX might be fixed by using an area
				PRINT(1, "Warning: SERVER_GET_INSTANCES_FOR: returning possibly truncated list of node id's\n");
			}
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_REGISTER_MEDIAADDON:
		{
			server_register_mediaaddon_request *msg = (server_register_mediaaddon_request *)data;
			server_register_mediaaddon_reply reply;
			gNodeManager->RegisterAddon(msg->ref, &reply.addonid);
			write_port(msg->reply_port, 0, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_UNREGISTER_MEDIAADDON:
		{
			server_unregister_mediaaddon_command *msg = (server_unregister_mediaaddon_command *)data;
			gNodeManager->UnregisterAddon(msg->addonid);
			break;
		}
		
		case SERVER_REGISTER_DORMANT_NODE:
		{
			xfer_server_register_dormant_node *msg = (xfer_server_register_dormant_node *)data;
			dormant_flavor_info dfi;
			if (msg->purge_id > 0)
				gNodeManager->InvalidateDormantFlavorInfo(msg->purge_id);
			rv = dfi.Unflatten(msg->dfi_type, &(msg->dfi), msg->dfi_size);
			ASSERT(rv == B_OK);
			gNodeManager->AddDormantFlavorInfo(dfi);	
			break;
		}
		
		case SERVER_GET_DORMANT_NODES:
		{
			xfer_server_get_dormant_nodes *msg = (xfer_server_get_dormant_nodes *)data;
			xfer_server_get_dormant_nodes_reply reply;
			dormant_node_info * infos = new dormant_node_info[msg->maxcount];
			reply.count = msg->maxcount;
			reply.result = gNodeManager->GetDormantNodes(
				infos, 
				&reply.count,
				msg->has_input ? &msg->inputformat : NULL,
				msg->has_output ? &msg->outputformat : NULL,
				msg->has_name ? msg->name : NULL,
				msg->require_kinds,
				msg->deny_kinds);
			if (reply.result != B_OK)
				reply.count = 0;
			write_port(msg->reply_port, 0, &reply, sizeof(reply));
			if (reply.count > 0)
				write_port(msg->reply_port, 0, infos, reply.count * sizeof(dormant_node_info));
			delete [] infos;
			break;
		}
		
		case SERVER_GET_DORMANT_FLAVOR_INFO:
		{
			xfer_server_get_dormant_flavor_info *msg = (xfer_server_get_dormant_flavor_info *)data;
			dormant_flavor_info dfi;
			status_t rv;

			rv = gNodeManager->GetDormantFlavorInfoFor(msg->addon, msg->flavor_id, &dfi);
			if (rv != B_OK) {
				xfer_server_get_dormant_flavor_info_reply reply;
				reply.result = rv;
				write_port(msg->reply_port, 0, &reply, sizeof(reply));
			} else {
				xfer_server_get_dormant_flavor_info_reply *reply;
				int replysize;
				replysize = sizeof(xfer_server_get_dormant_flavor_info_reply) + dfi.FlattenedSize();
				reply = (xfer_server_get_dormant_flavor_info_reply *)malloc(replysize);

				reply->dfi_size = dfi.FlattenedSize();
				reply->dfi_type = dfi.TypeCode();
				reply->result = dfi.Flatten(reply->dfi, reply->dfi_size);
				write_port(msg->reply_port, 0, reply, replysize);
				free(reply);
			}
			break;
		}

		case SERVER_SET_NODE_CREATOR:
		{
			const server_set_node_creator_request *request = reinterpret_cast<const server_set_node_creator_request *>(data);
			server_set_node_creator_reply reply;
			rv = gNodeManager->SetNodeCreator(request->node, request->creator);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_SHARED_BUFFER_AREA:
		{
			const server_get_shared_buffer_area_request *request = reinterpret_cast<const server_get_shared_buffer_area_request *>(data);
			server_get_shared_buffer_area_reply reply;

			reply.area = gBufferManager->SharedBufferListID();
			request->SendReply(B_OK, &reply, sizeof(reply));
			break;
		}
				
		case SERVER_REGISTER_BUFFER:
		{
			const server_register_buffer_request *request = reinterpret_cast<const server_register_buffer_request *>(data);
			server_register_buffer_reply reply;
			status_t status;
			if (request->info.buffer == 0) {
				reply.info = request->info; //size, offset, flags, area is kept
				// get a new beuffer id into reply.info.buffer 
				status = gBufferManager->RegisterBuffer(request->team, request->info.size, request->info.flags, request->info.offset, request->info.area, &reply.info.buffer);
			} else {
				reply.info = request->info; //buffer id is kept
				status = gBufferManager->RegisterBuffer(request->team, request->info.buffer, &reply.info.size, &reply.info.flags, &reply.info.offset, &reply.info.area);
			}
			request->SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_BUFFER:
		{
			const server_unregister_buffer_command *cmd = reinterpret_cast<const server_unregister_buffer_command *>(data);

			gBufferManager->UnregisterBuffer(cmd->team, cmd->bufferid);
			break;
		}
		
		case SERVER_REWINDTYPES:
		{
			const server_rewindtypes_request *request = reinterpret_cast<const server_rewindtypes_request *>(data);
			server_rewindtypes_reply reply;
			
			BString **types = NULL;
			
			rv = gMMediaFilesManager->RewindTypes(
					&types, &reply.count);
			if(reply.count>0) {
				// we create an area here, and pass it to the library, where it will be deleted.
				char *start_addr;
				size_t size = ((reply.count * B_MEDIA_NAME_LENGTH) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
				reply.area = create_area("rewind types", reinterpret_cast<void **>(&start_addr), B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
				if (reply.area < B_OK) {
					ERROR("SERVER_REWINDTYPES: failed to create area, error %s\n", strerror(reply.area));
					reply.count = 0;
					rv = B_ERROR;
				} else {
					for (int32 index = 0; index < reply.count; index++)
						strncpy(start_addr + B_MEDIA_NAME_LENGTH * index, types[index]->String(), B_MEDIA_NAME_LENGTH);
				}
			}
			
			delete types;
			
			rv = request->SendReply(rv, &reply, sizeof(reply));
			if (rv != B_OK)
				delete_area(reply.area); // if we couldn't send the message, delete the area
			break;
		}
		
		case SERVER_REWINDREFS:
		{
			const server_rewindrefs_request *request = reinterpret_cast<const server_rewindrefs_request *>(data);
			server_rewindrefs_reply reply;
			
			BString **items = NULL;
			
			rv = gMMediaFilesManager->RewindRefs(request->type,
					&items, &reply.count);
			// we create an area here, and pass it to the library, where it will be deleted.
			if(reply.count>0) {
				char *start_addr;
				size_t size = ((reply.count * B_MEDIA_NAME_LENGTH) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
				reply.area = create_area("rewind refs", reinterpret_cast<void **>(&start_addr), B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
				if (reply.area < B_OK) {
					ERROR("SERVER_REWINDREFS: failed to create area, error %s\n", strerror(reply.area));
					reply.count = 0;
					rv = B_ERROR;
				} else {
					for (int32 index = 0; index < reply.count; index++)
						strncpy(start_addr + B_MEDIA_NAME_LENGTH * index, items[index]->String(), B_MEDIA_NAME_LENGTH);
				}
			}
			
			delete items;

			rv = request->SendReply(rv, &reply, sizeof(reply));
			if (rv != B_OK)
				delete_area(reply.area); // if we couldn't send the message, delete the area
			break;
		}
		
		case SERVER_GETREFFOR:
		{
			const server_getreffor_request *request = reinterpret_cast<const server_getreffor_request *>(data);
			server_getreffor_reply reply;
			entry_ref *ref;

			rv = gMMediaFilesManager->GetRefFor(request->type, request->item, &ref);
			if(rv==B_OK) {
				reply.ref = *ref;
			}
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_SETREFFOR:
		{
			const server_setreffor_request *request = reinterpret_cast<const server_setreffor_request *>(data);
			server_setreffor_reply reply;
			entry_ref ref = request->ref;
			
			rv = gMMediaFilesManager->SetRefFor(request->type, request->item, ref);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_REMOVEREFFOR:
		{
			const server_removereffor_request *request = reinterpret_cast<const server_removereffor_request *>(data);
			server_removereffor_reply reply;
			entry_ref ref = request->ref;
			
			rv = gMMediaFilesManager->RemoveRefFor(request->type, request->item, ref);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		case SERVER_REMOVEITEM:
		{
			const server_removeitem_request *request = reinterpret_cast<const server_removeitem_request *>(data);
			server_removeitem_reply reply;

			rv = gMMediaFilesManager->RemoveItem(request->type, request->item);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_READERS:
		{
			const server_get_readers_request *request = reinterpret_cast<const server_get_readers_request *>(data);
			server_get_readers_reply reply;
			rv = gAddOnManager->GetReaders(reply.ref, &reply.count, MAX_READERS);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_DECODER_FOR_FORMAT:
		{
			const server_get_decoder_for_format_request *request = reinterpret_cast<const server_get_decoder_for_format_request *>(data);
			server_get_decoder_for_format_reply reply;
			rv = gAddOnManager->GetDecoderForFormat(&reply.ref, request->format);						 
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}
		
		default:
			printf("media_server: received unknown message code %#08lx\n",code);
	}
	TRACE("ServerApp::HandleMessage %#lx leave\n", code);
}

int32
ServerApp::controlthread(void *arg)
{
	char data[B_MEDIA_MESSAGE_SIZE];
	ServerApp *app;
	ssize_t size;
	int32 code;
	
	app = (ServerApp *)arg;
	while ((size = read_port_etc(app->control_port, &code, data, sizeof(data), 0, 0)) > 0)
		app->HandleMessage(code, data, size);

	return 0;
}

void 
ServerApp::MessageReceived(BMessage *msg)
{
	TRACE("ServerApp::MessageReceived %lx enter\n", msg->what);
	switch (msg->what) {
		case MEDIA_SERVER_REQUEST_NOTIFICATIONS:
		case MEDIA_SERVER_CANCEL_NOTIFICATIONS:
		case MEDIA_SERVER_SEND_NOTIFICATIONS:
			gNotificationManager->EnqueueMessage(msg);
			break;

		case MMEDIAFILESMANAGER_SAVE_TIMER:
			gMMediaFilesManager->TimerMessage();
			break;
		
		case MEDIA_SERVER_GET_FORMATS:
			gFormatManager->GetFormats(*msg);
			break;

		case MEDIA_SERVER_MAKE_FORMAT_FOR:
			gFormatManager->MakeFormatFor(*msg);
			break;

		default:
			inherited::MessageReceived(msg);
			//printf("\nnew media server: unknown message received\n");
			//msg->PrintToStream();
			break;
	}
	TRACE("ServerApp::MessageReceived %lx leave\n", msg->what);
}

int
main()
{
	new ServerApp;
	be_app->Run();
	delete be_app;
	return 0;
}
