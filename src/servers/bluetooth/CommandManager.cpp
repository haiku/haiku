/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI_command.h>

#include <malloc.h>

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


/* CONTROL BASEBAND */
void* buildReset(size_t* outsize)
{
    return buildCommand(OGF_CONTROL_BASEBAND, OCF_RESET, NULL, 0, outsize);
}


void* buildReadLocalName(size_t* outsize)
{
    return buildCommand(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME, NULL, 0, outsize);
}


/* LINK CONTROL */
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


/* OGF_INFORMATIONAL_PARAM */
void* buildReadBufferSize(size_t* outsize)
{
    return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_BUFFER_SIZE, NULL, 0, outsize);
}    


void* buildReadBdAddr(size_t* outsize)
{
	return buildCommand(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR, NULL, 0, outsize);
}    
