/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler
 */

/** Class for low-overhead port-based messaging */

#include <stdlib.h>
#include <string.h>
#include <new>

#include <ServerProtocol.h>
#include <LinkMsgReader.h>

#include "link_message.h"


//#define DEBUG_BPORTLINK
#ifdef DEBUG_BPORTLINK
#	include <stdio.h>
#	define STRACE(x) printf x
// those are defined in LinkMsgSender.cpp
extern const char *strcode(int32 code);
extern const char *bstrcode(int32 code);
#else
#	define STRACE(x) ;
#endif

static const int32 kInitialReceiveBufferSize = 2048;
static const int32 kMaxReceiveBufferSize = 2048;


LinkMsgReader::LinkMsgReader(port_id port)
	:
	fReceivePort(port), fRecvBuffer(NULL), fRecvPosition(0), fRecvStart(0),
	fRecvBufferSize(0), fDataSize(0),
	fReplySize(0), fReadError(B_OK)
{
}


LinkMsgReader::~LinkMsgReader()
{
	free(fRecvBuffer);
}


void
LinkMsgReader::SetPort(port_id port)
{
	fReceivePort = port;
}


port_id
LinkMsgReader::GetPort()
{
	return fReceivePort;
}


status_t
LinkMsgReader::GetNextMessage(int32 *code, bigtime_t timeout)
{
	int32 remaining;

	fReadError = B_OK;

	remaining = fDataSize - (fRecvStart + fReplySize);
	STRACE(("info: LinkMsgReader GetNextReply() reports %ld bytes remaining in buffer.\n", remaining));

	// find the position of the next message header in the buffer
	message_header *header;
	if (remaining <= 0) {
		status_t err = ReadFromPort(timeout);
		if (err < B_OK)
			return err;
		remaining = fDataSize;
		header = (message_header *)fRecvBuffer;
	} else {
		fRecvStart += fReplySize;	//start of the next message
		fRecvPosition = fRecvStart;
		header = (message_header *)(fRecvBuffer + fRecvStart);
	}

	// check we have a well-formed message
	if (remaining < (int32)sizeof(message_header)) {
		// we don't have enough data for a complete header
		STRACE(("error info: LinkMsgReader remaining %ld bytes is less than header size.\n", remaining));
		ResetBuffer();
		return B_ERROR;
	}

	fReplySize = header->size;
	if (fReplySize > remaining || fReplySize < (int32)sizeof(message_header)) {
		STRACE(("error info: LinkMsgReader message size of %ld bytes smaller than header size.\n", fReplySize));
		ResetBuffer();
		return B_ERROR;
	}

	*code = header->code;
	fRecvPosition += sizeof(message_header);

	STRACE(("info: LinkMsgReader got header %s [%ld %ld %ld] from port %ld.\n",
		strcode(header->code), fReplySize, header->code, header->flags, fReceivePort));

	return B_OK;
}


void
LinkMsgReader::ResetBuffer()
{
	fRecvPosition = 0;
	fRecvStart = 0;
	fDataSize = 0;
	fReplySize = 0;
}


status_t
LinkMsgReader::AdjustReplyBuffer(bigtime_t timeout)
{
	// Here we take advantage of the compiler's dead-code elimination
	if (kInitialReceiveBufferSize == kMaxReceiveBufferSize) {
		// fixed buffer size

		if (fRecvBuffer != NULL)
			return B_OK;

		fRecvBuffer = (char *)malloc(kInitialReceiveBufferSize);
		if (fRecvBuffer == NULL)
			return B_NO_MEMORY;
		fRecvBufferSize = kInitialReceiveBufferSize;
	} else {
		STRACE(("info: LinkMsgReader getting port_buffer_size().\n"));
		ssize_t buffersize;
		if (timeout == B_INFINITE_TIMEOUT)
			buffersize = port_buffer_size(fReceivePort);
		else
			buffersize = port_buffer_size_etc(fReceivePort, B_TIMEOUT, timeout);
		STRACE(("info: LinkMsgReader got port_buffer_size() = %ld.\n", buffersize));

		if (buffersize < 0)
			return (status_t)buffersize;

		// make sure our receive buffer is large enough
		if (buffersize > fRecvBufferSize) {
			if (buffersize <= kInitialReceiveBufferSize)
				buffersize = kInitialReceiveBufferSize;
			else
				buffersize = (buffersize + B_PAGE_SIZE) - (buffersize % B_PAGE_SIZE);
			if (buffersize > kMaxReceiveBufferSize)
				return B_ERROR;	//we can't continue

			STRACE(("info: LinkMsgReader setting receive buffersize to %ld.\n", buffersize));
			char *buffer = (char *)malloc(buffersize);
			if (buffer == NULL)
				return B_NO_MEMORY;

			free(fRecvBuffer);
			fRecvBuffer = buffer;
			fRecvBufferSize = buffersize;
		}
	}

	return B_OK;
}


