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


status_t
LinkMsgReader::GetNextMessage(int32 &code, bigtime_t timeout)
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
		fRecvStart += fReplySize;	// start of the next message
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

	code = header->code;
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
	if (kInitialBufferSize == kMaxBufferSize) {
		// fixed buffer size

		if (fRecvBuffer != NULL)
			return B_OK;

		fRecvBuffer = (char *)malloc(kInitialBufferSize);
		if (fRecvBuffer == NULL)
			return B_NO_MEMORY;

		fRecvBufferSize = kInitialBufferSize;
	} else {
		STRACE(("info: LinkMsgReader getting port_buffer_size().\n"));
		ssize_t bufferSize;
		if (timeout == B_INFINITE_TIMEOUT)
			bufferSize = port_buffer_size(fReceivePort);
		else
			bufferSize = port_buffer_size_etc(fReceivePort, B_TIMEOUT, timeout);
		STRACE(("info: LinkMsgReader got port_buffer_size() = %ld.\n", bufferSize));

		if (bufferSize < 0)
			return (status_t)bufferSize;

		// make sure our receive buffer is large enough
		if (bufferSize > fRecvBufferSize) {
			if (bufferSize <= (ssize_t)kInitialBufferSize)
				bufferSize = (ssize_t)kInitialBufferSize;
			else
				bufferSize = (bufferSize + B_PAGE_SIZE) - (bufferSize % B_PAGE_SIZE);
			if (bufferSize > (ssize_t)kMaxBufferSize)
				return B_ERROR;	// we can't continue

			STRACE(("info: LinkMsgReader setting receive buffersize to %ld.\n", bufferSize));
			char *buffer = (char *)malloc(bufferSize);
			if (buffer == NULL)
				return B_NO_MEMORY;

			free(fRecvBuffer);
			fRecvBuffer = buffer;
			fRecvBufferSize = bufferSize;
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

	int32 code;
	ssize_t bytesRead;

	STRACE(("info: LinkMsgReader reading port %ld.\n", fReceivePort));
	while (true) {
		if (timeout != B_INFINITE_TIMEOUT) {
			do {
				bytesRead = read_port_etc(fReceivePort, &code, fRecvBuffer,
					fRecvBufferSize, B_TIMEOUT, timeout);
			} while (bytesRead == B_INTERRUPTED);
		} else {
			do {
				bytesRead = read_port(fReceivePort, &code, fRecvBuffer, 
					fRecvBufferSize);
			} while (bytesRead == B_INTERRUPTED);
		}

		STRACE(("info: LinkMsgReader read %ld bytes.\n", bytesRead));
		if (bytesRead < B_OK)
			return bytesRead;

		// we just ignore incorrect messages, and don't bother our caller

		if (code != kLinkCode) {
			STRACE(("wrong port message %lx received.\n", code));
			continue;
		}

		// port read seems to be valid
		break;
	}

	fDataSize = bytesRead;
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
LinkMsgReader::ReadString(char **_string)
{
	int32 length = 0;
	status_t status;

	status = Read<int32>(&length);
	if (status < B_OK)
		return status;

	if (length >= 0) {
		char *string = (char *)malloc(length + 1);
		if (string == NULL) {
			fRecvPosition -= sizeof(int32);	// rewind the transaction
			return B_NO_MEMORY;
		}

		status = Read(string, length);
		if (status < B_OK) {
			free(string);
			fRecvPosition -= sizeof(int32);	// rewind the transaction
			return status;
		}

		// make sure the string is null terminated
		string[length] = '\0';

		*_string = string;
		return B_OK;
	} else {
		fRecvPosition -= sizeof(int32);	// rewind the transaction
		return B_ERROR;
	}
}

