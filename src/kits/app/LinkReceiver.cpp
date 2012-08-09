/*
 * Copyright 2001-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Artur Wyszynski <harakash@gmail.com>
 */


/*! Class for low-overhead port-based messaging */


#include <LinkReceiver.h>

#include <stdlib.h>
#include <string.h>
#include <new>

#include <ServerProtocol.h>
#include <String.h>
#include <Region.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>

#include "link_message.h"

//#define DEBUG_BPORTLINK
#ifdef DEBUG_BPORTLINK
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

//#define TRACE_LINK_RECEIVER_GRADIENTS
#ifdef TRACE_LINK_RECEIVER_GRADIENTS
#	include <OS.h>
#	define GTRACE(x) debug_printf x
#else
#	define GTRACE(x) ;
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
	fReadError = B_OK;

	int32 remaining = fDataSize - (fRecvStart + fReplySize);
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
		do {
			bufferSize = port_buffer_size_etc(fReceivePort,
				timeout == B_INFINITE_TIMEOUT ? B_RELATIVE_TIMEOUT : 0,
				timeout);
		} while (bufferSize == B_INTERRUPTED);

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
LinkReceiver::Read(void *data, ssize_t passedSize)
{
//	STRACE(("info: LinkReceiver Read()ing %ld bytes...\n", size));
	ssize_t size = passedSize;

	if (fReadError < B_OK)
		return fReadError;

	if (data == NULL || size < 1) {
		fReadError = B_BAD_VALUE;
		return B_BAD_VALUE;
	}

	if (fDataSize == 0 || fReplySize == 0)
		return B_NO_INIT;	// need to call GetNextReply() first

	bool useArea = false;
	if ((size_t)size >= kMaxBufferSize) {
		useArea = true;
		size = sizeof(area_id);
	}

	if (fRecvPosition + size > fRecvStart + fReplySize) {
		// reading past the end of current message
		fReadError = B_BAD_VALUE;
		return B_BAD_VALUE;
	}

	if (useArea) {
		area_id sourceArea;
		memcpy((void*)&sourceArea, fRecvBuffer + fRecvPosition, size);

		area_info areaInfo;
		if (get_area_info(sourceArea, &areaInfo) < B_OK)
			fReadError = B_BAD_VALUE;

		if (fReadError >= B_OK) {
			void* areaAddress = areaInfo.address;

			if (areaAddress && sourceArea >= B_OK) {
				memcpy(data, areaAddress, passedSize);
				delete_area(sourceArea);
			}
		}
	} else {
		memcpy(data, fRecvBuffer + fRecvPosition, size);
	}
	fRecvPosition += size;
	return fReadError;
}


