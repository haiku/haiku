/*
 * Copyright 2007-2009 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

#include <Entry.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Message.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <Window.h>

#include <TypeConstants.h>
#include <syslog.h>

#include <bluetoothserver_p.h>
#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/L2CAP/btL2CAP.h>
#include <bluetooth/bluetooth.h>

#include "BluetoothServer.h"
#include "DeskbarReplicant.h"
#include "LocalDeviceImpl.h"
#include "Debug.h"


status_t
DispatchEvent(struct hci_event_header* header, int32 code, size_t size)
{
	// we only handle events
	if (GET_PORTCODE_TYPE(code)!= BT_EVENT) {
		TRACE_BT("BluetoothServer: Wrong type frame code\n");
		return B_OK;
	}

	// fetch the LocalDevice who belongs this event
	LocalDeviceImpl* lDeviceImplementation = ((BluetoothServer*)be_app)->
		LocateLocalDeviceImpl(GET_PORTCODE_HID(code));

	if (lDeviceImplementation == NULL) {
		TRACE_BT("BluetoothServer: LocalDevice could not be fetched\n");
		return B_OK;
	}

	lDeviceImplementation->HandleEvent(header);

	return B_OK;
}


BluetoothServer::BluetoothServer()
	:
	BApplication(BLUETOOTH_SIGNATURE),
	fSDPThreadID(-1),
	fIsShuttingDown(false)
{
	fDeviceManager = new DeviceManager();
	fLocalDevicesList.MakeEmpty();

	fEventListener2 = new BluetoothPortListener(BT_USERLAND_PORT_NAME,
		(BluetoothPortListener::port_listener_func)&DispatchEvent);
}


bool BluetoothServer::QuitRequested(void)
{
	LocalDeviceImpl* lDeviceImpl = NULL;
	while ((lDeviceImpl = (LocalDeviceImpl*)
		fLocalDevicesList.RemoveItemAt(0)) != NULL)
		delete lDeviceImpl;

	_RemoveDeskbarIcon();

	// stop the SDP server thread
	fIsShuttingDown = true;

	status_t threadReturnStatus;
	wait_for_thread(fSDPThreadID, &threadReturnStatus);
	TRACE_BT("BluetoothServer server thread exited with: %s\n",
		strerror(threadReturnStatus));

	delete fEventListener2;
	TRACE_BT("Shutting down bluetooth_server.\n");

	return BApplication::QuitRequested();
}


void BluetoothServer::ArgvReceived(int32 argc, char **argv)
{
	if (argc > 1) {
		if (strcmp(argv[1], "--finish") == 0)
			PostMessage(B_QUIT_REQUESTED);
	}
}


void BluetoothServer::ReadyToRun(void)
{
	fDeviceManager->StartMonitoringDevice("bluetooth/h2");
	fDeviceManager->StartMonitoringDevice("bluetooth/h3");
	fDeviceManager->StartMonitoringDevice("bluetooth/h4");
	fDeviceManager->StartMonitoringDevice("bluetooth/h5");

	if (fEventListener2->Launch() != B_OK)
		TRACE_BT("General: Bluetooth event listener failed\n");
	else
		TRACE_BT("General: Bluetooth event listener Ready\n");

	_InstallDeskbarIcon();

	// Spawn the SDP server thread
	fSDPThreadID = spawn_thread(SDPServerThread, "SDP server thread",
		B_NORMAL_PRIORITY, this);

#define _USE_FAKE_SDP_SERVER
#ifdef _USE_FAKE_SDP_SERVER
	if (fSDPThreadID <= 0 || resume_thread(fSDPThreadID) != B_OK) {
		TRACE_BT("BluetoothServer: Failed launching the SDP server thread\n");
	}
#endif
}


void BluetoothServer::AppActivated(bool act)
{
	printf("Activated %d\n",act);
}


void BluetoothServer::MessageReceived(BMessage* message)
{
	BMessage reply;
	status_t status = B_WOULD_BLOCK; // mark somehow to do not reply anything

	switch (message->what)
	{
		case BT_MSG_ADD_DEVICE:
		{
			BString str;
			message->FindString("name", &str);

			TRACE_BT("BluetoothServer: Requested LocalDevice %s\n", str.String());

			BPath path(str.String());

			LocalDeviceImpl* lDeviceImpl
				= LocalDeviceImpl::CreateTransportAccessor(&path);

			if (lDeviceImpl->GetID() >= 0) {
				fLocalDevicesList.AddItem(lDeviceImpl);

				TRACE_BT("LocalDevice %s id=%" B_PRId32 " added\n", str.String(),
					lDeviceImpl->GetID());
			} else {
				TRACE_BT("BluetoothServer: Adding LocalDevice hci id invalid\n");
			}

			status = B_WOULD_BLOCK;
			/* TODO: This should be by user request only! */
			lDeviceImpl->Launch();
			break;
		}

		case BT_MSG_REMOVE_DEVICE:
		{
			LocalDeviceImpl* lDeviceImpl = LocateDelegateFromMessage(message);
			if (lDeviceImpl != NULL) {
				fLocalDevicesList.RemoveItem(lDeviceImpl);
				delete lDeviceImpl;
			}
			break;
		}

		case BT_MSG_COUNT_LOCAL_DEVICES:
			status = HandleLocalDevicesCount(message, &reply);
			break;

		case BT_MSG_ACQUIRE_LOCAL_DEVICE:
			status = HandleAcquireLocalDevice(message, &reply);
			break;

		case BT_MSG_HANDLE_SIMPLE_REQUEST:
			status = HandleSimpleRequest(message, &reply);
			break;

		case BT_MSG_GET_PROPERTY:
			status = HandleGetProperty(message, &reply);
			break;

		// Handle if the bluetooth preferences is running?
		case B_SOME_APP_LAUNCHED:
		{
			const char* signature;

			if (message->FindString("be:signature", &signature) == B_OK) {
				printf("input_server : %s\n", signature);
				if (strcmp(signature, "application/x-vnd.Be-TSKB") == 0) {

				}
			}
			return;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}

	// Can we reply right now?
	// TOD: review this condition
	if (status != B_WOULD_BLOCK) {
		reply.AddInt32("status", status);
		message->SendReply(&reply);
//		printf("Sending reply message for->\n");
//		message->PrintToStream();
	}
}


