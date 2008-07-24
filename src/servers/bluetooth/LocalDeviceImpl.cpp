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

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/bluetooth_error.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <bluetoothserver_p.h>
#include <ConnectionIncoming.h>
#include <PincodeWindow.h>

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

printf("### Incoming event: len = %d\n", event->elen);
for (int16 index = 0 ; index < event->elen + 2; index++ ) {
	printf("%x:",((uint8*)event)[index]);
}
printf("### \n");

	// Events here might have not been initated by us
	// TODO: ML mark as handled pass a reply by parameter and reply in common
    switch (event->ecode) {
	        case HCI_EVENT_HARDWARE_ERROR:
   				//HardwareError(event);    	
   			return;
			case HCI_EVENT_CONN_REQUEST:
				ConnectionRequest((struct hci_ev_conn_request*)(event+1), NULL); // incoming request
				return;
			break;

			case HCI_EVENT_CONN_COMPLETE:
				ConnectionComplete((struct hci_ev_conn_complete*)(event+1), NULL); // should belong to a request? 
				return;  														   // can be sporadic or initiated by us¿?...
			break;

			case HCI_EVENT_PIN_CODE_REQ:
				PinCodeRequest((struct hci_ev_pin_code_req*)(event+1), NULL);
				return;
			break;


   		default:
   			// lets go on
   		break;        
	}

	

	BMessage*	request = NULL;
	int32		eventIndexLocation;
	
	// Check if its a requested one
	if ( event->ecode == HCI_EVENT_CMD_COMPLETE ) {
	
		(Output::Instance()->Post("Incoming Command Complete\n", BLACKBOARD_EVENTS));
		request = FindPetition(event->ecode, ((struct hci_ev_cmd_complete*)(event+1))->opcode, &eventIndexLocation );
	
	} else if ( event->ecode == HCI_EVENT_CMD_STATUS ) {

		(Output::Instance()->Post("Incoming Command Status\n", BLACKBOARD_EVENTS));
		request = FindPetition(event->ecode, ((struct hci_ev_cmd_status*)(event+1))->opcode, &eventIndexLocation );

	} else 
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
    		InquiryComplete((uint8*)(event+1), request);
    	break;
    	
    	case HCI_EVENT_INQUIRY_RESULT:
    		InquiryResult((uint8*)(event+1), request);
		break;

		case HCI_EVENT_DISCONNECTION_COMPLETE:
			DisconnectionComplete((struct hci_ev_disconnection_complete_reply*)(event+1), request);
		break;
 	
		case HCI_EVENT_AUTH_COMPLETE:
		break;
 			
		case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
			RemoteNameRequestComplete((struct hci_ev_remote_name_request_complete_reply*)(event+1), request);	
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
			CommandComplete((struct hci_ev_cmd_complete*)(event+1), request, eventIndexLocation);
		break;

		case HCI_EVENT_CMD_STATUS:
 			CommandStatus((struct hci_ev_cmd_status*)(event+1), request, eventIndexLocation);
		break;

		case HCI_EVENT_FLUSH_OCCUR:
		break;
 	
		case HCI_EVENT_ROLE_CHANGE:
			RoleChange((struct hci_ev_role_change*)(event+1), request, eventIndexLocation);
		break;
 	
		case HCI_EVENT_NUM_COMP_PKTS:
		break;
 	
		case HCI_EVENT_MODE_CHANGE:
		break;
 	
		case HCI_EVENT_RETURN_LINK_KEYS:
		break;

		case HCI_EVENT_LINK_KEY_REQ:
		break;
 	
		case HCI_EVENT_LINK_KEY_NOTIFY:
			LinkKeyNotify((struct hci_ev_link_key_notify*)(event+1), request, eventIndexLocation);
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
			PageScanRepetitionModeChange((struct hci_ev_page_scan_rep_mode_change*)(event+1), request, eventIndexLocation);
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
LocalDeviceImpl::CommandComplete(struct hci_ev_cmd_complete* event, BMessage* request, int32 index) {

	int16   opcodeExpected;
	BMessage reply;
	
    Output::Instance()->Post(__FUNCTION__, BLACKBOARD_LD_OFFSET + GetID());
    Output::Instance()->Post("\n", BLACKBOARD_LD_OFFSET + GetID());
	
	// Handle command complete information
    request->FindInt16("opcodeExpected", index, &opcodeExpected);


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

			    Output::Instance()->Post("Positive reply for getAddress\n", BLACKBOARD_KIT);

            } else {
                reply.AddInt8("status", readbdaddr->status); 
                request->SendReply(&reply);
			    Output::Instance()->Post("Negative reply for getAddress\n", BLACKBOARD_KIT);
            }

 			// This request is not gonna be used anymore
            ClearWantedEvent(request);
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

 			// This request is not gonna be used anymore
            ClearWantedEvent(request);
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

 			// This request is not gonna be used anymore
            ClearWantedEvent(request);
     	}
        break;
        case PACK_OPCODE(OGF_LINK_CONTROL, OCF_PIN_CODE_REPLY):
        {
        	uint8* statusReply = (uint8*)(event+1);
			//TODO: This reply has to match the BDADDR of the outgoing message
        	reply.AddInt8("status", *statusReply);

            if (*statusReply == BT_OK) {

                Output::Instance()->Post("Positive reply for pincode accept\n", BLACKBOARD_KIT);

            } else {

                Output::Instance()->Post("Negative reply for pincode accept\n", BLACKBOARD_KIT);

            }

            printf("Sending reply ... %ld\n",request->SendReply(&reply));
            reply.PrintToStream();

 			// This request is not gonna be used anymore
            ClearWantedEvent(request);
     	}
        break;
		

        case PACK_OPCODE(OGF_LINK_CONTROL, OCF_PIN_CODE_NEG_REPLY):
        {
        	uint8* statusReply = (uint8*)(event+1);
			//TODO: This reply might to match the BDADDR of the outgoing message
			//  	xxx:FindPetition should be expanded....
        	reply.AddInt8("status", *statusReply);

            if (*statusReply == BT_OK) {

                Output::Instance()->Post("Positive reply for pincode reject\n", BLACKBOARD_KIT);

            } else {

                Output::Instance()->Post("Negative reply for pincode reject\n", BLACKBOARD_KIT);

            }

            printf("Sending reply ... %ld\n",request->SendReply(&reply));
            reply.PrintToStream();

 			// This request is not gonna be used anymore
            ClearWantedEvent(request);
     	}
        break;


		default:
		    Output::Instance()->Post("Command Complete not handled\n", BLACKBOARD_KIT);
		break;

    }
}


