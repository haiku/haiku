/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Distributed under the terms of the MIT License.
 */

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <boot/platform.h>

#include "network.h"
#include "pxe_undi.h"

//#define TRACE_NETWORK
#ifdef TRACE_NETWORK
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif

#ifdef TRACE_NETWORK

static void
hex_dump(const void *_data, int length)
{
	uint8 *data = (uint8*)_data;
	for (int i = 0; i < length; i++) {
		if (i % 4 == 0) {
			if (i % 32 == 0) {
				if (i != 0)
					TRACE("\n");
				TRACE("%03x: ", i);
			} else
				TRACE(" ");
		}

		TRACE("%02x", data[i]);
	}
	TRACE("\n");
}

#else	// !TRACE_NETWORK

#define hex_dump(data, length)

#endif	// !TRACE_NETWORK


// #pragma mark - PXEService


PXEService::PXEService()
	: fPxeData(NULL),
	  fClientIP(0),
	  fServerIP(0),
	  fRootPath(NULL)
{
}


PXEService::~PXEService()
{
	free(fRootPath);
}


status_t
PXEService::Init()
{
	// get !PXE struct
	fPxeData = pxe_undi_find_data();
	if (!fPxeData) {
		panic("can't find !PXE structure");
		return B_ERROR;
	}

	dprintf("PXE API entrypoint at %04x:%04x\n", fPxeData->EntryPointSP.seg, fPxeData->EntryPointSP.ofs);

	// get cached info
	PXENV_GET_CACHED_INFO cached_info;
	cached_info.PacketType = PXENV_PACKET_TYPE_CACHED_REPLY;
	cached_info.BufferSize = 0;
	cached_info.BufferLimit = 0;
	cached_info.Buffer.seg = 0;
	cached_info.Buffer.ofs = 0;
	uint16 res = call_pxe_bios(fPxeData, GET_CACHED_INFO, &cached_info);
	if (res != 0 || cached_info.Status != 0) {
		char s[100];
		snprintf(s, sizeof(s), "Can't determine IP address! PXENV_GET_CACHED_INFO res %x, status %x\n", res, cached_info.Status);
		panic("%s", s);
		return B_ERROR;
	}

	char *buf = (char *)(cached_info.Buffer.seg * 16 + cached_info.Buffer.ofs);
	fClientIP = ntohl(*(ip_addr_t *)(buf + 16));
	fServerIP = ntohl(*(ip_addr_t *)(buf + 20));
	fMACAddress = mac_addr_t((uint8*)(buf + 28));

	uint8* options = (uint8*)buf + 236;
	int optionsLen = int(cached_info.BufferSize) - 236;

	// check magic
	if (optionsLen < 4 || options[0] != 0x63 || options[1] != 0x82
		|| options[2] != 0x53 || options[3] != 0x63) {
		return B_OK;
	}
	options += 4;
	optionsLen -= 4;

	// parse DHCP options
	while (optionsLen > 0) {
		int option = *(options++);
		optionsLen--;

		// check end or pad option
		if (option == 0xff || optionsLen < 0)
			break;
		if (option == 0x00)
			continue;

		// other options have a len field
		int len = *(options++);
		optionsLen--;
		if (len > optionsLen)
			break;

		// root path option
		if (option == 17) {
			dprintf("root path option: \"%.*s\"\n", len, (char*)options);
			free(fRootPath);
			fRootPath = (char*)malloc(len + 1);
			if (!fRootPath)
				return B_NO_MEMORY;
			memcpy(fRootPath, options, len);
			fRootPath[len] = '\0';
		}

		options += len;
		optionsLen -= len;
	}

	return B_OK;
}


// #pragma mark - UNDI


UNDI::UNDI()
 :	fRxFinished(true)
{
	TRACE("UNDI::UNDI\n");
}


UNDI::~UNDI()
{
	TRACE("UNDI::~UNDI\n");
}


