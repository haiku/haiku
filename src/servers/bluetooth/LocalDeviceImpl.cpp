/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include "BluetoothServer.h"

#include "LocalDeviceImpl.h"
#include "CommandManager.h"
#include "Output.h"

#include <bluetooth/bluetooth_error.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <stdio.h>


#if 0
#pragma mark - Class methods -
#endif


// Factory methods
LocalDeviceImpl* 
LocalDeviceImpl::CreateControllerAccessor(BPath* path)
{
    HCIDelegate* hd = new HCIControllerAccessor(path);
    
    if ( hd != NULL)
        return new LocalDeviceImpl(hd);
    else
        return NULL;
}


LocalDeviceImpl* 
LocalDeviceImpl::CreateTransportAccessor(BPath* path)
{
    HCIDelegate* hd = new HCITransportAccessor(path);
    
    if ( hd != NULL)
        return new LocalDeviceImpl(hd);    
    else
        return NULL;        
}


LocalDeviceImpl::LocalDeviceImpl(HCIDelegate* hd) : LocalDeviceHandler(hd)
{

}

#if 0
#pragma mark - Event handling methods -
#endif

void 
LocalDeviceImpl::HandleEvent(struct hci_event_header* event)
{

printf("### Event comming: len = %d\n", event->elen);
for (int16 index = 0 ; index < event->elen + 2; index++ ) {
	printf("%x:",((uint8*)event)[index]);
}
printf("### \n");

	// Events here might have not been initated by us
    switch (event->ecode) {
	        case HCI_EVENT_HARDWARE_ERROR:
   				//HardwareError(event);    	
   			return;
			case HCI_EVENT_CONN_REQUEST:
	
			break;

   			default:
   				// lets go on
   			break;        
	}

	

	BMessage* request = NULL;
	
	// Check if its a requested one
	if ( event->ecode == HCI_EVENT_CMD_COMPLETE ) {
	
		(Output::Instance()->Post("Incoming Command Complete\n", BLACKBOARD_EVENTS));
		request = FindPetition(event->ecode, ((struct hci_ev_cmd_complete*)(event+1))->opcode );
	
	} else 
	// TODO: Command status should also be considered
	{	
		request = FindPetition(event->ecode);
	}
	
	if ( request == NULL) {
		(Output::Instance()->Post("Event could not be understood or delivered\n", BLACKBOARD_EVENTS));
		return;
	}

	// we are waiting for a reply
	switch (event->ecode) {
		case HCI_EVENT_INQUIRY_COMPLETE:
    	break;
    	
    	case HCI_EVENT_INQUIRY_RESULT:
		break;
 	
		case HCI_EVENT_CONN_COMPLETE:
		break;
 	 	
		case HCI_EVENT_DISCONNECTION_COMPLETE:
		break;
 	
		case HCI_EVENT_AUTH_COMPLETE:
		break;
 			
		case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
		break;
 	
		case HCI_EVENT_ENCRYPT_CHANGE:
		break;
 	
		case HCI_EVENT_CHANGE_CONN_LINK_KEY_COMPLETE:
		break;
 	
		case HCI_EVENT_MASTER_LINK_KEY_COMPL:
		break;
 	
		case HCI_EVENT_RMT_FEATURES:
		break;
 	
		case HCI_EVENT_RMT_VERSION:
		break;
 	
		case HCI_EVENT_QOS_SETUP_COMPLETE:
		break;

		case HCI_EVENT_CMD_COMPLETE:    				
 				CommandComplete((struct hci_ev_cmd_complete*)(event+1), request);
 		break;
 	
 		case HCI_EVENT_CMD_STATUS:
		break;

		case HCI_EVENT_FLUSH_OCCUR:
		break;
 	
		case HCI_EVENT_ROLE_CHANGE:
		break;
 	
		case HCI_EVENT_NUM_COMP_PKTS:
		break;
 	
		case HCI_EVENT_MODE_CHANGE:
		break;
 	
		case HCI_EVENT_RETURN_LINK_KEYS:
		break;
 	
		case HCI_EVENT_PIN_CODE_REQ:
		break;
 	
		case HCI_EVENT_LINK_KEY_REQ:
		break;
 	
		case HCI_EVENT_LINK_KEY_NOTIFY:
		break;
 		
		case HCI_EVENT_LOOPBACK_COMMAND:
		break;
 	
		case HCI_EVENT_DATA_BUFFER_OVERFLOW:
		break;
		
		case HCI_EVENT_MAX_SLOT_CHANGE:
		break;
 	
		case HCI_EVENT_READ_CLOCK_OFFSET_COMPL:
		break;
 	
		case HCI_EVENT_CON_PKT_TYPE_CHANGED:
		break;
 		
		case HCI_EVENT_QOS_VIOLATION:
		break;
 	
		case HCI_EVENT_PAGE_SCAN_REP_MODE_CHANGE:
		break;
 	
		case HCI_EVENT_FLOW_SPECIFICATION:
		break;
 				
		case HCI_EVENT_INQUIRY_RESULT_WITH_RSSI:
		break;
 	
		case HCI_EVENT_REMOTE_EXTENDED_FEATURES:
		break;
 	
		case HCI_EVENT_SYNCHRONOUS_CONNECTION_COMPLETED:
		break;
 	
		case HCI_EVENT_SYNCHRONOUS_CONNECTION_CHANGED:
 	
		break;
	}

}