void 
LocalDeviceImpl::CommandStatus(struct hci_ev_cmd_status* event, BMessage* request, int32 index) {

	int16   opcodeExpected;
	BMessage reply;
	
    Output::Instance()->Post(__FUNCTION__, BLACKBOARD_LD_OFFSET + GetID());
    Output::Instance()->Post("\n", BLACKBOARD_LD_OFFSET + GetID());
	
	// Handle command complete information
    request->FindInt16("opcodeExpected", index, &opcodeExpected);


	if (request->IsSourceWaiting() == false)
		Output::Instance()->Post("Nobody waiting for the event\n", BLACKBOARD_KIT);

    switch (opcodeExpected) {

        case PACK_OPCODE(OGF_LINK_CONTROL, OCF_INQUIRY):
        {
        	reply.what = BT_MSG_INQUIRY_STARTED;
        	reply.AddInt8("status", event->status);

            if (event->status == BT_OK) {
			    Output::Instance()->Post("Positive reply for inquiry status\n", BLACKBOARD_KIT);
            } else {
			    Output::Instance()->Post("Negative reply for inquiry status\n", BLACKBOARD_KIT);
            }

            printf("Sending reply ... %ld\n", request->SendReply(&reply));
            reply.PrintToStream();

            ClearWantedEvent(request, HCI_EVENT_CMD_STATUS, PACK_OPCODE(OGF_LINK_CONTROL, OCF_INQUIRY));
     	}
        break;
		
		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST):
		{
			ClearWantedEvent(request, HCI_EVENT_CMD_STATUS, PACK_OPCODE(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST));
		}
		break;
		/*
		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_ACCEPT_CONN_REQ):
		{	
			ClearWantedEvent(request, HCI_EVENT_CMD_STATUS, PACK_OPCODE(OGF_LINK_CONTROL, OCF_ACCEPT_CONN_REQ));
		}
		break;

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_REJECT_CONN_REQ):
		{
			ClearWantedEvent(request, HCI_EVENT_CMD_STATUS, PACK_OPCODE(OGF_LINK_CONTROL, OCF_REJECT_CONN_REQ));
		}
		break;*/


		default:
		    Output::Instance()->Post("Command Status not handled\n", BLACKBOARD_KIT);
		break;
	}

}


