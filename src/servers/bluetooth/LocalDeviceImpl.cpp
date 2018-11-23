/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include "BluetoothServer.h"

#include "LocalDeviceImpl.h"
#include "CommandManager.h"
#include "Debug.h"

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/bluetooth_error.h>
#include <bluetooth/LinkKeyUtils.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <bluetoothserver_p.h>
#include <ConnectionIncoming.h>
#include <PincodeWindow.h>

#include <stdio.h>
#include <new>


#if 0
#pragma mark - Class methods -
#endif


// Factory methods
LocalDeviceImpl*
LocalDeviceImpl::CreateControllerAccessor(BPath* path)
{
	HCIDelegate* delegate = new(std::nothrow) HCIControllerAccessor(path);
	if (delegate == NULL)
		return NULL;

	LocalDeviceImpl* device = new(std::nothrow) LocalDeviceImpl(delegate);
	if (device == NULL) {
		delete delegate;
		return NULL;
	}

	return device;
}


LocalDeviceImpl*
LocalDeviceImpl::CreateTransportAccessor(BPath* path)
{
	HCIDelegate* delegate = new(std::nothrow) HCITransportAccessor(path);
	if (delegate == NULL)
		return NULL;

	LocalDeviceImpl* device = new(std::nothrow) LocalDeviceImpl(delegate);
	if (device == NULL) {
		delete delegate;
		return NULL;
	}

	return device;
}


LocalDeviceImpl::LocalDeviceImpl(HCIDelegate* hd) : LocalDeviceHandler(hd)
{

}


LocalDeviceImpl::~LocalDeviceImpl()
{

}


void
LocalDeviceImpl::Unregister()
{
	BMessage* msg =	new	BMessage(BT_MSG_REMOVE_DEVICE);

	msg->AddInt32("hci_id", fHCIDelegate->Id());

	TRACE_BT("LocalDeviceImpl: Unregistering %" B_PRId32 "\n",
		fHCIDelegate->Id());

	be_app_messenger.SendMessage(msg);
}


#if 0
#pragma mark - Event handling methods -
#endif

template<typename T, typename Header>
inline T*
JumpEventHeader(Header* event)
{
	return (T*)(event + 1);
}


void
LocalDeviceImpl::HandleUnexpectedEvent(struct hci_event_header* event)
{
	// Events here might have not been initated by us
	// TODO: ML mark as handled pass a reply by parameter and reply in common
	switch (event->ecode) {
		case HCI_EVENT_HARDWARE_ERROR:
			HardwareError(
				JumpEventHeader<struct hci_ev_hardware_error>(event));
			break;

		case HCI_EVENT_CONN_REQUEST:
			ConnectionRequest(
				JumpEventHeader<struct hci_ev_conn_request>(event), NULL);
			break;

		case HCI_EVENT_NUM_COMP_PKTS:
			NumberOfCompletedPackets(
				JumpEventHeader<struct hci_ev_num_comp_pkts>(event));
			break;

		case HCI_EVENT_DISCONNECTION_COMPLETE:
			// should belong to a request?  can be sporadic or initiated by us?
			DisconnectionComplete(
				JumpEventHeader
					<struct hci_ev_disconnection_complete_reply>(event),
				NULL);
			break;
		case HCI_EVENT_PIN_CODE_REQ:
			PinCodeRequest(
				JumpEventHeader<struct hci_ev_pin_code_req>(event), NULL);
			break;
		default:
			// TODO: feedback unexpected not handled
			break;
	}
}


