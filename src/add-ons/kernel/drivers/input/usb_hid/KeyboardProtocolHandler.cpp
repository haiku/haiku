/*
 * Copyright 2008-2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */


#include <new>
#include <stdlib.h>
#include <string.h>

#include <usb/USB_hid.h>
#include <util/AutoLock.h>

#include <debug.h>

#include "Driver.h"
#include "KeyboardProtocolHandler.h"

#include "HIDCollection.h"
#include "HIDDevice.h"
#include "HIDReport.h"
#include "HIDReportItem.h"

#include <keyboard_mouse_driver.h>


#define LEFT_ALT_KEY	0x04
#define RIGHT_ALT_KEY	0x40
#define ALT_KEYS		(LEFT_ALT_KEY | RIGHT_ALT_KEY)

#define KEYBOARD_FLAG_READER	0x01
#define KEYBOARD_FLAG_DEBUGGER	0x02


static bool sDebugKeyboardFound = false;
static usb_id sDebugKeyboardPipe = 0;
static size_t sDebugKeyboardReportSize = 0;
static int32 sDebuggerCommandAdded = 0;


static int
debug_get_keyboard_config(int argc, char **argv)
{
	set_debug_variable("_usbPipeID", (uint64)sDebugKeyboardPipe);
	set_debug_variable("_usbReportSize", (uint64)sDebugKeyboardReportSize);
	return 0;
}


//	#pragma mark -


