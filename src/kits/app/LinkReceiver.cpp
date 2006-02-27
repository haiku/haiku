/*
 * Copyright 2001-2006, Haiku.
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
#include <LinkReceiver.h>
#include <String.h>

#include "link_message.h"


//#define DEBUG_BPORTLINK
#ifdef DEBUG_BPORTLINK
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

namespace BPrivate {

LinkReceiver::LinkReceiver(port_id port)
	:
	fReceivePort(port), fRecvBuffer(NULL), fRecvPosition(0), fRecvStart(0),
	fRecvBufferSize(0), fDataSize(0),
	fReplySize(0), fReadError(B_OK)
{
}


LinkReceiver::~LinkReceiver()
{
	free(fRecvBuffer);
}


void
LinkReceiver::SetPort(port_id port)
{
	fReceivePort = port;
}


status_t
LinkReceiver::GetNextMessage(int32 &code, bigtime_t timeout)
{
	int32 remaining;

	fReadError = B_OK;

	remaining = fDataSize - (fRecvStart + fReplySize);
	STRACE(("info: LinkReceiver GetNextReply() reports %ld bytes remaining in buffer.\n", remaining));

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
		STRACE(("error info: LinkReceiver remaining %ld bytes is less than header size.\n", remaining));
		ResetBuffer();
		return B_ERROR;
	}

	fReplySize = header->size;
	if (fReplySize > remaining || fReplySize < (int32)sizeof(message_header)) {
		STRACE(("error info: LinkReceiver message size of %ld bytes smaller than header size.\n", fReplySize));
		ResetBuffer();
		return B_ERROR;
	}

	code = header->code;
	fRecvPosition += sizeof(message_header);

	STRACE(("info: LinkReceiver got header %ld [%ld %ld %ld] from port %ld.\n",
		header->code, fReplySize, header->code, header->flags, fReceivePort));

	return B_OK;
}


bool
LinkReceiver::HasMessages() const
{
	return fDataSize - (fRecvStart + fReplySize) > 0
		|| port_count(fReceivePort) > 0;
}


bool
LinkReceiver::NeedsReply() const
{
	if (fReplySize == 0)
		return false;

	message_header *header = (message_header *)(fRecvBuffer + fRecvStart);
	return (header->flags & kNeedsReply) != 0;
}


int32
LinkReceiver::Code() const
{
	if (fReplySize == 0)
		return B_ERROR;

	message_header *header = (message_header *)(fRecvBuffer + fRecvStart);
	return header->code;
}


void
LinkReceiver::ResetBuffer()
{
	fRecvPosition = 0;
	fRecvStart = 0;
	fDataSize = 0;
	fReplySize = 0;
}


status_t
LinkReceiver::AdjustReplyBuffer(bigtime_t timeout)
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
		STRACE(("info: LinkReceiver getting port_buffer_size().\n"));
		ssize_t bufferSize;
		if (timeout == B_INFINITE_TIMEOUT)
			bufferSize = port_buffer_size(fReceivePort);
		else
			bufferSize = port_buffer_size_etc(fReceivePort, B_TIMEOUT, timeout);
		STRACE(("info: LinkReceiver got port_buffer_size() = %ld.\n", bufferSize));

		if (bufferSize < 0)
			return (status_t)bufferSize;

		// make sure our receive buffer is large enough
		if (bufferSize > fRecvBufferSize) {
			if (bufferSize <= (ssize_t)kInitialBufferSize)
				bufferSize = (ssize_t)kInitialBufferSize;
			else
				bufferSize = (bufferSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

			if (bufferSize > (ssize_t)kMaxBufferSize)
				return B_ERROR;	// we can't continue

			STRACE(("info: LinkReceiver setting receive buffersize to %ld.\n", bufferSize));
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
LinkReceiver::ReadFromPort(bigtime_t timeout)
{
	// we are here so it means we finished reading the buffer contents
	ResetBuffer();

	status_t err = AdjustReplyBuffer(timeout);
	if (err < B_OK)
		return err;

	int32 code;
	ssize_t bytesRead;

	STRACE(("info: LinkReceiver reading port %ld.\n", fReceivePort));
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

		STRACE(("info: LinkReceiver read %ld bytes.\n", bytesRead));
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
LinkReceiver::Read(void *data, ssize_t size)
{
//	STRACE(("info: LinkReceiver Read()ing %ld bytes...\n", size));
	if (fReadError < B_OK)
		return fReadError;

	if (data == NULL || size < 1) {
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
LinkReceiver::ReadString(char** _string, size_t* _length)
{
	int32 length = 0;
	status_t status;

	status = Read<int32>(&length);
	if (status < B_OK)
		return status;

	char *string;

	if (length < 0) {
		status = B_ERROR;
		goto err;
	}

	string = (char *)malloc(length + 1);
	if (string == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	if (length > 0) {
		status = Read(string, length);
		if (status < B_OK) {
			free(string);
			return status;
		}
	}

	// make sure the string is null terminated
	string[length] = '\0';

	if (_length)
		*_length = length;
	*_string = string;

	return B_OK;

err:
	fRecvPosition -= sizeof(int32);
		// rewind the transaction
	return status;
}


status_t
LinkReceiver::ReadString(BString &string, size_t* _length)
{
	int32 length = 0;
	status_t status;

	status = Read<int32>(&length);
	if (status < B_OK)
		return status;

	if (length < 0) {
		status = B_ERROR;
		goto err;
	}

	if (length > 0) {
		char* buffer = string.LockBuffer(length + 1);
		if (buffer == NULL) {
			status = B_NO_MEMORY;
			goto err;
		}

		status = Read(buffer, length);
		if (status < B_OK) {
			string.UnlockBuffer();
			goto err;
		}

		// make sure the string is null terminated
		buffer[length] = '\0';
		string.UnlockBuffer(length);
	} else
		string = "";

	if (_length)
		*_length = length;

	return B_OK;

err:
	fRecvPosition -= sizeof(int32);
		// rewind the transaction
	return status;
}


status_t
LinkReceiver::ReadString(char *buffer, size_t bufferLength)
{
	int32 length = 0;
	status_t status;

	status = Read<int32>(&length);
	if (status < B_OK)
		return status;

	if (length >= (int32)bufferLength) {
		status = B_BUFFER_OVERFLOW;
		goto err;
	}

	if (length < 0) {
		status = B_ERROR;
		goto err;
	}

	if (length > 0) {
		status = Read(buffer, length);
		if (status < B_OK)
			goto err;
	}

	// make sure the string is null terminated
	buffer[length] = '\0';
	return B_OK;

err:
	fRecvPosition -= sizeof(int32);
		// rewind the transaction
	return status;
}

}	// namespace BPrivate
