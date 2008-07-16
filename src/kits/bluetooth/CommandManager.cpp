/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI_command.h>

#include <malloc.h>
#include <string.h>

#include "CommandManager.h"


inline void* buildCommand(uint8 ogf, uint8 ocf, void** param, size_t psize, size_t* outsize)
{
	struct hci_command_header* header;
	
#ifdef BT_IOCTLS_PASS_SIZE
    header = (struct hci_command_header*) malloc(psize + sizeof(struct hci_command_header));
	*outsize = psize + sizeof(struct hci_command_header);
#else
	size_t* size = (size_t*)malloc(psize + sizeof(struct hci_command_header) + sizeof(size_t));
	*outsize = psize + sizeof(struct hci_command_header) + sizeof(size_t);

    *size = psize + sizeof(struct hci_command_header);
    header = (struct hci_command_header*) (((uint8*)size)+4);
#endif


    if (header != NULL) {

        header->opcode = B_HOST_TO_LENDIAN_INT16(PACK_OPCODE(ogf, ocf));
        header->clen = psize;

        if (param != NULL && psize != 0) {
            *param = ((uint8*)header) + sizeof(struct hci_command_header);
        }
    }
#ifdef BT_IOCTLS_PASS_SIZE
    return header;
#else
    return (void*)size;
#endif
}


#if 0
#pragma mark - CONTROL BASEBAND -
#endif


void* buildReset(size_t* outsize)
{
    return buildCommand(OGF_CONTROL_BASEBAND, OCF_RESET, NULL, 0, outsize);
}


void* buildReadLocalName(size_t* outsize)
{
    return buildCommand(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME, NULL, 0, outsize);
}


void* buildWriteScan(uint8 scanmode, size_t* outsize)
{
    struct hci_write_scan_enable* param;
	void* command = buildCommand(OGF_CONTROL_BASEBAND, OCF_WRITE_SCAN_ENABLE, (void**) &param, sizeof(struct hci_write_scan_enable), outsize);


    if (command != NULL) {
        param->scan = scanmode;
    }

    return command;

}


void* buildAuthEnable(uint8 auth, size_t* outsize)
{
    struct hci_write_authentication_enable* param;
	void* command = buildCommand(OGF_CONTROL_BASEBAND, OCF_WRITE_AUTH_ENABLE, (void**) &param, sizeof(struct hci_write_authentication_enable), outsize);


    if (command != NULL) {
        param->authentication = auth;
    }

    return command;

}


#if 0
#pragma mark - LINK CONTROL -
#endif


void* buildCreateConnection(bdaddr_t bdaddr)
{
	/*
	cm4.size = sizeof(struct hci_command_header)+sizeof(struct hci_cp_create_conn);
	cm4.header.opcode = B_HOST_TO_LENDIAN_INT16(hci_opcode_pack(OGF_LINK_CONTROL, OCF_CREATE_CONN));
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

	return NULL;
}


void* buildRemoteNameRequest(bdaddr_t bdaddr,uint8 pscan_rep_mode, uint16 clock_offset, size_t* outsize)
{

    struct hci_remote_name_request* param;
    void* command = buildCommand(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST, (void**) &param, sizeof(struct hci_remote_name_request), outsize);

    if (command != NULL) {
        param->bdaddr = bdaddr;
        param->pscan_rep_mode = pscan_rep_mode;
        param->clock_offset = clock_offset;
    }

    return command;
}


void* buildInquiry(uint32 lap, uint8 length, uint8 num_rsp, size_t* outsize)
{

    struct hci_cp_inquiry* param;
    void* command = buildCommand(OGF_LINK_CONTROL, OCF_INQUIRY, (void**) &param, sizeof(struct hci_cp_inquiry), outsize);

    if (command != NULL) {

		param->lap[2] = (lap >> 16) & 0xFF;
		param->lap[1] = (lap >>  8) & 0xFF;
    	param->lap[0] = (lap >>  0) & 0xFF;
    	param->length = length;
    	param->num_rsp = num_rsp;
    }

    return command;
}


void* buildInquiryCancel(size_t* outsize)
{

    return buildCommand(OGF_LINK_CONTROL, OCF_INQUIRY_CANCEL, NULL, 0, outsize);

}


void* buildPinCodeRequestReply(bdaddr_t bdaddr, uint8 length, char pincode[16], size_t* outsize)
{

    struct hci_cp_pin_code_reply* param;

    if (length > HCI_PIN_SIZE)  // PinCode cannot be longer than 16
	return NULL;

    void* command = buildCommand(OGF_LINK_CONTROL, OCF_PIN_CODE_REPLY, (void**) &param, sizeof(struct hci_cp_pin_code_reply), outsize);

    if (command != NULL) {

	param->bdaddr = bdaddr;
	param->pin_len = length;
    	memcpy(&param->pin_code, pincode, length);

    }

    return command;
}


void* buildPinCodeRequestNegativeReply(bdaddr_t bdaddr, size_t* outsize)
{

    struct hci_cp_pin_code_neg_reply* param;

    void* command = buildCommand(OGF_LINK_CONTROL, OCF_PIN_CODE_NEG_REPLY, 
                                 (void**) &param, sizeof(struct hci_cp_pin_code_neg_reply), outsize);

    if (command != NULL) {

		param->bdaddr = bdaddr;

    }

    return command;
}


void* buildAcceptConnectionRequest(bdaddr_t bdaddr, uint8 role, size_t* outsize)
{
	struct hci_cp_accept_conn_req* param;

	void* command = buildCommand(OGF_LINK_CONTROL, OCF_ACCEPT_CONN_REQ,
					(void**) &param, sizeof(struct hci_cp_accept_conn_req), outsize);

	if (command != NULL) {
		param->bdaddr = bdaddr;
		param->role = role;
	}

	return command;
}


void* buildRejectConnectionRequest(bdaddr_t bdaddr, size_t* outsize)
{
	struct hci_cp_reject_conn_req* param;

	void *command = buildCommand(OGF_LINK_CONTROL, OCF_REJECT_CONN_REQ,
					(void**) &param, sizeof(struct hci_cp_reject_conn_req), outsize);

	if (command != NULL) {
		param->bdaddr = bdaddr;
	}

	return command;
}


#if 0
#pragma mark - INFORMATIONAL_PARAM -
#endif


void* buildReadBufferSize(size_t* outsize)
{
    return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_BUFFER_SIZE, NULL, 0, outsize);
}


void* buildReadBdAddr(size_t* outsize)
{
	return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR, NULL, 0, outsize);
}