status_t
UNDI::Init()
{
	TRACE("UNDI::Init\n");

	PXENV_UNDI_GET_INFORMATION get_info;
	PXENV_UNDI_GET_STATE get_state;
	PXENV_UNDI_OPEN undi_open;
	uint16 res;

	status_t error = PXEService::Init();
	if (error != B_OK)
		return error;

	dprintf("client-ip: %lu.%lu.%lu.%lu, server-ip: %lu.%lu.%lu.%lu\n",
		(fClientIP >> 24) & 0xff, (fClientIP >> 16) & 0xff, (fClientIP >> 8) & 0xff, fClientIP & 0xff,
		(fServerIP >> 24) & 0xff, (fServerIP >> 16) & 0xff, (fServerIP >> 8) & 0xff, fServerIP & 0xff);

	SetIPAddress(fClientIP);

	undi_open.OpenFlag = 0;
	undi_open.PktFilter = FLTR_DIRECTED | FLTR_BRDCST | FLTR_PRMSCS;
	undi_open.R_Mcast_Buf.MCastAddrCount = 0;

	res = call_pxe_bios(fPxeData, UNDI_OPEN, &undi_open);
	if (res != 0 || undi_open.Status != 0) {
		dprintf("PXENV_UNDI_OPEN failed, res %x, status %x, ignoring\n", res, undi_open.Status);
	}

	res = call_pxe_bios(fPxeData, UNDI_GET_STATE, &get_state);
	if (res != 0 || get_state.Status != 0) {
		dprintf("PXENV_UNDI_GET_STATE failed, res %x, status %x, ignoring\n", res, get_state.Status);
	} else {
		switch (get_state.UNDIstate) {
			case PXE_UNDI_GET_STATE_STARTED:
				TRACE("PXE_UNDI_GET_STATE_STARTED\n");
				break;
			case PXE_UNDI_GET_STATE_INITIALIZED:
				TRACE("PXE_UNDI_GET_STATE_INITIALIZED\n");
				break;
			case PXE_UNDI_GET_STATE_OPENED:
				TRACE("PXE_UNDI_GET_STATE_OPENED\n");
				break;
			default:
				TRACE("unknown undi state 0x%02x\n", get_state.UNDIstate);
				break;
		}
	}

	res = call_pxe_bios(fPxeData, UNDI_GET_INFORMATION, &get_info);
	if (res != 0 || get_info.Status != 0) {
		dprintf("PXENV_UNDI_GET_INFORMATION failed, res %x, status %x\n", res, get_info.Status);
		return B_ERROR;
	}

	TRACE("Status = %x\n", get_info.Status);
	TRACE("BaseIo = %x\n", get_info.BaseIo);
	TRACE("IntNumber = %x\n", get_info.IntNumber);
	TRACE("MaxTranUnit = %x\n", get_info.MaxTranUnit);
	TRACE("HwType = %x\n", get_info.HwType);
	TRACE("HwAddrLen = %x\n", get_info.HwAddrLen);
	TRACE("RxBufCt = %x\n", get_info.RxBufCt);
	TRACE("TxBufCt = %x\n", get_info.TxBufCt);

	fMACAddress = get_info.CurrentNodeAddress;

	TRACE("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", fMACAddress[0], fMACAddress[1], fMACAddress[2], fMACAddress[3], fMACAddress[4], fMACAddress[5]);

	return B_OK;
}


mac_addr_t
UNDI::MACAddress() const
{
	return fMACAddress;
}


void *
UNDI::AllocateSendReceiveBuffer(size_t size)
{
	TRACE("UNDI::AllocateSendReceiveBuffer, size %ld\n", size);
	if (size > 0x3000)
		return NULL;

	return (void *)0x500;
}


void
UNDI::FreeSendReceiveBuffer(void *buffer)
{
	TRACE("UNDI::FreeSendReceiveBuffer\n");
}


ssize_t
UNDI::Send(const void *buffer, size_t size)
{
	TRACE("UNDI::Send, buffer %p, size %ld\n", buffer, size);

//	hex_dump(buffer, size);

	PXENV_UNDI_TRANSMIT undi_tx;
	PXENV_UNDI_TBD undi_tbd;

	undi_tx.Protocol = P_UNKNOWN;
	undi_tx.XmitFlag = XMT_DESTADDR;
	undi_tx.DestAddr.seg = SEG((char *)buffer + 16);
	undi_tx.DestAddr.ofs = OFS((char *)buffer + 16);
	undi_tx.TBD.seg = SEG(&undi_tbd);
	undi_tx.TBD.ofs = OFS(&undi_tbd);

	undi_tbd.ImmedLength = size;
	undi_tbd.Xmit.seg = SEG(buffer);
	undi_tbd.Xmit.ofs = OFS(buffer);
	undi_tbd.DataBlkCount = 0;

	uint16 res = call_pxe_bios(fPxeData, UNDI_TRANSMIT, &undi_tx);
	if (res != 0 || undi_tx.Status != 0) {
		dprintf("UNDI_TRANSMIT failed, res %x, status %x\n", res, undi_tx.Status);
		return 0;
	}

	TRACE("UNDI_TRANSMIT success\n");

	return size;
}


