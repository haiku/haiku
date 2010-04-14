/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <ring_buffer.h>

#include "Driver.h"
#include "HIDDevice.h"
#include "HIDReport.h"
#include "ProtocolHandler.h"

// includes for the different protocol handlers
#include "KeyboardDevice.h"
#include "MouseDevice.h"
//#include "GenericDevice.h"


ProtocolHandler::ProtocolHandler(HIDDevice *device, const char *basePath,
	size_t ringBufferSize)
	:	fStatus(B_NO_INIT),
		fDevice(device),
		fBasePath(basePath),
		fPublishPath(NULL),
		fRingBuffer(NULL)
{
	if (ringBufferSize > 0) {
		fRingBuffer = create_ring_buffer(ringBufferSize);
		if (fRingBuffer == NULL) {
			TRACE_ALWAYS("failed to create requested ring buffer\n");
			fStatus = B_NO_MEMORY;
			return;
		}
	}

	fStatus = B_OK;
}


ProtocolHandler::~ProtocolHandler()
{
	if (fRingBuffer) {
		delete_ring_buffer(fRingBuffer);
		fRingBuffer = NULL;
	}

	free(fPublishPath);
}


void
ProtocolHandler::SetPublishPath(char *publishPath)
{
	free(fPublishPath);
	fPublishPath = publishPath;
}


void
ProtocolHandler::AddHandlers(HIDDevice *device, ProtocolHandler ***handlerList,
	uint32 *handlerCount)
{
	TRACE("adding protocol handlers\n");

	HIDParser *parser = device->Parser();
	uint32 reportCount = parser->CountReports(HID_REPORT_TYPE_INPUT);

	*handlerCount = 0;
	*handlerList = (ProtocolHandler **)malloc(sizeof(ProtocolHandler *)
		* reportCount * 2 /* handler type count */);
	if (*handlerList == NULL) {
		TRACE_ALWAYS("out of memory allocating handler list\n");
		return;
	}

	uint32 usedCount = 0;
	ProtocolHandler *handler = NULL;
	for (uint32  i = 0; i < reportCount; i++) {
		HIDReport *report = parser->ReportAt(HID_REPORT_TYPE_INPUT, i);

		handler = KeyboardDevice::AddHandler(device, report);
		if (handler != NULL)
			(*handlerList)[usedCount++] = handler;
		handler = MouseDevice::AddHandler(device, report);
		if (handler != NULL)
			(*handlerList)[usedCount++] = handler;
	}

	if (usedCount == 0) {
		TRACE_ALWAYS("no handlers for hid device\n");
		return;
	}

	*handlerCount = usedCount;
	ProtocolHandler **shrunkList = (ProtocolHandler **)realloc(
		*handlerList, sizeof(ProtocolHandler *) * usedCount);
	if (shrunkList != NULL)
		*handlerList = shrunkList;

	TRACE("added %ld handlers for hid device\n", *handlerCount);
}


status_t
ProtocolHandler::Open(uint32 flags, uint32 *cookie)
{
	return fDevice->Open(this, flags);
}


status_t
ProtocolHandler::Close()
{
	return fDevice->Close(this);
}


status_t
ProtocolHandler::Control(uint32 *cookie, uint32 op, void *buffer, size_t length)
{
	TRACE_ALWAYS("control on base class\n");
	return B_ERROR;
}


int32
ProtocolHandler::RingBufferReadable()
{
	return ring_buffer_readable(fRingBuffer);
}


status_t
ProtocolHandler::RingBufferRead(void *buffer, size_t length)
{
	ring_buffer_user_read(fRingBuffer, (uint8 *)buffer, length);
	return B_OK;
}


status_t
ProtocolHandler::RingBufferWrite(const void *buffer, size_t length)
{
	ring_buffer_write(fRingBuffer, (const uint8 *)buffer, length);
	return B_OK;
}