void
LocalDeviceImpl::HandleExpectedRequest(struct hci_event_header* event,
	BMessage* request)
{
	// we are waiting for a reply
	switch (event->ecode) {
		case HCI_EVENT_INQUIRY_COMPLETE:
			InquiryComplete(JumpEventHeader<uint8>(event), request);
			break;

		case HCI_EVENT_INQUIRY_RESULT:
			InquiryResult(JumpEventHeader<uint8>(event), request);
			break;

		case HCI_EVENT_CONN_COMPLETE:
			ConnectionComplete(
				JumpEventHeader<struct hci_ev_conn_complete>(event), request);
			break;

		case HCI_EVENT_DISCONNECTION_COMPLETE:
			// should belong to a request?  can be sporadic or initiated by us?
			DisconnectionComplete(
				JumpEventHeader<struct hci_ev_disconnection_complete_reply>
				(event), request);
			break;

		case HCI_EVENT_AUTH_COMPLETE:

			break;

		case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
			RemoteNameRequestComplete(
				JumpEventHeader
					<struct hci_ev_remote_name_request_complete_reply>
				(event), request);
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

		case HCI_EVENT_FLUSH_OCCUR:
			break;

		case HCI_EVENT_ROLE_CHANGE:
			RoleChange(
				JumpEventHeader<struct hci_ev_role_change>(event), request);
			break;

		case HCI_EVENT_MODE_CHANGE:
			break;

		case HCI_EVENT_RETURN_LINK_KEYS:
			ReturnLinkKeys(
				JumpEventHeader<struct hci_ev_return_link_keys>(event));
			break;

		case HCI_EVENT_LINK_KEY_REQ:
			LinkKeyRequested(
				JumpEventHeader<struct hci_ev_link_key_req>(event), request);
			break;

		case HCI_EVENT_LINK_KEY_NOTIFY:
			LinkKeyNotify(
				JumpEventHeader<struct hci_ev_link_key_notify>(event), request);
			break;

		case HCI_EVENT_LOOPBACK_COMMAND:
			break;

		case HCI_EVENT_DATA_BUFFER_OVERFLOW:
			break;

		case HCI_EVENT_MAX_SLOT_CHANGE:
			MaxSlotChange(JumpEventHeader<struct hci_ev_max_slot_change>(event),
			request);
			break;

		case HCI_EVENT_READ_CLOCK_OFFSET_COMPL:
			break;

		case HCI_EVENT_CON_PKT_TYPE_CHANGED:
			break;

		case HCI_EVENT_QOS_VIOLATION:
			break;

		case HCI_EVENT_PAGE_SCAN_REP_MODE_CHANGE:
			PageScanRepetitionModeChange(
				JumpEventHeader<struct hci_ev_page_scan_rep_mode_change>(event),
				request);
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
LocalDeviceImpl::HandleEvent(struct hci_event_header* event)
{
/*
	printf("### Incoming event: len = %d\n", event->elen);
	for (int16 index = 0; index < event->elen + 2; index++) {
		printf("%x:", ((uint8*)event)[index]);
	}
	printf("### \n");
*/
	BMessage* request = NULL;
	int32 eventIndexLocation;

	// Check if it is a requested one
	switch (event->ecode) {
		case HCI_EVENT_CMD_COMPLETE:
		{
			struct hci_ev_cmd_complete* commandComplete
				= JumpEventHeader<struct hci_ev_cmd_complete>(event);

			TRACE_BT("LocalDeviceImpl: Incoming CommandComplete(%d) for %s\n", commandComplete->ncmd,
				BluetoothCommandOpcode(commandComplete->opcode));

			request = FindPetition(event->ecode, commandComplete->opcode,
				&eventIndexLocation);

			if (request != NULL)
				CommandComplete(commandComplete, request, eventIndexLocation);

			break;
		}
		case HCI_EVENT_CMD_STATUS:
		{
			struct hci_ev_cmd_status* commandStatus
				= JumpEventHeader<struct hci_ev_cmd_status>(event);

			TRACE_BT("LocalDeviceImpl: Incoming CommandStatus(%d)(%s) for %s\n", commandStatus->ncmd,
				BluetoothError(commandStatus->status),
				BluetoothCommandOpcode(commandStatus->opcode));

			request = FindPetition(event->ecode, commandStatus->opcode,
				&eventIndexLocation);
			if (request != NULL)
				CommandStatus(commandStatus, request, eventIndexLocation);

			break;
		}
		default:
			TRACE_BT("LocalDeviceImpl: Incoming %s event\n", BluetoothEvent(event->ecode));

			request = FindPetition(event->ecode);
			if (request != NULL)
				HandleExpectedRequest(event, request);

			break;
	}

	if (request == NULL) {
		TRACE_BT("LocalDeviceImpl: Event %s could not be understood or delivered\n",
			BluetoothEvent(event->ecode));
		HandleUnexpectedEvent(event);
	}
}


#if 0
#pragma mark -
#endif


void
LocalDeviceImpl::CommandComplete(struct hci_ev_cmd_complete* event,
	BMessage* request, int32 index)
{
	int16 opcodeExpected;
	BMessage reply;
	status_t status;

	// Handle command complete information
	request->FindInt16("opcodeExpected", index, &opcodeExpected);

	if (request->IsSourceWaiting() == false) {
		TRACE_BT("LocalDeviceImpl: Nobody waiting for the event\n");
	}

	switch ((uint16)opcodeExpected) {

		case PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_LOCAL_VERSION):
		{
			struct hci_rp_read_loc_version* version
				= JumpEventHeader<struct hci_rp_read_loc_version,
				struct hci_ev_cmd_complete>(event);


			if (version->status == BT_OK) {

				if (!IsPropertyAvailable("hci_version"))
					fProperties->AddInt8("hci_version", version->hci_version);

				if (!IsPropertyAvailable("hci_revision")) {
					fProperties->AddInt16("hci_revision",
						version->hci_revision);
				}

				if (!IsPropertyAvailable("lmp_version"))
					fProperties->AddInt8("lmp_version", version->lmp_version);

				if (!IsPropertyAvailable("lmp_subversion")) {
					fProperties->AddInt16("lmp_subversion",
						version->lmp_subversion);
				}

				if (!IsPropertyAvailable("manufacturer")) {
					fProperties->AddInt16("manufacturer",
						version->manufacturer);
				}
			}

			TRACE_BT("LocalDeviceImpl: Reply for Local Version %x\n", version->status);

			reply.AddInt8("status", version->status);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
			break;
		}

		case  PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_PG_TIMEOUT):
		{
			struct hci_rp_read_page_timeout* pageTimeout
				= JumpEventHeader<struct hci_rp_read_page_timeout,
				struct hci_ev_cmd_complete>(event);

			if (pageTimeout->status == BT_OK) {
				fProperties->AddInt16("page_timeout",
					pageTimeout->page_timeout);

				TRACE_BT("LocalDeviceImpl: Page Timeout=%x\n", pageTimeout->page_timeout);
			}

			reply.AddInt8("status", pageTimeout->status);
			reply.AddInt32("result", pageTimeout->page_timeout);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
			break;
		}

		case PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_LOCAL_FEATURES):
		{
			struct hci_rp_read_loc_features* features
				= JumpEventHeader<struct hci_rp_read_loc_features,
				struct hci_ev_cmd_complete>(event);

			if (features->status == BT_OK) {

				if (!IsPropertyAvailable("features")) {
					fProperties->AddData("features", B_ANY_TYPE,
						&features->features, 8);

					uint16 packetType = HCI_DM1 | HCI_DH1 | HCI_HV1;

					bool roleSwitch
						= (features->features[0] & LMP_RSWITCH) != 0;
					bool encryptCapable
						= (features->features[0] & LMP_ENCRYPT) != 0;

					if (features->features[0] & LMP_3SLOT)
						packetType |= (HCI_DM3 | HCI_DH3);

					if (features->features[0] & LMP_5SLOT)
						packetType |= (HCI_DM5 | HCI_DH5);

					if (features->features[1] & LMP_HV2)
						packetType |= (HCI_HV2);

					if (features->features[1] & LMP_HV3)
						packetType |= (HCI_HV3);

					fProperties->AddInt16("packet_type", packetType);
					fProperties->AddBool("role_switch_capable", roleSwitch);
					fProperties->AddBool("encrypt_capable", encryptCapable);

					TRACE_BT("LocalDeviceImpl: Packet type %x role switch %d encrypt %d\n",
						packetType, roleSwitch, encryptCapable);
				}

			}

			TRACE_BT("LocalDeviceImpl: Reply for Local Features %x\n", features->status);

			reply.AddInt8("status", features->status);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
			break;
		}

		case PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_BUFFER_SIZE):
		{
			struct hci_rp_read_buffer_size* buffer
				= JumpEventHeader<struct hci_rp_read_buffer_size,
				struct hci_ev_cmd_complete>(event);

			if (buffer->status == BT_OK) {

				if (!IsPropertyAvailable("acl_mtu"))
					fProperties->AddInt16("acl_mtu", buffer->acl_mtu);

				if (!IsPropertyAvailable("sco_mtu"))
					fProperties->AddInt8("sco_mtu", buffer->sco_mtu);

				if (!IsPropertyAvailable("acl_max_pkt"))
					fProperties->AddInt16("acl_max_pkt", buffer->acl_max_pkt);

				if (!IsPropertyAvailable("sco_max_pkt"))
					fProperties->AddInt16("sco_max_pkt", buffer->sco_max_pkt);

			}

			TRACE_BT("LocalDeviceImpl: Reply for Read Buffer Size %x\n", buffer->status);


			reply.AddInt8("status", buffer->status);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
		}
		break;

		case PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR):
		{
			struct hci_rp_read_bd_addr* readbdaddr
				= JumpEventHeader<struct hci_rp_read_bd_addr,
				struct hci_ev_cmd_complete>(event);

			if (readbdaddr->status == BT_OK) {
				reply.AddData("bdaddr", B_ANY_TYPE, &readbdaddr->bdaddr,
					sizeof(bdaddr_t));
			}

			TRACE_BT("LocalDeviceImpl: Read bdaddr status = %x\n", readbdaddr->status);

			reply.AddInt8("status", readbdaddr->status);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
		}
		break;


		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_CLASS_OF_DEV):
		{
			struct hci_read_dev_class_reply* classDev
				= JumpEventHeader<struct hci_read_dev_class_reply,
				struct hci_ev_cmd_complete>(event);

			if (classDev->status == BT_OK) {
				reply.AddData("devclass", B_ANY_TYPE, &classDev->dev_class,
					sizeof(classDev->dev_class));
			}

			TRACE_BT("LocalDeviceImpl: Read DeviceClass status = %x DeviceClass = [%x][%x][%x]\n",
				classDev->status, classDev->dev_class[0],
				classDev->dev_class[1], classDev->dev_class[2]);


			reply.AddInt8("status", classDev->status);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
			break;
		}

		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME):
		{
			struct hci_rp_read_local_name* readLocalName
				= JumpEventHeader<struct hci_rp_read_local_name,
				struct hci_ev_cmd_complete>(event);


			if (readLocalName->status == BT_OK) {
				reply.AddString("friendlyname",
					(const char*)readLocalName->local_name);
			}

			TRACE_BT("LocalDeviceImpl: Friendly name status %x\n", readLocalName->status);

			reply.AddInt8("status", readLocalName->status);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
			break;
		}

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_PIN_CODE_REPLY):
		{
			uint8* statusReply = (uint8*)(event + 1);

			// TODO: This reply has to match the BDADDR of the outgoing message
			TRACE_BT("LocalDeviceImpl: pincode accept status %x\n", *statusReply);

			reply.AddInt8("status", *statusReply);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			//ClearWantedEvent(request, HCI_EVENT_CMD_COMPLETE, opcodeExpected);
			ClearWantedEvent(request);
			break;
		}

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_PIN_CODE_NEG_REPLY):
		{
			uint8* statusReply = (uint8*)(event + 1);

			// TODO: This reply might match the BDADDR of the outgoing message
			// => FindPetition should be expanded....
			TRACE_BT("LocalDeviceImpl: pincode reject status %x\n", *statusReply);

			reply.AddInt8("status", *statusReply);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request, HCI_EVENT_CMD_COMPLETE, opcodeExpected);
			break;
		}

		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_STORED_LINK_KEY):
		{
			struct hci_read_stored_link_key_reply* linkKeyRetrieval
				= JumpEventHeader<struct hci_read_stored_link_key_reply,
				struct hci_ev_cmd_complete>(event);

			TRACE_BT("LocalDeviceImpl: Status %s MaxKeys=%d, KeysRead=%d\n",
				BluetoothError(linkKeyRetrieval->status),
				linkKeyRetrieval->max_num_keys,
				linkKeyRetrieval->num_keys_read);

			reply.AddInt8("status", linkKeyRetrieval->status);
			status = request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			ClearWantedEvent(request);
			break;
		}

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_LINK_KEY_NEG_REPLY):
		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_LINK_KEY_REPLY):
		{
			struct hci_cp_link_key_reply_reply* linkKeyReply
				= JumpEventHeader<struct hci_cp_link_key_reply_reply,
				struct hci_ev_cmd_complete>(event);

			TRACE_BT("LocalDeviceImpl: Status %s addresss=%s\n", BluetoothError(linkKeyReply->status),
				bdaddrUtils::ToString(linkKeyReply->bdaddr).String());

			ClearWantedEvent(request, HCI_EVENT_CMD_COMPLETE, opcodeExpected);
			break;
		}

		case  PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_SCAN_ENABLE):
		{
			struct hci_read_scan_enable* scanEnable
				= JumpEventHeader<struct hci_read_scan_enable,
				struct hci_ev_cmd_complete>(event);

			if (scanEnable->status == BT_OK) {
				fProperties->AddInt8("scan_enable", scanEnable->enable);

				TRACE_BT("LocalDeviceImpl: enable = %x\n", scanEnable->enable);
			}

			reply.AddInt8("status", scanEnable->status);
			reply.AddInt8("scan_enable", scanEnable->enable);
			status = request->SendReply(&reply);
			printf("Sending reply. scan_enable = %d\n", scanEnable->enable);
			// debug reply.PrintToStream();

			// This request is not gonna be used anymore
			ClearWantedEvent(request);
			break;
		}

		// place here all CC that just replies a uint8 status
		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_RESET):
		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_SCAN_ENABLE):
		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_CLASS_OF_DEV):
		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_PG_TIMEOUT):
		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_CA_TIMEOUT):
		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_AUTH_ENABLE):
		case PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_LOCAL_NAME):
		case PACK_OPCODE(OGF_VENDOR_CMD, OCF_WRITE_BCM2035_BDADDR):
		{
			reply.AddInt8("status", *(uint8*)(event + 1));

			TRACE_BT("LocalDeviceImpl: %s for %s status %x\n", __FUNCTION__,
				BluetoothCommandOpcode(opcodeExpected), *(uint8*)(event + 1));

			status = request->SendReply(&reply);
			printf("%s: Sending reply write...\n", __func__);
			if (status < B_OK)
				printf("%s: Error sending reply write!\n", __func__);

			ClearWantedEvent(request);
			break;
		}

		default:
			TRACE_BT("LocalDeviceImpl: Command Complete not handled\n");
			break;
	}
}


