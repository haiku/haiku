/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>


#include <Entry.h>
#include <Path.h>
#include <Message.h>
#include <Directory.h>
#include <String.h>
#include <Roster.h>


#include <TypeConstants.h>
#include <syslog.h>

#include <bluetoothserver_p.h>
#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/bluetooth_util.h>

#include "LocalDeviceImpl.h"
#include "BluetoothServer.h"
#include "Output.h"


BluetoothServer::BluetoothServer() : BApplication(BLUETOOTH_SIGNATURE)
{ 
	Output::Instance()->Run();
	Output::Instance()->SetTitle("Bluetooth message gathering");

	Output::Instance()->AddTab("General", BLACKBOARD_GENERAL);
	Output::Instance()->AddTab("Device Manager", BLACKBOARD_DEVICEMANAGER);
	Output::Instance()->AddTab("Events", BLACKBOARD_EVENTS);
	Output::Instance()->AddTab("Kit", BLACKBOARD_KIT);

	
	ShowWindow(Output::Instance());

    fDeviceManager = new DeviceManager();
	fLocalDevicesList.MakeEmpty();

	fEventListener = spawn_thread(notification_Thread, "BT port listener" , B_DISPLAY_PRIORITY , this);

   
}

bool BluetoothServer::QuitRequested(void)
{
	// Finish quitting
	Output::Instance()->Lock();
	Output::Instance()->Quit();


/*	HCIDelegate *hd = NULL;
	while ((hd = (HCIDelegate *)fDelegatesList.RemoveItem((int32)0)) !=NULL)
		delete hd;
*/

	printf("Accepting quitting of the application\n");
	return BApplication::QuitRequested();
}

void BluetoothServer::ArgvReceived(int32 argc, char **argv)
{
	if (argc>1) {
		if (strcmp(argv[1], "--finish") == 0)
			PostMessage(B_QUIT_REQUESTED);

	    else {

	
	    }
	}

}


void BluetoothServer::ReadyToRun(void) {
    
    fDeviceManager->StartMonitoringDevice("bluetooth/h2generic");
	
		
	// Launch the notifier thread
	if ( resume_thread(fEventListener) != B_OK ) 
	{
		Output::Instance()->Post("Bluetooth port listener failed\n", BLACKBOARD_GENERAL);
	}
    
    Output::Instance()->Post("Bluetooth server Ready\n", BLACKBOARD_GENERAL);
}

void BluetoothServer::AppActivated(bool act) {

	printf("Activated %d\n",act);

}



