/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sandor Vroemisse
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include "Keymap.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>
#include <File.h>

#include <input_globals.h>


static const uint32 kModifierKeys = B_SHIFT_KEY | B_CAPS_LOCK | B_CONTROL_KEY
	| B_OPTION_KEY | B_COMMAND_KEY | B_MENU_KEY;


static void
print_key(char *chars, int32 offset)
{
	int size = chars[offset++];

	switch (size) {
		case 0:
			// Not mapped
			fputs("N/A", stdout);
			break;

		case 1:
			// single-byte UTF-8/ASCII character
			fputc(chars[offset], stdout);
			break;

		default:
		{
			// 2-, 3-, or 4-byte UTF-8 character
			char *str = new char[size + 1];
			strncpy(str, &chars[offset], size);
			str[size] = 0;
			fputs(str, stdout);
			delete[] str;
			break;
		}
	}

	fputs("\t", stdout);
}


//	#pragma mark -


Keymap::Keymap()
	:
	fModificationMessage(NULL)
{
}


Keymap::~Keymap()
{
	delete fModificationMessage;
}


void
Keymap::SetTarget(BMessenger target, BMessage* modificationMessage)
{
	delete fModificationMessage;

	fTarget = target;
	fModificationMessage = modificationMessage;
}


void
Keymap::SetName(const char* name)
{
	strlcpy(fName, name, sizeof(fName));
}


void
Keymap::DumpKeymap()
{
	if (fKeys.version != 3)
		return;

	// Print a chart of the normal, shift, control, option, option+shift,
	// Caps, Caps+shift, Caps+option, and Caps+option+shift keys.
	puts("Key #\tn\ts\tc\to\tos\tC\tCs\tCo\tCos\n");

	for (uint8 i = 0; i < 128; i++) {
		printf(" 0x%02x\t", i);
		print_key(fChars, fKeys.normal_map[i]);
		print_key(fChars, fKeys.shift_map[i]);
		print_key(fChars, fKeys.control_map[i]);
		print_key(fChars, fKeys.option_map[i]);
		print_key(fChars, fKeys.option_shift_map[i]);
		print_key(fChars, fKeys.caps_map[i]);
		print_key(fChars, fKeys.caps_shift_map[i]);
		print_key(fChars, fKeys.option_caps_map[i]);
		print_key(fChars, fKeys.option_caps_shift_map[i]);
		fputs("\n", stdout);
	}
}


//!	Load a map from a file
status_t
Keymap::Load(const entry_ref& ref)
{
	BEntry entry;
	status_t status = entry.SetTo(&ref, true);
	if (status != B_OK)
		return status;

	BFile file(&entry, B_READ_ONLY);
	status = SetTo(file);
	if (status != B_OK)
		return status;

	// fetch name from attribute and fall back to filename

	ssize_t bytesRead = file.ReadAttr("keymap:name", B_STRING_TYPE, 0, fName,
		sizeof(fName));
	if (bytesRead > 0)
		fName[bytesRead] = '\0';
	else
		strlcpy(fName, ref.name, sizeof(fName));

	return B_OK;
}


//!	We save a map to a file
status_t
Keymap::Save(const entry_ref& ref)
{
	BFile file;
	status_t status = file.SetTo(&ref,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK) {
		printf("error %s\n", strerror(status));
		return status;
	}

	for (uint32 i = 0; i < sizeof(fKeys) / 4; i++)
		((uint32*)&fKeys)[i] = B_HOST_TO_BENDIAN_INT32(((uint32*)&fKeys)[i]);

	ssize_t bytesWritten = file.Write(&fKeys, sizeof(fKeys));
	if (bytesWritten < (ssize_t)sizeof(fKeys))
		status = bytesWritten < 0 ? bytesWritten : B_IO_ERROR;

	for (uint32 i = 0; i < sizeof(fKeys) / 4; i++)
		((uint32*)&fKeys)[i] = B_BENDIAN_TO_HOST_INT32(((uint32*)&fKeys)[i]);

	if (status == B_OK) {
		fCharsSize = B_HOST_TO_BENDIAN_INT32(fCharsSize);

		bytesWritten = file.Write(&fCharsSize, sizeof(uint32));
		if (bytesWritten < (ssize_t)sizeof(uint32))
			status = bytesWritten < 0 ? bytesWritten : B_IO_ERROR;

		fCharsSize = B_BENDIAN_TO_HOST_INT32(fCharsSize);
	}

	if (status == B_OK) {
		bytesWritten = file.Write(fChars, fCharsSize);
		if (bytesWritten < (ssize_t)fCharsSize)
			status = bytesWritten < 0 ? bytesWritten : B_IO_ERROR;
	}

	if (status == B_OK) {
		file.WriteAttr("keymap:name", B_STRING_TYPE, 0, fName, strlen(fName));
			// Failing would be non-fatal
	}

	return status;
}


