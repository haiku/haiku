/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "RemoteHWInterface.h"
#include "RemoteDrawingEngine.h"
#include "RemoteEventStream.h"
#include "RemoteMessage.h"

#include "NetReceiver.h"
#include "NetSender.h"
#include "StreamingRingBuffer.h"

#include <Autolock.h>
#include <NetEndpoint.h>

#include <new>
#include <string.h>


#define TRACE(x...)				/*debug_printf("RemoteHWInterface: "x)*/
#define TRACE_ALWAYS(x...)		debug_printf("RemoteHWInterface: "x)
#define TRACE_ERROR(x...)		debug_printf("RemoteHWInterface: "x)


struct callback_info {
	uint32				token;
	RemoteHWInterface::CallbackFunction	callback;
	void*				cookie;
};


RemoteHWInterface::RemoteHWInterface(const char* target)
	:
	HWInterface(),
	fTarget(target),
	fRemoteHost(NULL),
	fRemotePort(10900),
	fIsConnected(false),
	fProtocolVersion(100),
	fConnectionSpeed(0),
	fListenPort(10901),
	fSendEndpoint(NULL),
	fReceiveEndpoint(NULL),
	fSendBuffer(NULL),
	fReceiveBuffer(NULL),
	fSender(NULL),
	fReceiver(NULL),
	fEventThread(-1),
	fEventStream(NULL),
	fCallbackLocker("callback locker")
{
	fDisplayMode.virtual_width = 640;
	fDisplayMode.virtual_height = 480;
	fDisplayMode.space = B_RGB32;

	fRemoteHost = strdup(fTarget);
	char *portStart = strchr(fRemoteHost, ':');
	if (portStart != NULL) {
		portStart[0] = 0;
		portStart++;
		if (sscanf(portStart, "%lu", &fRemotePort) != 1) {
			fInitStatus = B_BAD_VALUE;
			return;
		}

		fListenPort = fRemotePort + 1;
	}

	fSendEndpoint = new(std::nothrow) BNetEndpoint();
	if (fSendEndpoint == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
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

	fSender = new(std::nothrow) NetSender(fSendEndpoint, fSendBuffer);
	if (fSender == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fReceiver = new(std::nothrow) NetReceiver(fReceiveEndpoint, fReceiveBuffer);
	if (fReceiver == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fEventStream = new(std::nothrow) RemoteEventStream();
	if (fEventStream == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fSendEndpoint->SetTimeout(3 * 1000 * 1000);
	fInitStatus = _Connect();
	if (fInitStatus != B_OK)
		return;

	fEventThread = spawn_thread(_EventThreadEntry, "remote event thread",
		B_NORMAL_PRIORITY, this);
	if (fEventThread < 0) {
		fInitStatus = fEventThread;
		return;
	}

	resume_thread(fEventThread);
}


RemoteHWInterface::~RemoteHWInterface()
{
	delete fReceiver;
	delete fReceiveBuffer;

	delete fSendBuffer;
	delete fSender;

	delete fReceiveEndpoint;
	delete fSendEndpoint;

	delete fEventStream;

	free(fRemoteHost);
}


status_t
RemoteHWInterface::Initialize()
{
	return fInitStatus;
}


status_t
RemoteHWInterface::Shutdown()
{
	_Disconnect();
	return B_OK;
}


DrawingEngine*
RemoteHWInterface::CreateDrawingEngine()
{
	return new(std::nothrow) RemoteDrawingEngine(this);
}


EventStream*
RemoteHWInterface::CreateEventStream()
{
	return fEventStream;
}


status_t
RemoteHWInterface::AddCallback(uint32 token, CallbackFunction callback,
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
RemoteHWInterface::RemoveCallback(uint32 token)
{
	BAutolock lock(fCallbackLocker);
	int32 index = fCallbacks.BinarySearchIndexByKey(token, &_CallbackCompare);
	if (index < 0)
		return false;

	delete fCallbacks.RemoveItemAt(index);
	return true;
}


callback_info*
RemoteHWInterface::_FindCallback(uint32 token)
{
	BAutolock lock(fCallbackLocker);
	return fCallbacks.BinarySearchByKey(token, &_CallbackCompare);
}


int
RemoteHWInterface::_CallbackCompare(const uint32* key,
	const callback_info* info)
{
	if (info->token == *key)
		return 0;

	if (info->token < *key)
		return -1;

	return 1;
}


int32
RemoteHWInterface::_EventThreadEntry(void* data)
{
	return ((RemoteHWInterface*)data)->_EventThread();
}


status_t
RemoteHWInterface::_EventThread()
{
	RemoteMessage message(fReceiveBuffer, fSendBuffer);
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

				TRACE_ERROR("unhandled remote event code %u\n", code);
				break;
			}
		}
	}
}


status_t
RemoteHWInterface::_Connect()
{
	TRACE("connecting to host \"%s\" port %lu\n", fRemoteHost, fRemotePort);
	status_t result = fSendEndpoint->Connect(fRemoteHost, (uint16)fRemotePort);
	if (result != B_OK) {
		TRACE_ERROR("failed to connect to host \"%s\" port %lu\n", fRemoteHost,
			fRemotePort);
		return result;
	}

	RemoteMessage message(fReceiveBuffer, fSendBuffer);
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
	return B_OK;
}


void
RemoteHWInterface::_Disconnect()
{
	if (fIsConnected) {
		RemoteMessage message(NULL, fSendBuffer);
		message.Start(RP_CLOSE_CONNECTION);
		message.Flush();
		fIsConnected = false;
	}

	if (fSendEndpoint != NULL)
		fSendEndpoint->Close();
	if (fReceiveEndpoint != NULL)
		fReceiveEndpoint->Close();
}


status_t
RemoteHWInterface::SetMode(const display_mode& mode)
{
	// The display mode depends on the screen resolution of the client, we
	// don't allow to change it.
	return B_UNSUPPORTED;
}


void
RemoteHWInterface::GetMode(display_mode* mode)
{
	if (mode == NULL || !ReadLock())
		return;

	*mode = fDisplayMode;
	ReadUnlock();
}


status_t
RemoteHWInterface::GetDeviceInfo(accelerant_device_info* info)
{
	if (!ReadLock())
		return B_ERROR;

	info->version = fProtocolVersion;
	info->dac_speed = fConnectionSpeed;
	info->memory = 33554432; // 32MB
	snprintf(info->name, sizeof(info->name), "Haiku, Inc. RemoteHWInterface");
	snprintf(info->chipset, sizeof(info->chipset), "Haiku, Inc. Chipset");
	snprintf(info->serial_no, sizeof(info->serial_no), fTarget);

	ReadUnlock();
	return B_OK;
}


status_t
RemoteHWInterface::GetFrameBufferConfig(frame_buffer_config& config)
{
	// We don't actually have a frame buffer.
	return B_UNSUPPORTED;
}


status_t
RemoteHWInterface::GetModeList(display_mode** _modes, uint32* _count)
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
RemoteHWInterface::GetPixelClockLimits(display_mode* mode, uint32* low,
	uint32* high)
{
	return B_UNSUPPORTED;
}


status_t
RemoteHWInterface::GetTimingConstraints(display_timing_constraints* constraints)
{
	return B_UNSUPPORTED;
}


status_t
RemoteHWInterface::ProposeMode(display_mode* candidate, const display_mode* low,
	const display_mode* high)
{
	return B_UNSUPPORTED;
}


status_t
RemoteHWInterface::SetDPMSMode(uint32 state)
{
	return B_UNSUPPORTED;
}


uint32
RemoteHWInterface::DPMSMode()
{
	return B_UNSUPPORTED;
}


uint32
RemoteHWInterface::DPMSCapabilities()
{
	return 0;
}


sem_id
RemoteHWInterface::RetraceSemaphore()
{
	return -1;
}


status_t
RemoteHWInterface::WaitForRetrace(bigtime_t timeout)
{
	return B_UNSUPPORTED;
}


void
RemoteHWInterface::SetCursor(ServerCursor* cursor)
{
	HWInterface::SetCursor(cursor);
	RemoteMessage message(NULL, fSendBuffer);
	message.Start(RP_SET_CURSOR);
	message.AddCursor(Cursor().Get());
}


void
RemoteHWInterface::SetCursorVisible(bool visible)
{
	HWInterface::SetCursorVisible(visible);
	RemoteMessage message(NULL, fSendBuffer);
	message.Start(RP_SET_CURSOR_VISIBLE);
	message.Add(visible);
}


void
RemoteHWInterface::MoveCursorTo(float x, float y)
{
	HWInterface::MoveCursorTo(x, y);
	RemoteMessage message(NULL, fSendBuffer);
	message.Start(RP_MOVE_CURSOR_TO);
	message.Add(x);
	message.Add(y);
}


void
RemoteHWInterface::SetDragBitmap(const ServerBitmap* bitmap,
	const BPoint& offsetFromCursor)
{
	HWInterface::SetDragBitmap(bitmap, offsetFromCursor);
	RemoteMessage message(NULL, fSendBuffer);
	message.Start(RP_SET_CURSOR);
	message.AddCursor(CursorAndDragBitmap().Get());
}


RenderingBuffer*
RemoteHWInterface::FrontBuffer() const
{
	return NULL;
}


RenderingBuffer*
RemoteHWInterface::BackBuffer() const
{
	return NULL;
}


bool
RemoteHWInterface::IsDoubleBuffered() const
{
	return false;
}


status_t
RemoteHWInterface::InvalidateRegion(BRegion& region)
{
	RemoteMessage message(NULL, fSendBuffer);
	message.Start(RP_INVALIDATE_REGION);
	message.AddRegion(region);
	return B_OK;
}


status_t
RemoteHWInterface::Invalidate(const BRect& frame)
{
	RemoteMessage message(NULL, fSendBuffer);
	message.Start(RP_INVALIDATE_RECT);
	message.Add(frame);
	return B_OK;
}


status_t
RemoteHWInterface::CopyBackToFront(const BRect& frame)
{
	return B_OK;
}
