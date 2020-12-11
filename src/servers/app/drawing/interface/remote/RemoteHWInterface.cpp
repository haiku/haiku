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

#include "SystemPalette.h"

#include <Autolock.h>
#include <NetEndpoint.h>

#include <new>
#include <string.h>


#define TRACE(x...)				/*debug_printf("RemoteHWInterface: " x)*/
#define TRACE_ALWAYS(x...)		debug_printf("RemoteHWInterface: " x)
#define TRACE_ERROR(x...)		debug_printf("RemoteHWInterface: " x)


struct callback_info {
	uint32				token;
	RemoteHWInterface::CallbackFunction	callback;
	void*				cookie;
};


RemoteHWInterface::RemoteHWInterface(const char* target)
	:
	HWInterface(),
	fTarget(target),
	fIsConnected(false),
	fProtocolVersion(100),
	fConnectionSpeed(0),
	fListenPort(10901),
	fListenEndpoint(NULL),
	fSendBuffer(NULL),
	fReceiveBuffer(NULL),
	fSender(NULL),
	fReceiver(NULL),
	fEventThread(-1),
	fEventStream(NULL),
	fCallbackLocker("callback locker")
{
	memset(&fFallbackMode, 0, sizeof(fFallbackMode));
	fFallbackMode.virtual_width = 640;
	fFallbackMode.virtual_height = 480;
	fFallbackMode.space = B_RGB32;
	_FillDisplayModeTiming(fFallbackMode);

	fCurrentMode = fClientMode = fFallbackMode;

	if (sscanf(fTarget, "%" B_SCNu16, &fListenPort) != 1) {
		fInitStatus = B_BAD_VALUE;
		return;
	}

	fListenEndpoint.SetTo(new(std::nothrow) BNetEndpoint());
	if (fListenEndpoint.Get() == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fInitStatus = fListenEndpoint->Bind(fListenPort);
	if (fInitStatus != B_OK)
		return;

	fSendBuffer.SetTo(new(std::nothrow) StreamingRingBuffer(16 * 1024));
	if (fSendBuffer.Get() == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fInitStatus = fSendBuffer->InitCheck();
	if (fInitStatus != B_OK)
		return;

	fReceiveBuffer.SetTo(new(std::nothrow) StreamingRingBuffer(16 * 1024));
	if (fReceiveBuffer.Get() == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fInitStatus = fReceiveBuffer->InitCheck();
	if (fInitStatus != B_OK)
		return;

	fReceiver.SetTo(new(std::nothrow) NetReceiver(fListenEndpoint.Get(), fReceiveBuffer.Get(),
		_NewConnectionCallback, this));
	if (fReceiver.Get() == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fEventStream.SetTo(new(std::nothrow) RemoteEventStream());
	if (fEventStream.Get() == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

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
	//TODO: check order
	fReceiver.Unset();
	fReceiveBuffer.Unset();

	fSendBuffer.Unset();
	fSender.Unset();

	fListenEndpoint.Unset();

	fEventStream.Unset();
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
	return fEventStream.Get();
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
	RemoteMessage message(fReceiveBuffer.Get(), NULL);
	while (true) {
		uint16 code;
		status_t result = message.NextMessage(code);
		if (result != B_OK) {
			TRACE_ERROR("failed to read message from receiver: %s\n",
				strerror(result));
			return result;
		}

		TRACE("got message code %" B_PRIu16 " with %" B_PRIu32 " bytes\n", code,
			message.DataLeft());

		if (code >= RP_MOUSE_MOVED && code <= RP_MODIFIERS_CHANGED) {
			// an input event, dispatch to the event stream
			if (fEventStream->EventReceived(message))
				continue;
		}

		switch (code) {
			case RP_INIT_CONNECTION:
			{
				RemoteMessage reply(NULL, fSendBuffer.Get());
				reply.Start(RP_INIT_CONNECTION);
				status_t result = reply.Flush();
				(void)result;
				TRACE("init connection result: %s\n", strerror(result));
				break;
			}

			case RP_UPDATE_DISPLAY_MODE:
			{
				int32 width, height;
				message.Read(width);
				result = message.Read(height);
				if (result != B_OK) {
					TRACE_ERROR("failed to read display mode\n");
					break;
				}

				fIsConnected = true;
				fClientMode.virtual_width = width;
				fClientMode.virtual_height = height;
				_FillDisplayModeTiming(fClientMode);
				_NotifyScreenChanged();
				break;
			}

			case RP_GET_SYSTEM_PALETTE:
			{
				RemoteMessage reply(NULL, fSendBuffer.Get());
				reply.Start(RP_GET_SYSTEM_PALETTE_RESULT);

				const color_map *map = SystemColorMap();
				uint32 count = (uint32)B_COUNT_OF(map->color_list);

				reply.Add(count);
				for (size_t i = 0; i < count; i++) {
					const rgb_color &color = map->color_list[i];
					reply.Add(color.red);
					reply.Add(color.green);
					reply.Add(color.blue);
					reply.Add(color.alpha);
				}

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
RemoteHWInterface::_NewConnectionCallback(void *cookie, BNetEndpoint &endpoint)
{
	return ((RemoteHWInterface *)cookie)->_NewConnection(endpoint);
}


status_t
RemoteHWInterface::_NewConnection(BNetEndpoint &endpoint)
{
	fSender.Unset();

	fSendBuffer->MakeEmpty();

	BNetEndpoint *sendEndpoint = new(std::nothrow) BNetEndpoint(endpoint);
	if (sendEndpoint == NULL)
		return B_NO_MEMORY;

	fSender.SetTo(new(std::nothrow) NetSender(sendEndpoint, fSendBuffer.Get()));
	if (fSender.Get() == NULL) {
		delete sendEndpoint;
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
RemoteHWInterface::_Disconnect()
{
	if (fIsConnected) {
		RemoteMessage message(NULL, fSendBuffer.Get());
		message.Start(RP_CLOSE_CONNECTION);
		message.Flush();
		fIsConnected = false;
	}

	if (fListenEndpoint.Get() != NULL)
		fListenEndpoint->Close();
}


status_t
RemoteHWInterface::SetMode(const display_mode& mode)
{
	TRACE("set mode: %" B_PRIu16 " %" B_PRIu16 "\n", mode.virtual_width,
		mode.virtual_height);
	fCurrentMode = mode;
	return B_OK;
}


void
RemoteHWInterface::GetMode(display_mode* mode)
{
	if (mode == NULL || !ReadLock())
		return;

	*mode = fCurrentMode;
	ReadUnlock();

	TRACE("get mode: %" B_PRIu16 " %" B_PRIu16 "\n", mode->virtual_width,
		mode->virtual_height);
}


status_t
RemoteHWInterface::GetPreferredMode(display_mode* mode)
{
	*mode = fClientMode;
	return B_OK;
}


status_t
RemoteHWInterface::GetDeviceInfo(accelerant_device_info* info)
{
	if (!ReadLock())
		return B_ERROR;

	info->version = fProtocolVersion;
	info->dac_speed = fConnectionSpeed;
	info->memory = 33554432; // 32MB
	strlcpy(info->name, "Haiku, Inc. RemoteHWInterface", sizeof(info->name));
	strlcpy(info->chipset, "Haiku, Inc. Chipset", sizeof(info->chipset));
	strlcpy(info->serial_no, fTarget, sizeof(info->serial_no));

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

	display_mode* modes = new(std::nothrow) display_mode[2];
	if (modes == NULL)
		return B_NO_MEMORY;

	modes[0] = fFallbackMode;
	modes[1] = fClientMode;
	*_modes = modes;
	*_count = 2;

	return B_OK;
}


status_t
RemoteHWInterface::GetPixelClockLimits(display_mode* mode, uint32* low,
	uint32* high)
{
	TRACE("get pixel clock limits unsupported\n");
	return B_UNSUPPORTED;
}


status_t
RemoteHWInterface::GetTimingConstraints(display_timing_constraints* constraints)
{
	TRACE("get timing constraints unsupported\n");
	return B_UNSUPPORTED;
}


status_t
RemoteHWInterface::ProposeMode(display_mode* candidate, const display_mode* low,
	const display_mode* high)
{
	TRACE("propose mode: %" B_PRIu16 " %" B_PRIu16 "\n",
		candidate->virtual_width, candidate->virtual_height);
	return B_OK;
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


status_t
RemoteHWInterface::SetBrightness(float)
{
	return B_UNSUPPORTED;
}


status_t
RemoteHWInterface::GetBrightness(float*)
{
	return B_UNSUPPORTED;
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
	RemoteMessage message(NULL, fSendBuffer.Get());
	message.Start(RP_SET_CURSOR);
	message.AddCursor(CursorAndDragBitmap().Get());
}


void
RemoteHWInterface::SetCursorVisible(bool visible)
{
	HWInterface::SetCursorVisible(visible);
	RemoteMessage message(NULL, fSendBuffer.Get());
	message.Start(RP_SET_CURSOR_VISIBLE);
	message.Add(visible);
}


void
RemoteHWInterface::MoveCursorTo(float x, float y)
{
	HWInterface::MoveCursorTo(x, y);
	RemoteMessage message(NULL, fSendBuffer.Get());
	message.Start(RP_MOVE_CURSOR_TO);
	message.Add(x);
	message.Add(y);
}


void
RemoteHWInterface::SetDragBitmap(const ServerBitmap* bitmap,
	const BPoint& offsetFromCursor)
{
	HWInterface::SetDragBitmap(bitmap, offsetFromCursor);
	RemoteMessage message(NULL, fSendBuffer.Get());
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
RemoteHWInterface::InvalidateRegion(const BRegion& region)
{
	RemoteMessage message(NULL, fSendBuffer.Get());
	message.Start(RP_INVALIDATE_REGION);
	message.AddRegion(region);
	return B_OK;
}


status_t
RemoteHWInterface::Invalidate(const BRect& frame)
{
	RemoteMessage message(NULL, fSendBuffer.Get());
	message.Start(RP_INVALIDATE_RECT);
	message.Add(frame);
	return B_OK;
}


status_t
RemoteHWInterface::CopyBackToFront(const BRect& frame)
{
	return B_OK;
}


void
RemoteHWInterface::_FillDisplayModeTiming(display_mode &mode)
{
	mode.timing.pixel_clock
		= (uint64_t)mode.virtual_width * mode.virtual_height * 60 / 1000;
	mode.timing.h_display = mode.timing.h_sync_start = mode.timing.h_sync_end
		= mode.timing.h_total = mode.virtual_width;
	mode.timing.v_display = mode.timing.v_sync_start = mode.timing.v_sync_end
		= mode.timing.v_total = mode.virtual_height;
}
