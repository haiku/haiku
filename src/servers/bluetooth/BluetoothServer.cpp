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
    	
		case BT_MSG_ADQUIRE_LOCAL_DEVICE:
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


		/* Handle if the bluetooth preferences is running */
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
	
	/* Can we reply right now? */
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

#if 0

void BluetoothServer::DevicesWatching(void) {

	BDirectory* hdoses;
	BEntry entrada;
	BPath path;

	//status_t err;
	int fd1 = -1;

	// only cheks the actual driver which we have
	hdoses = new BDirectory("/dev/bus/bluetooth/h2/");
//	hdoses = new BDirectory("/dev/bus/bluetooth/h3/...");
//	hdoses = new BDirectory("/dev/bus/bluetooth/h4/...");
	Output::Instance()->Post("Exploring present devices ...\n",1);
	
	while (hdoses->GetNextEntry(&entrada,true) == B_OK) {
	Output::Instance()->Post((char*)path.Path(), 1);		
		entrada.GetPath(&path);
		if (entrada.IsDirectory()) {
			Output::Instance()->Post((char*)path.Path(),1);
			BDirectory* driver_directory = new BDirectory(path.Path());					
			BEntry driver_entry;

			syslog(LOG_ALERT, "Bluetooth driver %s\n",path.Path());
			fprintf(stderr, "Bluetooth driver %s\n",path.Path());
						
			while (driver_directory->GetNextEntry(&driver_entry,true) == B_OK) {
				Output::Instance()->Post((char*)path.Path(),1);
				driver_entry.GetPath(&path);
				fd1 = open(path.Path(), O_RDWR);
				

			
				// TODO: Watching all folders under and set some kind of internal structure that hold all
				//		 LocalDevices. 
				// devloop = new DeviceLooper();
				// devloop->StartMonitoringDevice("bus/bluetooth/h2");

				if (fd1 < 0) {
					syslog(LOG_ALERT,BT "Error opening device %s\n",path.Path());
					fprintf(stderr, "Error opening device %s\n",path.Path());
				} 
				else 
				{
					struct  {
						size_t	size;
						struct hci_command_header header;
						//struct hci_rp_read_bd_addr body;
					} __attribute__ ((packed)) cm1;

					struct  {
						size_t	size;
						struct hci_command_header header;
						struct hci_cp_inquiry body;
					} __attribute__ ((packed)) cm2;

					struct  {
						size_t	size;
						struct hci_command_header header;
						struct hci_remote_name_request body;
					} __attribute__ ((packed)) cm3;

					struct  {
						size_t	size;
						struct hci_command_header header;
						struct hci_cp_create_conn body;
					} __attribute__ ((packed)) cm4;
					
					 
					syslog(LOG_ALERT,BT "Registering device %s\n",path.Path());
					fprintf(stderr, "Opening %s\n",path.Path());

					cm1.size = sizeof(struct hci_command_header);
					cm1.header.opcode = B_HOST_TO_LENDIAN_INT16(hci_opcode_pack(OGF_CONTROL_BASEBAND, OCF_RESET));
					cm1.header.clen = 0;					
					ioctl(fd1, ISSUE_BT_COMMAND, &cm1, sizeof(cm1));	

					/*cm1.size = sizeof(struct hci_command_header);
					cm1.header.opcode = B_HOST_TO_LENDIAN_INT16(hci_opcode_pack(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME));
					cm1.header.clen = 0;					
					ioctl(fd1, ISSUE_BT_COMMAND, &cm1, sizeof(cm1));	

					cm1.size = sizeof(struct hci_command_header);
					cm1.header.opcode = B_HOST_TO_LENDIAN_INT16(hci_opcode_pack(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR));
					cm1.header.clen = 0;					
					ioctl(fd1, ISSUE_BT_COMMAND, &cm1, sizeof(cm1));	

					cm2.size = sizeof(struct hci_command_header)+sizeof(struct hci_cp_inquiry);
					cm2.header.opcode = B_HOST_TO_LENDIAN_INT16(hci_opcode_pack(OGF_LINK_CTL, OCF_INQUIRY));
					cm2.body.lap[0] = ((B_BT_GIAC & 0x000000FF));
					cm2.body.lap[1] = ((B_BT_GIAC & 0x0000FF00) >> 8);
					cm2.body.lap[2] = ((B_BT_GIAC & 0x00FF0000) >> 16);	
					cm2.body.length = 0x15;
					cm2.body.num_rsp = 8;
					cm2.header.clen = 5;					
					ioctl(fd1, ISSUE_BT_COMMAND, &cm2, sizeof(cm1));	
					snooze(60*1000*1000);*/
					cm3.size = sizeof(struct hci_command_header)+sizeof(struct hci_remote_name_request);
					cm3.header.opcode = B_HOST_TO_LENDIAN_INT16(hci_opcode_pack(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST));
					cm3.body.bdaddr.b[0] = 0x92;
					cm3.body.bdaddr.b[1] = 0xd3;
					cm3.body.bdaddr.b[2] = 0xaf;
					cm3.body.bdaddr.b[3] = 0xd9;
					cm3.body.bdaddr.b[4] = 0x0a;
					cm3.body.bdaddr.b[5] = 0x00;
					cm3.body.pscan_rep_mode = 1;
					cm3.body.clock_offset = 0xc7;
					cm3.header.clen = 10;					
					ioctl(fd1, ISSUE_BT_COMMAND, &cm3, sizeof(cm3));
					/*
					cm4.size = sizeof(struct hci_command_header)+sizeof(struct hci_cp_create_conn);
					cm4.header.opcode = B_HOST_TO_LENDIAN_INT16(hci_opcode_pack(OGF_LINK_CTL, OCF_CREATE_CONN));
					cm4.body.bdaddr.b[0] = 0x92;
					cm4.body.bdaddr.b[1] = 0xd3;
					cm4.body.bdaddr.b[2] = 0xaf;
					cm4.body.bdaddr.b[3] = 0xd9;
					cm4.body.bdaddr.b[4] = 0x0a;
					cm4.body.bdaddr.b[5] = 0x00;
					cm4.body.pkt_type = 0xFFFF;
					cm4.body.pscan_rep_mode = 1;
					cm4.body.pscan_mode = 0;
					cm4.body.clock_offset = 0xc7;
					cm4.body.role_switch = 1;
					cm4.header.clen = 13;					
					ioctl(fd1, ISSUE_BT_COMMAND, &cm4, sizeof(cm4));
					*/
				}		
			}
		}
	}

	syslog(LOG_ALERT,BT "All devices registered\n");	
	fprintf(stderr, "Waiting with opened devices\n");

}
#endif

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

