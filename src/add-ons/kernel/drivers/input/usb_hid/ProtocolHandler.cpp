/*
 * Copyright 2009-2011, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <ring_buffer.h>

#include "Driver.h"
#include "HIDCollection.h"
#include "HIDDevice.h"
#include "HIDReport.h"
#include "ProtocolHandler.h"

// includes for the different protocol handlers
#include "KeyboardProtocolHandler.h"
#include "MouseProtocolHandler.h"
//#include "JoystickProtocolHandler.h"


ProtocolHandler::ProtocolHandler(HIDDevice *device, const char *basePath,
	size_t ringBufferSize)
	:	fStatus(B_NO_INIT),
		fDevice(device),
		fBasePath(basePath),
		fPublishPath(NULL),
		fRingBuffer(NULL),
		fNextHandler(NULL)
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
ProtocolHandler::AddHandlers(HIDDevice &device, ProtocolHandler *&handlerList,
	uint32 &handlerCount)
{
	TRACE("adding protocol handlers\n");

	HIDParser &parser = device.Parser();
	HIDCollection *rootCollection = parser.RootCollection();
	if (rootCollection == NULL)
		return;

	uint32 appCollectionCount = rootCollection->CountChildrenFlat(
		COLLECTION_APPLICATION);
	TRACE("root collection holds %lu application collection%s\n",
		appCollectionCount, appCollectionCount != 1 ? "s" : "");

	handlerCount = 0;
	for (uint32  i = 0; i < appCollectionCount; i++) {
		HIDCollection *collection = rootCollection->ChildAtFlat(
			COLLECTION_APPLICATION, i);
		if (collection == NULL)
			continue;

		TRACE("collection usage page %u usage id %u\n",
			collection->UsagePage(), collection->UsageID());

		KeyboardProtocolHandler::AddHandlers(device, *collection, handlerList);
		MouseProtocolHandler::AddHandlers(device, *collection, handlerList);
		//JoystickProtocolHandler::AddHandlers(device, *collection, handlerList);
	}

	ProtocolHandler *handler = handlerList;
	while (handler != NULL) {
		handler = handler->NextHandler();
		handlerCount++;
	}

	if (handlerCount == 0) {
		TRACE_ALWAYS("no handlers for hid device\n");
		return;
	}

	TRACE("added %ld handlers for hid device\n", handlerCount);
}


status_t
ProtocolHandler::Open(uint32 flags, uint32 *cookie)
{
	return fDevice->Open(this, flags);
}


status_t
ProtocolHandler::Close(uint32 *cookie)
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


void
ProtocolHandler::SetNextHandler(ProtocolHandler *nextHandler)
{
	fNextHandler = nextHandler;
}
