/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		François Revol <revol@free.fr>
 */
#ifndef HTML5_HW_INTERFACE_H
#define HTML5_HW_INTERFACE_H

#include "HWInterface.h"

#include <Locker.h>
#include <ObjectList.h>

class BNetEndpoint;
class StreamingRingBuffer;
class NetSender;
class NetReceiver;
class CanvasEventStream;
class CanvasMessage;
class WebServer;

struct callback_info;


//XXX: CanvasHWInterface ??
class HTML5HWInterface : public HWInterface {
public:
									HTML5HWInterface(const char* target);
virtual								~HTML5HWInterface();

virtual	status_t					Initialize();
virtual	status_t					Shutdown();

virtual	DrawingEngine*				CreateDrawingEngine();
virtual	EventStream*				CreateEventStream();

virtual	status_t					SetMode(const display_mode& mode);
virtual	void						GetMode(display_mode* mode);

virtual status_t					GetDeviceInfo(accelerant_device_info* info);
virtual status_t					GetFrameBufferConfig(
										frame_buffer_config& config);

virtual status_t					GetModeList(display_mode** _modeList,
										uint32* _count);
virtual status_t					GetPixelClockLimits(display_mode* mode,
										uint32* _low, uint32* _high);
virtual status_t					GetTimingConstraints(
										display_timing_constraints* constraints);
virtual status_t					ProposeMode(display_mode* candidate,
										const display_mode* low,
										const display_mode* high);

virtual sem_id						RetraceSemaphore();
virtual status_t					WaitForRetrace(
										bigtime_t timeout = B_INFINITE_TIMEOUT);

virtual status_t					SetDPMSMode(uint32 state);
virtual uint32						DPMSMode();
virtual uint32						DPMSCapabilities();

		// cursor handling
virtual	void						SetCursor(ServerCursor* cursor);
virtual	void						SetCursorVisible(bool visible);
virtual	void						MoveCursorTo(float x, float y);
virtual	void						SetDragBitmap(const ServerBitmap* bitmap,
										const BPoint& offsetFormCursor);

		// frame buffer access
virtual	RenderingBuffer*			FrontBuffer() const;
virtual	RenderingBuffer*			BackBuffer() const;
virtual	bool						IsDoubleBuffered() const;

virtual	status_t					InvalidateRegion(BRegion& region);
virtual	status_t					Invalidate(const BRect& frame);
virtual	status_t					CopyBackToFront(const BRect& frame);

		// drawing engine interface
		StreamingRingBuffer*		ReceiveBuffer() { return fReceiveBuffer; }
		StreamingRingBuffer*		SendBuffer() { return fSendBuffer; }

typedef bool (*CallbackFunction)(void* cookie, CanvasMessage& message);

		status_t					AddCallback(uint32 token,
										CallbackFunction callback,
										void* cookie);
		bool						RemoveCallback(uint32 token);

private:
		callback_info*				_FindCallback(uint32 token);
static	int							_CallbackCompare(const uint32* key,
										const callback_info* info);

static	int32						_EventThreadEntry(void* data);
		status_t					_EventThread();

		status_t					_Connect();
		void						_Disconnect();

		const char*					fTarget;
		char*						fRemoteHost;
		uint32						fRemotePort;

		status_t					fInitStatus;
		bool						fIsConnected;
		uint32						fProtocolVersion;
		uint32						fConnectionSpeed;
		display_mode				fDisplayMode;
		uint16						fListenPort;

//		BNetEndpoint*				fSendEndpoint;
		BNetEndpoint*				fReceiveEndpoint;

		StreamingRingBuffer*		fSendBuffer;
		StreamingRingBuffer*		fReceiveBuffer;

		WebServer*					fServer;
//		NetSender*					fSender;
//		NetReceiver*				fReceiver;

		thread_id					fEventThread;
		CanvasEventStream*			fEventStream;

		BLocker						fCallbackLocker;
		BObjectList<callback_info>	fCallbacks;
};

#endif // HTML5_HW_INTERFACE_H
