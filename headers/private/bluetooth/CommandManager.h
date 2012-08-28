/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist
 * Copyright 2012 Fredrik Modéen [firstname]@[lastname].se
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _COMMAND_MANAGER_H
#define _COMMAND_MANAGER_H

#include <malloc.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/bluetooth_error.h>
#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <Message.h>
#include <Messenger.h>

#include <bluetoothserver_p.h>

#define typed_command(type) type, sizeof(type)

template <typename Type = void, int paramSize = 0,
	int HeaderSize = HCI_COMMAND_HDR_SIZE>
class BluetoothCommand {

public:
	BluetoothCommand(uint8 ogf, uint8 ocf)
	{
		fHeader = (struct hci_command_header*) fBuffer;

		if (paramSize != 0)
			fContent = (Type*)(fHeader + 1);
		else
			// avoid pointing outside in case of not having parameters
			fContent = (Type*)fHeader;

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


status_t
NonParameterCommandRequest(uint8 ofg, uint8 ocf, int32* result, hci_id hId,
	BMessenger* messenger);

template<typename PARAMETERCONTAINER, typename PARAMETERTYPE>
status_t
SingleParameterCommandRequest(uint8 ofg, uint8 ocf, PARAMETERTYPE parameter,
	int32* result, hci_id hId, BMessenger* messenger)
{
	int8 bt_status = BT_ERROR;

	BluetoothCommand<typed_command(PARAMETERCONTAINER)>
		simpleCommand(ofg, ocf);

	simpleCommand->param = parameter;

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	simpleCommand->param = parameter;

	request.AddInt32("hci_id", hId);
	request.AddData("raw command", B_ANY_TYPE, simpleCommand.Data(),
		simpleCommand.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(ofg, ocf));

	if (messenger->SendMessage(&request, &reply) == B_OK) {
		reply.FindInt8("status", &bt_status);
		if (result != NULL)
			reply.FindInt32("result", result);
	}

	return bt_status;
}


/* CONTROL BASEBAND */
void* buildReset(size_t* outsize);
void* buildReadLocalName(size_t* outsize);
void* buildReadScan(size_t* outsize);
void* buildWriteScan(uint8 scanmode, size_t* outsize);
void* buildReadClassOfDevice(size_t* outsize);

/* LINK CONTROL */
void* buildRemoteNameRequest(bdaddr_t bdaddr, uint8 pscan_rep_mode,
	uint16 clock_offset, size_t* outsize);
void* buildInquiry(uint32 lap, uint8 length, uint8 num_rsp, size_t* outsize);
void* buildInquiryCancel(size_t* outsize);
void* buildPinCodeRequestReply(bdaddr_t bdaddr, uint8 length, char pincode[16],
	size_t* outsize);
void* buildPinCodeRequestNegativeReply(bdaddr_t bdaddr, size_t* outsize);
void* buildAcceptConnectionRequest(bdaddr_t bdaddr, uint8 role,
	size_t* outsize);
void* buildRejectConnectionRequest(bdaddr_t bdaddr, size_t* outsize);

/* OGF_INFORMATIONAL_PARAM */
void* buildReadLocalVersionInformation(size_t* outsize);
void* buildReadBufferSize(size_t* outsize);
void* buildReadBdAddr(size_t* outsize);

#endif