void
LocalDeviceImpl::CommandStatus(struct hci_ev_cmd_status* event,
	BMessage* request, int32 index)
{
	int16 opcodeExpected;
	BMessage reply;

	// Handle command complete information
	request->FindInt16("opcodeExpected", index, &opcodeExpected);

	if (request->IsSourceWaiting() == false) {
		TRACE_BT("LocalDeviceImpl: Nobody waiting for the event\n");
	}

	switch (opcodeExpected) {

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_INQUIRY):
		{
			reply.what = BT_MSG_INQUIRY_STARTED;

			TRACE_BT("LocalDeviceImpl: Inquiry status %x\n", event->status);

			reply.AddInt8("status", event->status);
			request->SendReply(&reply);
			//printf("Sending reply... %ld\n", status);
			// debug reply.PrintToStream();

			ClearWantedEvent(request, HCI_EVENT_CMD_STATUS,
				PACK_OPCODE(OGF_LINK_CONTROL, OCF_INQUIRY));
		}
		break;

		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST):
		case PACK_OPCODE(OGF_LINK_CONTROL, OCF_CREATE_CONN):
		{
			if (event->status == BT_OK) {
				ClearWantedEvent(request, HCI_EVENT_CMD_STATUS, opcodeExpected);
			} else {
				TRACE_BT("LocalDeviceImpl: Command Status for remote friendly name %x\n",
					event->status);

				reply.AddInt8("status", event->status);
				request->SendReply(&reply);
				//printf("Sending reply... %ld\n", status);
				// debug reply.PrintToStream();

				ClearWantedEvent(request, HCI_EVENT_CMD_STATUS, opcodeExpected);
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
			TRACE_BT("LocalDeviceImpl: Command Status not handled\n");
		break;
	}
}