status_t
Keymap::SetModifier(uint32 keyCode, uint32 modifier)
{
	const uint32 kSingleModifierKeys = B_LEFT_SHIFT_KEY | B_RIGHT_SHIFT_KEY
		| B_LEFT_COMMAND_KEY | B_RIGHT_COMMAND_KEY | B_LEFT_CONTROL_KEY
		| B_RIGHT_CONTROL_KEY | B_LEFT_OPTION_KEY | B_RIGHT_OPTION_KEY;

	if ((modifier & kSingleModifierKeys) != 0)
		modifier &= kSingleModifierKeys;
	else if ((modifier & kModifierKeys) != 0)
		modifier &= kModifierKeys;

	if (modifier == B_CAPS_LOCK)
		fKeys.caps_key = keyCode;
	else if (modifier == B_NUM_LOCK)
		fKeys.num_key = keyCode;
	else if (modifier == B_SCROLL_LOCK)
		fKeys.scroll_key = keyCode;
	else if (modifier == B_LEFT_SHIFT_KEY)
		fKeys.left_shift_key = keyCode;
	else if (modifier == B_RIGHT_SHIFT_KEY)
		fKeys.right_shift_key = keyCode;
	else if (modifier == B_LEFT_COMMAND_KEY)
		fKeys.left_command_key = keyCode;
	else if (modifier == B_RIGHT_COMMAND_KEY)
		fKeys.right_command_key = keyCode;
	else if (modifier == B_LEFT_CONTROL_KEY)
		fKeys.left_control_key = keyCode;
	else if (modifier == B_RIGHT_CONTROL_KEY)
		fKeys.right_control_key = keyCode;
	else if (modifier == B_LEFT_OPTION_KEY)
		fKeys.left_option_key = keyCode;
	else if (modifier == B_RIGHT_OPTION_KEY)
		fKeys.right_option_key = keyCode;
	else if (modifier == B_MENU_KEY)
		fKeys.menu_key = keyCode;
	else
		return B_BAD_VALUE;

	if (fModificationMessage != NULL)
		fTarget.SendMessage(fModificationMessage);

	return B_OK;
}


//! Enables/disables the "deadness" of the given keycode/modifier combo.
void
Keymap::SetDeadKeyEnabled(uint32 keyCode, uint32 modifiers, bool enabled)
{
	uint32 tableMask = 0;
	int32 offset = Offset(keyCode, modifiers, &tableMask);
	uint8 deadKeyIndex = DeadKeyIndex(offset);
	if (deadKeyIndex > 0) {
		uint32* deadTables[] = {
			&fKeys.acute_tables,
			&fKeys.grave_tables,
			&fKeys.circumflex_tables,
			&fKeys.dieresis_tables,
			&fKeys.tilde_tables
		};

		if (enabled)
			(*deadTables[deadKeyIndex - 1]) |= tableMask;
		else
			(*deadTables[deadKeyIndex - 1]) &= ~tableMask;

		if (fModificationMessage != NULL)
			fTarget.SendMessage(fModificationMessage);
	}
}


/*! Returns the trigger character string that is currently set for the dead
	key with the given index (which is 1..5).
*/
void
Keymap::GetDeadKeyTrigger(dead_key_index deadKeyIndex, BString& outTrigger)
{
	outTrigger = "";
	if (deadKeyIndex < 1 || deadKeyIndex > 5)
		return;

	int32 deadOffsets[] = {
		fKeys.acute_dead_key[1],
		fKeys.grave_dead_key[1],
		fKeys.circumflex_dead_key[1],
		fKeys.dieresis_dead_key[1],
		fKeys.tilde_dead_key[1]
	};

	int32 offset = deadOffsets[deadKeyIndex - 1];
	if (offset < 0 || offset >= (int32)fCharsSize)
		return;

	uint32 deadNumBytes = fChars[offset];
	if (!deadNumBytes)
		return;

	outTrigger.SetTo(&fChars[offset + 1], deadNumBytes);
}


