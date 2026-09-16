/*
 * Copyright 2026, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef EVDEV_KEYMAP_H
#define EVDEV_KEYMAP_H


#include <ATKeymap.h>
#include <usb/USB_hid_page_telephony.h>


#define HID_KB(code) ((B_HID_USAGE_PAGE_KEYBOARD << 16) | (B_HID_UID_KB_##code))
#define HID_KP(code) ((B_HID_USAGE_PAGE_KEYBOARD << 16) | (B_HID_UID_KP_##code))
#define HID_TEL(code) ((B_HID_USAGE_PAGE_TELEPHONY << 16) | (B_HID_UID_TEL_##code))


// Keymap reference:
// linux/include/uapi/linux/input-event-codes.h, commit 45065a5.
// HID reference:
// linux/drivers/hid/hid-input.c and include/linux/hid.h, commit d89f04a.
// Up to and including 83, these are AT keymap keys.
// 8-bit keycodes are in the table, if we ever need higher ones they will need another lookup
// method.
// Notes:
// * - Overloaded keys.
//     These are keys only that appear on one country's keyboard (Japan/Korea) or one maker (on
//     Macs), For example, ZENKAKU/HANKAKU (全角・半角) will produce ` on non-Japanese keymaps;
//     conversely, ` will trigger the zenkaku/hankaku function when Japanese keymap is active. Refer
//     to Japanese.keymap, Korean.keymap, US.keymap, keyboard.dox for all the oddities.
// ** - legacy, most likely not worth adding
// KP stands for keypad keys.
static const uint32 EvdevKeymap[] = {
	0x00, // unmapped         84
	0x11, // zenkaku/hankaku      * (JP)
	0x69, // <
	0x0c, // F11
	0x0d, // F12
	0x6b, // RO                   * (JP)
	HID_KB(LANG_3), // katakana   90  (JP)
	HID_KB(LANG_4), // hiragana       (JP)
	0x6d, // henkan
	0x6e, // katakana/hiragana
	0x6c, // muhenkan
	HID_KB(INTERNATIONAL_6), // KP jp comma  (JP)
	0x5b, // KP enter
	0x60, // Right Control
	0x23, // KP /
	0x0e, // Print Screen
	0x5f, // Right Alt        100
	0x00, // Linefeed             **
	0x20, // Home
	0x57, // Up Arrow
	0x21, // Page Up
	0x61, // Left Arrow
	0x63, // Right Arrow
	0x35, // End
	0x62, // Down Arrow
	0x36, // Page Down
	0x1f, // Insert           110
	0x34, // Delete
	0x00, // Macro                **
	HID_CONSUMER(MUTE),
	HID_CONSUMER(VOLUME_DECREMENT),
	HID_CONSUMER(VOLUME_INCREMENT),
	HID_GD(SYSTEM_POWER_DOWN),
	0x6a, // KP =					* (Mac)
	0x00, // KP +/-				**
	0x10, // Pause
	HID_CONSUMER(AC_DESKTOP_SHOW_ALL_WINDOWS), // 120
	0x70, // KP ,					* (BR)
	HID_KB(LANG_1), // Hangeul			(KO)
	HID_KB(LANG_2), // Hanja			(KO)
	0x6a, // Yen					* (JP)
	0x66, // Left Meta			* (Mac)
	0x67, // Right Meta			* (Mac)
	0x68, // Compose				* (Linux)

	// Sun keyboards keys
	HID_KB(STOP),
	HID_KB(AGAIN),
	HID_KB(MENU), //			130
	HID_KB(UNDO),
	HID_KB(SELECT),
	HID_KB(COPY),
	HID_KB(EXECUTE),
	HID_KB(PASTE),
	HID_KB(FIND),
	HID_KB(CUT),
	HID_KB(HELP),
	0x68, // Menu
	HID_CONSUMER(AL_CALCULATOR), // 140

	// Multimedia keys block
	0x00, // Setup				**
	HID_GD(SYSTEM_SLEEP), // Sleep
	HID_GD(SYSTEM_WAKE_UP), // Wake Up
	HID_CONSUMER(AL_LOCAL_MACHINE_BROWSER), // File ("My Computer")
	0x00, // Send File			**
	0x00, // Delete File			**
	0x00, // Transfer				**
	0x00, // Prog1				**
	0x00, // Prog2				**
	HID_CONSUMER(AL_INTERNET_BROWSER), // WWW		150
	0x00, // MS-DOS				**
	HID_CONSUMER(AL_TERMINAL_LOCK_SCREEN), // Coffee (lock/screensaver)
	0x00, // Rotate Display		** (tablet)
	0x00, // Cycle Windows		**
	HID_CONSUMER(AL_EMAIL_READER),
	HID_CONSUMER(AC_BOOKMARKS),
	0x00, // Computer				** (HID boards send File instead)
	HID_CONSUMER(AC_BACK),
	HID_CONSUMER(AC_FORWARD),
	0x00, // Close CD			160	**
	HID_CONSUMER(EJECT),
	0x00, // Eject/Close CD		**
	HID_CONSUMER(SCAN_NEXT_TRACK),
	HID_CONSUMER(PLAY_PAUSE),
	HID_CONSUMER(SCAN_PREVIOUS_TRACK),
	HID_CONSUMER(STOP), // Stop CD
	HID_CONSUMER(RECORD),
	HID_CONSUMER(REWIND),
	HID_CONSUMER(MEDIA_SELECT_TELEPHONE),
	0x00, // ISO				170	**
	HID_CONSUMER(AL_CONSUMER_CONTROL_CONFIGURATION),
	HID_CONSUMER(AC_HOME),
	HID_CONSUMER(AC_REFRESH),
	HID_CONSUMER(AC_EXIT),
	0x00, // Move					**
	HID_CONSUMER(AC_EDIT),
	HID_CONSUMER(AC_SCROLL_UP),
	HID_CONSUMER(AC_SCROLL_DOWN),
	HID_KP(LPAREN), // KP (
	HID_KP(RPAREN), // KP )			180
	HID_CONSUMER(AC_NEW),
	HID_CONSUMER(AC_REDO_REPEAT),

	// Extended function key block
	HID_KB(F13),
	HID_KB(F14),
	HID_KB(F15),
	HID_KB(F16),
	HID_KB(F17),
	HID_KB(F18),
	HID_KB(F19),
	HID_KB(F20), // 190
	HID_KB(F21),
	HID_KB(F22),
	HID_KB(F23),
	HID_KB(F24),

	0x00,
	0x00,
	0x00,
	0x00,
	0x00, // 195-199 unused in evdev

	// Second multimedia keys block
	0x00, // Play CD			200	** (superseded by KEY_PLAY)
	0x00, // Pause CD				** (superseded by KEY_PAUSE)
	0x00, // Prog3				**
	0x00, // Prog4				**
	HID_CONSUMER(AC_DESKTOP_SHOW_ALL_APPLICATIONS),
	0x00, // Suspend				**
	HID_CONSUMER(AC_CLOSE),
	HID_CONSUMER(PLAY),
	HID_CONSUMER(FAST_FORWARD),
	HID_CONSUMER(BASS_BOOST),
	HID_CONSUMER(AC_PRINT), // 	210
	0x00, // HP					** (HP)
	HID_CONSUMER(SNAPSHOT),
	0x00, // Sound				**
	0x00, // Question				**
	0x00, // Email				** (superseded by Mail)
	HID_CONSUMER(AL_NETWORK_CHAT),
	HID_CONSUMER(AC_SEARCH),
	0x00, // Connect				**
	HID_CONSUMER(AL_CHECKBOOK_FINANCE),
	0x00, // Sport			220	**
	0x00, // Shop					**
	0x00, // Alt Erase			**
	HID_CONSUMER(AC_CANCEL),
	HID_CONSUMER(DISPLAY_BRIGHTNESS_DECREMENT),
	HID_CONSUMER(DISPLAY_BRIGHTNESS_INCREMENT),
	0x00, // Media				** (superseded by Config)
	HID_GD(SYSTEM_DISPLAY_TOGGLE), // Switch Video Mode
	HID_CONSUMER(KEYBOARD_BACKLIGHT_OOC), // Kbd Illum Toggle
	HID_CONSUMER(KEYBOARD_BRIGHTNESS_DECREMENT), // Kbd Illum Down
	HID_CONSUMER(KEYBOARD_BRIGHTNESS_INCREMENT), // Kbd Illum Up	230
	HID_CONSUMER(AC_SEND), // Send
	HID_CONSUMER(AC_REPLY), // Reply
	HID_CONSUMER(AC_FORWARD_MSG), // Forward Mail
	HID_CONSUMER(AC_SAVE), // Save
	HID_CONSUMER(AL_DOCUMENTS), // Documents
	0x00, // Battery				**
	0x00, // Bluetooth			**
	0x00, // WLAN					**
	0x00, // UWB					**
	0x00, // Unknown			240
	HID_CONSUMER(MODE_SETUP), // Video Next (in HUT, "Mode Setup")
	0x00, // Video Prev			**
	0x00, // Brightness Cycle		**
	HID_CONSUMER(DISPLAY_SET_AUTO_BRIGHTNESS), // Brightness Auto
	0x00, // Display Off			**
	0x00, // WWAN					**
	HID_GD(WIRELESS_RADIO_BUTTON), // RF Kill
	HID_TEL(PHONE_MUTE), // Mic Mute (Phone Mute)
};


/*! Convert a key in evdev keymap to equivalent BeOS/Haiku key.  */
static inline uint32
evdev_to_haiku_keymap(uint16 evdevKey)
{
	if (evdevKey == 0)
		return 0x00;
	if (evdevKey < 84)
		return kATKeycodeMap[evdevKey - 1];
	if (static_cast<size_t>(evdevKey - 84) >= B_COUNT_OF(EvdevKeymap))
		return 0x00;
	return EvdevKeymap[evdevKey - 84];
}


#endif // EVDEV_KEYMAP_H