KeyboardProtocolHandler::KeyboardProtocolHandler(HIDReport &inputReport,
	HIDReport *outputReport)
	:
	ProtocolHandler(inputReport.Device(), "input/keyboard/usb/", 512),
	fInputReport(inputReport),
	fOutputReport(outputReport),
	fRepeatDelay(300000),
	fRepeatRate(35000),
	fCurrentRepeatDelay(B_INFINITE_TIMEOUT),
	fCurrentRepeatKey(0),
	fKeyCount(0),
	fModifierCount(0),
	fLastModifiers(0),
	fCurrentKeys(NULL),
	fLastKeys(NULL),
	fHasReader(0),
	fHasDebugReader(false)
{
	mutex_init(&fLock, "usb keyboard");

	// find modifiers and keys
	bool debugUsable = false;
	for (uint32 i = 0; i < inputReport.CountItems(); i++) {
		HIDReportItem *item = inputReport.ItemAt(i);
		if (!item->HasData())
			continue;

		if (item->UsagePage() == B_HID_USAGE_PAGE_KEYBOARD
			|| item->UsagePage() == B_HID_USAGE_PAGE_CONSUMER
			|| item->UsagePage() == B_HID_USAGE_PAGE_BUTTON) {
			TRACE("keyboard item with usage %lx\n", item->UsageMinimum());

			if (item->Array()) {
				// normal or "consumer"/button keys handled as array items
				if (fKeyCount < MAX_KEYS)
					fKeys[fKeyCount++] = item;

				if (item->UsagePage() == B_HID_USAGE_PAGE_KEYBOARD)
					debugUsable = true;
			} else {
				if (item->UsagePage() == B_HID_USAGE_PAGE_KEYBOARD
					&& item->UsageID() >= B_HID_UID_KB_LEFT_CONTROL
					&& item->UsageID() <= B_HID_UID_KB_RIGHT_GUI) {
					// modifiers are generally implemented as bitmaps
					if (fModifierCount < MAX_MODIFIERS)
						fModifiers[fModifierCount++] = item;

					debugUsable = true;
				}
			}
		}
	}

	if (!sDebugKeyboardFound && debugUsable) {
		// It's a keyboard, not just some additional buttons, set up the kernel
		// debugger info here so that it is ready on panics or crashes that
		// don't go through the emergency keys. If we also found LEDs we assume
		// it is a full sized keyboard and discourage further setting the info.
		sDebugKeyboardPipe = fInputReport.Device()->InterruptPipe();
		sDebugKeyboardReportSize = fInputReport.Parser()->MaxReportSize();
		if (outputReport != NULL)
			sDebugKeyboardFound = true;
	}

	TRACE("keyboard device with %lu keys and %lu modifiers\n", fKeyCount,
		fModifierCount);
	TRACE("input report: %u; output report: %u\n", inputReport.ID(),
		outputReport != NULL ? outputReport->ID() : 255);

	fLastKeys = (uint16 *)malloc(fKeyCount * 2 * sizeof(uint16));
	fCurrentKeys = &fLastKeys[fKeyCount];
	if (fLastKeys == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	// find leds if we have an output report
	for (uint32 i = 0; i < MAX_LEDS; i++)
		fLEDs[i] = NULL;

	if (outputReport != NULL) {
		for (uint32 i = 0; i < outputReport->CountItems(); i++) {
			HIDReportItem *item = outputReport->ItemAt(i);
			if (!item->HasData())
				continue;

			// the led item array is identity mapped with what we get from
			// the input_server for the set-leds command
			if (item->UsagePage() == B_HID_USAGE_PAGE_LED) {
				switch (item->UsageID()) {
					case B_HID_UID_LED_NUM_LOCK:
						fLEDs[0] = item;
						break;
					case B_HID_UID_LED_CAPS_LOCK:
						fLEDs[1] = item;
						break;
					case B_HID_UID_LED_SCROLL_LOCK:
						fLEDs[2] = item;
						break;
				}
			}
		}
	}

	if (atomic_add(&sDebuggerCommandAdded, 1) == 0) {
		add_debugger_command("get_usb_keyboard_config",
			&debug_get_keyboard_config,
			"Gets the required config of the USB keyboard");
	}
}


KeyboardProtocolHandler::~KeyboardProtocolHandler()
{
	free(fLastKeys);

	if (atomic_add(&sDebuggerCommandAdded, -1) == 1) {
		remove_debugger_command("get_usb_keyboard_config",
			&debug_get_keyboard_config);
	}

	mutex_destroy(&fLock);
}


void
KeyboardProtocolHandler::AddHandlers(HIDDevice &device,
	HIDCollection &collection, ProtocolHandler *&handlerList)
{
	bool handled = false;
	switch (collection.UsagePage()) {
		case B_HID_USAGE_PAGE_GENERIC_DESKTOP:
		{
			switch (collection.UsageID()) {
				case B_HID_UID_GD_KEYBOARD:
				case B_HID_UID_GD_KEYPAD:
				case B_HID_UID_GD_SYSTEM_CONTROL:
					handled = true;
			}

			break;
		}

		case B_HID_USAGE_PAGE_CONSUMER:
		{
			switch (collection.UsageID()) {
				case B_HID_UID_CON_CONSUMER_CONTROL:
					handled = true;
			}

			break;
		}
	}

	if (!handled) {
		TRACE("collection not a supported keyboard subset\n");
		return;
	}

	HIDParser &parser = device.Parser();
	uint32 maxReportCount = parser.CountReports(HID_REPORT_TYPE_INPUT);
	if (maxReportCount == 0)
		return;

	uint32 inputReportCount = 0;
	HIDReport *inputReports[maxReportCount];
	collection.BuildReportList(HID_REPORT_TYPE_INPUT, inputReports,
		inputReportCount);

	TRACE("input report count: %lu\n", inputReportCount);

	for (uint32 i = 0; i < inputReportCount; i++) {
		HIDReport *inputReport = inputReports[i];

		bool mayHaveOutput = false;
		bool foundKeyboardUsage = false;
		for (uint32 j = 0; j < inputReport->CountItems(); j++) {
			HIDReportItem *item = inputReport->ItemAt(j);
			if (!item->HasData())
				continue;

			if (item->UsagePage() == B_HID_USAGE_PAGE_KEYBOARD
				|| (item->UsagePage() == B_HID_USAGE_PAGE_CONSUMER
					&& item->Array())
				|| (item->UsagePage() == B_HID_USAGE_PAGE_BUTTON
					&& item->Array())) {
				// found at least one item with a keyboard usage or with
				// a consumer/button usage that is handled like a key
				mayHaveOutput = item->UsagePage() == B_HID_USAGE_PAGE_KEYBOARD;
				foundKeyboardUsage = true;
				break;
			}
		}

		if (!foundKeyboardUsage)
			continue;

		bool foundOutputReport = false;
		HIDReport *outputReport = NULL;
		do {
			// try to find the led output report
			maxReportCount =  parser.CountReports(HID_REPORT_TYPE_OUTPUT);
			if (maxReportCount == 0)
				break;

			uint32 outputReportCount = 0;
			HIDReport *outputReports[maxReportCount];
			collection.BuildReportList(HID_REPORT_TYPE_OUTPUT,
				outputReports, outputReportCount);

			for (uint32  j = 0; j < outputReportCount; j++) {
				outputReport = outputReports[j];

				for (uint32 k = 0; k < outputReport->CountItems(); k++) {
					HIDReportItem *item = outputReport->ItemAt(k);
					if (item->UsagePage() == B_HID_USAGE_PAGE_LED) {
						foundOutputReport = true;
						break;
					}
				}

				if (foundOutputReport)
					break;
			}
		} while (false);

		ProtocolHandler *newHandler = new(std::nothrow) KeyboardProtocolHandler(
			*inputReport, foundOutputReport ? outputReport : NULL);
		if (newHandler == NULL) {
			TRACE("failed to allocated keyboard protocol handler\n");
			continue;
		}

		newHandler->SetNextHandler(handlerList);
		handlerList = newHandler;
	}
}


status_t
KeyboardProtocolHandler::Open(uint32 flags, uint32 *cookie)
{
	status_t status = ProtocolHandler::Open(flags, cookie);
	if (status != B_OK) {
		TRACE_ALWAYS("keyboard device failed to open: %s\n",
			strerror(status));
		return status;
	}

	if (Device()->OpenCount() == 1) {
		fCurrentRepeatDelay = B_INFINITE_TIMEOUT;
		fCurrentRepeatKey = 0;
	}

	return B_OK;
}


status_t
KeyboardProtocolHandler::Close(uint32 *cookie)
{
	if ((*cookie & KEYBOARD_FLAG_DEBUGGER) != 0)
		fHasDebugReader = false;
	if ((*cookie & KEYBOARD_FLAG_READER) != 0)
		atomic_and(&fHasReader, 0);

	return ProtocolHandler::Close(cookie);
}


status_t
KeyboardProtocolHandler::Control(uint32 *cookie, uint32 op, void *buffer,
	size_t length)
{
	switch (op) {
		case KB_READ:
		{
			if (*cookie == 0) {
				if (atomic_or(&fHasReader, 1) != 0)
					return B_BUSY;

				// We're the first, so we become the only reader
				*cookie = KEYBOARD_FLAG_READER;
			}

			while (true) {
				MutexLocker locker(fLock);

				bigtime_t enterTime = system_time();
				while (RingBufferReadable() == 0) {
					status_t result = _ReadReport(fCurrentRepeatDelay);
					if (result != B_OK && result != B_TIMED_OUT)
						return result;

					if (!Device()->IsOpen())
						return B_ERROR;

					if (RingBufferReadable() == 0 && fCurrentRepeatKey != 0
						&& system_time() - enterTime > fCurrentRepeatDelay) {
						// this case is for handling key repeats, it means no
						// interrupt transfer has happened or it didn't produce any
						// new key events, but a repeated key down is due
						_WriteKey(fCurrentRepeatKey, true);

						// the next timeout is reduced to the repeat_rate
						fCurrentRepeatDelay = fRepeatRate;
						break;
					}
				}

				if (fHasDebugReader && (*cookie & KEYBOARD_FLAG_DEBUGGER)
						== 0) {
					// Handover buffer to the debugger instead
					locker.Unlock();
					snooze(25000);
					continue;
				}

				// process what is in the ring_buffer, it could be written
				// there because we handled an interrupt transfer or because
				// we wrote the current repeat key
				return RingBufferRead(buffer, sizeof(raw_key_info));
			}
		}

		case KB_SET_LEDS:
		{
			uint8 ledData[4];
			if (user_memcpy(ledData, buffer, sizeof(ledData)) != B_OK)
				return B_BAD_ADDRESS;
			return _SetLEDs(ledData);
		}

		case KB_SET_KEY_REPEAT_RATE:
		{
			int32 repeatRate;
			if (user_memcpy(&repeatRate, buffer, sizeof(repeatRate)) != B_OK)
				return B_BAD_ADDRESS;

			if (repeatRate == 0 || repeatRate > 1000000)
				return B_BAD_VALUE;

			fRepeatRate = 10000000 / repeatRate;
			return B_OK;
		}

		case KB_GET_KEY_REPEAT_RATE:
		{
			int32 repeatRate = 10000000 / fRepeatRate;
			if (user_memcpy(buffer, &repeatRate, sizeof(repeatRate)) != B_OK)
				return B_BAD_ADDRESS;
			return B_OK;
		}

		case KB_SET_KEY_REPEAT_DELAY:
			if (user_memcpy(&fRepeatDelay, buffer, sizeof(fRepeatDelay))
					!= B_OK)
				return B_BAD_ADDRESS;
			return B_OK;

		case KB_GET_KEY_REPEAT_DELAY:
			if (user_memcpy(buffer, &fRepeatDelay, sizeof(fRepeatDelay))
					!= B_OK)
				return B_BAD_ADDRESS;
			return B_OK;

		case KB_SET_DEBUG_READER:
			if (fHasDebugReader)
				return B_BUSY;

			*cookie |= KEYBOARD_FLAG_DEBUGGER;
			fHasDebugReader = true;
			return B_OK;
	}

	TRACE_ALWAYS("keyboard device unhandled control 0x%08" B_PRIx32 "\n", op);
	return B_ERROR;
}


void
KeyboardProtocolHandler::_WriteKey(uint32 key, bool down)
{
	raw_key_info info;
	info.keycode = key;
	info.is_keydown = down;
	info.timestamp = system_time();
	RingBufferWrite(&info, sizeof(raw_key_info));
}


status_t
KeyboardProtocolHandler::_SetLEDs(uint8 *data)
{
	if (fOutputReport == NULL || fOutputReport->Device()->IsRemoved())
		return B_ERROR;

	for (uint32 i = 0; i < MAX_LEDS; i++) {
		if (fLEDs[i] == NULL)
			continue;

		fLEDs[i]->SetData(data[i]);
	}

	return fOutputReport->SendReport();
}


status_t
KeyboardProtocolHandler::_ReadReport(bigtime_t timeout)
{
	status_t result = fInputReport.WaitForReport(timeout);
	if (result != B_OK) {
		if (fInputReport.Device()->IsRemoved()) {
			TRACE("device has been removed\n");
			return B_ERROR;
		}

		if (result != B_TIMED_OUT && result != B_INTERRUPTED) {
			// we expect timeouts as we do repeat key handling this way,
			// interrupts happen when other reports come in on the same
			// endpoint
			TRACE_ALWAYS("error waiting for report: %s\n", strerror(result));
		}

		// signal that we simply want to try again
		return B_OK;
	}

	TRACE("got keyboard input report\n");

	uint8 modifiers = 0;
	for (uint32 i = 0; i < fModifierCount; i++) {
		HIDReportItem *modifier = fModifiers[i];
		if (modifier == NULL)
			break;

		if (modifier->Extract() == B_OK && modifier->Valid()) {
			modifiers |= (modifier->Data() & 1)
				<< (modifier->UsageID() - B_HID_UID_KB_LEFT_CONTROL);
		}
	}

	for (uint32 i = 0; i < fKeyCount; i++) {
		HIDReportItem *key = fKeys[i];
		if (key == NULL)
			break;

		if (key->Extract() == B_OK && key->Valid())
			fCurrentKeys[i] = key->Data();
		else
			fCurrentKeys[i] = 0;
	}

	fInputReport.DoneProcessing();

	static const uint32 kModifierTable[] = {
		KEY_ControlL,
		KEY_ShiftL,
		KEY_AltL,
		KEY_WinL,
		KEY_ControlR,
		KEY_ShiftR,
		KEY_AltR,
		KEY_WinR
	};

	// find modifier changes and push them into the buffer
	uint8 modifierChange = fLastModifiers ^ modifiers;
	for (uint8 i = 0; modifierChange; i++, modifierChange >>= 1) {
		if (modifierChange & 1)
			_WriteKey(kModifierTable[i], (modifiers >> i) & 1);
	}

	fLastModifiers = modifiers;

	static const uint32 kKeyTable[] = {
		0x00,	// ERROR
		0x00,	// ERROR
		0x00,	// ERROR
		0x00,	// ERROR
		0x3c,	// A
		0x50,	// B
		0x4e,	// C
		0x3e,	// D
		0x29,	// E
		0x3f,	// F
		0x40,	// G
		0x41,	// H
		0x2e,	// I
		0x42,	// J
		0x43,	// K
		0x44,	// L
		0x52,	// M
		0x51,	// N
		0x2f,	// O
		0x30,	// P
		0x27,	// Q
		0x2a,	// R
		0x3d,	// S
		0x2b,	// T
		0x2d,	// U
		0x4f,	// V
		0x28,	// W
		0x4d,	// X
		0x2c,	// Y
		0x4c,	// Z
		0x12,	// 1
		0x13,	// 2
		0x14,	// 3
		0x15,	// 4
		0x16,	// 5
		0x17,	// 6
		0x18,	// 7
		0x19,	// 8
		0x1a,	// 9
		0x1b,	// 0
		0x47,	// enter
		0x01,	// Esc
		0x1e,	// Backspace
		0x26,	// Tab
		0x5e,	// Space
		0x1c,	// -
		0x1d,	// =
		0x31,	// [
		0x32,	// ]
		0x33,	// backslash
		0x33,	// backslash
		0x45,	// ;
		0x46,	// '
		0x11,	// `
		0x53,	// ,
		0x54,	// .
		0x55,	// /
		KEY_CapsLock,	// Caps
		0x02,	// F1
		0x03,	// F2
		0x04,	// F3
		0x05,	// F4
		0x06,	// F5
		0x07,	// F6
		0x08,	// F7
		0x09,	// F8
		0x0a,	// F9
		0x0b,	// F10
		0x0c,	// F11
		0x0d,	// F12
		0x0e,	// PrintScreen
		KEY_Scroll,	// Scroll Lock
		KEY_Pause,	// Pause (0x7f with Ctrl)
		0x1f,	// Insert
		0x20,	// Home
		0x21,	// Page up
		0x34,	// Delete
		0x35,	// End
		0x36,	// Page down
		0x63,	// Right arrow
		0x61,	// Left arrow
		0x62,	// Down arrow
		0x57,	// Up arrow
		0x22,	// Num Lock
		0x23,	// Pad /
		0x24,	// Pad *
		0x25,	// Pad -
		0x3a,	// Pad +
		0x5b,	// Pad Enter
		0x58,	// Pad 1
		0x59,	// Pad 2
		0x5a,	// Pad 3
		0x48,	// Pad 4
		0x49,	// Pad 5
		0x4a,	// Pad 6
		0x37,	// Pad 7
		0x38,	// Pad 8
		0x39,	// Pad 9
		0x64,	// Pad 0
		0x65,	// Pad .
		0x69,	// <
		KEY_Menu,	// Menu
		KEY_Power,	// Power
		KEY_NumEqual,	// Pad =
		0x00,	// F13 unmapped
		0x00,	// F14 unmapped
		0x00,	// F15 unmapped
		0x00,	// F16 unmapped
		0x00,	// F17 unmapped
		0x00,	// F18 unmapped
		0x00,	// F19 unmapped
		0x00,	// F20 unmapped
		0x00,	// F21 unmapped
		0x00,	// F22 unmapped
		0x00,	// F23 unmapped
		0x00,	// F24 unmapped
		0x00,	// Execute unmapped
		0x00,	// Help unmapped
		0x00,	// Menu unmapped
		0x00,	// Select unmapped
		0x00,	// Stop unmapped
		0x00,	// Again unmapped
		0x00,	// Undo unmapped
		0x00,	// Cut unmapped
		0x00,	// Copy unmapped
		0x00,	// Paste unmapped
		0x00,	// Find unmapped
		0x00,	// Mute unmapped
		0x00,	// Volume up unmapped
		0x00,	// Volume down unmapped
		0x00,	// CapsLock unmapped
		0x00,	// NumLock unmapped
		0x00,	// Scroll lock unmapped
		0x70,	// Keypad . on Brazilian ABNT2
		0x00,	// = sign
		0x6b,	// Ro (\\ key, japanese)
		0x6e,	// Katakana/Hiragana, second key right to spacebar, japanese
		0x6a,	// Yen (macron key, japanese)
		0x6d,	// Henkan, first key right to spacebar, japanese
		0x6c,	// Muhenkan, key left to spacebar, japanese
	};

	static const size_t kKeyTableSize
		= sizeof(kKeyTable) / sizeof(kKeyTable[0]);

	bool phantomState = true;
	for (size_t i = 0; i < fKeyCount; i++) {
		if (fCurrentKeys[i] != 1
			|| fKeys[i]->UsagePage() != B_HID_USAGE_PAGE_KEYBOARD) {
			phantomState = false;
			break;
		}
	}

	if (phantomState) {
		// no valid key information is present in this state and we don't
		// want to overwrite our last buffer as otherwise we generate
		// spurious key ups now and spurious key downs when leaving the
		// phantom state again
		return B_OK;
	}

	static bool sysReqPressed = false;

	bool keyDown = false;
	uint16 *current = fLastKeys;
	uint16 *compare = fCurrentKeys;
	for (int32 twice = 0; twice < 2; twice++) {
		for (size_t i = 0; i < fKeyCount; i++) {
			if (current[i] == 0 || (current[i] == 1
				&& fKeys[i]->UsagePage() == B_HID_USAGE_PAGE_KEYBOARD))
				continue;

			bool found = false;
			for (size_t j = 0; j < fKeyCount; j++) {
				if (compare[j] == current[i]) {
					found = true;
					break;
				}
			}

			if (found)
				continue;

			// a change occured
			uint32 key = 0;
			if (fKeys[i]->UsagePage() == B_HID_USAGE_PAGE_KEYBOARD) {
				if (current[i] < kKeyTableSize)
					key = kKeyTable[current[i]];

				if (key == KEY_Pause && (modifiers & ALT_KEYS) != 0)
					key = KEY_Break;
				else if (key == 0xe && (modifiers & ALT_KEYS) != 0) {
					key = KEY_SysRq;
					sysReqPressed = keyDown;
				} else if (sysReqPressed && keyDown
					&& current[i] >= 4 && current[i] <= 29
					&& (fLastModifiers & ALT_KEYS) != 0) {
					// Alt-SysReq+letter was pressed
					sDebugKeyboardPipe
						= fInputReport.Device()->InterruptPipe();
					sDebugKeyboardReportSize
						= fInputReport.Parser()->MaxReportSize();

					char letter = current[i] - 4 + 'a';

					if (debug_emergency_key_pressed(letter)) {
						// we probably have lost some keys, so reset our key
						// state
						sysReqPressed = false;
						continue;
					}
				}
			}

			if (key == 0) {
				// unmapped normal key or consumer/button key
				key = fKeys[i]->UsageMinimum() + current[i];
			}

			_WriteKey(key, keyDown);

			if (keyDown) {
				// repeat handling
				fCurrentRepeatKey = key;
				fCurrentRepeatDelay = fRepeatDelay;
			} else {
				// cancel the repeats if they are for this key
				if (fCurrentRepeatKey == key) {
					fCurrentRepeatDelay = B_INFINITE_TIMEOUT;
					fCurrentRepeatKey = 0;
				}
			}
		}

		current = fCurrentKeys;
		compare = fLastKeys;
		keyDown = true;
	}

	memcpy(fLastKeys, fCurrentKeys, fKeyCount * sizeof(uint32));
	return B_OK;
}