ssize_t
UNDI::Receive(void *buffer, size_t size)
{
	//TRACE("UNDI::Receive, buffer %p, size %ld\n", buffer, size);

	PXENV_UNDI_ISR undi_isr;
	uint16 res;

	if (!fRxFinished) {
		TRACE("continue receive...\n");

		undi_isr.FuncFlag = PXENV_UNDI_ISR_IN_GET_NEXT;
		res = call_pxe_bios(fPxeData, UNDI_ISR, &undi_isr);
		if (res != 0 || undi_isr.Status != 0) {
			dprintf("PXENV_UNDI_ISR_IN_GET_NEXT failed, res %x, status %x\n", res, undi_isr.Status);
			fRxFinished = true;
			return 0;
		}

	} else {

		undi_isr.FuncFlag = PXENV_UNDI_ISR_IN_START;

		res = call_pxe_bios(fPxeData, UNDI_ISR, &undi_isr);
		if (res != 0 || undi_isr.Status != 0) {
			dprintf("PXENV_UNDI_ISR_IN_START failed, res %x, status %x\n", res, undi_isr.Status);
			return -1;
		}

		if (undi_isr.FuncFlag != PXENV_UNDI_ISR_OUT_OURS) {
//			TRACE("not ours\n");
			return -1;
		}

		// send EOI to pic ?

//		TRACE("PXENV_UNDI_ISR_OUT_OURS\n");

		undi_isr.FuncFlag = PXENV_UNDI_ISR_IN_PROCESS;
		res = call_pxe_bios(fPxeData, UNDI_ISR, &undi_isr);
		if (res != 0 || undi_isr.Status != 0) {
			dprintf("PXENV_UNDI_ISR_IN_PROCESS failed, res %x, status %x\n", res, undi_isr.Status);
			return -1;
		}
	}

	switch (undi_isr.FuncFlag) {
		case PXENV_UNDI_ISR_OUT_TRANSMIT:
			TRACE("PXENV_UNDI_ISR_OUT_TRANSMIT\n");
			fRxFinished = false;
			return 0;

		case PXENV_UNDI_ISR_OUT_RECEIVE:
			TRACE("PXENV_UNDI_ISR_OUT_RECEIVE\n");
//			TRACE("BufferLength %d\n", undi_isr.BufferLength);
//			TRACE("FrameLength %d\n", undi_isr.FrameLength);
//			TRACE("FrameHeaderLength %d\n", undi_isr.FrameHeaderLength);
			if (undi_isr.FrameLength > undi_isr.BufferLength)
				panic("UNDI::Receive: multi buffer frames not supported");
			if (size > undi_isr.BufferLength)
				size = undi_isr.BufferLength;
			memcpy(buffer, (const void *)(undi_isr.Frame.seg * 16 + undi_isr.Frame.ofs), size);
//			hex_dump(buffer, size);
			fRxFinished = false;
			return size;

		case PXENV_UNDI_ISR_OUT_BUSY:
			TRACE("PXENV_UNDI_ISR_OUT_BUSY\n");
			fRxFinished = true;
			return -1;

		case PXENV_UNDI_ISR_OUT_DONE:
			TRACE("PXENV_UNDI_ISR_OUT_DONE\n");
			fRxFinished = true;
			return -1;

		default:
			TRACE("default!!!\n");
			return -1;
	}
}


// #pragma mark - TFTP

TFTP::TFTP()
{
}


TFTP::~TFTP()
{
}


status_t
TFTP::Init()
{
	status_t error = PXEService::Init();
	if (error != B_OK)
		return error;



	return B_OK;
}


uint16
TFTP::ServerPort() const
{
	return 69;
}