void
LocalDeviceImpl::InquiryResult(uint8* numberOfResponses, BMessage* request)
{
	BMessage reply(BT_MSG_INQUIRY_DEVICE);

	uint8 responses = *numberOfResponses;

	// skipping here the number of responses
	reply.AddData("info", B_ANY_TYPE, numberOfResponses + 1,
		(*numberOfResponses) * sizeof(struct inquiry_info) );

	reply.AddInt8("count", *numberOfResponses);

	TRACE_BT("LocalDeviceImpl: %s #responses=%d\n",
		__FUNCTION__, *numberOfResponses);

	struct inquiry_info* info = JumpEventHeader<struct inquiry_info, uint8>
		(numberOfResponses);

	while (responses > 0) {
		TRACE_BT("LocalDeviceImpl: page_rep=%d scan_period=%d, scan=%d clock=%d\n",
			info->pscan_rep_mode, info->pscan_period_mode, info->pscan_mode,
			info->clock_offset);
		responses--;
		info++;
	}

	printf("%s: Sending reply...\n", __func__);
	status_t status = request->SendReply(&reply);
	if (status < B_OK)
		printf("%s: Error sending reply!\n", __func__);
}


void
LocalDeviceImpl::InquiryComplete(uint8* status, BMessage* request)
{
	BMessage reply(BT_MSG_INQUIRY_COMPLETED);

	reply.AddInt8("status", *status);

	printf("%s: Sending reply...\n", __func__);
	status_t stat = request->SendReply(&reply);
	if (stat < B_OK)
		printf("%s: Error sending reply!\n", __func__);

	ClearWantedEvent(request);
}


