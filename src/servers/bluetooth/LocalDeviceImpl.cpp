/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include "BluetoothServer.h"

#include "LocalDeviceImpl.h"
#include "CommandManager.h"
#include "Output.h"

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/bluetooth_error.h>
#include <bluetooth/LinkKeyUtils.h>
#include <bluetooth/RemoteDevice.h>
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

	if (hd != NULL)
		return new LocalDeviceImpl(hd);
	else
		return NULL;
}


LocalDeviceImpl*
LocalDeviceImpl::CreateTransportAccessor(BPath* path)
{
	HCIDelegate* hd = new HCITransportAccessor(path);

	if (hd != NULL)
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
	for (int16 index = 0; index < event->elen + 2; index++ ) {
		printf("%x:", ((uint8*)event)[index]);
	}
	printf("### \n");

	Output::Instance()->Postf(BLACKBOARD_LD(GetID()),"Incomming %s event\n", 
		GetEvent(event->ecode));

	// Events here might have not been initated by us
	// TODO: ML mark as handled pass a reply by parameter and reply in common
	switch (event->ecode) {
		case HCI_EVENT_HARDWARE_ERROR:
			//HardwareError(event);
			return;
		break;
		case HCI_EVENT_CONN_REQUEST:
			ConnectionRequest((struct hci_ev_conn_request*)(event+1), NULL);
			return;
		break;
		case HCI_EVENT_CONN_COMPLETE:
			// should belong to a request?  can be sporadic or initiated by us¿?...
			ConnectionComplete((struct hci_ev_conn_complete*)(event+1), NULL);
			return;
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

	// Check if it is a requested one
	if (event->ecode == HCI_EVENT_CMD_COMPLETE) {
		request = FindPetition(event->ecode, ((struct hci_ev_cmd_complete*)
				(event+1))->opcode, &eventIndexLocation);

	} else if (event->ecode == HCI_EVENT_CMD_STATUS) {

		request = FindPetition(event->ecode, ((struct hci_ev_cmd_status*)
				(event+1))->opcode, &eventIndexLocation);

	} else 	{
		request = FindPetition(event->ecode);
	}

	if (request == NULL) {
		Output::Instance()->Postf(BLACKBOARD_LD(GetID()),
			"Event %x could not be understood or delivered\n", event->ecode);
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
			// should belong to a request?  can be sporadic or initiated by us¿?...
			DisconnectionComplete((struct hci_ev_disconnection_complete_reply*)
				(event+1), request);
		break;

		case HCI_EVENT_AUTH_COMPLETE:
		break;

		case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
			RemoteNameRequestComplete((struct hci_ev_remote_name_request_complete_reply*)
				(event+1), request);
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
			CommandComplete((struct hci_ev_cmd_complete*)(event+1),
				request, eventIndexLocation);
		break;

		case HCI_EVENT_CMD_STATUS:
			CommandStatus((struct hci_ev_cmd_status*)(event+1), request,
				eventIndexLocation);
		break;

		case HCI_EVENT_FLUSH_OCCUR:
		break;

		case HCI_EVENT_ROLE_CHANGE:
			RoleChange((struct hci_ev_role_change*)(event+1), request,
				eventIndexLocation);
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
			LinkKeyNotify((struct hci_ev_link_key_notify*)(event+1),
				request, eventIndexLocation);
		break;

		case HCI_EVENT_LOOPBACK_COMMAND:
		break;

		case HCI_EVENT_DATA_BUFFER_OVERFLOW:
		break;

		case HCI_EVENT_MAX_SLOT_CHANGE:
			MaxSlotChange((struct hci_ev_max_slot_change*)(event+1), request,
				eventIndexLocation);

		break;

		case HCI_EVENT_READ_CLOCK_OFFSET_COMPL:
		break;

		case HCI_EVENT_CON_PKT_TYPE_CHANGED:
		break;

		case HCI_EVENT_QOS_VIOLATION:
		break;

		case HCI_EVENT_PAGE_SCAN_REP_MODE_CHANGE:
			PageScanRepetitionModeChange((struct hci_ev_page_scan_rep_mode_change*)
				(event+1), request, eventIndexLocation);
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
LocalDeviceImpl::CommandComplete(struct hci_ev_cmd_complete* event, BMessage* request,
				 int32 index) {

	int16   opcodeExpected;
	BMessage reply;

	// Handle command complete information
	request->FindInt16("opcodeExpected", index, &opcodeExpected);

	if (request->IsSourceWaiting() == false)
		Output::Instance()->Post("Nobody waiting for the event\n", BLACKBOARD_KIT);

	Output::Instance()->Postf(BLACKBOARD_LD(GetID()),"%s(%d) for %s\n",__FUNCTION__,
		event->ncmd,GetCommand(opcodeExpected));

	switch ((uint16)opcodeExpected) {

		case PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_LOCAL_VERSION):
		{
			struct hci_rp_read_loc_version* version = (struct hci_rp_read_loc_version*)(event+1);


			if (version->status == BT_OK) {

				if (!IsPropertyAvailable("hci_version"))
					fProperties->AddInt8("hci_version", version->hci_version);

				if (!IsPropertyAvailable("hci_revision"))
					fProperties->AddInt16("hci_revision", version->hci_revision);

				if (!IsPropertyAvailable("lmp_version"))
					fProperties->AddInt8("lmp_version", version->lmp_version);

				if (!IsPropertyAvailable("lmp_subversion"))
					fProperties->AddInt16("lmp_subversion", version->lmp_subversion);

				if (!IsPropertyAvailable("manufacturer"))
					fProperties->AddInt16("manufacturer", version->manufacturer);

			}

			Output::Instance()->Postf(BLACKBOARD_KIT, "Reply for Local Version %x\n", version->status);

			reply.AddInt8("status", version->status);
			printf("Sending reply ... %ld\n", request->SendReply(&reply));
			reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
		}
		break;

		case PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_BUFFER_SIZE):
		{
			struct hci_rp_read_buffer_size* buffer = (struct hci_rp_read_buffer_size*)(event+1);

			if (buffer->status == BT_OK) {

				if (!IsPropertyAvailable("acl_mtu"))
					fProperties->AddInt16("acl_mtu", buffer->acl_mtu);

				if (!IsPropertyAvailable("sco_mtu"))
					fProperties->AddInt8("sco_mtu", buffer->sco_mtu);

				if (!IsPropertyAvailable("acl_max_pkt"))
					fProperties->AddInt16("acl_max_pkt", buffer->acl_max_pkt);

				if (!IsPropertyAvailable("sco_max_pkt"))
					fProperties->AddInt16("sco_max_pkt", buffer->sco_max_pkt);

				Output::Instance()->Postf(BLACKBOARD_KIT,
					"Reply for Read Buffer Size %x\n", buffer->status);
			}

			reply.AddInt8("status", buffer->status);
			printf("Sending reply ... %ld\n", request->SendReply(&reply));
			reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
		}
		break;

		case PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR):
		{
			struct hci_rp_read_bd_addr* readbdaddr = (struct hci_rp_read_bd_addr*)(event+1);

			if (readbdaddr->status == BT_OK) {
				reply.AddData("bdaddr", B_ANY_TYPE, &readbdaddr->bdaddr, sizeof(bdaddr_t));
				Output::Instance()->Post("Positive reply for getAddress\n", BLACKBOARD_KIT);
			} else {
				Output::Instance()->Post("Negative reply for getAddress\n", BLACKBOARD_KIT);
			}

			reply.AddInt8("status", readbdaddr->status);
			printf("Sending reply ... %ld\n", request->SendReply(&reply));
			reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
		}
		break;


		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_CLASS_OF_DEV):
		{
			struct hci_read_dev_class_reply* classDev = (struct hci_read_dev_class_reply*)(event+1);

			if (classDev->status == BT_OK) {
				reply.AddData("devclass", B_ANY_TYPE, &classDev->dev_class, sizeof(classDev->dev_class));
				Output::Instance()->Post("Positive reply for getDeviceClass\n", BLACKBOARD_KIT);

			} else {
				Output::Instance()->Post("Negative reply for getDeviceClass\n", BLACKBOARD_KIT);
			}

			reply.AddInt8("status", classDev->status);
			printf("Sending reply ... %ld\n", request->SendReply(&reply));
			reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
		}
		break;

		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME):
		{
			struct hci_rp_read_local_name* readLocalName = (struct hci_rp_read_local_name*)(event+1);

			if (readLocalName->status == BT_OK) {
				reply.AddString("friendlyname", (const char*)readLocalName->local_name );
				Output::Instance()->Post("Positive reply for friendly name\n", BLACKBOARD_KIT);

			} else {
				Output::Instance()->Post("Negative reply for friendly name\n", BLACKBOARD_KIT);
			}

			reply.AddInt8("status", readLocalName->status);
			printf("Sending reply ... %ld\n", request->SendReply(&reply));
			reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
		}
		break;

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_PIN_CODE_REPLY):
		{
			uint8* statusReply = (uint8*)(event+1);

			//TODO: This reply has to match the BDADDR of the outgoing message
			if (*statusReply == BT_OK) {
				Output::Instance()->Post("Positive reply for pincode accept\n", BLACKBOARD_KIT);
			} else {
				Output::Instance()->Post("Negative reply for pincode accept\n", BLACKBOARD_KIT);
			}

			reply.AddInt8("status", *statusReply);
			printf("Sending reply ... %ld\n", request->SendReply(&reply));
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

			if (*statusReply == BT_OK) {
				Output::Instance()->Post("Positive reply for pincode reject\n", BLACKBOARD_KIT);
			} else {
				Output::Instance()->Post("Negative reply for pincode reject\n", BLACKBOARD_KIT);
			}

			reply.AddInt8("status", *statusReply);
			printf("Sending reply ... %ld\n",request->SendReply(&reply));
			reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
		}
		break;

		// place here all CC that just replies a uint8 status
		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_RESET):
		case PACK_OPCODE(OGF_VENDOR_CMD, OCF_WRITE_BCM2035_BDADDR):
		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_SCAN_ENABLE):
		{
			reply.AddInt8("status", *(uint8*)(event+1));

			Output::Instance()->Postf(BLACKBOARD_LD(GetID()),"%s for %s status %x\n",
						__FUNCTION__,GetCommand(opcodeExpected), *(uint8*)(event+1));

			request->SendReply(&reply);

			ClearWantedEvent(request);
		}
		break;

		default:
			Output::Instance()->Post("Command Complete not handled\n", BLACKBOARD_KIT);
		break;

	}
}


