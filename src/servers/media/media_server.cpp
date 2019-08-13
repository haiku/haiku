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
char __dont_remove_copyright_from_binary[] = "Copyright (c) 2002, 2003 "
	"Marcus Overhagen <Marcus@Overhagen.de>";


#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Autolock.h>
#include <Directory.h>
#include <Roster.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Messenger.h>
#include <Server.h>

#include <syscalls.h>

#include "AppManager.h"
#include "BufferManager.h"
#include "DataExchange.h"
#include "MediaDebug.h"
#include "MediaFilesManager.h"
#include "MediaMisc.h"
#include "NodeManager.h"
#include "NotificationManager.h"
#include "ServerInterface.h"
#include "media_server.h"


AppManager* gAppManager;
BufferManager* gBufferManager;
MediaFilesManager* gMediaFilesManager;
NodeManager* gNodeManager;
NotificationManager* gNotificationManager;


#define REPLY_TIMEOUT ((bigtime_t)500000)


class ServerApp : public BServer {
public:
								ServerApp(status_t& error);
	virtual						~ServerApp();

protected:
	virtual void				ArgvReceived(int32 argc, char** argv);
	virtual void				ReadyToRun();
	virtual bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);

private:
			void				_HandleMessage(int32 code, const void* data,
									size_t size);
			void				_LaunchAddOnServer();
			void				_QuitAddOnServer();

private:
			port_id				_ControlPort() const { return fControlPort; }

			static	int32		_ControlThread(void* arg);

			BLocker				fLocker;
			port_id				fControlPort;
			thread_id			fControlThread;
};


ServerApp::ServerApp(status_t& error)
 	:
	BServer(B_MEDIA_SERVER_SIGNATURE, true, &error),
	fLocker("media server locker")
{
 	gNotificationManager = new NotificationManager;
 	gBufferManager = new BufferManager;
	gAppManager = new AppManager;
	gNodeManager = new NodeManager;
	gMediaFilesManager = new MediaFilesManager;

	fControlPort = create_port(64, MEDIA_SERVER_PORT_NAME);
	fControlThread = spawn_thread(_ControlThread, "media_server control", 105,
		this);
	resume_thread(fControlThread);

	if (be_roster->StartWatching(BMessenger(this, this),
			B_REQUEST_QUIT) != B_OK) {
		TRACE("ServerApp: Can't find the registrar.");
	}
}


ServerApp::~ServerApp()
{
	TRACE("ServerApp::~ServerApp()\n");

	delete_port(fControlPort);
	wait_for_thread(fControlThread, NULL);

	if (be_roster->StopWatching(BMessenger(this, this)) != B_OK)
		TRACE("ServerApp: Can't unregister roster notifications.");

	delete gNotificationManager;
	delete gBufferManager;
	delete gAppManager;
	delete gNodeManager;
	delete gMediaFilesManager;
}


void
ServerApp::ReadyToRun()
{
	gNodeManager->LoadState();

	// make sure any previous media_addon_server is gone
	_QuitAddOnServer();
	// and start a new one
	_LaunchAddOnServer();

}


bool
ServerApp::QuitRequested()
{
	TRACE("ServerApp::QuitRequested()\n");
	gMediaFilesManager->SaveState();
	gNodeManager->SaveState();

	_QuitAddOnServer();

	return true;
}


void
ServerApp::ArgvReceived(int32 argc, char **argv)
{
	for (int arg = 1; arg < argc; arg++) {
		if (strstr(argv[arg], "dump") != NULL) {
			gAppManager->Dump();
			gNodeManager->Dump();
			gBufferManager->Dump();
			gNotificationManager->Dump();
			gMediaFilesManager->Dump();
		}
		if (strstr(argv[arg], "buffer") != NULL)
			gBufferManager->Dump();
		if (strstr(argv[arg], "node") != NULL)
			gNodeManager->Dump();
		if (strstr(argv[arg], "files") != NULL)
			gMediaFilesManager->Dump();
		if (strstr(argv[arg], "quit") != NULL)
			PostMessage(B_QUIT_REQUESTED);
	}
}