void
LocalDeviceImpl::RemoteNameRequestComplete(
	struct hci_ev_remote_name_request_complete_reply* remotename,
	BMessage* request)
{
	BMessage reply;

	if (remotename->status == BT_OK) {
		reply.AddString("friendlyname", (const char*)remotename->remote_name );
	}

	reply.AddInt8("status", remotename->status);

	TRACE_BT("LocalDeviceImpl: %s for %s with status %s\n",
		BluetoothEvent(HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE),
		bdaddrUtils::ToString(remotename->bdaddr).String(),
		BluetoothError(remotename->status));

	status_t status = request->SendReply(&reply);
	if (status < B_OK)
		printf("%s: Error sending reply to BMessage request: %s!\n",
			__func__, strerror(status));

	// This request is not gonna be used anymore
	ClearWantedEvent(request);
}


void
LocalDeviceImpl::ConnectionRequest(struct hci_ev_conn_request* event,
	BMessage* request)
{
	size_t size;
	void* command;

	TRACE_BT("LocalDeviceImpl: Connection Incoming type %x from %s...\n",
		event->link_type, bdaddrUtils::ToString(event->bdaddr).String());

	// TODO: add a possible request in the queue
	if (true) { // Check Preferences if we are to accept this connection

		// Keep ourselves as slave
		command = buildAcceptConnectionRequest(event->bdaddr, 0x01 , &size);

		BMessage* newrequest = new BMessage;
		newrequest->AddInt16("eventExpected", HCI_EVENT_CMD_STATUS);
		newrequest->AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL,
			OCF_ACCEPT_CONN_REQ));

		newrequest->AddInt16("eventExpected", HCI_EVENT_CONN_COMPLETE);
		newrequest->AddInt16("eventExpected", HCI_EVENT_PIN_CODE_REQ);
		newrequest->AddInt16("eventExpected", HCI_EVENT_ROLE_CHANGE);
		newrequest->AddInt16("eventExpected", HCI_EVENT_LINK_KEY_NOTIFY);
		newrequest->AddInt16("eventExpected",
			HCI_EVENT_PAGE_SCAN_REP_MODE_CHANGE);

		#if 0
		newrequest->AddInt16("eventExpected", HCI_EVENT_MAX_SLOT_CHANGE);
		newrequest->AddInt16("eventExpected", HCI_EVENT_DISCONNECTION_COMPLETE);
		#endif

		AddWantedEvent(newrequest);

		if ((fHCIDelegate)->IssueCommand(command, size) == B_ERROR) {
			TRACE_BT("LocalDeviceImpl: Command issued error for Accepting connection\n");
				// remove the request?
		} else {
			TRACE_BT("LocalDeviceImpl: Command issued for Accepting connection\n");
		}
	}
}


