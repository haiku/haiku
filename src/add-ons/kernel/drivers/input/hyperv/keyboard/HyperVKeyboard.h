/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_KEYBOARD_H_
#define _HYPERV_KEYBOARD_H_


#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <condition_variable.h>
#include <lock.h>
#include <util/AutoLock.h>
#include <util/ring_buffer.h>

#include <hyperv.h>
#include <hyperv_spec.h>
#include <keyboard_mouse_driver.h>

#include "HyperVKeyboardProtocol.h"


//#define TRACE_HYPERV_KEYBOARD
#ifdef TRACE_HYPERV_KEYBOARD
#	define TRACE(x...) dprintf("\33[94mhyperv_keyboard:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[94mhyperv_keyboard:\33[0m " x)
#define ERROR(x...)			dprintf("\33[94mhyperv_keyboard:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define HYPERV_KEYBOARD_DRIVER_MODULE_NAME	"drivers/input/hyperv_keyboard/driver_v1"
#define HYPERV_KEYBOARD_DEVICE_MODULE_NAME	"drivers/input/hyperv_keyboard/device_v1"
#define HYPERV_KEYBOARD_DEVICE_ID_GENERATOR	"hyperv_keyboard/device_id"

// Buffer needs to be large enough to reasonably handle paste clipboard feature
#define HYPERV_KEYBOARD_KEY_BUFFER_SIZE 500


extern device_manager_info* gDeviceManager;


class HyperVKeyboard {
public:
								HyperVKeyboard(device_node* node);
								~HyperVKeyboard();
			status_t			InitCheck() const { return fStatus; }
			status_t			Register();

			status_t			Open();
			status_t			Close();
			status_t			Control(uint32 op, void* buffer, size_t length);

private:
	static	void				_CallbackHandler(void* arg);
			void				_Callback();
			void				_HandleEvent(hv_keyboard_msg_event* message);
			status_t			_Connect();
			status_t			_SetIndicators(led_info ledInfo);
			bool				_CanWriteKeystroke();
			void				_WriteKeystroke(raw_key_info* keyInfo);
			status_t			_ReadKeystroke(raw_key_info* keyInfo);

private:
			device_node*		fNode;
			status_t			fStatus;
			int32				fOpenCount;
			uint8*				fPacket;
			ConditionVariable	fProtocolRespEvent;

			ring_buffer*		fKeyBuffer;
			mutex				fKeyBufferLock;
			sem_id				fKeyBufferSem;

			hyperv_device_interface*	fHyperV;
			hyperv_device				fHyperVCookie;
};


#endif // _HYPERV_KEYBOARD_H_