void 
LocalDeviceImpl::InquiryResult(uint8* numberOfResponses, BMessage* request)
{

	BMessage reply(BT_MSG_INQUIRY_DEVICE);

	
    reply.AddData("info", B_ANY_TYPE, numberOfResponses+1 // skipping here the number of responses
                        , (*numberOfResponses) * sizeof(struct inquiry_info) );
	
    printf("%s: Sending reply ... %ld\n",__FUNCTION__, request->SendReply(&reply));

}


void 
LocalDeviceImpl::InquiryComplete(uint8* status, BMessage* request)
{
	BMessage reply(BT_MSG_INQUIRY_COMPLETED);
	
	reply.AddInt8("status", *status);
	

    printf("%s: Sending reply ... %ld\n",__FUNCTION__, request->SendReply(&reply));
//    (request->ReturnAddress()).SendMessage(&reply);

    ClearWantedEvent(request);
}


void
LocalDeviceImpl::RemoteNameRequestComplete(struct hci_ev_remote_name_request_complete_reply* remotename, BMessage* request)
{
	BMessage reply;
	
	reply.AddInt8("status", remotename->status);
	
    if (remotename->status == BT_OK) {

        reply.AddString("friendlyname", (const char*)remotename->remote_name );
	    Output::Instance()->Post("Positive reply for remote friendly name\n", BLACKBOARD_KIT);

    } else {

	    Output::Instance()->Post("Negative reply for remote friendly name\n", BLACKBOARD_KIT);

    }

    printf("Sending reply ... %ld\n", request->SendReply(&reply));
    reply.PrintToStream();

	// This request is not gonna be used anymore
    ClearWantedEvent(request);

}


void
LocalDeviceImpl::ConnectionRequest(struct hci_ev_conn_request* event, BMessage* request)
{
	size_t size;
	void* command;

	printf("Connection type %d from %s\n", event->link_type, bdaddrUtils::ToString(event->bdaddr));
	(Output::Instance()->Post("Connection Incoming ...\n", BLACKBOARD_EVENTS));


    /* TODO: add a possible request in the queue */
	if (true) { // Check Preferences if we are to accept this connection
	    command = buildAcceptConnectionRequest(event->bdaddr, 0x01 /*slave*/, &size);


		BMessage* newrequest = new BMessage;
		newrequest->AddInt16("eventExpected",  HCI_EVENT_CMD_STATUS);
		newrequest->AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL, OCF_ACCEPT_CONN_REQ));

		newrequest->AddInt16("eventExpected",  HCI_EVENT_PIN_CODE_REQ);
		newrequest->AddInt16("eventExpected",  HCI_EVENT_ROLE_CHANGE);
		newrequest->AddInt16("eventExpected",  HCI_EVENT_LINK_KEY_NOTIFY);
		newrequest->AddInt16("eventExpected",  HCI_EVENT_PAGE_SCAN_REP_MODE_CHANGE);
		
		AddWantedEvent(newrequest);

	    if ((fHCIDelegate)->IssueCommand(command, size) == B_ERROR) {
			(Output::Instance()->Post("Command issue error for ConnAccpq\n", BLACKBOARD_EVENTS));
			// remove the request
			
		}
		else {
			(Output::Instance()->Post("Command issued for ConnAccp\n", BLACKBOARD_EVENTS));


		}
	}

}