status_t
TFTP::ReceiveFile(const char* fileName, uint8** data, size_t* size)
{
	// get file size
	pxenv_tftp_get_fsize getFileSize;
	getFileSize.server_ip.num = htonl(fServerIP);
	getFileSize.gateway_ip.num = 0;
	strlcpy(getFileSize.file_name, fileName, sizeof(getFileSize.file_name));

	uint16 res = call_pxe_bios(fPxeData, TFTP_GET_FILE_SIZE, &getFileSize);
	if (res != 0 || getFileSize.status != 0) {
		dprintf("TFTP_GET_FILE_SIZE failed, res %x, status %x\n", res,
			getFileSize.status);

		return B_ERROR;
	}

	uint32 fileSize = getFileSize.file_size;
	dprintf("size of boot archive \"%s\": %lu\n", fileName, fileSize);

	// allocate memory for the data
	uint8* fileData = NULL;
	if (platform_allocate_region((void**)&fileData, fileSize,
			B_READ_AREA | B_WRITE_AREA, false) != B_OK) {
		TRACE(("TFTP: allocating memory for file data failed\n"));
		return B_NO_MEMORY;
	}

	// open TFTP connection
	pxenv_tftp_open openConnection;
	openConnection.server_ip.num = htonl(fServerIP);
	openConnection.gateway_ip.num = 0;
	strlcpy(openConnection.file_name, fileName, sizeof(getFileSize.file_name));
	openConnection.port = htons(ServerPort());
	openConnection.packet_size = 1456;

	res = call_pxe_bios(fPxeData, TFTP_OPEN, &openConnection);
	if (res != 0 || openConnection.status != 0) {
		dprintf("TFTP_OPEN failed, res %x, status %x\n", res,
			openConnection.status);

		platform_free_region(fileData, fileSize);
		return B_ERROR;
	}

	uint16 packetSize = openConnection.packet_size;
	dprintf("successfully opened TFTP connection, packet size %u\n",
		packetSize);

	// check, if the file is too big for the TFTP protocol
	if (fileSize > 0xffff * (uint32)packetSize) {
		dprintf("TFTP: File is too big to be transferred via TFTP\n");
		_Close();
		platform_free_region(fileData, fileSize);
		return B_ERROR;
	}

	// transfer the file
	status_t error = B_OK;
	uint32 remainingBytes = fileSize;
	uint8* buffer = fileData;
	while (remainingBytes > 0) {
		void* scratchBuffer = (void*)0x07C00;
		pxenv_tftp_read readPacket;
		readPacket.buffer.seg = SEG(scratchBuffer);
		readPacket.buffer.ofs = OFS(scratchBuffer);

		res = call_pxe_bios(fPxeData, TFTP_READ, &readPacket);
		if (res != 0 || readPacket.status != 0) {
			dprintf("TFTP_READ failed, res %x, status %x\n", res,
				readPacket.status);
			error = B_ERROR;
			break;
		}

		uint32 bytesRead = readPacket.buffer_size;
		if (bytesRead > remainingBytes) {
			dprintf("TFTP: Read more bytes than should be remaining!");
			error = B_ERROR;
			break;
		}

		memcpy(buffer, scratchBuffer, bytesRead);
		buffer += bytesRead;
		remainingBytes -= bytesRead;
	}

	// close TFTP connection
	_Close();

	if (error == B_OK) {
		dprintf("TFTP: Successfully received file\n");
		*data = fileData;
		*size = fileSize;
	} else {
		platform_free_region(fileData, fileSize);
	}

	return error;
}

status_t
TFTP::_Close()
{
	// close TFTP connection
	pxenv_tftp_close closeConnection;
	uint16 res = call_pxe_bios(fPxeData, TFTP_CLOSE, &closeConnection);
	if (res != 0 || closeConnection.status != 0) {
		dprintf("TFTP_CLOSE failed, res %x, status %x\n", res,
			closeConnection.status);
		return B_ERROR;
	}

	return B_OK;
}



// #pragma mark -


status_t
platform_net_stack_init()
{
	TRACE("platform_net_stack_init\n");

	UNDI *interface = new(nothrow) UNDI;
	if (!interface)
		return B_NO_MEMORY;

	status_t error = interface->Init();
	if (error != B_OK) {
		TRACE("platform_net_stack_init: interface init failed\n");
		delete interface;
		return error;
	}

	error = NetStack::Default()->AddEthernetInterface(interface);
	if (error != B_OK) {
		delete interface;
		return error;
	}

	return B_OK;
}