void 
LocalDeviceImpl::CommandComplete(struct hci_ev_cmd_complete* event, BMessage* request) {
    
	int16   opcodeExpected;
	BMessage reply;
	
    Output::Instance()->Post(__FUNCTION__, BLACKBOARD_LD_OFFSET + GetID());
    Output::Instance()->Post("\n", BLACKBOARD_LD_OFFSET + GetID());
	
	// Handle command complete information
    request->FindInt16("opcodeExpected", 0 /*REVIEW!*/, &opcodeExpected);


	if (request->IsSourceWaiting() == false)
		Output::Instance()->Post("Nobody waiting for the event\n", BLACKBOARD_KIT);                	
    
    
    switch (opcodeExpected) {
        
        case PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR):
        {    
        	struct hci_rp_read_bd_addr* readbdaddr = (struct hci_rp_read_bd_addr*)(event+1);
        	       	
            if (readbdaddr->status == BT_OK) {
                
                                
                reply.AddData("bdaddr", B_ANY_TYPE, &readbdaddr->bdaddr, sizeof(bdaddr_t));     
                reply.AddInt32("status", readbdaddr->status);

                printf("Sending reply ... %ld\n",request->SendReply(&reply));                                
                reply.PrintToStream();

                
			    Output::Instance()->Post("Positive reply for getAdress\n", BLACKBOARD_KIT);

            } else {
                reply.AddInt8("status", readbdaddr->status); 
                request->SendReply(&reply);    
			    Output::Instance()->Post("Negative reply for getAdress\n", BLACKBOARD_KIT);                
            }
                        
            ClearWantedEvent(request, PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR));
     	}       
        break;

        case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME):
        {    
        	struct hci_rp_read_local_name* readLocalName = (struct hci_rp_read_local_name*)(event+1);
        	
        	reply.AddInt8("status", readLocalName->status);
        	
            if (readLocalName->status == BT_OK) {
                                
                reply.AddString("friendlyname", (const char*)readLocalName->local_name );                    
			    Output::Instance()->Post("Positive reply for friendly name\n", BLACKBOARD_KIT);

            } else {

			    Output::Instance()->Post("Negative reply for friendly name\n", BLACKBOARD_KIT);                

            }

            printf("Sending reply ... %ld\n",request->SendReply(&reply));                                
            reply.PrintToStream();
                        
            ClearWantedEvent(request, PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME));
     	}       
        break;

        case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_SCAN_ENABLE):
        {    
        	uint8* statusReply = (uint8*)(event+1);
        	
        	reply.AddInt8("status", *statusReply);
        	
            if (*statusReply == BT_OK) {
                                
                Output::Instance()->Post("Positive reply for scanmode\n", BLACKBOARD_KIT);

            } else {

                Output::Instance()->Post("Negative reply for scanmode\n", BLACKBOARD_KIT);

            }

            printf("Sending reply ... %ld\n",request->SendReply(&reply));                                
            reply.PrintToStream();
                        
            ClearWantedEvent(request, PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_SCAN_ENABLE));
     	}       
        break;
		
		default:
		    Output::Instance()->Post("Command Complete not handled\n", BLACKBOARD_KIT);                		
		break;
		
		    
    }        
}


#if 0
#pragma mark - Request Methods -
#endif


status_t 
LocalDeviceImpl::GetAddress(bdaddr_t* bdaddr, BMessage* request)
{	
	ssize_t size;
	
	if (fProperties->FindData("bdaddr", B_ANY_TYPE, 0, (const void **)bdaddr, &size) == B_OK) {
	    
		(Output::Instance()->Post("BDADDR already present in server\n", BLACKBOARD_EVENTS));
	    /* We have this info, returning back */
	    return B_OK;    
	    
	} else {
		size_t size;
				
	    void* command = buildReadBdAddr(&size);
	    
	    /* Adding a wanted event in the queue */
	    request->AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	    request->AddInt16("opcodeExpected", PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR));
	    
	    printf("Adding request... %p\n", request);
	    AddWantedEvent(request);
		request->PrintToStream();
		
	    if (((HCITransportAccessor*)fHCIDelegate)->IssueCommand(command, size) == B_ERROR)
			(Output::Instance()->Post("Command issue error\n", BLACKBOARD_EVENTS));
	
		(Output::Instance()->Post("Command issued for GetAddress\n", BLACKBOARD_EVENTS));
	    return B_WOULD_BLOCK;
	}
	
}


status_t
LocalDeviceImpl::GetFriendlyName(BString str, BMessage* request)
{
	
	if (fProperties->FindString("friendlyname", &str) == B_OK) {
	    
		(Output::Instance()->Post("Friendly name already present in server\n", BLACKBOARD_EVENTS));
	    /* We have this info, returning back */
	    return B_OK;    
	    
	} else {
		size_t size;
				
	    void* command = buildReadLocalName(&size);
	    
	    /* Adding a wanted event in the queue */
	    request->AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	    request->AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME));
	    
	    printf("Adding request... %p\n", request);
	    AddWantedEvent(request);
		request->PrintToStream();
		
	    if (((HCITransportAccessor*)fHCIDelegate)->IssueCommand(command, size) == B_ERROR)
			(Output::Instance()->Post("Command issue error\n", BLACKBOARD_EVENTS));
	
		(Output::Instance()->Post("Command issued for GetFriendlyname\n", BLACKBOARD_EVENTS));
	    
	    return B_WOULD_BLOCK;
	}

}


status_t 
LocalDeviceImpl::ProcessSimpleRequest(BMessage* request)
{	
	ssize_t size;
	void* command = NULL;

	if (request->FindData("raw command", B_ANY_TYPE, 0, (const void **)&command, &size) == B_OK) 
	    if (((HCITransportAccessor*)fHCIDelegate)->IssueCommand(command, size) == B_ERROR)
			(Output::Instance()->Post("Command issue error\n", BLACKBOARD_EVENTS));
		else
		{
		    AddWantedEvent(request);
			return B_OK;
		}
	else {
		(Output::Instance()->Post("No command specified for simple request\n", BLACKBOARD_KIT));
	}	

	return B_ERROR;
}
