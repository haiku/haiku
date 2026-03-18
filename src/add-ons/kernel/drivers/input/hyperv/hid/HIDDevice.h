/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_HID_DEVICE_H_
#define _HYPERV_HID_DEVICE_H_

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <condition_variable.h>
#include <device_manager.h>
#include <kernel.h>
#include <lock.h>
#include <fs/devfs.h>

#include <hyperv.h>

#include "HIDParser.h"
#include "HyperVHIDProtocol.h"

class ProtocolHandler;

class HIDDevice {
public:
									HIDDevice(hyperv_device_interface* hyperv,
										hyperv_device hyperv_cookie);
									~HIDDevice();

			status_t				InitCheck() const { return fStatus; }
			bool					IsOpen() const { return fOpenCount > 0; }
			status_t				Open(ProtocolHandler* handler, uint32 flags);
			status_t				Close(ProtocolHandler* handler);
			int32					OpenCount() const { return fOpenCount; }

			void					Removed();
			bool					IsRemoved() const { return fRemoved; }

			status_t				MaybeScheduleTransfer(HIDReport* report);
			status_t				SendReport(HIDReport* report);
			HIDParser&				Parser() { return fParser; }
			ProtocolHandler*		ProtocolHandlerAt(uint32 index) const;

private:
	static	void					_CallbackHandler(void* data);
			void					_Callback();
			void					_HandleInputReport(hv_hid_msg_input_report* reportMessage);
			status_t				_Connect();
			status_t				_AddWheelHIDItem(HIDReport* report);

private:
			status_t				fStatus;

			int32					fOpenCount;
			bool					fRemoved;

			HIDParser				fParser;
			uint32					fProtocolHandlerCount;
			ProtocolHandler*		fProtocolHandlerList;

			hyperv_device_interface*		fHyperV;
			hyperv_device					fHyperVCookie;
			hv_hid_msg_protocol_response	fProtocolResponse;
			hv_hid_msg_initial_dev_info*	fDeviceInfo;
			uint32							fDeviceInfoLength;

			ConditionVariable		fProtocolRespEvent;
			ConditionVariable		fDeviceInfoEvent;
			uint8*					fPacket;
			uint16					fLastX;
			uint16					fLastY;
};


#endif // _HYPERV_HID_DEVICE_H_