#if 0
#pragma mark -
#endif

LocalDeviceImpl*
BluetoothServer::LocateDelegateFromMessage(BMessage* message)
{
	LocalDeviceImpl* lDeviceImpl = NULL;
	hci_id hid;

	if (message->FindInt32("hci_id", &hid) == B_OK) {
		// Try to find out when a ID was specified
		int index;
		for (index = 0; index < fLocalDevicesList.CountItems(); index ++) {
			lDeviceImpl = fLocalDevicesList.ItemAt(index);
			if (lDeviceImpl->GetID() == hid)
				break;
		}
	}

	return lDeviceImpl;

}


LocalDeviceImpl*
BluetoothServer::LocateLocalDeviceImpl(hci_id hid)
{
	// Try to find out when a ID was specified
	int index;

	for (index = 0; index < fLocalDevicesList.CountItems(); index++) {
		LocalDeviceImpl* lDeviceImpl = fLocalDevicesList.ItemAt(index);
		if (lDeviceImpl->GetID() == hid)
			return lDeviceImpl;
	}

	return NULL;
}


#if 0
#pragma - Messages reply
#endif

status_t
BluetoothServer::HandleLocalDevicesCount(BMessage* message, BMessage* reply)
{
	TRACE_BT("BluetoothServer: count requested\n");

	return reply->AddInt32("count", fLocalDevicesList.CountItems());
}