status_t
LinkMsgReader::ReadFromPort(bigtime_t timeout)
{
	// we are here so it means we finished reading the buffer contents
	ResetBuffer();

	status_t err = AdjustReplyBuffer(timeout);
	if (err < B_OK)
		return err;

	int32 protocol;
	ssize_t bytesread;	
	STRACE(("info: LinkMsgReader reading port %ld.\n", fReceivePort));
	if (timeout != B_INFINITE_TIMEOUT) {
		do {
			bytesread = read_port_etc(fReceivePort, &protocol, fRecvBuffer,
				fRecvBufferSize, B_TIMEOUT, timeout);
		} while(bytesread == B_INTERRUPTED);
	} else {
		do {
			bytesread = read_port(fReceivePort, &protocol, fRecvBuffer, 
				fRecvBufferSize);
		} while(bytesread == B_INTERRUPTED);
	}

	STRACE(("info: LinkMsgReader read %ld bytes.\n", bytesread));
	if (bytesread < B_OK)
		return bytesread;

	// TODO: we only need AS_SERVER_PORTLINK when all OBOS uses Link messages
	if (protocol != AS_SERVER_PORTLINK && protocol != AS_SERVER_SESSION)
		return B_ERROR;
	if (protocol == AS_SERVER_PORTLINK && bytesread != *((int32 *)fRecvBuffer))
		// should only be one message for PORTLINK so the size declared in the header
		// (the first int32 in the header) should be the same as bytesread
		return B_ERROR;

	fDataSize = bytesread;
	return B_OK;
}


status_t
LinkMsgReader::Read(void *data, ssize_t size)
{
//	STRACE(("info: LinkMsgReader Read()ing %ld bytes...\n", size));
	if (fReadError < B_OK)
		return fReadError;

	if (size < 1) {
		fReadError = B_BAD_VALUE;
		return B_BAD_VALUE;
	}

	if (fDataSize == 0 || fReplySize == 0)
		return B_NO_INIT;	// need to call GetNextReply() first

	if (fRecvPosition + size > fRecvStart + fReplySize) {
		// reading past the end of current message
		fReadError = B_BAD_VALUE;
		return B_BAD_VALUE;
	}

	memcpy(data, fRecvBuffer + fRecvPosition, size);
	fRecvPosition += size;
	return fReadError;
}


status_t
LinkMsgReader::ReadString(char **string)
{
	status_t err;
	int32 len = 0;

	err = Read<int32>(&len);
	if (err < B_OK)
		return err;

	if (len) {
		*string = (char *)malloc(len);
		if (*string == NULL) {
			fRecvPosition -= sizeof(int32);	//rewind the transaction
			return B_NO_MEMORY;
		}

		err = Read(*string, len);
		if (err < B_OK) {
			free(*string);
			*string = NULL;
			fRecvPosition -= sizeof(int32);	//rewind the transaction
			return err;
		}
		(*string)[len-1] = '\0';
		return B_OK;
	} else {
		fRecvPosition -= sizeof(int32);	//rewind the transaction
		return B_ERROR;
	}
}

