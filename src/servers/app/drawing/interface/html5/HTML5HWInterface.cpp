/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		François Revol <revol@free.fr>
 */

#include "HTML5HWInterface.h"
#include "HTML5DrawingEngine.h"
#include "CanvasEventStream.h"
#include "CanvasMessage.h"

#include "WebHandler.h"
#include "WebServer.h"
#include "WebWorker.h"
//#include "NetReceiver.h"
//#include "NetSender.h"
#include "StreamingRingBuffer.h"

#include "desktop.html.h"
#include "haiku.js.h"

#include <Autolock.h>
#include <NetEndpoint.h>

#include <new>
#include <string.h>


#define TRACE(x...)				debug_printf("HTML5HWInterface: "x)
#define TRACE_ALWAYS(x...)		debug_printf("HTML5HWInterface: "x)
#define TRACE_ERROR(x...)		debug_printf("HTML5HWInterface: "x)


struct callback_info {
	uint32				token;
	HTML5HWInterface::CallbackFunction	callback;
	void*				cookie;
};


HTML5HWInterface::HTML5HWInterface(const char* target)
	:
	HWInterface(),
	fTarget(target),
	fRemoteHost(NULL),
	fRemotePort(10900),
	fIsConnected(false),
	fProtocolVersion(100),
	fConnectionSpeed(0),
	fListenPort(10901),
	fReceiveEndpoint(NULL),
	fSendBuffer(NULL),
	fReceiveBuffer(NULL),
	fServer(NULL),