status_t
BluetoothServer::HandleAcquireLocalDevice(BMessage* message, BMessage* reply)
{
	hci_id hid;
	ssize_t size;
	bdaddr_t bdaddr;
	LocalDeviceImpl* lDeviceImpl = NULL;
	static int32 lastIndex = 0;

	if (message->FindInt32("hci_id", &hid) == B_OK)	{
		TRACE_BT("BluetoothServer: GetLocalDevice requested with id\n");
		lDeviceImpl = LocateDelegateFromMessage(message);

	} else if (message->FindData("bdaddr", B_ANY_TYPE,
		(const void**)&bdaddr, &size) == B_OK) {

		// Try to find out when the user specified the address
		TRACE_BT("BluetoothServer: GetLocalDevice requested with bdaddr\n");
		for (lastIndex = 0; lastIndex < fLocalDevicesList.CountItems();
			lastIndex ++) {
			// TODO: Only possible if the property is available
			// bdaddr_t local;
			// lDeviceImpl = fLocalDevicesList.ItemAt(lastIndex);
			// if ((lDeviceImpl->GetAddress(&local, message) == B_OK)
			// 	&& bacmp(&local, &bdaddr)) {
			// 	break;
			// }
		}

	} else {
		// Careless, any device not performing operations will be fine
		TRACE_BT("BluetoothServer: GetLocalDevice plain request\n");
		// from last assigned till end
		for (int index = lastIndex + 1;
			index < fLocalDevicesList.CountItems();	index++) {
			lDeviceImpl= fLocalDevicesList.ItemAt(index);
			if (lDeviceImpl != NULL && lDeviceImpl->Available()) {
				printf("Requested local device %" B_PRId32 "\n",
					lDeviceImpl->GetID());
				TRACE_BT("BluetoothServer: Device available: %" B_PRId32 "\n", lDeviceImpl->GetID());
				lastIndex = index;
				break;
			}
		}

		// from starting till last assigned if not yet found
		if (lDeviceImpl == NULL) {
			for (int index = 0; index <= lastIndex ; index ++) {
				lDeviceImpl = fLocalDevicesList.ItemAt(index);
				if (lDeviceImpl != NULL && lDeviceImpl->Available()) {
					printf("Requested local device %" B_PRId32 "\n",
						lDeviceImpl->GetID());
					TRACE_BT("BluetoothServer: Device available: %" B_PRId32 "\n", lDeviceImpl->GetID());
					lastIndex = index;
					break;
				}
			}
		}
	}

	if (lastIndex <= fLocalDevicesList.CountItems() && lDeviceImpl != NULL
		&& lDeviceImpl->Available()) {

		hid = lDeviceImpl->GetID();
		lDeviceImpl->Acquire();

		TRACE_BT("BluetoothServer: Device acquired %" B_PRId32 "\n", hid);
		return reply->AddInt32("hci_id", hid);
	}

	return B_ERROR;

}


status_t
BluetoothServer::HandleSimpleRequest(BMessage* message, BMessage* reply)
{
	LocalDeviceImpl* lDeviceImpl = LocateDelegateFromMessage(message);
	if (lDeviceImpl == NULL) {
		return B_ERROR;
	}

	const char* propertyRequested;

	// Find out if there is a property being requested,
	if (message->FindString("property", &propertyRequested) == B_OK) {
		// Check if the property has been already retrieved
		if (lDeviceImpl->IsPropertyAvailable(propertyRequested)) {
			// Dump everything
			reply->AddMessage("properties", lDeviceImpl->GetPropertiesMessage());
			return B_OK;
		}
	}

	// we are gonna need issue the command ...
	if (lDeviceImpl->ProcessSimpleRequest(DetachCurrentMessage()) == B_OK)
		return B_WOULD_BLOCK;
	else {
		lDeviceImpl->Unregister();
		return B_ERROR;
	}

}


status_t
BluetoothServer::HandleGetProperty(BMessage* message, BMessage* reply)
{
	// User side will look for the reply in a result field and will
	// not care about status fields, therefore we return OK in all cases

	LocalDeviceImpl* lDeviceImpl = LocateDelegateFromMessage(message);
	if (lDeviceImpl == NULL) {
		return B_ERROR;
	}

	const char* propertyRequested;

	// Find out if there is a property being requested,
	if (message->FindString("property", &propertyRequested) == B_OK) {

		TRACE_BT("BluetoothServer: Searching %s property...\n", propertyRequested);

		// Check if the property has been already retrieved
		if (lDeviceImpl->IsPropertyAvailable(propertyRequested)) {

			// 1 bytes requests
			if (strcmp(propertyRequested, "hci_version") == 0
				|| strcmp(propertyRequested, "lmp_version") == 0
				|| strcmp(propertyRequested, "sco_mtu") == 0) {

				uint8 result = lDeviceImpl->GetPropertiesMessage()->
					FindInt8(propertyRequested);
				reply->AddInt32("result", result);

			// 2 bytes requests
			} else if (strcmp(propertyRequested, "hci_revision") == 0
					|| strcmp(propertyRequested, "lmp_subversion") == 0
					|| strcmp(propertyRequested, "manufacturer") == 0
					|| strcmp(propertyRequested, "acl_mtu") == 0
					|| strcmp(propertyRequested, "acl_max_pkt") == 0
					|| strcmp(propertyRequested, "sco_max_pkt") == 0
					|| strcmp(propertyRequested, "packet_type") == 0 ) {

				uint16 result = lDeviceImpl->GetPropertiesMessage()->
					FindInt16(propertyRequested);
				reply->AddInt32("result", result);

			// 1 bit requests
			} else if (strcmp(propertyRequested, "role_switch_capable") == 0
					|| strcmp(propertyRequested, "encrypt_capable") == 0) {

				bool result = lDeviceImpl->GetPropertiesMessage()->
					FindBool(propertyRequested);

				reply->AddInt32("result", result);



			} else {
				TRACE_BT("BluetoothServer: Property %s could not be satisfied\n", propertyRequested);
			}
		}
	}

	return B_OK;
}