void
ServerApp::_LaunchAddOnServer()
{
	// Try to launch media_addon_server by mime signature.
	// If it fails (for example on the Live CD, where the executable
	// hasn't yet been mimesetted), try from this application's
	// directory
	status_t err = be_roster->Launch(B_MEDIA_ADDON_SERVER_SIGNATURE);
	if (err == B_OK)
		return;

	app_info info;
	BEntry entry;
	BDirectory dir;
	entry_ref ref;

	err = GetAppInfo(&info);
	err |= entry.SetTo(&info.ref);
	err |= entry.GetParent(&entry);
	err |= dir.SetTo(&entry);
	err |= entry.SetTo(&dir, "media_addon_server");
	err |= entry.GetRef(&ref);

	if (err == B_OK)
		be_roster->Launch(&ref);
	if (err == B_OK)
		return;

	BAlert* alert = new BAlert("media_server", "Launching media_addon_server "
		"failed.\n\nmedia_server will terminate", "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	fprintf(stderr, "Launching media_addon_server (%s) failed: %s\n",
		B_MEDIA_ADDON_SERVER_SIGNATURE, strerror(err));
	exit(1);
}


void
ServerApp::_QuitAddOnServer()
{
	// nothing to do if it's already terminated
	if (!be_roster->IsRunning(B_MEDIA_ADDON_SERVER_SIGNATURE))
		return;

	// send a quit request to the media_addon_server
	BMessenger msger(B_MEDIA_ADDON_SERVER_SIGNATURE);
	if (!msger.IsValid()) {
		ERROR("Trouble terminating media_addon_server. Messenger invalid\n");
	} else {
		BMessage msg(B_QUIT_REQUESTED);
		status_t err = msger.SendMessage(&msg, (BHandler *)NULL, 2000000);
			// 2 sec timeout
		if (err != B_OK) {
			ERROR("Trouble terminating media_addon_server (2): %s\n",
				strerror(err));
		}
	}

	// wait 5 seconds for it to terminate
	for (int i = 0; i < 50; i++) {
		if (!be_roster->IsRunning(B_MEDIA_ADDON_SERVER_SIGNATURE))
			return;
		snooze(100000); // 100 ms
	}

	// try to kill it (or many of them), up to 10 seconds
	for (int i = 0; i < 50; i++) {
		team_id id = be_roster->TeamFor(B_MEDIA_ADDON_SERVER_SIGNATURE);
		if (id < 0)
			break;
		kill_team(id);
		snooze(200000); // 200 ms
	}

	if (be_roster->IsRunning(B_MEDIA_ADDON_SERVER_SIGNATURE)) {
		ERROR("Trouble terminating media_addon_server, it's still running\n");
	}
}