status_t
LinkReceiver::ReadString(char** _string, size_t* _length)
{
	int32 length = 0;
	status_t status = Read<int32>(&length);

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
	status_t status = Read<int32>(&length);

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
	status_t status = Read<int32>(&length);

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

status_t
LinkReceiver::ReadRegion(BRegion* region)
{
	status_t status = Read(&region->fCount, sizeof(long));
	if (status >= B_OK)
		status = Read(&region->fBounds, sizeof(clipping_rect));
	if (status >= B_OK) {
		if (!region->_SetSize(region->fCount))
			status = B_NO_MEMORY;
		else {
			status = Read(region->fData,
				region->fCount * sizeof(clipping_rect));
		}
		if (status < B_OK)
			region->MakeEmpty();
	}
	return status;
}


static BGradient*
gradient_for_type(BGradient::Type type)
{
	switch (type) {
		case BGradient::TYPE_LINEAR:
			return new (std::nothrow) BGradientLinear();
		case BGradient::TYPE_RADIAL:
			return new (std::nothrow) BGradientRadial();
		case BGradient::TYPE_RADIAL_FOCUS:
			return new (std::nothrow) BGradientRadialFocus();
		case BGradient::TYPE_DIAMOND:
			return new (std::nothrow) BGradientDiamond();
		case BGradient::TYPE_CONIC:
			return new (std::nothrow) BGradientConic();
		case BGradient::TYPE_NONE:
			return new (std::nothrow) BGradient();
	}
	return NULL;
}


status_t
LinkReceiver::ReadGradient(BGradient** _gradient)
{
	GTRACE(("LinkReceiver::ReadGradient\n"));

	BGradient::Type gradientType;
	int32 colorsCount;
	Read(&gradientType, sizeof(BGradient::Type));
	status_t status = Read(&colorsCount, sizeof(int32));
	if (status != B_OK)
		return status;

	BGradient* gradient = gradient_for_type(gradientType);
	if (!gradient)
		return B_NO_MEMORY;

	*_gradient = gradient;

	if (colorsCount > 0) {
		BGradient::ColorStop stop;
		for (int i = 0; i < colorsCount; i++) {
			if ((status = Read(&stop, sizeof(BGradient::ColorStop))) != B_OK)
				return status;
			if (!gradient->AddColorStop(stop, i))
				return B_NO_MEMORY;
		}
	}

	switch (gradientType) {
		case BGradient::TYPE_LINEAR:
		{
			GTRACE(("LinkReceiver::ReadGradient> type == TYPE_LINEAR\n"));
			BGradientLinear* linear = (BGradientLinear*)gradient;
			BPoint start;
			BPoint end;
			Read(&start, sizeof(BPoint));
			if ((status = Read(&end, sizeof(BPoint))) != B_OK)
				return status;
			linear->SetStart(start);
			linear->SetEnd(end);
			return B_OK;
		}
		case BGradient::TYPE_RADIAL:
		{
			GTRACE(("LinkReceiver::ReadGradient> type == TYPE_RADIAL\n"));
			BGradientRadial* radial = (BGradientRadial*)gradient;
			BPoint center;
			float radius;
			Read(&center, sizeof(BPoint));
			if ((status = Read(&radius, sizeof(float))) != B_OK)
				return status;
			radial->SetCenter(center);
			radial->SetRadius(radius);
			return B_OK;
		}
		case BGradient::TYPE_RADIAL_FOCUS:
		{
			GTRACE(("LinkReceiver::ReadGradient> type == TYPE_RADIAL_FOCUS\n"));
			BGradientRadialFocus* radialFocus =
				(BGradientRadialFocus*)gradient;
			BPoint center;
			BPoint focal;
			float radius;
			Read(&center, sizeof(BPoint));
			Read(&focal, sizeof(BPoint));
			if ((status = Read(&radius, sizeof(float))) != B_OK)
				return status;
			radialFocus->SetCenter(center);
			radialFocus->SetFocal(focal);
			radialFocus->SetRadius(radius);
			return B_OK;
		}
		case BGradient::TYPE_DIAMOND:
		{
			GTRACE(("LinkReceiver::ReadGradient> type == TYPE_DIAMOND\n"));
			BGradientDiamond* diamond = (BGradientDiamond*)gradient;
			BPoint center;
			if ((status = Read(&center, sizeof(BPoint))) != B_OK)
				return status;
			diamond->SetCenter(center);
			return B_OK;
		}
		case BGradient::TYPE_CONIC:
		{
			GTRACE(("LinkReceiver::ReadGradient> type == TYPE_CONIC\n"));
			BGradientConic* conic = (BGradientConic*)gradient;
			BPoint center;
			float angle;
			Read(&center, sizeof(BPoint));
			if ((status = Read(&angle, sizeof(float))) != B_OK)
				return status;
			conic->SetCenter(center);
			conic->SetAngle(angle);
			return B_OK;
		}
		case BGradient::TYPE_NONE:
		{
			GTRACE(("LinkReceiver::ReadGradient> type == TYPE_NONE\n"));
			break;
		}
	}

	return B_ERROR;
}


}	// namespace BPrivate