void
LocalDeviceImpl::CommandStatus(struct hci_ev_cmd_status* event, BMessage* request,
				int32 index) {

	int16   opcodeExpected;
	BMessage reply;

	// Handle command complete information
	request->FindInt16("opcodeExpected", index, &opcodeExpected);

	Output::Instance()->Postf(BLACKBOARD_LD(GetID()),"%s(%d) for %s\n",__FUNCTION__,
		event->ncmd,GetCommand(opcodeExpected));

	if (request->IsSourceWaiting() == false)
		Output::Instance()->Post("Nobody waiting for the event\n", BLACKBOARD_KIT);

	switch (opcodeExpected) {

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_INQUIRY):
		{
			reply.what = BT_MSG_INQUIRY_STARTED;

			if (event->status == BT_OK) {
				Output::Instance()->Post("Positive reply for inquiry status\n", BLACKBOARD_KIT);
			} else {
				Output::Instance()->Post("Negative reply for inquiry status\n", BLACKBOARD_KIT);
			}

			reply.AddInt8("status", event->status);
			printf("Sending reply ... %ld\n", request->SendReply(&reply));
			reply.PrintToStream();

			ClearWantedEvent(request, HCI_EVENT_CMD_STATUS,
				PACK_OPCODE(OGF_LINK_CONTROL, OCF_INQUIRY));
		}
		break;

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST):
		{
			if (event->status==BT_OK) {
				ClearWantedEvent(request, HCI_EVENT_CMD_STATUS,
					PACK_OPCODE(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST));
			}
			else {
				Output::Instance()->Post("Wrong Command Status for remote friendly name\n", BLACKBOARD_KIT);

				reply.AddInt8("status", event->status);
				printf("Sending reply ... %ld\n", request->SendReply(&reply));
				reply.PrintToStream();

				ClearWantedEvent(request);
			}
		}
		break;
		/*
		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_ACCEPT_CONN_REQ):
		{
			ClearWantedEvent(request, HCI_EVENT_CMD_STATUS,
				PACK_OPCODE(OGF_LINK_CONTROL, OCF_ACCEPT_CONN_REQ));
		}
		break;

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_REJECT_CONN_REQ):
		{
			ClearWantedEvent(request, HCI_EVENT_CMD_STATUS,
				PACK_OPCODE(OGF_LINK_CONTROL, OCF_REJECT_CONN_REQ));
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

	// skipping here the number of responses
	reply.AddData("info", B_ANY_TYPE, numberOfResponses + 1
		, (*numberOfResponses) * sizeof(struct inquiry_info) );

	reply.AddInt8("count", *numberOfResponses);

	printf("%s: Sending reply ... %ld\n",__FUNCTION__, request->SendReply(&reply));

}


void
LocalDeviceImpl::InquiryComplete(uint8* status, BMessage* request)
{
	BMessage reply(BT_MSG_INQUIRY_COMPLETED);

	reply.AddInt8("status", *status);
	printf("%s: Sending reply ... %ld\n",__FUNCTION__, request->SendReply(&reply));

	ClearWantedEvent(request);
}


void
LocalDeviceImpl::RemoteNameRequestComplete(
	struct hci_ev_remote_name_request_complete_reply* remotename, BMessage* request)
{
	BMessage reply;

	if (remotename->status == BT_OK) {

		reply.AddString("friendlyname", (const char*)remotename->remote_name );
		Output::Instance()->Post("Positive reply for remote friendly name\n", BLACKBOARD_KIT);
	} else {
		Output::Instance()->Post("Negative reply for remote friendly name\n", BLACKBOARD_KIT);
	}

	reply.AddInt8("status", remotename->status);
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

	Output::Instance()->Postf(BLACKBOARD_LD(GetID()),
		"Connection Incoming type %x from %s...\n",
		event->link_type, bdaddrUtils::ToString(event->bdaddr));

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
		//newrequest->AddInt16("eventExpected",  HCI_EVENT_MAX_SLOT_CHANGE);
		//newrequest->AddInt16("eventExpected",  HCI_EVENT_DISCONNECTION_COMPLETE);

		AddWantedEvent(newrequest);

		if ((fHCIDelegate)->IssueCommand(command, size) == B_ERROR) {
			Output::Instance()->Postf(BLACKBOARD_LD(GetID()),
				"Command issue error for ConnAccept\n");
				// remove the request ¿?
		} else {
			Output::Instance()->Postf(BLACKBOARD_LD(GetID()),
				"Command issue for ConnAccept\n");
		}
	}
}


void
LocalDeviceImpl::ConnectionComplete(struct hci_ev_conn_complete* event, BMessage* request)
{

	if (event->status == BT_OK) {
		uint8	cod[3] = {0,0,0};

		ConnectionIncoming* iConnection = new ConnectionIncoming(
			new RemoteDevice(event->bdaddr, cod));
		iConnection->Show();


		Output::Instance()->Postf(BLACKBOARD_LD(GetID()),
			"%s: Address %s handle=%#x type=%d encrypt=%d\n", __FUNCTION__,
				bdaddrUtils::ToString(event->bdaddr), event->handle,
				event->link_type, event->encrypt_mode);

	} else {
		Output::Instance()->Postf(BLACKBOARD_LD(GetID()),
			"%s: failed with status %#x\n", __FUNCTION__, event->status);
	}



}


void
LocalDeviceImpl::DisconnectionComplete(struct hci_ev_disconnection_complete_reply* event, BMessage* request)
{
	Output::Instance()->Post("Disconnected\n", BLACKBOARD_KIT);

	Output::Instance()->Postf(BLACKBOARD_LD(GetID()),"%s: Handle=%#x, reason=%x status=%x\n",
		__FUNCTION__, event->handle, event->reason, event->status);

	ClearWantedEvent(request);

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
	Output::Instance()->Postf(BLACKBOARD_LD(GetID()),"%s: Address %s role=%d status=%d\n",
		 __FUNCTION__, bdaddrUtils::ToString(event->bdaddr), event->role, event->status);
}


void
LocalDeviceImpl::PageScanRepetitionModeChange(struct hci_ev_page_scan_rep_mode_change* event,
	BMessage* request, int32 index)
{
	Output::Instance()->Postf(BLACKBOARD_LD(GetID()),"%s: Address %s type=%d\n",
		__FUNCTION__, bdaddrUtils::ToString(event->bdaddr), event->page_scan_rep_mode);
}


void
LocalDeviceImpl::LinkKeyNotify(hci_ev_link_key_notify *event,
	BMessage* request, int32 index)
{
	Output::Instance()->Postf(BLACKBOARD_LD(GetID()),"%s: Address %s, key=%s, type=%d\n",
		__FUNCTION__, bdaddrUtils::ToString(event->bdaddr),
		LinkKeyUtils::ToString(event->link_key), event->key_type);
}


void
LocalDeviceImpl::MaxSlotChange(struct hci_ev_max_slot_change *event,
	BMessage *request, int32 index)
{
	Output::Instance()->Postf(BLACKBOARD_LD(GetID()),"%s: Handle=%#x, max slots=%d\n",
		 __FUNCTION__, event->handle, event->lmp_max_slots);
}


#if 0
#pragma mark - Request Methods -
#endif


status_t
LocalDeviceImpl::ProcessSimpleRequest(BMessage* request)
{
	ssize_t size;
	void* command = NULL;

	if (request->FindData("raw command", B_ANY_TYPE, 0,
		(const void **)&command, &size) == B_OK) {

		AddWantedEvent(request);
		// LEAK: is command buffer freed within the Message?
		if (((HCITransportAccessor*)fHCIDelegate)->IssueCommand(command, size)
			== B_ERROR) {
			// TODO: - Reply the request with error!
			//       - Remove the just added request
			(Output::Instance()->Post("Command issue error\n", BLACKBOARD_KIT));
		} else {
			return B_OK;
		}
	} else {
		(Output::Instance()->Post("No command specified for simple request\n",
			BLACKBOARD_KIT));
	}

	return B_ERROR;
}