#if 0
#pragma mark -
#endif

int32
BluetoothServer::SDPServerThread(void* data)
{
	const BluetoothServer* server = (BluetoothServer*)data;

	// Set up the SDP socket
	struct sockaddr_l2cap loc_addr = { 0 };
	int socketServer;
	int client;
	status_t status;
	char buffer[512] = "";

	TRACE_BT("SDP: SDP server thread up...\n");

	socketServer = socket(PF_BLUETOOTH, SOCK_STREAM, BLUETOOTH_PROTO_L2CAP);

	if (socketServer < 0) {
		TRACE_BT("SDP: Could not create server socket ...\n");
		return B_ERROR;
	}

	// bind socket to port 0x1001 of the first available
	// bluetooth adapter
	loc_addr.l2cap_family = AF_BLUETOOTH;
	loc_addr.l2cap_bdaddr = BDADDR_ANY;
	loc_addr.l2cap_psm = B_HOST_TO_LENDIAN_INT16(1);
	loc_addr.l2cap_len = sizeof(struct sockaddr_l2cap);

	status = bind(socketServer, (struct sockaddr*)&loc_addr,
		sizeof(struct sockaddr_l2cap));

	if (status < 0) {
		TRACE_BT("SDP: Could not bind server socket (%s)...\n", strerror(status));
		return status;
	}

	// setsockopt(sock, SOL_L2CAP, SO_L2CAP_OMTU, &omtu, len );
	// getsockopt(sock, SOL_L2CAP, SO_L2CAP_IMTU, &omtu, &len );

	// Listen for up to 10 connections
	status = listen(socketServer, 10);

	if (status != B_OK) {
		TRACE_BT("SDP: Could not listen server socket (%s)...\n", strerror(status));
		return status;
	}

	while (!server->fIsShuttingDown) {

		TRACE_BT("SDP: Waiting connection for socket (%s)...\n", strerror(status));

		uint len = sizeof(struct sockaddr_l2cap);
		client = accept(socketServer, (struct sockaddr*)&loc_addr, &len);

		TRACE_BT("SDP: Incomming connection... %d\n", client);

		ssize_t receivedSize;

		do {
			receivedSize = recv(client, buffer, 29 , 0);
			if (receivedSize < 0)
				TRACE_BT("SDP: Error reading client socket\n");
			else {
				TRACE_BT("SDP: Received from SDP client: %ld:\n", receivedSize);
				for (int i = 0; i < receivedSize ; i++)
					TRACE_BT("SDP: %x:", buffer[i]);

				TRACE_BT("\n");
			}
		} while (receivedSize >= 0);

		snooze(5000000);
		TRACE_BT("SDP: Waiting for next connection...\n");
	}

	// Close the socket
	close(socketServer);

	return B_NO_ERROR;
}


void
BluetoothServer::ShowWindow(BWindow* pWindow)
{
	pWindow->Lock();
	if (pWindow->IsHidden())
		pWindow->Show();
	else
		pWindow->Activate();
	pWindow->Unlock();
}


void
BluetoothServer::_InstallDeskbarIcon()
{
	app_info appInfo;
	be_app->GetAppInfo(&appInfo);

	BDeskbar deskbar;

	if (deskbar.HasItem(kDeskbarItemName)) {
		_RemoveDeskbarIcon();
	}

	status_t res = deskbar.AddItem(&appInfo.ref);
	if (res != B_OK)
		TRACE_BT("Failed adding deskbar icon: %" B_PRId32 "\n", res);
}


void
BluetoothServer::_RemoveDeskbarIcon()
{
	BDeskbar deskbar;
	status_t res = deskbar.RemoveItem(kDeskbarItemName);
	if (res != B_OK)
		TRACE_BT("Failed removing Deskbar icon: %" B_PRId32 ": \n", res);
}


#if 0
#pragma mark -
#endif

int
main(int /*argc*/, char** /*argv*/)
{
	BluetoothServer* bluetoothServer = new BluetoothServer;

	bluetoothServer->Run();
	delete bluetoothServer;

	return 0;
}

