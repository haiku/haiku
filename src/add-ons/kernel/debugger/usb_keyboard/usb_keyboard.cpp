/*
 * Copyright 2009-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <debug.h>
#include <debugger_keymaps.h>

static bool sUseUSBKeyboard = false;
static uint8 sUSBTransferData[64];
static uint8 sLastTransferData[64];
static size_t sUSBTransferLength = 0;
static void *sUSBPipe = NULL;

// simple ring buffer
static int sBufferedChars[32];
static uint8 sBufferSize = sizeof(sBufferedChars) / sizeof(sBufferedChars[0]);
static uint8 sBufferedCharCount = 0;
static uint8 sBufferWriteIndex = 0;
static uint8 sBufferReadIndex = 0;

#define MODIFIER_CONTROL	0x01
#define MODIFIER_SHIFT		0x02
#define MODIFIER_ALT		0x04

static uint32 sModifierTable[] = {
	MODIFIER_CONTROL,
	MODIFIER_SHIFT,
	MODIFIER_ALT,
	0,
	MODIFIER_CONTROL,
	MODIFIER_SHIFT,
	MODIFIER_ALT,
	0
};

static uint8 sKeyTable[] = {
	0,	// ERROR
	0,	// ERROR
	0,	// ERROR
	0,	// ERROR
	30,	// A
	48,	// B
	46,	// C
	32,	// D
	18,	// E
	33,	// F
	34,	// G
	35,	// H
	23,	// I
	36,	// J
	37,	// K
	38,	// L
	50,	// M
	49,	// N
	24,	// O
	25,	// P
	16,	// Q
	19,	// R
	31,	// S
	20,	// T
	22,	// U
	47,	// V
	17,	// W
	45,	// X
	21,	// Y
	44,	// Z
	2,	// 1
	3,	// 2
	4,	// 3
	5,	// 4
	6,	// 5
	7,	// 6
	8,	// 7
	9,	// 8
	10,	// 9
	11,	// 0
	28,	// enter
	1,	// Esc
	14,	// Backspace
	15,	// Tab
	57,	// Space
	12,	// -
	13,	// =
	26,	// [
	27,	// ]
	43,	// backslash
	80,	// backslash
	39,	// ;
	40,	// '
	41,	// `
	51,	// ,
	52,	// .
	53,	// /
	0,	// Caps
	0,	// F1
	0,	// F2
	0,	// F3
	0,	// F4
	0,	// F5
	0,	// F6
	0,	// F7
	0,	// F8
	0,	// F9
	0,	// F10
	0,	// F11
	0,	// F12
	0,	// PrintScreen
	0,	// Scroll Lock
	0,	// Pause (0x7f with Ctrl)
	0,	// Insert
	0x80 | 'H',	// Home
	0x80 | '5',	// Page up
	0x80 | '3',	// Delete
	0x80 | 'F',	// End
	0x80 | '6',	// Page down
	0x80 | 'C',	// Right arrow
	0x80 | 'D',	// Left arrow
	0x80 | 'B',	// Down arrow
	0x80 | 'A',	// Up arrow
	0,	// Num Lock
	53,	// Pad /
	55,	// Pad *
	12,	// Pad -
	54,	// Pad +
	28,	// Pad Enter
	2,	// Pad 1
	3,	// Pad 2
	4,	// Pad 3
	5,	// Pad 4
	6,	// Pad 5
	7,	// Pad 6
	8,	// Pad 7
	9,	// Pad 8
	10,	// Pad 9
	11,	// Pad 0
	52,	// Pad .
	86,	// <
	0,	// Menu
	0,	// Power
	13	// Pad =
};

static size_t sKeyTableSize = sizeof(sKeyTable) / sizeof(sKeyTable[0]);


static void
enter_debugger(void)
{
	if (!has_debugger_command("get_usb_keyboard_config")
		|| !has_debugger_command("get_usb_pipe_for_id")
		|| (!has_debugger_command("uhci_process_transfer")
			&& !has_debugger_command("ohci_process_transfer")))
		return;

	unset_debug_variable("_usbPipe");
	unset_debug_variable("_usbReportSize");

	evaluate_debug_command("get_usb_keyboard_config");
	sUSBTransferLength = get_debug_variable("_usbReportSize", 0);
	if (sUSBTransferLength == 0 || sUSBTransferLength > sizeof(sUSBTransferData))
		return;

	evaluate_debug_command("get_usb_pipe_for_id");
	sUSBPipe = (void *)get_debug_variable("_usbPipe", 0);
	if (sUSBPipe == NULL)
		return;

	sUseUSBKeyboard = true;
}


static void
exit_debugger(void)
{
	if (sUseUSBKeyboard) {
		// make sure a possibly pending transfer is canceled
		set_debug_variable("_usbPipe", (uint64)sUSBPipe);
		evaluate_debug_command("uhci_process_transfer cancel");

		sUseUSBKeyboard = false;
	}
}


static void
write_key(int key)
{
	sBufferedChars[sBufferWriteIndex++] = key;
	sBufferWriteIndex %= sBufferSize;
	sBufferedCharCount++;
}


static int
debugger_getchar(void)
{
	if (!sUseUSBKeyboard)
		return -1;

	if (sBufferedCharCount == 0) {
		set_debug_variable("_usbPipe", (uint64)sUSBPipe);
		set_debug_variable("_usbTransferData", (uint64)sUSBTransferData);
		set_debug_variable("_usbTransferLength", (uint64)sUSBTransferLength);
		if (evaluate_debug_command("uhci_process_transfer") != 0)
			return -1;

		bool phantomState = true;
		for (size_t i = 2; i < sUSBTransferLength; i++) {
			if (sUSBTransferData[i] != 0x01) {
				phantomState = false;
				break;
			}
		}

		if (phantomState)
			return -1;

		uint8 modifiers = 0;
		for (uint32 i = 0; i < 8; i++) {
			if (sUSBTransferData[0] & (1 << i))
				modifiers |= sModifierTable[i];
		}

		uint8 *current = sUSBTransferData;
		uint8 *compare = sLastTransferData;
		for (uint32 i = 2; i < sUSBTransferLength; i++) {
			if (current[i] == 0x00 || current[i] == 0x01)
				continue;

			bool found = false;
			for (uint32 j = 2; j < sUSBTransferLength; j++) {
				if (compare[j] == current[i]) {
					found = true;
					break;
				}
			}

			if (found)
				continue;

			if (current[i] >= sKeyTableSize)
				continue;

			int result = -1;
			uint8 key = sKeyTable[current[i]];
			if (key & 0x80) {
				write_key(27);
				write_key('[');

				key &= ~0x80;
				write_key(key);

				if (key == '5' || key == '6' || key == '3')
					write_key('~');

				continue;
			} else if (modifiers & MODIFIER_CONTROL) {
				char c = kShiftedKeymap[key];
				if (c >= 'A' && c <= 'Z')
					result = 0x1f & c;
			} else if (modifiers & MODIFIER_ALT)
				result = kAltedKeymap[key];
			else if (modifiers & MODIFIER_SHIFT)
				result = kShiftedKeymap[key];
			else
				result = kUnshiftedKeymap[key];

			if (result < 0)
				continue;

			write_key(result);
		}

		for (uint32 i = 0; i < sUSBTransferLength; i++)
			sLastTransferData[i] = sUSBTransferData[i];
	}

	if (sBufferedCharCount == 0)
		return -1;

	int result = sBufferedChars[sBufferReadIndex++];
	sBufferReadIndex %= sBufferSize;
	sBufferedCharCount--;
	return result;
}


static status_t
std_ops(int32 op, ...)
{
	if (op == B_MODULE_INIT || op == B_MODULE_UNINIT)
		return B_OK;

	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/usb_keyboard/v1",
		0,
		&std_ops
	},

	&enter_debugger,
	&exit_debugger,
	NULL,
	&debugger_getchar
};

module_info *modules[] = {
	(module_info *)&sModuleInfo,
	NULL
};

