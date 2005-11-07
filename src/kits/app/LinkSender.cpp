/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/** Class for low-overhead port-based messaging */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include <ServerProtocol.h>
#include <LinkSender.h>

#include "link_message.h"


//#define DEBUG_BPORTLINK
#ifdef DEBUG_BPORTLINK
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

static const size_t kWatermark = kInitialBufferSize - 24;
	// if a message is started after this mark, the buffer is flushed automatically

namespace BPrivate {

LinkSender::LinkSender(port_id port)
	:
	fPort(port),
	fBuffer(NULL),
	fBufferSize(0),

	fCurrentEnd(0),
	fCurrentStart(0),
	fCurrentStatus(B_OK)
{
}


LinkSender::~LinkSender()
{
	free(fBuffer);
}


void
LinkSender::SetPort(port_id port)
{
	fPort = port;
}


status_t
LinkSender::StartMessage(int32 code, size_t minSize)
{
	// end previous message
	if (EndMessage() < B_OK)	
		CancelMessage();

	if (minSize > kMaxBufferSize - sizeof(message_header))
		return fCurrentStatus = B_BUFFER_OVERFLOW;

	minSize += sizeof(message_header);

	// Eventually flush buffer to make space for the new message.
	// Note, we do not take the actual buffer size into account to not
	// delay the time between buffer flushes too much.
	if (fBufferSize > 0 && (minSize > SpaceLeft() || fCurrentStart >= kWatermark)) {
		status_t status = Flush();
		if (status < B_OK)
			return status;
	}

	if (minSize > fBufferSize) {
		if (AdjustBuffer(minSize) != B_OK)
			return fCurrentStatus = B_NO_MEMORY;
	}

	message_header *header = (message_header *)(fBuffer + fCurrentStart);
	header->size = 0;
		// will be set later
	header->code = code;
	header->flags = 0;

	STRACE(("info: LinkSender buffered header %ld (%lx) [%lu %lu %lu].\n",
		code, code, header->size, header->code, header->flags));

	fCurrentEnd += sizeof(message_header);
	return B_OK;
}


status_t
LinkSender::EndMessage(bool needsReply)
{
	if (fCurrentEnd == fCurrentStart || fCurrentStatus < B_OK)
		return fCurrentStatus;

	// record the size of the message
	message_header *header = (message_header *)(fBuffer + fCurrentStart);
	header->size = CurrentMessageSize();
	if (needsReply)
		header->flags |= needsReply;

	STRACE(("info: LinkSender EndMessage() of size %ld.\n", header->size));

	// bump to start of next message
	fCurrentStart = fCurrentEnd;	
	return B_OK;
}


void
LinkSender::CancelMessage()
{
	fCurrentEnd = fCurrentStart;
	fCurrentStatus = B_OK;
}


status_t
LinkSender::Attach(const void *data, size_t size)
{
	if (fCurrentStatus < B_OK)
		return fCurrentStatus;

	if (size == 0)
		return fCurrentStatus = B_BAD_VALUE;

	if (fCurrentEnd == fCurrentStart)
		return B_NO_INIT;	// need to call StartMessage() first

	if (SpaceLeft() < size) {
		// we have to make space for the data

		status_t status = FlushCompleted(size + CurrentMessageSize());
		if (status < B_OK)
			return fCurrentStatus = status;
	}

	memcpy(fBuffer + fCurrentEnd, data, size);
	fCurrentEnd += size;

	return B_OK;
}


status_t
LinkSender::AttachString(const char *string, int32 length)
{
	if (string == NULL)
		string = "";

	if (length == -1)
		length = strlen(string);

	status_t status = Attach<int32>(length);
	if (status < B_OK)
		return status;

	if (length > 0) {
		status = Attach(string, length);
		if (status < B_OK)
			fCurrentEnd -= sizeof(int32);	// rewind the transaction
	}

	return status;
}


status_t
LinkSender::AdjustBuffer(size_t newSize, char **_oldBuffer)
{
	// make sure the new size is within bounds
	if (newSize <= kInitialBufferSize)
		newSize = kInitialBufferSize;
	else if (newSize > kMaxBufferSize)
		return B_BUFFER_OVERFLOW;
	else if (newSize > kInitialBufferSize)
		newSize = (newSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	char *buffer = NULL;
	if (newSize == fBufferSize) {
		// keep existing buffer
		if (_oldBuffer)
			*_oldBuffer = fBuffer;
		return B_OK;
	}

	// create new larger buffer
	buffer = (char *)malloc(newSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if (_oldBuffer)
		*_oldBuffer = fBuffer;
	else
		free(fBuffer);

	fBuffer = buffer;
	fBufferSize = newSize;
	return B_OK;
}


status_t
LinkSender::FlushCompleted(size_t newBufferSize)
{
	// we need to hide the incomplete message so that it's not flushed
	int32 end = fCurrentEnd;
	int32 start = fCurrentStart;
	fCurrentEnd = fCurrentStart;

	status_t status = Flush();
	if (status < B_OK) {
		fCurrentEnd = end;
		return status;
	}

	char *oldBuffer = NULL;
	status = AdjustBuffer(newBufferSize, &oldBuffer);
	if (status != B_OK)
		return status;

	// move the incomplete message to the start of the buffer
	fCurrentEnd = end - start;
	if (oldBuffer != fBuffer) {
		memcpy(fBuffer, oldBuffer + start, fCurrentEnd);
		free(oldBuffer);
	} else
		memmove(fBuffer, fBuffer + start, fCurrentEnd);

	return B_OK;
}


status_t
LinkSender::Flush(bigtime_t timeout, bool needsReply)
{
	if (fCurrentStatus < B_OK)
		return fCurrentStatus;

	EndMessage(needsReply);
	if (fCurrentStart == 0)
		return B_OK;

	STRACE(("info: LinkSender Flush() waiting to send messages of %ld bytes on port %ld.\n",
		fCurrentEnd, fPort));

	status_t err;
	if (timeout != B_INFINITE_TIMEOUT) {
		do {
			err = write_port_etc(fPort, kLinkCode, fBuffer,
				fCurrentEnd, B_RELATIVE_TIMEOUT, timeout);
		} while (err == B_INTERRUPTED);
	} else {
		do {
			err = write_port(fPort, kLinkCode, fBuffer, fCurrentEnd);
		} while (err == B_INTERRUPTED);
	}

	if (err < B_OK) {
		STRACE(("error info: LinkSender Flush() failed for %ld bytes (%s) on port %ld.\n",
			fCurrentEnd, strerror(err), fPort));
		return err;
	}

	STRACE(("info: LinkSender Flush() messages total of %ld bytes on port %ld.\n",
		fCurrentEnd, fPort));

	fCurrentEnd = 0;
	fCurrentStart = 0;

	return B_OK;
}

}	// namespace BPrivate