void
LocalDeviceImpl::ConnectionComplete(struct hci_ev_conn_complete* event, BMessage* request)
{

	if (event->status == BT_OK) {

		ConnectionIncoming* iConnection = new ConnectionIncoming(new RemoteDevice(event->bdaddr));
		iConnection->Show();
		printf("%s: Address %s handle=%#x type=%d encrypt=%d\n", __FUNCTION__, bdaddrUtils::ToString(event->bdaddr), event->handle,
                                                                 event->link_type, event->encrypt_mode);

	} else {
		printf("%s: failed with status %#x\n", __FUNCTION__, event->status);
	}
	


}


void
LocalDeviceImpl::DisconnectionComplete(struct hci_ev_disconnection_complete_reply* event, BMessage * request)
{
	Output::Instance()->Post("Disconnected\n", BLACKBOARD_KIT);
}


void
LocalDeviceImpl::PinCodeRequest(struct hci_ev_pin_code_req* event, BMessage* request)
{
	
	PincodeWindow* iPincode = new PincodeWindow(event->bdaddr, GetID());
	iPincode->Show();

}


void
LocalDeviceImpl::RoleChange(hci_ev_role_change *event, BMessage* request, int32 index)
{
	printf("%s: Address %s role=%d status=%d\n", __FUNCTION__, bdaddrUtils::ToString(event->bdaddr), event->role, event->status);
}

void
LocalDeviceImpl::PageScanRepetitionModeChange(struct hci_ev_page_scan_rep_mode_change* event, BMessage* request, int32 index)
{
	printf("%s: Address %s type=%d\n", __FUNCTION__, bdaddrUtils::ToString(event->bdaddr), event->page_scan_rep_mode);
}


void
LocalDeviceImpl::LinkKeyNotify(hci_ev_link_key_notify *event, BMessage* request, int32 index)
{

	printf("%s: Address %s [----] type=%d\n", __FUNCTION__, bdaddrUtils::ToString(event->bdaddr), event->key_type);

}


#if 0
#pragma mark - Request Methods -
#endif


status_t 
LocalDeviceImpl::GetAddress(bdaddr_t* bdaddr, BMessage* request)
{	
	ssize_t ssize;
	
	if (fProperties->FindData("bdaddr", B_ANY_TYPE, 0, (const void **)bdaddr, &ssize) == B_OK) {
	    

	    /* We have this info, returning back */
	    return B_OK;    
	    
	} else {
		size_t size;
				
	    void* command = buildReadBdAddr(&size);
	    
	    /* Adding a wanted event in the queue */
	    request->AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	    request->AddInt16("opcodeExpected", PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR));
	    
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

	if (request->FindData("raw command", B_ANY_TYPE, 0, (const void **)&command, &size) == B_OK) {
		
		AddWantedEvent(request);
		    
	    if (((HCITransportAccessor*)fHCIDelegate)->IssueCommand(command, size) == B_ERROR) {
			// TODO: - Reply the request with error!
			//       - Remove the just added request
			(Output::Instance()->Post("Command issue error\n", BLACKBOARD_EVENTS));
		}
		else {

			return B_OK;
		}
	}
	else {
		(Output::Instance()->Post("No command specified for simple request\n", BLACKBOARD_KIT));
	}	

	return B_ERROR;
}
