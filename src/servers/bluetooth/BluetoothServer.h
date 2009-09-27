/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BLUETOOTH_SERVER_APP_H
#define _BLUETOOTH_SERVER_APP_H

#include <stdlib.h>

#include <Application.h>
#include <ObjectList.h>
#include <OS.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI.h>
#include <bluetooth/HCI/btHCI_transport.h>
#include <bluetooth/HCI/btHCI_command.h>

#include "HCIDelegate.h"
#include "DeviceManager.h"
#include "LocalDeviceImpl.h"

#include <PortListener.h>

#define BT "bluetooth_server: "

typedef enum {
	BLACKBOARD_GENERAL = 0,
	BLACKBOARD_DEVICEMANAGER,
	BLACKBOARD_KIT,
	BLACKBOARD_SDP,
	// more blackboards
	BLACKBOARD_END
} BluetoothServerBlackBoardIndex;

#define BLACKBOARD_LD(X) (BLACKBOARD_END+X-HCI_DEVICE_INDEX_OFFSET)

typedef BObjectList<LocalDeviceImpl> LocalDevicesList;
typedef PortListener<struct hci_event_header, 
	HCI_MAX_EVENT_SIZE, // Event Body can hold max 255 + 2 header
	24					// Some devices have sent chunks of 24 events(inquiry result)
	> BluetoothPortListener;

class BluetoothServer : public BApplication
{
public:

	BluetoothServer();

	virtual bool QuitRequested(void);
	virtual void ArgvReceived(int32 argc, char **argv);
	virtual void ReadyToRun(void);


	virtual void AppActivated(bool act);
	virtual void MessageReceived(BMessage *message);

	static int32 SDPServerThread(void* data);
	
	/* Messages reply */
	status_t	HandleLocalDevicesCount(BMessage* message, BMessage* reply);
	status_t    HandleAcquireLocalDevice(BMessage* message, BMessage* reply);
	
	status_t    HandleGetProperty(BMessage* message, BMessage* reply);
	status_t    HandleSimpleRequest(BMessage* message, BMessage* reply);


    LocalDeviceImpl*    LocateLocalDeviceImpl(hci_id hid);
	
private:

	LocalDeviceImpl*	LocateDelegateFromMessage(BMessage* message);

	void 				ShowWindow(BWindow* pWindow);

	void				_InstallDeskbarIcon();
	void				_RemoveDeskbarIcon();

	LocalDevicesList   	fLocalDevicesList;


	// Notification system
	BluetoothPortListener*	fEventListener2;
	
	DeviceManager*			fDeviceManager;
	
	BPoint 					fCenter;
	
	thread_id				fSDPThreadID;
	
	bool					fIsShuttingDown;
};

#endif
