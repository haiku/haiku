/*
	Driver for USB Human Interface Devices.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.

	Interpretation code based on the previous usb_hid driver which was written
	by  Jérôme Duval.
*/
#include "Driver.h"
#include "MouseDevice.h"
#include <string.h>
#include <usb/USB_hid.h>

// input server private for mouse_movement, MS_READ, etc...
#include "kb_mouse_driver.h"


MouseDevice::MouseDevice(usb_device device, usb_pipe interruptPipe,
	size_t interfaceIndex, report_insn *instructions, size_t instructionCount,
	size_t totalReportSize)
	:	HIDDevice(device, interruptPipe, interfaceIndex, instructions,
			instructionCount, totalReportSize, 512),
		fLastButtons(0),
		fClickCount(0),
		fLastClickTime(0),
		fClickSpeed(250000),
		fMaxButtons(16)
{
	SetBaseName("input/mouse/usb/");
}


status_t
MouseDevice::Control(uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case MS_READ:
			while (_RingBufferReadable() == 0) {
				if (!_IsTransferUnprocessed()) {
					status_t result = _ScheduleTransfer();
					if (result != B_OK)
						return result;
				}

				// NOTE: this thread is now blocking until the semaphore is
				// released in the callback function
				status_t result = acquire_sem_etc(fTransferNotifySem, 1,
					B_CAN_INTERRUPT, B_INFINITE_TIMEOUT);
				if (result != B_OK)
					return result;

				result = _InterpretBuffer();
				_SetTransferProcessed();
				if (result != B_OK)
					return result;
			}

			return _RingBufferRead(buffer, sizeof(mouse_movement));

		case MS_NUM_EVENTS:
			return _RingBufferReadable() / sizeof(mouse_movement);

		case MS_SET_CLICKSPEED:
#ifdef __HAIKU__
				return user_memcpy(&fClickSpeed, buffer, sizeof(bigtime_t));
#else
				fClickSpeed = *(bigtime_t *)buffer;
				return B_OK;
#endif
	}

	return B_ERROR;
}


status_t
MouseDevice::_InterpretBuffer()
{
	if (fTransferStatus != B_OK) {
		if (IsRemoved())
			return B_ERROR;

		if (fTransferStatus == B_DEV_STALLED
			&& gUSBModule->clear_feature(fInterruptPipe,
				USB_FEATURE_ENDPOINT_HALT) != B_OK)
			return B_ERROR;

		return B_OK;
	}

	mouse_movement info;
	memset(&info, 0, sizeof(info));
	for (size_t i = 0; i < fInstructionCount; i++) {
		const report_insn *instruction = &fInstructions[i];
		int32 value = (((fTransferBuffer[instruction->byte_idx + 1] << 8)
			| fTransferBuffer[instruction->byte_idx]) >> instruction->bit_pos)
			& ((1 << instruction->num_bits) - 1);

		switch (instruction->usage_page) {
			case USAGE_PAGE_BUTTON:
				if (instruction->usage_id - 1 < fMaxButtons)
					info.buttons |= (value & 1) << (instruction->usage_id - 1);
				break;

			case USAGE_PAGE_GENERIC_DESKTOP:
				if (instruction->is_phy_signed)
					value = sign_extend(value, instruction->num_bits);

				switch (instruction->usage_id) {
					case USAGE_ID_X:
						info.xdelta = value;
						break;

					case USAGE_ID_Y:
						info.ydelta = -value;
						break;

					case USAGE_ID_WHEEL:
						info.wheel_ydelta = -value;
						break;
				}
				break;
		}
	}

	bigtime_t timestamp = system_time();
	if (info.buttons != 0) {
		if (fLastButtons == 0) {
			if (fLastClickTime + fClickSpeed > timestamp)
				fClickCount++;
			else
				fClickCount = 1;
		}

		fLastClickTime = timestamp;
		info.clicks = fClickCount;
	}

	fLastButtons = info.buttons;
	info.timestamp = timestamp;
	return _RingBufferWrite(&info, sizeof(mouse_movement));
}