void
LocalDeviceImpl::ConnectionComplete(struct hci_ev_conn_complete* event,
	BMessage* request)
{

	if (event->status == BT_OK) {
		uint8 cod[3] = {0, 0, 0};

		// TODO: Review, this rDevice is leaked
		ConnectionIncoming* iConnection = new ConnectionIncoming(
			new RemoteDevice(event->bdaddr, cod));
		iConnection->Show();

		TRACE_BT("LocalDeviceImpl: %s: Address %s handle=%#x type=%d encrypt=%d\n", __FUNCTION__,
				bdaddrUtils::ToString(event->bdaddr).String(), event->handle,
				event->link_type, event->encrypt_mode);

	} else {
		TRACE_BT("LocalDeviceImpl: %s: failed with error %s\n", __FUNCTION__,
			BluetoothError(event->status));
	}

	// it was expected
	if (request != NULL) {
		BMessage reply;
		reply.AddInt8("status", event->status);

		if (event->status == BT_OK)
			reply.AddInt16("handle", event->handle);

		printf("%s: Sending reply...\n", __func__);
		status_t status = request->SendReply(&reply);
		if (status < B_OK)
			printf("%s: Error sending reply!\n", __func__);
		// debug reply.PrintToStream();

		// This request is not gonna be used anymore
		ClearWantedEvent(request);
	}

}