void
ServerApp::_HandleMessage(int32 code, const void* data, size_t size)
{
	TRACE("ServerApp::HandleMessage %#" B_PRIx32 " enter\n", code);
	switch (code) {
		case SERVER_CHANGE_FLAVOR_INSTANCES_COUNT:
		{
			const server_change_flavor_instances_count_request& request
				= *static_cast<
					const server_change_flavor_instances_count_request*>(data);
			server_change_flavor_instances_count_reply reply;
			status_t status = B_BAD_VALUE;

			if (request.delta == 1) {
				status = gNodeManager->IncrementFlavorInstancesCount(
					request.add_on_id, request.flavor_id, request.team);
			} else if (request.delta == -1) {
				status = gNodeManager->DecrementFlavorInstancesCount(
					request.add_on_id, request.flavor_id, request.team);
			}
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_RESCAN_DEFAULTS:
		{
			gNodeManager->RescanDefaultNodes();
			break;
		}

		case SERVER_REGISTER_APP:
		{
			const server_register_app_request& request = *static_cast<
				const server_register_app_request*>(data);
			server_register_app_reply reply;

			status_t status = gAppManager->RegisterTeam(request.team,
				request.messenger);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_APP:
		{
			const server_unregister_app_request& request = *static_cast<
				const server_unregister_app_request*>(data);
			server_unregister_app_reply reply;

			status_t status = gAppManager->UnregisterTeam(request.team);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_ADD_ON_REF:
		{
			const server_get_add_on_ref_request& request = *static_cast<
				const server_get_add_on_ref_request*>(data);
			server_get_add_on_ref_reply reply;

			entry_ref ref;
			reply.result = gNodeManager->GetAddOnRef(request.add_on_id, &ref);
			reply.ref = ref;

			request.SendReply(reply.result, &reply, sizeof(reply));
			break;
		}

		case SERVER_NODE_ID_FOR:
		{
			const server_node_id_for_request& request
				= *static_cast<const server_node_id_for_request*>(data);
			server_node_id_for_reply reply;

			status_t status = gNodeManager->FindNodeID(request.port,
				&reply.node_id);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_LIVE_NODE_INFO:
		{
			const server_get_live_node_info_request& request = *static_cast<
				const server_get_live_node_info_request*>(data);
			server_get_live_node_info_reply reply;

			status_t status = gNodeManager->GetLiveNodeInfo(request.node,
				&reply.live_info);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_LIVE_NODES:
		{
			const server_get_live_nodes_request& request
				= *static_cast<const server_get_live_nodes_request*>(data);
			server_get_live_nodes_reply reply;
			LiveNodeList nodes;

			status_t status = gNodeManager->GetLiveNodes(nodes,
				request.max_count,
				request.has_input ? &request.input_format : NULL,
				request.has_output ? &request.output_format : NULL,
				request.has_name ? request.name : NULL, request.require_kinds);

			reply.count = nodes.size();
			reply.area = -1;

			live_node_info* infos = reply.live_info;
			area_id area = -1;

			if (reply.count > MAX_LIVE_INFO) {
				// We create an area here, and transfer it to the client
				size_t size = (reply.count * sizeof(live_node_info)
					+ B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

				area = create_area("get live nodes", (void**)&infos,
					B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
				if (area < 0) {
					reply.area = area;
					reply.count = 0;
				}
			}

			for (int32 index = 0; index < reply.count; index++)
				infos[index] = nodes[index];

			if (area >= 0) {
				// transfer the area to the target team
				reply.area = _kern_transfer_area(area, &reply.address,
					B_ANY_ADDRESS, request.team);
				if (reply.area < 0) {
					delete_area(area);
					reply.count = 0;
				}
			}

			status = request.SendReply(status, &reply, sizeof(reply));
			if (status != B_OK && reply.area >= 0) {
				// if we couldn't send the message, delete the area
				delete_area(reply.area);
			}
			break;
		}

		case SERVER_GET_NODE_FOR:
		{
			const server_get_node_for_request& request
				= *static_cast<const server_get_node_for_request*>(data);
			server_get_node_for_reply reply;

			status_t status = gNodeManager->GetCloneForID(request.node_id,
				request.team, &reply.clone);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_RELEASE_NODE:
		{
			const server_release_node_request& request
				= *static_cast<const server_release_node_request*>(data);
			server_release_node_reply reply;

			status_t status = gNodeManager->ReleaseNode(request.node,
				request.team);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_RELEASE_NODE_ALL:
		{
			const server_release_node_request& request
				= *static_cast<const server_release_node_request*>(data);
			server_release_node_reply reply;

			status_t status = gNodeManager->ReleaseNodeAll(request.node.node);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_REGISTER_NODE:
		{
			const server_register_node_request& request
				= *static_cast<const server_register_node_request*>(data);
			server_register_node_reply reply;

			status_t status = gNodeManager->RegisterNode(request.add_on_id,
				request.flavor_id, request.name, request.kinds, request.port,
				request.team, request.timesource_id, &reply.node_id);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_NODE:
		{
			const server_unregister_node_request& request
				= *static_cast<const server_unregister_node_request*>(data);
			server_unregister_node_reply reply;

			status_t status = gNodeManager->UnregisterNode(request.node_id,
				request.team, &reply.add_on_id, &reply.flavor_id);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_PUBLISH_INPUTS:
		{
			const server_publish_inputs_request& request
				= *static_cast<const server_publish_inputs_request*>(data);
			server_publish_inputs_reply reply;
			status_t status;

			if (request.count <= MAX_INPUTS) {
				status = gNodeManager->PublishInputs(request.node,
					request.inputs, request.count);
			} else {
				media_input* inputs;
				area_id clone;
				clone = clone_area("media_inputs clone", (void**)&inputs,
					B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, request.area);
				if (clone < B_OK) {
					ERROR("SERVER_PUBLISH_INPUTS: failed to clone area, "
						"error %#" B_PRIx32 "\n", clone);
					status = clone;
				} else {
					status = gNodeManager->PublishInputs(request.node, inputs,
						request.count);
					delete_area(clone);
				}
			}
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_PUBLISH_OUTPUTS:
		{
			const server_publish_outputs_request& request
				= *static_cast<const server_publish_outputs_request*>(data);
			server_publish_outputs_reply reply;
			status_t status;

			if (request.count <= MAX_OUTPUTS) {
				status = gNodeManager->PublishOutputs(request.node,
					request.outputs, request.count);
			} else {
				media_output* outputs;
				area_id clone;
				clone = clone_area("media_outputs clone", (void**)&outputs,
					B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, request.area);
				if (clone < B_OK) {
					ERROR("SERVER_PUBLISH_OUTPUTS: failed to clone area, "
						"error %#" B_PRIx32 "\n", clone);
					status = clone;
				} else {
					status = gNodeManager->PublishOutputs(request.node, outputs,
						request.count);
					delete_area(clone);
				}
			}
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_NODE:
		{
			const server_get_node_request& request
				= *static_cast<const server_get_node_request*>(data);
			server_get_node_reply reply;

			status_t status = gNodeManager->GetClone(request.type, request.team,
				&reply.node, reply.input_name, &reply.input_id);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_SET_NODE:
		{
			const server_set_node_request& request
				= *static_cast<const server_set_node_request*>(data);
			server_set_node_reply reply;

			status_t status = gNodeManager->SetDefaultNode(request.type,
				request.use_node ? &request.node : NULL,
				request.use_dni ? &request.dni : NULL,
				request.use_input ?  &request.input : NULL);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_DORMANT_NODE_FOR:
		{
			const server_get_dormant_node_for_request& request
				= *static_cast<const server_get_dormant_node_for_request*>(
					data);
			server_get_dormant_node_for_reply reply;

			status_t status = gNodeManager->GetDormantNodeInfo(request.node,
				&reply.node_info);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_INSTANCES_FOR:
		{
			const server_get_instances_for_request& request
				= *static_cast<const server_get_instances_for_request*>(data);
			server_get_instances_for_reply reply;

			status_t status = gNodeManager->GetInstances(request.add_on_id,
				request.flavor_id, reply.node_id, &reply.count,
				min_c(request.max_count, MAX_NODE_ID));
			if (reply.count == MAX_NODE_ID
				&& request.max_count > MAX_NODE_ID) {
				// TODO: might be fixed by using an area
				PRINT(1, "Warning: SERVER_GET_INSTANCES_FOR: returning "
					"possibly truncated list of node id's\n");
			}
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_SET_NODE_TIMESOURCE:
		{
			const server_set_node_timesource_request& request
				= *static_cast<const server_set_node_timesource_request*>(data);
			server_set_node_timesource_reply reply;
			status_t result = gNodeManager->SetNodeTimeSource(request.node_id,
				request.timesource_id);
			request.SendReply(result, &reply, sizeof(reply));
			break;
		}

		case SERVER_REGISTER_ADD_ON:
		{
			const server_register_add_on_request& request = *static_cast<
				const server_register_add_on_request*>(data);
			server_register_add_on_reply reply;

			gNodeManager->RegisterAddOn(request.ref, &reply.add_on_id);
			request.SendReply(B_OK, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_ADD_ON:
		{
			const server_unregister_add_on_command& request = *static_cast<
				const server_unregister_add_on_command*>(data);
			gNodeManager->UnregisterAddOn(request.add_on_id);
			break;
		}

		case SERVER_REGISTER_DORMANT_NODE:
		{
			const server_register_dormant_node_command& command
				= *static_cast<const server_register_dormant_node_command*>(
					data);
			if (command.purge_id > 0)
				gNodeManager->InvalidateDormantFlavorInfo(command.purge_id);

			dormant_flavor_info dormantFlavorInfo;
			status_t status = dormantFlavorInfo.Unflatten(command.type,
				command.flattened_data, command.flattened_size);
			if (status == B_OK)
				gNodeManager->AddDormantFlavorInfo(dormantFlavorInfo);
			break;
		}

		case SERVER_GET_DORMANT_NODES:
		{
			const server_get_dormant_nodes_request& request
				= *static_cast<const server_get_dormant_nodes_request*>(data);

			server_get_dormant_nodes_reply reply;
			reply.count = request.max_count;

			dormant_node_info* infos
				= new(std::nothrow) dormant_node_info[reply.count];
			if (infos != NULL) {
				reply.result = gNodeManager->GetDormantNodes(infos,
					&reply.count,
					request.has_input ? &request.input_format : NULL,
					request.has_output ? &request.output_format : NULL,
					request.has_name ? request.name : NULL,
					request.require_kinds, request.deny_kinds);
			} else
				reply.result = B_NO_MEMORY;

			if (reply.result != B_OK)
				reply.count = 0;

			request.SendReply(reply.result, &reply, sizeof(reply));
			if (reply.count > 0) {
				write_port(request.reply_port, 0, infos,
					reply.count * sizeof(dormant_node_info));
			}
			delete[] infos;
			break;
		}

		case SERVER_GET_DORMANT_FLAVOR_INFO:
		{
			const server_get_dormant_flavor_info_request& request
				= *static_cast<const server_get_dormant_flavor_info_request*>(
					data);
			dormant_flavor_info dormantFlavorInfo;

			status_t status = gNodeManager->GetDormantFlavorInfoFor(
				request.add_on_id, request.flavor_id, &dormantFlavorInfo);
			if (status != B_OK) {
				server_get_dormant_flavor_info_reply reply;
				reply.result = status;
				request.SendReply(reply.result, &reply, sizeof(reply));
			} else {
				size_t replySize
					= sizeof(server_get_dormant_flavor_info_reply)
						+ dormantFlavorInfo.FlattenedSize();
				server_get_dormant_flavor_info_reply* reply
					= (server_get_dormant_flavor_info_reply*)malloc(
						replySize);
				if (reply != NULL) {
					reply->type = dormantFlavorInfo.TypeCode();
					reply->flattened_size = dormantFlavorInfo.FlattenedSize();
					reply->result = dormantFlavorInfo.Flatten(
						reply->flattened_data, reply->flattened_size);

					request.SendReply(reply->result, reply, replySize);
					free(reply);
				} else {
					server_get_dormant_flavor_info_reply reply;
					reply.result = B_NO_MEMORY;
					request.SendReply(reply.result, &reply, sizeof(reply));
				}
			}
			break;
		}

		case SERVER_SET_NODE_CREATOR:
		{
			const server_set_node_creator_request& request
				= *static_cast<const server_set_node_creator_request*>(data);
			server_set_node_creator_reply reply;
			status_t status = gNodeManager->SetNodeCreator(request.node,
				request.creator);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_SHARED_BUFFER_AREA:
		{
			const server_get_shared_buffer_area_request& request
				= *static_cast<const server_get_shared_buffer_area_request*>(
					data);
			server_get_shared_buffer_area_reply reply;

			reply.area = gBufferManager->SharedBufferListArea();
			request.SendReply(reply.area >= 0 ? B_OK : reply.area, &reply,
				sizeof(reply));
			break;
		}

		case SERVER_REGISTER_BUFFER:
		{
			const server_register_buffer_request& request
				= *static_cast<const server_register_buffer_request*>(data);
			server_register_buffer_reply reply;
			status_t status;

			if (request.info.buffer == 0) {
				reply.info = request.info;
				// size, offset, flags, area is kept
				// get a new beuffer id into reply.info.buffer
				status = gBufferManager->RegisterBuffer(request.team,
					request.info.size, request.info.flags,
					request.info.offset, request.info.area,
					&reply.info.buffer);
			} else {
				reply.info = request.info; // buffer id is kept
				status = gBufferManager->RegisterBuffer(request.team,
					request.info.buffer, &reply.info.size, &reply.info.flags,
					&reply.info.offset, &reply.info.area);
			}
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_BUFFER:
		{
			const server_unregister_buffer_command& request = *static_cast<
				const server_unregister_buffer_command*>(data);

			gBufferManager->UnregisterBuffer(request.team, request.buffer_id);
			break;
		}

		case SERVER_GET_MEDIA_FILE_TYPES:
		{
			const server_get_media_types_request& request
				= *static_cast<const server_get_media_types_request*>(data);

			server_get_media_types_reply reply;
			area_id area = gMediaFilesManager->GetTypesArea(reply.count);
			if (area >= 0) {
				// transfer the area to the target team
				reply.area = _kern_transfer_area(area, &reply.address,
					B_ANY_ADDRESS, request.team);
				if (reply.area < 0) {
					delete_area(area);
					reply.area = B_ERROR;
					reply.count = 0;
				}
			}

			status_t status = request.SendReply(
				reply.area < 0 ? reply.area : B_OK, &reply, sizeof(reply));
			if (status != B_OK) {
				// if we couldn't send the message, delete the area
				delete_area(reply.area);
			}
			break;
		}

		case SERVER_GET_MEDIA_FILE_ITEMS:
		{
			const server_get_media_items_request& request
				= *static_cast<const server_get_media_items_request*>(data);

			server_get_media_items_reply reply;
			area_id area = gMediaFilesManager->GetItemsArea(request.type,
				reply.count);
			if (area >= 0) {
				// transfer the area to the target team
				reply.area = _kern_transfer_area(area, &reply.address,
					B_ANY_ADDRESS, request.team);
				if (reply.area < 0) {
					delete_area(area);
					reply.area = B_ERROR;
					reply.count = 0;
				}
			} else
				reply.area = area;

			status_t status = request.SendReply(
				reply.area < 0 ? reply.area : B_OK, &reply, sizeof(reply));
			if (status != B_OK) {
				// if we couldn't send the message, delete the area
				delete_area(reply.area);
			}
			break;
		}

		case SERVER_GET_REF_FOR:
		{
			const server_get_ref_for_request& request
				= *static_cast<const server_get_ref_for_request*>(data);
			server_get_ref_for_reply reply;
			entry_ref* ref;

			status_t status = gMediaFilesManager->GetRefFor(request.type,
				request.item, &ref);
			if (status == B_OK)
				reply.ref = *ref;

			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_SET_REF_FOR:
		{
			const server_set_ref_for_request& request
				= *static_cast<const server_set_ref_for_request*>(data);
			server_set_ref_for_reply reply;
			entry_ref ref = request.ref;

			status_t status = gMediaFilesManager->SetRefFor(request.type,
				request.item, ref);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_INVALIDATE_MEDIA_ITEM:
		{
			const server_invalidate_item_request& request
				= *static_cast<const server_invalidate_item_request*>(data);
			server_invalidate_item_reply reply;

			status_t status = gMediaFilesManager->InvalidateItem(request.type,
				request.item);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_REMOVE_MEDIA_ITEM:
		{
			const server_remove_media_item_request& request
				= *static_cast<const server_remove_media_item_request*>(data);
			server_remove_media_item_reply reply;

			status_t status = gMediaFilesManager->RemoveItem(request.type,
				request.item);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_ITEM_AUDIO_GAIN:
		{
			const server_get_item_audio_gain_request& request
				= *static_cast<const server_get_item_audio_gain_request*>(data);
			server_get_item_audio_gain_reply reply;

			status_t status = gMediaFilesManager->GetAudioGainFor(request.type,
				request.item, &reply.gain);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_SET_ITEM_AUDIO_GAIN:
		{
			const server_set_item_audio_gain_request& request
				= *static_cast<const server_set_item_audio_gain_request*>(data);
			server_set_ref_for_reply reply;

			status_t status = gMediaFilesManager->SetAudioGainFor(request.type,
				request.item, request.gain);
			request.SendReply(status, &reply, sizeof(reply));
			break;
		}

		default:
			printf("media_server: received unknown message code %#08" B_PRIx32
				"\n", code);
	}
	TRACE("ServerApp::HandleMessage %#" B_PRIx32 " leave\n", code);
}


status_t
ServerApp::_ControlThread(void* _server)
{
	ServerApp* server = (ServerApp*)_server;

	char data[B_MEDIA_MESSAGE_SIZE];
	ssize_t size;
	int32 code;
	while ((size = read_port_etc(server->_ControlPort(), &code, data,
			sizeof(data), 0, 0)) > 0) {
		server->_HandleMessage(code, data, size);
	}

	return B_OK;
}


void
ServerApp::MessageReceived(BMessage* msg)
{
	TRACE("ServerApp::MessageReceived %" B_PRIu32 " enter\n", msg->what);
	switch (msg->what) {
		case MEDIA_SERVER_REQUEST_NOTIFICATIONS:
		case MEDIA_SERVER_CANCEL_NOTIFICATIONS:
		case MEDIA_SERVER_SEND_NOTIFICATIONS:
			gNotificationManager->EnqueueMessage(msg);
			break;

		case MEDIA_FILES_MANAGER_SAVE_TIMER:
			gMediaFilesManager->TimerMessage();
			break;

		case MEDIA_SERVER_ADD_SYSTEM_BEEP_EVENT:
			gMediaFilesManager->HandleAddSystemBeepEvent(msg);
			break;

		case MEDIA_SERVER_RESCAN_COMPLETED:
			gAppManager->NotifyRosters();
			break;

		case B_SOME_APP_QUIT:
		{
			BString mimeSig;
			if (msg->FindString("be:signature", &mimeSig) != B_OK)
				return;

			if (mimeSig == B_MEDIA_ADDON_SERVER_SIGNATURE)
				gNodeManager->CleanupDormantFlavorInfos();

			team_id id;
			if (msg->FindInt32("be:team", &id) == B_OK
					&& gAppManager->HasTeam(id)) {
				gAppManager->UnregisterTeam(id);
			}
			break;
		}

		default:
			BApplication::MessageReceived(msg);
			TRACE("\nmedia_server: unknown message received!\n");
			break;
	}
	TRACE("ServerApp::MessageReceived %" B_PRIu32 " leave\n", msg->what);
}


//	#pragma mark -


int
main()
{
	status_t status;
	ServerApp app(status);

	if (status == B_OK)
		app.Run();

	return status == B_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