//	fSender(NULL),
//	fReceiver(NULL),
	fEventThread(-1),
	fEventStream(NULL),
	fCallbackLocker("callback locker")
{
	fDisplayMode.virtual_width = 640;
	fDisplayMode.virtual_height = 480;
	fDisplayMode.space = B_RGB32;

	//TODO: Cleanup; parse host ??
	fRemoteHost = strdup(fTarget);
	if (strncmp(fRemoteHost, "html5:", 6) != 0) {
		fInitStatus = B_BAD_VALUE;
		return;
	}
	char *portStart = fRemoteHost + 5;//strchr(fRemoteHost + 6, ':');
	if (portStart != NULL) {
		portStart[0] = 0;
		portStart++;
		if (sscanf(portStart, "%lu", &fRemotePort) != 1) {
			fInitStatus = B_BAD_VALUE;
			return;
		}

		fListenPort = fRemotePort;
	}

	fReceiveEndpoint = new(std::nothrow) BNetEndpoint();
	if (fReceiveEndpoint == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fInitStatus = fReceiveEndpoint->Bind(fListenPort);
	if (fInitStatus != B_OK)
		return;

	fSendBuffer = new(std::nothrow) StreamingRingBuffer(16 * 1024);
	if (fSendBuffer == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fInitStatus = fSendBuffer->InitCheck();
	if (fInitStatus != B_OK)
		return;

	fReceiveBuffer = new(std::nothrow) StreamingRingBuffer(16 * 1024);
	if (fReceiveBuffer == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fInitStatus = fReceiveBuffer->InitCheck();
	if (fInitStatus != B_OK)
		return;

	fServer = new(std::nothrow) WebServer(fReceiveEndpoint);
	if (fServer == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	WebHandler *handler;
	handler = new(std::nothrow) WebHandler("output", fSendBuffer);
	if (handler == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}
	handler->SetMultipart();
	handler->SetType("multipart/x-mixed-replace;boundary=x");
	fServer->AddHandler(handler);

	handler = new(std::nothrow) WebHandler("input", fReceiveBuffer);
	if (handler == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}
	fServer->AddHandler(handler);

	handler = new(std::nothrow) WebHandler("desktop.html",
		new(std::nothrow) BMemoryIO(desktop_html, sizeof(desktop_html) - 1));
	if (handler == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}
	fServer->AddHandler(handler);

	handler = new(std::nothrow) WebHandler("haiku.js",
		new(std::nothrow) BMemoryIO(haiku_js, sizeof(haiku_js) - 1));
	if (handler == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}
	fServer->AddHandler(handler);


	fEventStream = new(std::nothrow) CanvasEventStream();
	if (fEventStream == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

//	fInitStatus = _Connect();
//	if (fInitStatus != B_OK)
//		return;
	fDisplayMode.virtual_width = 800;
	fDisplayMode.virtual_height = 600;


	fEventThread = spawn_thread(_EventThreadEntry, "HTML5 event thread",
		B_NORMAL_PRIORITY, this);
	if (fEventThread < 0) {
		fInitStatus = fEventThread;
		return;
	}

	resume_thread(fEventThread);
}


HTML5HWInterface::~HTML5HWInterface()
{
//	delete fReceiver;
	delete fReceiveBuffer;

	delete fSendBuffer;
//	delete fSender;

	delete fReceiveEndpoint;
//	delete fSendEndpoint;
	delete fServer;

	delete fEventStream;

	free(fRemoteHost);
}


status_t
HTML5HWInterface::Initialize()
{
	return fInitStatus;
}


status_t
HTML5HWInterface::Shutdown()
{
	_Disconnect();
	return B_OK;
}


DrawingEngine*
HTML5HWInterface::CreateDrawingEngine()
{
	return new(std::nothrow) HTML5DrawingEngine(this);
}


EventStream*
HTML5HWInterface::CreateEventStream()
{
	return fEventStream;
}


status_t
HTML5HWInterface::AddCallback(uint32 token, CallbackFunction callback,
	void* cookie)
{
	BAutolock lock(fCallbackLocker);
	int32 index = fCallbacks.BinarySearchIndexByKey(token, &_CallbackCompare);
	if (index >= 0)
		return B_NAME_IN_USE;

	callback_info* info = new(std::nothrow) callback_info;
	if (info == NULL)
		return B_NO_MEMORY;

	info->token = token;
	info->callback = callback;
	info->cookie = cookie;

	fCallbacks.AddItem(info, -index - 1);
	return B_OK;
}


bool
HTML5HWInterface::RemoveCallback(uint32 token)
{
	BAutolock lock(fCallbackLocker);
	int32 index = fCallbacks.BinarySearchIndexByKey(token, &_CallbackCompare);
	if (index < 0)
		return false;

	delete fCallbacks.RemoveItemAt(index);
	return true;
}


callback_info*
HTML5HWInterface::_FindCallback(uint32 token)
{
	BAutolock lock(fCallbackLocker);
	return fCallbacks.BinarySearchByKey(token, &_CallbackCompare);
}


int
HTML5HWInterface::_CallbackCompare(const uint32* key,
	const callback_info* info)
{
	if (info->token == *key)
		return 0;

	if (info->token < *key)
		return -1;

	return 1;
}


int32
HTML5HWInterface::_EventThreadEntry(void* data)
{
	return ((HTML5HWInterface*)data)->_EventThread();
}


status_t
HTML5HWInterface::_EventThread()
{
	CanvasMessage message(fReceiveBuffer, fSendBuffer);
	while (true) {
		uint16 code;
		status_t result = message.NextMessage(code);
		if (result != B_OK) {
			TRACE_ERROR("failed to read message from receiver: %s\n",
				strerror(result));
			return result;
		}

		TRACE("got message code %u with %lu bytes\n", code, message.DataLeft());

		if (code >= RP_MOUSE_MOVED && code <= RP_MODIFIERS_CHANGED) {
			// an input event, dispatch to the event stream
			if (fEventStream->EventReceived(message))
				continue;
		}

		switch (code) {
			case RP_UPDATE_DISPLAY_MODE:
			{
				// TODO: implement, we only handle it in the context of the
				// initial mode setup on connect
				break;
			}

			default:
			{
				uint32 token;
				if (message.Read(token) == B_OK) {
					callback_info* info = _FindCallback(token);
					if (info != NULL && info->callback(info->cookie, message))
						break;
				}

				TRACE_ERROR("unhandled HTML5 event code %u\n", code);
				break;
			}
		}
	}
}


status_t
HTML5HWInterface::_Connect()
{
#if 0
	TRACE("connecting to host \"%s\" port %lu\n", fRemoteHost, fRemotePort);
	status_t result = fSendEndpoint->Connect(fRemoteHost, (uint16)fRemotePort);
	if (result != B_OK) {
		TRACE_ERROR("failed to connect to host \"%s\" port %lu\n", fRemoteHost,
			fRemotePort);
		return result;
	}

	CanvasMessage message(fReceiveBuffer, fSendBuffer);
	message.Start(RP_INIT_CONNECTION);
	message.Add(fListenPort);
	result = message.Flush();
	if (result != B_OK) {
		TRACE_ERROR("failed to send init connection message\n");
		return result;
	}

	uint16 code;
	result = message.NextMessage(code);
	if (result != B_OK) {
		TRACE_ERROR("failed to read message from receiver: %s\n",
			strerror(result));
		return result;
	}

	TRACE("code %u with %lu bytes of data\n", code, message.DataLeft());
	if (code != RP_UPDATE_DISPLAY_MODE) {
		TRACE_ERROR("invalid connection init code %u\n", code);
		return B_ERROR;
	}

	int32 width, height;
	message.Read(width);
	result = message.Read(height);
	if (result != B_OK) {
		TRACE_ERROR("failed to get initial display mode\n");
		return result;
	}

	fDisplayMode.virtual_width = width;
	fDisplayMode.virtual_height = height;
#endif
	return B_OK;
}


void
HTML5HWInterface::_Disconnect()
{
	if (fIsConnected) {
		CanvasMessage message(NULL, fSendBuffer);
		message.Start(RP_CLOSE_CONNECTION);
		message.Flush();
		fIsConnected = false;
	}

	if (fReceiveEndpoint != NULL)
		fReceiveEndpoint->Close();
}


status_t
HTML5HWInterface::SetMode(const display_mode& mode)
{
	// The display mode depends on the screen resolution of the client, we
	// don't allow to change it.
	return B_UNSUPPORTED;
}


void
HTML5HWInterface::GetMode(display_mode* mode)
{
	if (mode == NULL || !ReadLock())
		return;

	*mode = fDisplayMode;
	ReadUnlock();
}


status_t
HTML5HWInterface::GetDeviceInfo(accelerant_device_info* info)
{
	if (!ReadLock())
		return B_ERROR;

	info->version = fProtocolVersion;
	info->dac_speed = fConnectionSpeed;
	info->memory = 33554432; // 32MB
	snprintf(info->name, sizeof(info->name), "Haiku, Inc. HTML5HWInterface");
	snprintf(info->chipset, sizeof(info->chipset), "Haiku, Inc. Chipset");
	snprintf(info->serial_no, sizeof(info->serial_no), fTarget);

	ReadUnlock();
	return B_OK;
}


status_t
HTML5HWInterface::GetFrameBufferConfig(frame_buffer_config& config)
{
	// We don't actually have a frame buffer.
	return B_UNSUPPORTED;
}


status_t
HTML5HWInterface::GetModeList(display_mode** _modes, uint32* _count)
{
	AutoReadLocker _(this);

	display_mode* modes = new(std::nothrow) display_mode[1];
	if (modes == NULL)
		return B_NO_MEMORY;

	modes[0] = fDisplayMode;
	*_modes = modes;
	*_count = 1;
	return B_OK;
}


status_t
HTML5HWInterface::GetPixelClockLimits(display_mode* mode, uint32* low,
	uint32* high)
{
	return B_UNSUPPORTED;
}


status_t
HTML5HWInterface::GetTimingConstraints(display_timing_constraints* constraints)
{
	return B_UNSUPPORTED;
}


status_t
HTML5HWInterface::ProposeMode(display_mode* candidate, const display_mode* low,
	const display_mode* high)
{
	return B_UNSUPPORTED;
}


status_t
HTML5HWInterface::SetDPMSMode(uint32 state)
{
	return B_UNSUPPORTED;
}


uint32
HTML5HWInterface::DPMSMode()
{
	return (IsConnected() ? B_DPMS_ON : B_DPMS_SUSPEND);
}


uint32
HTML5HWInterface::DPMSCapabilities()
{
	return B_DPMS_ON | B_DPMS_SUSPEND;
}


sem_id
HTML5HWInterface::RetraceSemaphore()
{
	return -1;
}


status_t
HTML5HWInterface::WaitForRetrace(bigtime_t timeout)
{
	return B_UNSUPPORTED;
}


void
HTML5HWInterface::SetCursor(ServerCursor* cursor)
{
	HWInterface::SetCursor(cursor);
	if (!IsConnected())
		return;
	CanvasMessage message(NULL, fSendBuffer);
	message.Start(RP_SET_CURSOR);
	message.AddCursor(Cursor().Get());
}


void
HTML5HWInterface::SetCursorVisible(bool visible)
{
	HWInterface::SetCursorVisible(visible);
	if (!IsConnected())
		return;
	CanvasMessage message(NULL, fSendBuffer);
	message.Start(RP_SET_CURSOR_VISIBLE);
	message.Add(visible);
}


void
HTML5HWInterface::MoveCursorTo(float x, float y)
{
	HWInterface::MoveCursorTo(x, y);
	if (!IsConnected())
		return;
	CanvasMessage message(NULL, fSendBuffer);
	message.Start(RP_MOVE_CURSOR_TO);
	message.Add(x);
	message.Add(y);
}


void
HTML5HWInterface::SetDragBitmap(const ServerBitmap* bitmap,
	const BPoint& offsetFromCursor)
{
	HWInterface::SetDragBitmap(bitmap, offsetFromCursor);
	if (!IsConnected())
		return;
	CanvasMessage message(NULL, fSendBuffer);
	message.Start(RP_SET_CURSOR);
	message.AddCursor(CursorAndDragBitmap().Get());
}


RenderingBuffer*
HTML5HWInterface::FrontBuffer() const
{
	return NULL;
}


RenderingBuffer*
HTML5HWInterface::BackBuffer() const
{
	return NULL;
}


bool
HTML5HWInterface::IsDoubleBuffered() const
{
	return false;
}


status_t
HTML5HWInterface::InvalidateRegion(BRegion& region)
{
	CanvasMessage message(NULL, fSendBuffer);
	if (!IsConnected())
		return B_OK;
	message.Start(RP_INVALIDATE_REGION);
	message.AddRegion(region);
	return B_OK;
}


status_t
HTML5HWInterface::Invalidate(const BRect& frame)
{
	CanvasMessage message(NULL, fSendBuffer);
	if (!IsConnected())
		return B_OK;
	message.Start(RP_INVALIDATE_RECT);
	message.Add(frame);
	return B_OK;
}


status_t
HTML5HWInterface::CopyBackToFront(const BRect& frame)
{
	return B_OK;
}