/*! Sets the trigger character string that shall be used for the dead key
	with the given index (which is 1..5).
*/
void
Keymap::SetDeadKeyTrigger(dead_key_index deadKeyIndex, const BString& trigger)
{
	if (deadKeyIndex < 1 || deadKeyIndex > 5)
		return;

	int32 deadOffsets[] = {
		fKeys.acute_dead_key[1],
		fKeys.grave_dead_key[1],
		fKeys.circumflex_dead_key[1],
		fKeys.dieresis_dead_key[1],
		fKeys.tilde_dead_key[1]
	};

	int32 offset = deadOffsets[deadKeyIndex - 1];
	if (offset < 0 || offset >= (int32)fCharsSize)
		return;

	if (_SetChars(offset, trigger.String(), trigger.Length())) {
		// reset modifier table such that new dead key is enabled wherever
		// it is available
		uint32* deadTables[] = {
			&fKeys.acute_tables,
			&fKeys.grave_tables,
			&fKeys.circumflex_tables,
			&fKeys.dieresis_tables,
			&fKeys.tilde_tables
		};
		*deadTables[deadKeyIndex - 1]
			= B_NORMAL_TABLE | B_SHIFT_TABLE | B_CONTROL_TABLE | B_OPTION_TABLE
				| B_OPTION_SHIFT_TABLE | B_CAPS_TABLE | B_CAPS_SHIFT_TABLE
				| B_OPTION_CAPS_TABLE | B_OPTION_CAPS_SHIFT_TABLE;

		if (fModificationMessage != NULL)
			fTarget.SendMessage(fModificationMessage);
	}
}


//! We make our input server use the map in /boot/home/config/settings/Keymap
status_t
Keymap::Use()
{
	return _restore_key_map_();
}


void
Keymap::SetKey(uint32 keyCode, uint32 modifiers, int8 deadKey,
	const char* bytes, int32 numBytes)
{
	int32 offset = Offset(keyCode, modifiers);
	if (offset < 0)
		return;

	if (numBytes == -1)
		numBytes = strlen(bytes);
	if (numBytes > 6)
		return;

	if (_SetChars(offset, bytes, numBytes)) {
		if (fModificationMessage != NULL)
			fTarget.SendMessage(fModificationMessage);
	}
}


Keymap&
Keymap::operator=(const Keymap& other)
{
	if (this == &other)
		return *this;

	delete[] fChars;
	delete fModificationMessage;

	fChars = new(std::nothrow) char[other.fCharsSize];
	if (fChars != NULL) {
		memcpy(fChars, other.fChars, other.fCharsSize);
		fCharsSize = other.fCharsSize;
	} else
		fCharsSize = 0;

	memcpy(&fKeys, &other.fKeys, sizeof(key_map));
	strlcpy(fName, other.fName, sizeof(fName));

	fTarget = other.fTarget;

	if (other.fModificationMessage != NULL)
		fModificationMessage = new BMessage(*other.fModificationMessage);

	return *this;
}


bool
Keymap::_SetChars(int32 offset, const char* bytes, int32 numBytes)
{
	int32 oldNumBytes = fChars[offset];

	if (oldNumBytes == numBytes
		&& !memcmp(&fChars[offset + 1], bytes, numBytes)) {
		// nothing to do
		return false;
	}

	int32 diff = numBytes - oldNumBytes;
	if (diff != 0) {
		fCharsSize += diff;

		if (diff > 0) {
			// make space for the new data
			char* chars = new(std::nothrow) char[fCharsSize];
			if (chars != NULL) {
				memcpy(chars, fChars, offset + oldNumBytes + 1);
				memcpy(&chars[offset + 1 + numBytes],
					&fChars[offset + 1 + oldNumBytes],
					fCharsSize - 2 - offset - diff);
				delete[] fChars;
				fChars = chars;
			} else
				return false;
		} else if (diff < 0) {
			// shrink table
			memmove(&fChars[offset + numBytes], &fChars[offset + oldNumBytes],
				fCharsSize - offset - 2 - diff);
		}

		// update offsets
		int32* data = fKeys.control_map;
		int32 size = sizeof(fKeys.control_map) / 4 * 9
			+ sizeof(fKeys.acute_dead_key) / 4 * 5;
		for (int32 i = 0; i < size; i++) {
			if (data[i] > offset)
				data[i] += diff;
		}
	}

	memcpy(&fChars[offset + 1], bytes, numBytes);
	fChars[offset] = numBytes;

	return true;
}
