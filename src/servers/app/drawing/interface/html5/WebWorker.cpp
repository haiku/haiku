/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		François Revol <revol@free.fr>
 */

#include "WebHandler.h"
#include "WebWorker.h"

#include "StreamingRingBuffer.h"

#include <NetEndpoint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE(x...)			debug_printf("WebWorker: "x)
#define TRACE_ERROR(x...)	debug_printf("WebWorker: "x)


WebWorker::WebWorker(BNetEndpoint *endpoint, WebHandler *handler)
	:
	fHandler(handler),
	fWorkThread(-1),
	fStopThread(false),
	fEndpoint(endpoint)
{
	fWorkThread = spawn_thread(_WorkEntry, "html5 http worker",
		B_NORMAL_PRIORITY, this);
	resume_thread(fWorkThread);
}


WebWorker::~WebWorker()
{
	fStopThread = true;

	if (fEndpoint != NULL)
		fEndpoint->Close();

	//int32 result;
	//wait_for_thread(fWorkThread, &result);
		// TODO: find out why closing the endpoint doesn't notify the waiter

	kill_thread(fWorkThread);
}


int32
WebWorker::_WorkEntry(void *data)
{
	return ((WebWorker *)data)->_Work();
}


status_t
WebWorker::_Work()
{
	off_t pos = 0LL, contentSize = -1LL;
	int32 errorCount = 0;
	status_t result;
	TRACE("new endpoint connection: %p\n", fEndpoint);

	BDataIO *io = fHandler->fData;
	BPositionIO *pio = dynamic_cast<BPositionIO *>(io);
	StreamingRingBuffer *rb = fHandler->fTarget;

	if (pio)
		pio->GetSize(&contentSize);

	BString headers("HTTP/1.1 200 OK\r\n");
	headers << "Server: app_server(Haiku)\r\n";
	headers << "Accept-Ranges: bytes\r\n";
	headers << "Connection: ";
	if (fHandler->IsMultipart())
		headers << "close";
	else
		headers << "keep-alive";
	headers << "\r\n";
	if (fHandler->fType.Length())
		headers << "Content-Type: " << fHandler->fType << "\r\n";
	if (contentSize > 0)
		headers << "Content-Length: " << contentSize << "\r\n";
	headers << "\r\n";

	result = fEndpoint->Send(headers.String(), headers.Length());
	if (result < headers.Length()) {
		TRACE_ERROR("sending headers failed: %s\n",
			strerror(result));
		return result;
	}

	// send initial multipart boundary
	// XXX: this is messy!
	if (fHandler->IsMultipart()) {
		static const char boundary[] = "--x\r\n\r\n";
		result = fEndpoint->Send(boundary, sizeof(boundary) - 1);
		if (result < (signed)sizeof(boundary) - 1) {
			TRACE_ERROR("writing to peer failed: %s\n",
				strerror(result));
			return result;
		}
	}

	while (!fStopThread) {
		uint8 buffer[4096];
		int32 readSize = 0;
		if (pio)
			readSize = pio->ReadAt(pos, buffer, sizeof(buffer));
		else if (io)
			readSize = io->Read(buffer, sizeof(buffer));
		else if (rb)
			readSize = rb->Read(buffer, sizeof(buffer));
		TRACE("readSize %ld\n", readSize);

		if (readSize < 0) {
			TRACE_ERROR("read failed, closing connection: %s\n",
				strerror(readSize));
			return readSize;
		}

		if (readSize == 0) {
			//XXX:should just break;
//			TRACE("read 0 bytes, retrying\n");
			snooze(100 * 1000);
			errorCount++;
			if (errorCount == 5) {
				TRACE_ERROR("failed to read, assuming disconnect\n");
				fEndpoint->Close();
				break;
			}

			continue;
		}
		pos += readSize;

		errorCount = 0;
		result = fEndpoint->Send(buffer, readSize);
TRACE("writeSize %ld\n", result);
		if (result < readSize) {
			TRACE_ERROR("writing to peer failed: %s\n",
				strerror(result));
			return result;
		}		
	}

	return B_OK;
}