void BluetoothServer::MessageReceived(BMessage *message)
{
	BMessage reply;
	status_t status = B_WOULD_BLOCK; // mark somehow.. do not reply anything
	
	switch(message->what)
	{
	
		case BT_MSG_ADD_DEVICE:
		{
			BString str;
	        message->FindString("name", &str);        
			BPath path(str.String());

	  	    (Output::Instance()->Post( str.String(), BLACKBOARD_GENERAL));
     	    (Output::Instance()->Post(" requested LocalDevice\n", BLACKBOARD_GENERAL));
	        
	        LocalDeviceImpl* ldi = LocalDeviceImpl::CreateTransportAccessor(&path);

	        if (ldi->GetID() >= 0) {
                      fLocalDevicesList.AddItem(ldi);
					  Output::Instance()->AddTab("Local Device", BLACKBOARD_LD_OFFSET + ldi->GetID());
				  	  (Output::Instance()->Post( str.String(), BLACKBOARD_LD_OFFSET + ldi->GetID()));
					  (Output::Instance()->Post(" LocalDevice added\n", BLACKBOARD_LD_OFFSET + ldi->GetID()));
			 

            } else {
					  (Output::Instance()->Post("Adding LocalDevice failed\n", BLACKBOARD_GENERAL));            
            }
            
            status = B_WOULD_BLOCK;
        }   
			
		case BT_MSG_COUNT_LOCAL_DEVICES: 
			status = HandleLocalDevicesCount(message, &reply);
    	break;	
    	
		case BT_MSG_ACQUIRE_LOCAL_DEVICE:
			status = HandleAcquireLocalDevice(message, &reply);
		break;

        case BT_MSG_GET_FRIENDLY_NAME:
            status = HandleGetFriendlyName(message, &reply);
        break;
        
        case BT_MSG_GET_ADDRESS:
            status = HandleGetAddress(message, &reply);
        break;

        case BT_MSG_HANDLE_SIMPLE_REQUEST:
            status = HandleSimpleRequest(message, &reply);
        break;


		/* Handle if the bluetooth preferences is running?? */
		case B_SOME_APP_LAUNCHED:
   		{
			const char *signature;
			// TODO: what's this for?
			if (message->FindString("be:signature", &signature)==B_OK) {
				printf("input_server : %s\n", signature);
				if (strcmp(signature, "application/x-vnd.Be-TSKB")==0) {

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
	    printf("Sending reply message\n"); 
	}

}

#if 0
#pragma mark -
#endif

LocalDeviceImpl* 
BluetoothServer::LocateDelegateFromMessage(BMessage* message)
{
    LocalDeviceImpl* ldi = NULL;
 	hci_id hid;
    
    if (message->FindInt32("hci_id", &hid) == B_OK)	    
	{
	    /* Try to find out when a ID was specified */
	    int index;
	    
	    for (index = 0; index < fLocalDevicesList.CountItems() ; index ++) {

		    ldi = fLocalDevicesList.ItemAt(index);
		    if (ldi->GetID() == hid)  {            
		        break;
		    }		    
        }
    }
        	
    return ldi;
}

LocalDeviceImpl*
BluetoothServer::LocateLocalDeviceImpl(hci_id hid) 
{

    /* Try to find out when a ID was specified */
    int index;
    
    for (index = 0; index < fLocalDevicesList.CountItems() ; index ++) {

	    LocalDeviceImpl* ldi = fLocalDevicesList.ItemAt(index);
	    if (ldi->GetID() == hid)  {
	        return ldi;
	    }		    
    }
    
    return NULL;
}


#if 0
#pragma - Messages reply
#endif

status_t
BluetoothServer::HandleLocalDevicesCount(BMessage* message, BMessage* reply)
{
	return reply->AddInt32("count", fLocalDevicesList.CountItems());
}


status_t
BluetoothServer::HandleAcquireLocalDevice(BMessage* message, BMessage* reply)
{
	hci_id hid;
	ssize_t size;
	bdaddr_t bdaddr;
	LocalDeviceImpl* ldi = NULL;
    int32 index = 0;
	
	if (message->FindInt32("hci_id", &hid) == B_OK)	    
	{
	    Output::Instance()->Post("GetLocalDevice requested with id\n", BLACKBOARD_KIT);
        ldi = LocateDelegateFromMessage(message);						
		
	} else if (message->FindData("bdaddr", B_ANY_TYPE, (const void**)&bdaddr, &size ) == B_OK)
	{
	    /* Try to find out when the user specified the address */
	    Output::Instance()->Post("GetLocalDevice requested with bdaddr\n", BLACKBOARD_KIT);	    
	    for (index = 0; index < fLocalDevicesList.CountItems() ; index ++) {	        
	        
	        bdaddr_t local;
		    ldi = fLocalDevicesList.ItemAt(index);
		    if ((ldi->GetAddress(&local, message) == B_OK) && bacmp(&local, &bdaddr))  {
		        break;
		    }		    		    
        }		    
	    
	} else
	{
	    /* Careless, any device not performing operations will be fine */	    	    
	    Output::Instance()->Post("GetLocalDevice requested\n", BLACKBOARD_KIT);	    
	    for (index = 0; index < fLocalDevicesList.CountItems() ; index ++) {
	        
		    ldi = fLocalDevicesList.ItemAt(index);
		    printf("Requesting local device %ld\n", ldi->GetID());
		    if (ldi != NULL && ldi->Available())
		    {
  			    printf("dev ours %ld\n", ldi->GetID());
		        break;
		    }
		    
        }	
	}

    if (index <= fLocalDevicesList.CountItems() && ldi != NULL && ldi->Available())
	{
	    Output::Instance()->Post("Device acquired\n", BLACKBOARD_KIT);
	    ldi->Acquire();
	    return reply->AddInt32("hci_id", hid);
	}

	return B_ERROR;	
	
}


status_t
BluetoothServer::HandleGetFriendlyName(BMessage* message, BMessage* reply)
{
    LocalDeviceImpl* ldi = LocateDelegateFromMessage(message);
    BString name;
        
    if (ldi == NULL)
    	return B_ERROR;
    	
    /* If the device was ocupied... Autlock?->LocalDeviceImpl will decide */
    if (ldi->GetFriendlyName(name, DetachCurrentMessage()) == B_OK) {
        
        return reply->AddString("friendlyname", name);
	}
	
	return B_WOULD_BLOCK; 
}


status_t
BluetoothServer::HandleGetAddress(BMessage* message, BMessage* reply)
{
    LocalDeviceImpl* ldi = LocateDelegateFromMessage(message);
    bdaddr_t bdaddr;

    if (ldi == NULL)
    	return B_ERROR;
        
    /* If the device was ocupied... Autlock?->LocalDeviceImpl will decide */
	status_t status = ldi->GetAddress(&bdaddr, DetachCurrentMessage());
    if ( status == B_OK) {
        
        return reply->AddData("bdaddr", B_ANY_TYPE, &bdaddr, sizeof(bdaddr_t));
	}
	
	return status; 
}


status_t
BluetoothServer::HandleSimpleRequest(BMessage* message, BMessage* reply)
{

    LocalDeviceImpl* ldi = LocateDelegateFromMessage(message);
	BString propertyRequested;
		
	// Find out if there is a property being requested, 
    if (message->FindString("property", &propertyRequested) == B_OK) {
    	// Check if the property has been already retrieved
    	
    	if (ldi->IsPropertyAvailable(propertyRequested)) {
    	
    	    // Dump everything
    		reply->AddMessage("properties",ldi->GetPropertiesMessage());
    		return B_OK;
    	}
    	               	
    }
	
	// we are gonna need issue the command ...
	if (ldi->ProcessSimpleRequest(DetachCurrentMessage()) == B_OK)
		return B_WOULD_BLOCK;
	else
		return B_ERROR;

}


#if 0
#pragma mark -
#endif


int32 
BluetoothServer::notification_Thread(void* data)
{
	BPortNot notifierd( (BluetoothServer*) data , BT_USERLAND_PORT_NAME);
	
	notifierd.loop();
	return B_NO_ERROR;
}

int32 
BluetoothServer::sdp_server_Thread(void* data)
{

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


#if 0
#pragma mark -
#endif

int
main(int /*argc*/, char** /*argv*/)
{
	setbuf(stdout,NULL);

	BluetoothServer* bluetoothServer = new BluetoothServer;

	bluetoothServer->Run();
	delete bluetoothServer;

	return 0;
}