void
LocalDeviceImpl::DisconnectionComplete(
	struct hci_ev_disconnection_complete_reply* event, BMessage* request)
{
	TRACE_BT("LocalDeviceImpl: %s: Handle=%#x, reason=%s status=%x\n", __FUNCTION__, event->handle,
		BluetoothError(event->reason), event->status);

	if (request != NULL) {
		BMessage reply;
		reply.AddInt8("status", event->status);

		printf("%s: Sending reply...\n", __func__);
		status_t status = request->SendReply(&reply);
		if (status < B_OK)
			printf("%s: Error sending reply!\n", __func__);
		// debug reply.PrintToStream();

		ClearWantedEvent(request);
	}
}


void
LocalDeviceImpl::PinCodeRequest(struct hci_ev_pin_code_req* event,
	BMessage* request)
{
	PincodeWindow* iPincode = new PincodeWindow(event->bdaddr, GetID());
	iPincode->Show();
}


void
LocalDeviceImpl::RoleChange(hci_ev_role_change* event, BMessage* request)
{
	TRACE_BT("LocalDeviceImpl: %s: Address %s role=%d status=%d\n", __FUNCTION__,
		bdaddrUtils::ToString(event->bdaddr).String(), event->role, event->status);
}


void
LocalDeviceImpl::PageScanRepetitionModeChange(
	struct hci_ev_page_scan_rep_mode_change* event, BMessage* request)
{
	TRACE_BT("LocalDeviceImpl: %s: Address %s type=%d\n",	__FUNCTION__,
		bdaddrUtils::ToString(event->bdaddr).String(), event->page_scan_rep_mode);
}


void
LocalDeviceImpl::LinkKeyNotify(hci_ev_link_key_notify* event,
	BMessage* request)
{
	TRACE_BT("LocalDeviceImpl: %s: Address %s, key=%s, type=%d\n", __FUNCTION__,
		bdaddrUtils::ToString(event->bdaddr).String(),
		LinkKeyUtils::ToString(event->link_key).String(), event->key_type);
}


