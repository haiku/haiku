/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _COMMAND_MANAGER_H
#define _COMMAND_MANAGER_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI_command.h>

#include <malloc.h>
#include <string.h>


#define typed_command(type) type,sizeof(type)

/* Experimental stack allocated alternative to build commands */
template <typename Type = void, int paramSize = 0, int HeaderSize = HCI_COMMAND_HDR_SIZE>
class BluetoothCommand {

public:
	BluetoothCommand(uint8 ogf, uint8 ocf)
	{
		fHeader = (struct hci_command_header*) fBuffer;
		
		if (paramSize != 0)
			fContent = (Type*)(fHeader + 1);
		else
			fContent = (Type*)fHeader; // avoid pointing outside in case of not having parameters
		
		fHeader->opcode = B_HOST_TO_LENDIAN_INT16(PACK_OPCODE(ogf, ocf));
		fHeader->clen = paramSize; 
	}

	Type*
	operator->() const
	{
 		return fContent;
	}
 
	void* 
	Data() const
	{
		return (void*)fBuffer;
	}
	
	size_t Size() const
	{
		return HeaderSize + paramSize;
	}
	
private:
	char fBuffer[paramSize + HeaderSize];
	Type* fContent;
	struct hci_command_header* fHeader;
};

/* CONTROL BASEBAND */
void* buildReset(size_t* outsize);
void* buildReadLocalName(size_t* outsize);
void* buildWriteScan(uint8 scanmode, size_t* outsize);
void* buildAuthEnable(uint8 auth, size_t* outsize);
void* buildReadClassOfDevice(size_t* outsize);

/* LINK CONTROL */
void* buildRemoteNameRequest(bdaddr_t bdaddr,uint8 pscan_rep_mode, uint16 clock_offset, size_t* outsize);
void* buildInquiry(uint32 lap, uint8 length, uint8 num_rsp, size_t* outsize);
void* buildInquiryCancel(size_t* outsize);
void* buildPinCodeRequestReply(bdaddr_t bdaddr, uint8 length, char pincode[16], size_t* outsize);
void* buildPinCodeRequestNegativeReply(bdaddr_t bdaddr, size_t* outsize);
void* buildAcceptConnectionRequest(bdaddr_t bdaddr, uint8 role, size_t* outsize);
void* buildRejectConnectionRequest(bdaddr_t bdaddr, size_t* outsize);

/* OGF_INFORMATIONAL_PARAM */
void* buildReadLocalVersionInformation(size_t* outsize);
void* buildReadBufferSize(size_t* outsize);
void* buildReadBdAddr(size_t* outsize);

#endif
