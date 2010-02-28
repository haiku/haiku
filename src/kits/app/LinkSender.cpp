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
#include "syscalls.h"

//#define DEBUG_BPORTLINK
#ifdef DEBUG_BPORTLINK
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

static const size_t kMaxStringSize = 4096;
static const size_t kWatermark = kInitialBufferSize - 24;
	// if a message is started after this mark, the buffer is flushed automatically

namespace BPrivate {

LinkSender::LinkSender(port_id port)
	:
	fPort(port),
	fTargetTeam(-1),
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

	if (minSize > kMaxBufferSize - sizeof(message_header)) {
		// we will handle this case in Attach, using an area
		minSize = sizeof(area_id);
	}

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
LinkSender::Attach(const void *passedData, size_t passedSize)
{
	size_t size = passedSize;
	const void* data = passedData;

	if (fCurrentStatus < B_OK)
		return fCurrentStatus;

	if (size == 0)
		return fCurrentStatus = B_BAD_VALUE;

	if (fCurrentEnd == fCurrentStart)
		return B_NO_INIT;	// need to call StartMessage() first

	bool useArea = false;
	if (size >= kMaxBufferSize) {
		useArea = true;
		size = sizeof(area_id);
	}

	if (SpaceLeft() < size) {
		// we have to make space for the data

		status_t status = FlushCompleted(size + CurrentMessageSize());
		if (status < B_OK)
			return fCurrentStatus = status;
	}

	area_id senderArea = -1;
	if (useArea) {
		if (fTargetTeam < 0) {
			port_info info;
			status_t result = get_port_info(fPort, &info);
			if (result != B_OK)
				return result;
			fTargetTeam = info.team;
		}
		void* address = NULL;
		off_t alignedSize = (passedSize + B_PAGE_SIZE) & ~(B_PAGE_SIZE - 1);
		senderArea = create_area("LinkSenderArea", &address, B_ANY_ADDRESS,
			alignedSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

		if (senderArea < B_OK)
			return senderArea;
			
		data = &senderArea;
		memcpy(address, passedData, passedSize);

		area_id areaID = senderArea;
		senderArea = _kern_transfer_area(senderArea, &address,
			B_ANY_ADDRESS, fTargetTeam);
		
		if (senderArea < B_OK) {
			delete_area(areaID);
			return senderArea;
		}
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

	size_t maxLength = strlen(string);
	if (length == -1) {
		length = (int32)maxLength;

		// we should report an error here
		if (maxLength > kMaxStringSize)
			length = 0;
	} else if (length > (int32)maxLength)
		length = maxLength;

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

	if (newSize == fBufferSize) {
		// keep existing buffer
		if (_oldBuffer)
			*_oldBuffer = fBuffer;
		return B_OK;
	}

	// create new larger buffer
	char *buffer = (char *)malloc(newSize);
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