void
LocalDeviceImpl::LinkKeyRequested(struct hci_ev_link_key_req* keyRequested,
	BMessage* request)
{
	TRACE_BT("LocalDeviceImpl: %s: Address %s\n", __FUNCTION__,
		bdaddrUtils::ToString(keyRequested->bdaddr).String());

	// TODO:
	// Here we are suposed to check the BDADDR received, look into the server
	// (RemoteDevice Database) if we have any pas link key interchanged with
	// the given address if we have we are to accept it will "Link key Request
	// Reply". As we dont not have such database yet, we will always deny it
	// forcing the remote device to start a pairing.

	BluetoothCommand<typed_command(hci_cp_link_key_neg_reply)>
		linkKeyNegativeReply(OGF_LINK_CONTROL, OCF_LINK_KEY_NEG_REPLY);

	bdaddrUtils::Copy(linkKeyNegativeReply->bdaddr, keyRequested->bdaddr);

	if ((fHCIDelegate)->IssueCommand(linkKeyNegativeReply.Data(),
		linkKeyNegativeReply.Size()) == B_ERROR) {
		TRACE_BT("LocalDeviceImpl: Command issued error for reply %s\n", __FUNCTION__);
	} else {
		TRACE_BT("LocalDeviceImpl: Command issued in reply of  %s\n", __FUNCTION__);
	}

	if (request != NULL)
		ClearWantedEvent(request, HCI_EVENT_LINK_KEY_REQ);

}


void
LocalDeviceImpl::ReturnLinkKeys(struct hci_ev_return_link_keys* returnedKeys)
{
	TRACE_BT("LocalDeviceImpl: %s: #keys=%d\n",
		__FUNCTION__, returnedKeys->num_keys);

	uint8 numKeys = returnedKeys->num_keys;

	struct link_key_info* linkKeys = &returnedKeys->link_keys;

	while (numKeys > 0) {

		TRACE_BT("LocalDeviceImpl: Address=%s key=%s\n",
			bdaddrUtils::ToString(linkKeys->bdaddr).String(),
			LinkKeyUtils::ToString(linkKeys->link_key).String());

		linkKeys++;
	}
}


void
LocalDeviceImpl::MaxSlotChange(struct hci_ev_max_slot_change* event,
	BMessage* request)
{
	TRACE_BT("LocalDeviceImpl: %s: Handle=%#x, max slots=%d\n", __FUNCTION__,
		event->handle, event->lmp_max_slots);
}


void
LocalDeviceImpl::HardwareError(struct hci_ev_hardware_error* event)
{
	TRACE_BT("LocalDeviceImpl: %s: hardware code=%#x\n",	__FUNCTION__, event->hardware_code);
}


void
LocalDeviceImpl::NumberOfCompletedPackets(struct hci_ev_num_comp_pkts* event)
{
	TRACE_BT("LocalDeviceImpl: %s: #Handles=%d\n", __FUNCTION__, event->num_hndl);

	struct handle_and_number* numberPackets
		= JumpEventHeader<handle_and_number, hci_ev_num_comp_pkts>(event);

	for (uint8 i = 0; i < event->num_hndl; i++) {

		TRACE_BT("LocalDeviceImpl: %s: Handle=%d #packets=%d\n", __FUNCTION__, numberPackets->handle,
			numberPackets->num_completed);

			numberPackets++;
	}
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

		// Give the chance of just issuing the command
		int16 eventFound;
		if (request->FindInt16("eventExpected", &eventFound) == B_OK)
			AddWantedEvent(request);
		// LEAK: is command buffer freed within the Message?
		if (((HCITransportAccessor*)fHCIDelegate)->IssueCommand(command, size)
			== B_ERROR) {
			// TODO:
			// Reply the request with error!
			// Remove the just added request
			TRACE_BT("LocalDeviceImpl: ## ERROR Command issue, REMOVING!\n");
			ClearWantedEvent(request);

		} else {
			return B_OK;
		}
	} else {
		TRACE_BT("LocalDeviceImpl: No command specified for simple request\n");
	}

	return B_ERROR;
}
