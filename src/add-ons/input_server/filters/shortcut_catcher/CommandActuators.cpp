/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Fredrik Modéen
 */


#include "CommandActuators.h"


#include <stdio.h>
#include <stdlib.h>


#include <String.h>
#include <Roster.h>
#include <Alert.h>
#include <Screen.h>
#include <Rect.h>
#include <View.h>
#include <Directory.h>
#include <Entry.h>
#include <List.h>
#include <Beep.h>


#include "ParseCommandLine.h"
#include "KeyInfos.h"

#define IS_KEY_DOWN(msg) ((msg->what == B_KEY_DOWN) \
	|| (msg->what == B_UNMAPPED_KEY_DOWN))

// Factory function
CommandActuator*
CreateCommandActuator(const char* command)
{
	CommandActuator* act = NULL;
	int32 argc;
	char** argv = ParseArgvFromString(command, argc);
	if (command[0] == '*') {
		if (argc > 0) {
			char* c = argv[0] + 1;
			if (strcmp(c, "InsertString") == 0)
				act = new KeyStrokeSequenceCommandActuator(argc, argv);
			else if (strcmp(c, "MoveMouse") == 0)
				act = new MoveMouseByCommandActuator(argc, argv);
			else if (strcmp(c, "MoveMouseTo") == 0)
				act = new MoveMouseToCommandActuator(argc, argv);
			else if (strcmp(c, "MouseButton") == 0)
				act = new MouseButtonCommandActuator(argc, argv);
			else if (strcmp(c, "LaunchHandler") == 0)
				act = new MIMEHandlerCommandActuator(argc, argv);
			else if (strcmp(c, "Multi") == 0)
				act = new MultiCommandActuator(argc, argv);
			else if (strcmp(c, "MouseDown") == 0)
				act = new MouseDownCommandActuator(argc, argv);
			else if (strcmp(c, "MouseUp") == 0)
				act = new MouseUpCommandActuator(argc, argv);
			else if (strcmp(c, "SendMessage") == 0)
				act = new SendMessageCommandActuator(argc, argv);
			else
				act = new BeepCommandActuator(argc, argv);
		}
	} else
		act = new LaunchCommandActuator(argc, argv);

	FreeArgv(argv);
	return act;
}


///////////////////////////////////////////////////////////////////////////////
//
// CommandActuator
//
///////////////////////////////////////////////////////////////////////////////
CommandActuator::CommandActuator(int32 argc, char** argv)
{
	// empty
}


CommandActuator::CommandActuator(BMessage* from)
	:
	BArchivable(from)
{
	// empty
}


status_t
CommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);
	return ret;
}


///////////////////////////////////////////////////////////////////////////////
//
// LaunchCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
LaunchCommandActuator::LaunchCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv),
	fArgv(CloneArgv(argv)),
	fArgc(argc)
{
	// empty
}


LaunchCommandActuator::LaunchCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	BList argList;
	const char* temp;
	int idx = 0;
	while (from->FindString("largv", idx++, &temp) == B_NO_ERROR) {
		if (temp) {
			char* copy = new char[strlen(temp) + 1];
			strcpy(copy, temp);
			argList.AddItem(copy);
		}
	}

	fArgc = argList.CountItems();
	fArgv = new char*[fArgc+ 1];

	for (int i = 0; i < fArgc; i++)
		fArgv[i] = (char*) argList.ItemAt(i);

	fArgv[fArgc] = NULL;// terminate the array
}


LaunchCommandActuator::~LaunchCommandActuator()
{
	FreeArgv(fArgv);
}


filter_result
LaunchCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg)) {
		// cause KeyEventAsync() to be called asynchronously
		*setAsyncData = (void*) true;
	}
	return B_SKIP_MESSAGE;
}


status_t
LaunchCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = CommandActuator::Archive(into, deep);

	for (int i = 0; i < fArgc; i++)
		into->AddString("largv", fArgv[i]);

	return ret;
}


BArchivable*
LaunchCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "LaunchCommandActuator"))
		return new LaunchCommandActuator(from);
	else
		return NULL;
}


void
LaunchCommandActuator::KeyEventAsync(const BMessage* keyMsg,
	void* asyncData)
{
	if (be_roster) {
		status_t err = B_OK;
		BString str;
		BString str1("Shortcuts launcher error");
		if (fArgc < 1)
			str << "You didn't specify a command for this hotkey.";
		else if ((err = LaunchCommand(fArgv, fArgc)) != B_NO_ERROR) {
			str << "Can't launch " << fArgv[0];
			str << ", no such file exists.";
			str << " Please check your Shortcuts settings.";
		}

		if (fArgc < 1 || err != B_NO_ERROR)
			(new BAlert(str1.String(), str.String(), "OK"))->Go(NULL);
	}
}


///////////////////////////////////////////////////////////////////////////////
//
// MouseCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
MouseCommandActuator::MouseCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv),
	fWhichButtons(B_PRIMARY_MOUSE_BUTTON)
{
	if (argc > 1) {
		fWhichButtons = 0;

		for (int i = 1; i < argc; i++) {
			int buttonNumber = atoi(argv[i]);

			switch(buttonNumber) {
				case 1:
					fWhichButtons |= B_PRIMARY_MOUSE_BUTTON;
				break;
				case 2:
					fWhichButtons |= B_SECONDARY_MOUSE_BUTTON;
				break;
				case 3:
					fWhichButtons |= B_TERTIARY_MOUSE_BUTTON;
				break;
			}
		}
	}
}


MouseCommandActuator::MouseCommandActuator(BMessage* from)
	:
	CommandActuator(from),
	fWhichButtons(B_PRIMARY_MOUSE_BUTTON)
{
	from->FindInt32("buttons", &fWhichButtons);
}


MouseCommandActuator::~MouseCommandActuator()
{
	// empty
}


status_t
MouseCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = CommandActuator::Archive(into, deep);
	into->AddInt32("buttons", fWhichButtons);
	return ret;
}


int32
MouseCommandActuator::_GetWhichButtons() const
{
	return fWhichButtons;
}


void
MouseCommandActuator::_GenerateMouseButtonEvent(bool mouseDown,
	const BMessage* keyMsg, BList* outlist, BMessage* lastMouseMove)
{
	BMessage* fakeMouse = new BMessage(*lastMouseMove);
	fakeMouse->what = mouseDown ? B_MOUSE_DOWN : B_MOUSE_UP;

	// Update the buttons to reflect which mouse buttons we are faking
	fakeMouse->RemoveName("buttons");

	if (mouseDown)
		fakeMouse->AddInt32("buttons", fWhichButtons);

	// Trey sez you gotta keep then "when"'s increasing if you want
	// click & drag to work!
	int64 when;

	const BMessage* lastMessage;

	if (outlist->CountItems() > 0) {
		int nr = outlist->CountItems() - 1;
		lastMessage = (const BMessage*)outlist->ItemAt(nr);
	} else
		lastMessage =keyMsg;

	if (lastMessage->FindInt64("when", &when) == B_NO_ERROR) {
		when++;
		fakeMouse->RemoveName("when");
		fakeMouse->AddInt64("when", when);
	}
	outlist->AddItem(fakeMouse);
}


///////////////////////////////////////////////////////////////////////////////
//
// MouseDownCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
MouseDownCommandActuator::MouseDownCommandActuator(int32 argc, char** argv)
	:
	MouseCommandActuator(argc, argv)
{
	// empty
}


MouseDownCommandActuator::MouseDownCommandActuator(BMessage* from)
	:
	MouseCommandActuator(from)
{
	// empty
}


MouseDownCommandActuator::~MouseDownCommandActuator()
{
	// empty
}


filter_result
MouseDownCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg))
		_GenerateMouseButtonEvent(true, keyMsg, outlist, lastMouseMove);

	return B_DISPATCH_MESSAGE;
}


status_t
MouseDownCommandActuator::Archive(BMessage* into, bool deep) const
{
	return MouseCommandActuator::Archive(into, deep);
}


BArchivable*
MouseDownCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MouseDownCommandActuator"))
		return new MouseDownCommandActuator(from);
	else
		return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// MouseUpCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
MouseUpCommandActuator::MouseUpCommandActuator(int32 argc, char** argv)
	:
	MouseCommandActuator(argc, argv)
{
	// empty
}


MouseUpCommandActuator::MouseUpCommandActuator(BMessage* from)
	:
	MouseCommandActuator(from)
{
	// empty
}


MouseUpCommandActuator::~MouseUpCommandActuator()
{
	// empty
}


filter_result
MouseUpCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg))
		_GenerateMouseButtonEvent(false, keyMsg, outlist, lastMouseMove);
	return B_DISPATCH_MESSAGE;
}


status_t
MouseUpCommandActuator::Archive(BMessage* into, bool deep) const
{
	return MouseCommandActuator::Archive(into, deep);
}


BArchivable*
MouseUpCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MouseUpCommandActuator"))
		return new MouseUpCommandActuator(from);
	else
		return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// MouseButtonCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
MouseButtonCommandActuator::MouseButtonCommandActuator(int32 argc, char** argv)
	:
	MouseCommandActuator(argc, argv),
	fKeyDown(false)
{
	// empty
}


MouseButtonCommandActuator::MouseButtonCommandActuator(BMessage* from)
	:
	MouseCommandActuator(from),
	fKeyDown(false)
{
	// empty
}


MouseButtonCommandActuator::~MouseButtonCommandActuator()
{
	// empty
}


filter_result
MouseButtonCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg) != fKeyDown) {
		_GenerateMouseButtonEvent(IS_KEY_DOWN(keyMsg), keyMsg, outlist,
			lastMouseMove);
		fKeyDown = IS_KEY_DOWN(keyMsg);
		return B_DISPATCH_MESSAGE;
	} else
		// This will handle key-repeats, which we don't want turned into lots
		// of B_MOUSE_DOWN messages.
		return B_SKIP_MESSAGE;
}


status_t
MouseButtonCommandActuator::Archive(BMessage* into, bool deep) const
{
	return MouseCommandActuator::Archive(into, deep);
}


BArchivable*
MouseButtonCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MouseButtonCommandActuator"))
		return new MouseButtonCommandActuator(from);
	else
		return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// KeyStrokeSequenceCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
KeyStrokeSequenceCommandActuator::KeyStrokeSequenceCommandActuator(int32 argc,
	char** argv)
	:
	CommandActuator(argc, argv)
{
	for (int s = 1; s < argc; s++) {
		fSequence.Append(argv[s]);
		if (s < argc - 1)
			fSequence.Append(" ");
	}

	// Find any insert-unicode-here sequences and replace them with spaces...
	int32 nextStart;
	while ((nextStart = fSequence.FindFirst("$$")) >= 0) {
		int32 nextEnd = fSequence.FindFirst("$$", nextStart + 2);
		if (nextEnd >= 0) {
			uint32 customKey= 0;
			int32 unicodeVal= 0;
			uint32 customMods = 0;
			BString sub;
			fSequence.CopyInto(sub, nextStart + 2, nextEnd-(nextStart + 2));
			sub.ToLower();

			if ((sub.FindFirst('-') >= 0) || ((sub.Length() > 0)
				&& ((sub.String()[0] < '0') || (sub.String()[0] > '9')))) {

				const char* s = sub.String();
				while (*s == '-') s++;// go past any initial dashes

				bool lastWasDash = true;
				while (*s) {
					if (lastWasDash) {
						if (strncmp(s, "shift",5) == 0)
							customMods |=B_LEFT_SHIFT_KEY| B_SHIFT_KEY;
						else if (strncmp(s, "leftsh", 6) == 0)
							customMods |=B_LEFT_SHIFT_KEY| B_SHIFT_KEY;
						else if (strncmp(s, "rightsh",7) == 0)
							customMods |=B_RIGHT_SHIFT_KEY | B_SHIFT_KEY;
						else if (strncmp(s, "alt",3) == 0)
							customMods |=B_LEFT_COMMAND_KEY| B_COMMAND_KEY;
						else if (strncmp(s, "leftalt",7) == 0)
							customMods |=B_LEFT_COMMAND_KEY| B_COMMAND_KEY;
						else if (strncmp(s, "rightalt", 8) == 0)
							customMods |=B_RIGHT_COMMAND_KEY | B_COMMAND_KEY;
						else if (strncmp(s, "com",3) == 0)
							customMods |=B_LEFT_COMMAND_KEY| B_COMMAND_KEY;
						else if (strncmp(s, "leftcom",7) == 0)
							customMods |=B_LEFT_COMMAND_KEY| B_COMMAND_KEY;
						else if (strncmp(s, "rightcom", 8) == 0)
							customMods |=B_RIGHT_COMMAND_KEY | B_COMMAND_KEY;
						else if (strncmp(s, "con",3) == 0)
							customMods |=B_LEFT_CONTROL_KEY| B_CONTROL_KEY;
						else if (strncmp(s, "leftcon",7) == 0)
							customMods |=B_LEFT_CONTROL_KEY| B_CONTROL_KEY;
						else if (strncmp(s, "rightcon", 8) == 0)
							customMods |=B_RIGHT_CONTROL_KEY | B_CONTROL_KEY;
						else if (strncmp(s, "win",3) == 0)
							customMods |=B_LEFT_OPTION_KEY | B_OPTION_KEY;
						else if (strncmp(s, "leftwin",7) == 0)
							customMods |=B_LEFT_OPTION_KEY | B_OPTION_KEY;
						else if (strncmp(s, "rightwin", 8) == 0)
							customMods |=B_RIGHT_OPTION_KEY| B_OPTION_KEY;
						else if (strncmp(s, "opt",3) == 0)
							customMods |=B_LEFT_OPTION_KEY | B_OPTION_KEY;
						else if (strncmp(s, "leftopt",7) == 0)
							customMods |=B_LEFT_OPTION_KEY | B_OPTION_KEY;
						else if (strncmp(s, "rightopt", 8) == 0)
							customMods |=B_RIGHT_OPTION_KEY| B_OPTION_KEY;
						else if (strncmp(s, "menu", 4) == 0)
							customMods |=B_MENU_KEY;
						else if (strncmp(s, "caps", 4) == 0)
							customMods |=B_CAPS_LOCK;
						else if (strncmp(s, "scroll", 6) == 0)
							customMods |=B_SCROLL_LOCK;
						else if (strncmp(s, "num",3) == 0)
							customMods |=B_NUM_LOCK;
						else if (customKey == 0) {
							BString arg = s;
							int32 dashIdx = arg.FindFirst('-');

							if (dashIdx >= 0)
								arg.Truncate(dashIdx);

							uint32 key = (uint32)FindKeyCode(arg.String());

							if (key > 0) {
								customKey = key;
								const char* u = GetKeyUTF8(key);

								//Parse the UTF8 back into an int32
								switch(strlen(u)) {
									case 1:
										unicodeVal = ((uint32)(u[0]&0x7F));
										break;
									case 2:
										unicodeVal = ((uint32)(u[1]&0x3F)) |
											(((uint32)(u[0]&0x1F)) << 6);
										break;
									case 3:
										unicodeVal = ((uint32)(u[2]&0x3F)) |
											(((uint32)(u[1]&0x3F)) << 6) |
											(((uint32)(u[0]&0x0F)) << 12);
										break;
									default: unicodeVal = 0;
										break;
								}
							}
						}
						lastWasDash = false;
					} else
						lastWasDash = (*s == '-');
					s++;
				}

				// If we have a letter, try to make it the correct case
				if ((unicodeVal >= 'A') && (unicodeVal <= 'Z')) {
					if ((customMods &B_SHIFT_KEY) == 0)
						unicodeVal += 'a'-'A';
				} else if ((unicodeVal >= 'a') && (unicodeVal <= 'z'))
					if ((customMods &B_SHIFT_KEY) != 0)
						unicodeVal -= 'a'-'A';
			} else {
				unicodeVal = strtol(&(fSequence.String())[nextStart + 2], NULL,
					 0);
				customMods = (uint32) -1;
			}

			if (unicodeVal == 0)
				unicodeVal = ' ';

			BString newStr = fSequence;
			newStr.Truncate(nextStart);
			fOverrides.AddItem((void*)unicodeVal);
			fOverrideOffsets.AddItem((void*)newStr.Length());
			fOverrideModifiers.AddItem((void*)customMods);
			fOverrideKeyCodes.AddItem((void*)customKey);
			newStr.Append(((unicodeVal > 0) && (unicodeVal < 127)) ?
				((char)unicodeVal): ' ',1);
			newStr.Append(&fSequence.String()[nextEnd + 2]);
			fSequence = newStr;
		} else
			break;
	}
	_GenerateKeyCodes();
}


KeyStrokeSequenceCommandActuator::KeyStrokeSequenceCommandActuator(
	BMessage* from)
	:
	CommandActuator(from)
{
	const char* seq;
	if (from->FindString("sequence", 0, &seq) == B_NO_ERROR)
		fSequence = seq;

	int32 temp;
	for (int32 i = 0; from->FindInt32("ooffsets", i, &temp) == B_NO_ERROR;
		i++) {
		fOverrideOffsets.AddItem((void*)temp);

		if (from->FindInt32("overrides", i, &temp) != B_NO_ERROR)
			temp = ' ';

		fOverrides.AddItem((void*)temp);

		if (from->FindInt32("omods", i, &temp) != B_NO_ERROR)
			temp = -1;

		fOverrideModifiers.AddItem((void*)temp);

		if (from->FindInt32("okeys", i, &temp) != B_NO_ERROR)
			temp = 0;

		fOverrideKeyCodes.AddItem((void*)temp);
	}
	_GenerateKeyCodes();
}


KeyStrokeSequenceCommandActuator::~KeyStrokeSequenceCommandActuator()
{
	delete [] fKeyCodes;
	delete [] fModCodes;
	delete [] fStates;
}


void
KeyStrokeSequenceCommandActuator::_GenerateKeyCodes()
{
	int slen = fSequence.Length();
	fKeyCodes = new int32[slen];
	fModCodes = new int32[slen];
	fStates = new uint8[slen * 16];

	memset(fStates, 0, slen * 16);

	key_map* map;
	char* keys;
	get_key_map(&map, &keys);
	for (int i = 0; i < slen; i++) {
		uint32 overrideKey= 0;
		uint32 overrideMods = (uint32)-1;
		for (int32 j = fOverrideOffsets.CountItems()-1; j >= 0; j--) {
			if ((int32)fOverrideOffsets.ItemAt(j) == i) {
				overrideKey= (uint32) fOverrideKeyCodes.ItemAt(j);
				overrideMods = (uint32) fOverrideModifiers.ItemAt(j);
				break;
			}
		}

		uint8* states = &fStates[i * 16];
		int32& mod = fModCodes[i];
		if (overrideKey == 0) {
			// Gotta do reverse-lookups to find out the raw keycodes for a
			// given character. Expensive--there oughtta be a better way to do
			// this.
			char next = fSequence.ByteAt(i);
			int32 key = _LookupKeyCode(map, keys, map->normal_map, next, states
							, mod, 0);
			if (key < 0)
				key = _LookupKeyCode(map, keys, map->shift_map, next, states,
							mod, B_LEFT_SHIFT_KEY | B_SHIFT_KEY);

			if (key < 0)
				key = _LookupKeyCode(map, keys, map->caps_map, next, states,
							mod, B_CAPS_LOCK);

			if (key < 0)
				key = _LookupKeyCode(map, keys, map->caps_shift_map, next,
							states, mod, B_LEFT_SHIFT_KEY | B_SHIFT_KEY
							| B_CAPS_LOCK);

			if (key < 0)
				key = _LookupKeyCode(map, keys, map->option_map, next, states,
							mod, B_LEFT_OPTION_KEY | B_OPTION_KEY);

			if (key < 0)
				key = _LookupKeyCode(map, keys, map->option_shift_map, next,
							states, mod, B_LEFT_OPTION_KEY | B_OPTION_KEY
							| B_LEFT_SHIFT_KEY | B_SHIFT_KEY);

			if (key < 0)
				key = _LookupKeyCode(map, keys, map->option_caps_map, next,
							states, mod, B_LEFT_OPTION_KEY | B_OPTION_KEY
							| B_CAPS_LOCK);

			if (key < 0)
				key = _LookupKeyCode(map, keys, map->option_caps_shift_map,
							next, states, mod, B_LEFT_OPTION_KEY | B_OPTION_KEY
							 | B_CAPS_LOCK | B_LEFT_SHIFT_KEY | B_SHIFT_KEY);

			if (key < 0)
				key = _LookupKeyCode(map, keys, map->control_map, next, states,
							 mod, B_CONTROL_KEY);

			fKeyCodes[i] = (key >= 0) ? key : 0;
		}

		if (overrideMods != (uint32)-1) {
			mod = (int32) overrideMods;

			// Clear any bits that might have been set by the lookups...
			_SetStateBit(states, map->caps_key,false);
			_SetStateBit(states, map->scroll_key,false);
			_SetStateBit(states, map->num_key, false);
			_SetStateBit(states, map->menu_key,false);
			_SetStateBit(states, map->left_shift_key,false);
			_SetStateBit(states, map->right_shift_key, false);
			_SetStateBit(states, map->left_command_key,false);
			_SetStateBit(states, map->right_command_key, false);
			_SetStateBit(states, map->left_control_key,false);
			_SetStateBit(states, map->right_control_key, false);
			_SetStateBit(states, map->left_option_key, false);
			_SetStateBit(states, map->right_option_key,false);

			// And then set any bits that were specified in our override.
			if (mod & B_CAPS_LOCK)
				_SetStateBit(states, map->caps_key);

			if (mod & B_SCROLL_LOCK)
				_SetStateBit(states, map->scroll_key);

			if (mod & B_NUM_LOCK)
				_SetStateBit(states, map->num_key);

			if (mod & B_MENU_KEY)
				_SetStateBit(states, map->menu_key);

			if (mod & B_LEFT_SHIFT_KEY)
				_SetStateBit(states, map->left_shift_key);

			if (mod & B_RIGHT_SHIFT_KEY)
				_SetStateBit(states, map->right_shift_key);

			if (mod & B_LEFT_COMMAND_KEY)
				_SetStateBit(states, map->left_command_key);

			if (mod & B_RIGHT_COMMAND_KEY)
				_SetStateBit(states, map->right_command_key);

			if (mod & B_LEFT_CONTROL_KEY)
				_SetStateBit(states, map->left_control_key);

			if (mod & B_RIGHT_CONTROL_KEY)
				_SetStateBit(states, map->right_control_key);

			if (mod & B_LEFT_OPTION_KEY)
				_SetStateBit(states, map->left_option_key);

			if (mod & B_RIGHT_OPTION_KEY)
				_SetStateBit(states, map->right_option_key);
		}

		if (overrideKey > 0) {
			if (overrideKey > 127)
				overrideKey = 0;// invalid value!?

			fKeyCodes[i] = overrideKey;
			_SetStateBit(states, overrideKey);
		}
	}
	free(keys);
	free(map);
}


int32
KeyStrokeSequenceCommandActuator::_LookupKeyCode(key_map* map, char* keys,
	int32 offsets[128], char c, uint8* setStates, int32& setMod, int32 setTo)
	const
{
	for (int i = 0; i < 128; i++) {
		if (keys[offsets[i]+ 1] == c) {
			_SetStateBit(setStates, i);

			if (setTo & B_SHIFT_KEY)
				_SetStateBit(setStates, map->left_shift_key);

			if (setTo & B_OPTION_KEY)
				_SetStateBit(setStates, map->left_option_key);

			if (setTo & B_CONTROL_KEY)
				_SetStateBit(setStates, map->left_control_key);

			if (setTo & B_CAPS_LOCK)
				_SetStateBit(setStates, map->caps_key);

			setMod = setTo;
			return i;
		}
	}
	return -1;
}


void
KeyStrokeSequenceCommandActuator::_SetStateBit(uint8* setStates, uint32 key,
	bool on) const
{
	if (on)
		setStates[key / 8] |= (0x80 >> (key%8));
	else
		setStates[key / 8] &= ~(0x80 >> (key%8));
}


status_t
KeyStrokeSequenceCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = CommandActuator::Archive(into, deep);
	into->AddString("sequence", fSequence.String());
	int32 numOverrides = fOverrideOffsets.CountItems();
	status_t tmp = B_OK;
	for (int32 i = 0; i < numOverrides; i++) {
		ret = into->AddInt32("ooffsets", (int32)fOverrideOffsets.ItemAt(i));
		if (ret != B_NO_ERROR)
			tmp = B_ERROR;

		ret = into->AddInt32("overrides", (int32)fOverrides.ItemAt(i));
		if (ret != B_NO_ERROR)
			tmp = B_ERROR;

		ret = into->AddInt32("omods", (int32)fOverrideModifiers.ItemAt(i));
		if (ret != B_NO_ERROR)
			tmp = B_ERROR;

		ret = into->AddInt32("okeys", (int32)fOverrideKeyCodes.ItemAt(i));
	}

	if (tmp == B_ERROR)
 		return tmp;
	else
		return ret;
}


filter_result
KeyStrokeSequenceCommandActuator::KeyEvent(const BMessage* keyMsg,
	BList* outlist, void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg)) {
		BMessage temp(*keyMsg);
		int numChars = fSequence.Length();
		for (int i = 0; i < numChars; i++) {
			char nextChar = fSequence.ByteAt(i);

			temp.RemoveName("modifiers");
			temp.AddInt32("modifiers", fModCodes[i]);
			temp.RemoveName("key");
			temp.AddInt32("key", fKeyCodes[i]);
			temp.RemoveName("raw_char");
			temp.AddInt32("raw_char", (int32) nextChar);
			temp.RemoveName("byte");

			int32 override = -1;
			for (int32 j = fOverrideOffsets.CountItems()-1; j >= 0; j--) {
				int32 offset = (int32) fOverrideOffsets.ItemAt(j);
				if (offset == i) {
					override = (int32) fOverrides.ItemAt(j);
					break;
				}
			}

			char t[4];
			if (override >= 0) {
				if (override < 0x80) {
					// one-byte encoding
					t[0] = (char) override;
					t[1] = 0x00;
				} else if (override < 0x800) {
					// two-byte encoding
					t[0] = 0xC0 | ((char)((override & 0x7C0)>>6));
					t[1] = 0x80 | ((char)((override & 0x03F)>>0));
					t[2] = 0x00;
				} else {
					// three-byte encoding
					t[0] = 0xE0 | ((char)((override & 0xF000)>>12));
					t[1] = 0x80 | ((char)((override & 0x0FC0)>>6));
					t[2] = 0x80 | ((char)((override & 0x003F)>>0));
					t[3] = 0x00;
				}
			} else {
				t[0] = nextChar;
				t[1] = 0x00;
			}

			temp.RemoveName("byte");

			for (int m = 0; t[m] != 0x00; m++)
				temp.AddInt8("byte", t[m]);

			temp.RemoveName("states");
			temp.AddData("states", B_UINT8_TYPE, &fStates[i * 16], 16, true, 16);
			temp.RemoveName("bytes");
			temp.AddString("bytes", t);
			temp.what = B_KEY_DOWN;
			outlist->AddItem(new BMessage(temp));
			temp.what = B_KEY_UP;
			outlist->AddItem(new BMessage(temp));
		}
		return B_DISPATCH_MESSAGE;
	}
	else
		return B_SKIP_MESSAGE;
}


BArchivable*
KeyStrokeSequenceCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KeyStrokeSequenceCommandActuator"))
		return new KeyStrokeSequenceCommandActuator(from);
	else
		return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// MIMEHandlerCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
MIMEHandlerCommandActuator::MIMEHandlerCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv),
	fMimeType((argc > 1) ? argv[1] : "")
{
	// empty
}


MIMEHandlerCommandActuator::MIMEHandlerCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	const char* temp;
	if (from->FindString("mimeType", 0, &temp) == B_NO_ERROR)
		fMimeType = temp;
}


MIMEHandlerCommandActuator::~MIMEHandlerCommandActuator()
{
	// empty
}


status_t
MIMEHandlerCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = CommandActuator::Archive(into, deep);
	into->AddString("mimeType", fMimeType.String());
	return ret;
}


filter_result
MIMEHandlerCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg))
		// cause KeyEventAsync() to be called asynchronously
		*setAsyncData = (void*) true;
	return B_SKIP_MESSAGE;
}


void
MIMEHandlerCommandActuator::KeyEventAsync(const BMessage* keyMsg,
	void* asyncData)
{
	if (be_roster) {
		BString str;
		BString str1("Shortcuts MIME launcher error");
		status_t ret = be_roster->Launch(fMimeType.String());
		if ((ret != B_NO_ERROR) && (ret != B_ALREADY_RUNNING)) {
			str << "Can't launch handler for ";
			str << ", no such MIME type exists. Please check your Shortcuts";
			str << " settings.";
			(new BAlert(str1.String(), str.String(), "OK"))->Go(NULL);
		}
	}
}


BArchivable* MIMEHandlerCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MIMEHandlerCommandActuator"))
		return new MIMEHandlerCommandActuator(from);
	else
		return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// BeepCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
BeepCommandActuator::BeepCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv)
{
	// empty
}


BeepCommandActuator::BeepCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	// empty
}


BeepCommandActuator::~BeepCommandActuator()
{
	// empty
}


status_t
BeepCommandActuator::Archive(BMessage* into, bool deep) const
{
	return CommandActuator::Archive(into, deep);
}


BArchivable*
BeepCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BeepCommandActuator"))
		return new BeepCommandActuator(from);
	else
		return NULL;
}


filter_result
BeepCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg))
		beep();

	return B_SKIP_MESSAGE;
}


///////////////////////////////////////////////////////////////////////////////
//
// MultiCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
MultiCommandActuator::MultiCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	BMessage msg;
	for (int i = 0; from->FindMessage("subs", i, &msg) == B_NO_ERROR; i++) {
		BArchivable* subObj = instantiate_object(&msg);
		if (subObj) {
			CommandActuator* ca = dynamic_cast < CommandActuator*>(subObj);

			if (ca)
				fSubActuators.AddItem(ca);
			else
				delete subObj;
		}
	}
}


MultiCommandActuator::MultiCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv)
{
	for (int i = 1; i < argc; i++) {
		CommandActuator* sub = CreateCommandActuator(argv[i]);

		if (sub)
			fSubActuators.AddItem(sub);
		else
			printf("Error creating subActuator from [%s]\n", argv[i]);
	}
}


MultiCommandActuator::~MultiCommandActuator()
{
	int numSubs = fSubActuators.CountItems();
	for (int i = 0; i < numSubs; i++)
		delete ((CommandActuator*) fSubActuators.ItemAt(i));
}


status_t
MultiCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = CommandActuator::Archive(into, deep);
	if (ret != B_NO_ERROR)
		return ret;

	int numSubs = fSubActuators.CountItems();
	for (int i = 0; i < numSubs; i++) {
		BMessage msg;
		ret = ((CommandActuator*)fSubActuators.ItemAt(i))->Archive(&msg, deep);

		if (ret != B_NO_ERROR)
			return ret;

		into->AddMessage("subs", &msg);
	}
	return B_NO_ERROR;
}


BArchivable*
MultiCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MultiCommandActuator"))
		return new MultiCommandActuator(from);
	else
		return NULL;
}


filter_result
MultiCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** asyncData, BMessage* lastMouseMove)
{
	BList* aDataList = NULL; // demand-allocated
	filter_result res = B_SKIP_MESSAGE;
	int numSubs = fSubActuators.CountItems();
	for (int i = 0; i < numSubs; i++) {
		void* aData = NULL;
		status_t next = ((CommandActuator*)fSubActuators.ItemAt(i))->
			KeyEvent(keyMsg, outlist, &aData, lastMouseMove);

		if (next == B_DISPATCH_MESSAGE)
			// dispatch message if at least one sub wants it dispatched
			res = B_DISPATCH_MESSAGE;

		if (aData) {
			if (aDataList == NULL)
				*asyncData = aDataList = new BList;

			while (aDataList->CountItems() < i - 1)
				aDataList->AddItem(NULL);
			aDataList->AddItem(aData);
		}
	}
	return res;
}


void
MultiCommandActuator::KeyEventAsync(const BMessage* keyUpMsg, void* asyncData)
{
	BList* list = (BList*) asyncData;
	int numSubs = list->CountItems();
	for (int i = 0; i < numSubs; i++) {
		void* aData = list->ItemAt(i);
		if (aData)
			((CommandActuator*) fSubActuators.ItemAt(i))->
				KeyEventAsync(keyUpMsg, aData);
	}
	delete list;
}


///////////////////////////////////////////////////////////////////////////////
//
// MoveMouseCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
MoveMouseCommandActuator::MoveMouseCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	if (from->FindFloat("xPercent", &fXPercent) != B_NO_ERROR)
		fXPercent = 0.0f;

	if (from->FindFloat("yPercent", &fYPercent) != B_NO_ERROR)
		fYPercent = 0.0f;

	if (from->FindFloat("xPixels", &fXPixels) != B_NO_ERROR)
		fXPixels = 0;

	if (from->FindFloat("yPixels", &fYPixels) != B_NO_ERROR)
		fYPixels = 0;
}


MoveMouseCommandActuator::MoveMouseCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv),
	fXPercent(0.0f),
	fYPercent(0.0f),
	fXPixels(0),
	fYPixels(0)
{
	if (argc > 1)
		_ParseArg(argv[1], fXPercent, fXPixels);

	if (argc > 2)
		_ParseArg(argv[2], fYPercent, fYPixels);
}


MoveMouseCommandActuator::~MoveMouseCommandActuator()
{
	// empty
}


status_t
MoveMouseCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = CommandActuator::Archive(into, deep);
	into->AddFloat("xPercent", fXPercent);
	into->AddFloat("yPercent", fYPercent);
	into->AddFloat("xPixels", fXPixels);
	into->AddFloat("yPixels", fYPixels);
	return ret;
}


void
MoveMouseCommandActuator::CalculateCoords(float& setX, float& setY) const
{
	BScreen s;
	BRect frame = s.Frame();
	setX = (frame.Width() * fXPercent) + fXPixels;
	setY = (frame.Height() * fYPercent) + fYPixels;
}


BMessage*
MoveMouseCommandActuator::CreateMouseMovedMessage(const BMessage* origMsg,
	BPoint p, BList* outlist) const
{
	// Force p into the screen space
	{
		BScreen s;
		p.ConstrainTo(s.Frame());
	}

	BMessage* newMsg = new BMessage(B_MOUSE_MOVED);

	newMsg->AddPoint("where", p);

	int32 buttons = 0;
	(void)origMsg->FindInt32("buttons", &buttons);

	if (buttons == 0)
		buttons = 1;

	newMsg->AddInt32("buttons", buttons);

	// Trey sez you gotta keep then "when"'s increasing if you want click&drag
	// to work!
	const BMessage* lastMessage;
	int nr = outlist->CountItems() - 1;

	if (outlist->CountItems() > 0)
		lastMessage = (const BMessage*)outlist->ItemAt(nr);
	else
		lastMessage = origMsg;

	int64 when;

	if (lastMessage->FindInt64("when", &when) == B_NO_ERROR) {
		when++;
		newMsg->RemoveName("when");
		newMsg->AddInt64("when", when);
	}
	return newMsg;
}


static bool IsNumeric(char c);
static bool IsNumeric(char c)
{
	return (((c >= '0') && (c <= '9')) || (c == '.') || (c == '-'));
}


// Parse a string of the form "10", "10%", "10+ 10%", or "10%+ 10"
void
MoveMouseCommandActuator::_ParseArg(const char* arg, float& setPercent,
	float& setPixels) const
{
	char* temp = new char[strlen(arg) + 1];
	strcpy(temp, arg);

	// Find the percent part, if any
	char* percent = strchr(temp, '%');
	if (percent) {
		// Rewind to one before the beginning of the number
		char* beginNum = percent - 1;
		while (beginNum >= temp) {
			char c = *beginNum;
			if (IsNumeric(c))
				beginNum--;
			else
				break;
		}

		// parse the number
		setPercent = atof(++beginNum)/100.0f;

		// Now white it out to ease finding the other #
		while (beginNum <= percent)
			*(beginNum++) = ' ';
	}

	// Find the pixel part, if any
	char* pixel = temp;
	while (!IsNumeric(*pixel)) {
		if (*pixel == '\0')
			break;
		pixel++;
	}
	setPixels = atof(pixel);
 delete [] temp;
}


///////////////////////////////////////////////////////////////////////////////
//
// MoveMouseToCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
MoveMouseToCommandActuator::MoveMouseToCommandActuator(BMessage* from)
	:
	MoveMouseCommandActuator(from)
{
	// empty
}


MoveMouseToCommandActuator::MoveMouseToCommandActuator(int32 argc, char** argv)
	:
	MoveMouseCommandActuator(argc, argv)
{
	// empty
}


MoveMouseToCommandActuator::~MoveMouseToCommandActuator()
{
	// empty
}


status_t
MoveMouseToCommandActuator::Archive(BMessage* into, bool deep) const
{
	return MoveMouseCommandActuator::Archive(into, deep);
}


BArchivable*
MoveMouseToCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MoveMouseToCommandActuator"))
		return new MoveMouseToCommandActuator(from);
	else
		return NULL;
}


filter_result
MoveMouseToCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg)) {
		float x, y;
		CalculateCoords(x, y);
		BPoint p(x, y);
		BMessage* newMsg = CreateMouseMovedMessage(keyMsg, p, outlist);
		*lastMouseMove = *newMsg;
		outlist->AddItem(newMsg);
		return B_DISPATCH_MESSAGE;
	}
	return B_SKIP_MESSAGE;
}


///////////////////////////////////////////////////////////////////////////////
//
// MoveMouseByCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
MoveMouseByCommandActuator::MoveMouseByCommandActuator(BMessage* from)
	:
	MoveMouseCommandActuator(from)
{
	// empty
}


MoveMouseByCommandActuator::MoveMouseByCommandActuator(int32 argc, char** argv)
	:
	MoveMouseCommandActuator(argc, argv)
{
	// empty
}


MoveMouseByCommandActuator::~MoveMouseByCommandActuator()
{
	// empty
}


status_t MoveMouseByCommandActuator::Archive(BMessage* into, bool deep) const
{
 status_t ret = MoveMouseCommandActuator::Archive(into, deep);
 return ret;
}


BArchivable*
MoveMouseByCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MoveMouseByCommandActuator"))
		return new MoveMouseByCommandActuator(from);
	else
		return NULL;
}


filter_result
MoveMouseByCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg)) {
		// Get the current mouse position
		BPoint p;
		if (lastMouseMove->FindPoint("where", &p) == B_NO_ERROR) {
			// Get the desired offset
			BPoint diff;
			CalculateCoords(diff.x, diff.y);
			p += diff;

			BMessage* newMsg = CreateMouseMovedMessage(keyMsg, p, outlist);
			*lastMouseMove = *newMsg;
			outlist->AddItem(newMsg);
			return B_DISPATCH_MESSAGE;
		}
	}
	return B_SKIP_MESSAGE;
}


///////////////////////////////////////////////////////////////////////////////
//
// SendMessageCommandActuator
//
///////////////////////////////////////////////////////////////////////////////
SendMessageCommandActuator::SendMessageCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv),
	fSignature((argc > 1) ? argv[1] : "")
{
	// Parse the what code. It may be in any of the following formats:
	// 12356 (int)
	// 'HELO' (chars enclosed in single quotes)
	// 0x12ab3c (hex)

	if (argc > 2) {
		const char* whatStr = argv[2];
		if ((whatStr[0] == '\'')
			&& (strlen(whatStr) == 6)
			&& (whatStr[5] == '\'')) {
			// Translate the characters into the uint32 they stand for.
			// Note that we must do this in a byte-endian-independant fashion
			// (no casting!)
			fSendMsg.what = 0;
			uint32 mult = 1;
			for (int i = 0; i < 4; i++) {
				fSendMsg.what += ((uint32)(whatStr[4 - i]))* mult;
				mult <<= 8;
			}
		} else if (strncmp(whatStr, "0x", 2) == 0)
			// translate hex string to decimal
			fSendMsg.what = strtoul(&whatStr[2], NULL, 16);
		else
			fSendMsg.what = atoi(whatStr);
	} else
		fSendMsg.what = 0;

	for (int i = 3; i < argc; i++) {
		type_code tc = B_BOOL_TYPE;// default type when no value is present
		const char* arg = argv[i];
		BString argStr(arg);
		const char* equals = strchr(arg, ' = ');
		const char* value = "true";// default if no value is present

		if (equals) {
			tc = B_STRING_TYPE;// default type when value is present
			value = equals + 1;
			const char* colon = strchr(arg, ':');
			if (colon > equals)
				colon = NULL;// colons after the equals sign don't count

			if (colon) {
				const char* typeStr = colon + 1;
				if (strncasecmp(typeStr, "string", 6) == 0)
					tc = B_STRING_TYPE;
				else if (strncasecmp(typeStr, "int8", 4) == 0)
					tc = B_INT8_TYPE;
				else if (strncasecmp(typeStr, "int16", 5) == 0)
					tc = B_INT16_TYPE;
				else if (strncasecmp(typeStr, "int32", 5) == 0)
					tc = B_INT32_TYPE;
				else if (strncasecmp(typeStr, "int64", 5) == 0)
					tc = B_INT64_TYPE;
				else if (strncasecmp(typeStr, "bool", 4) == 0)
					tc = B_BOOL_TYPE;
				else if (strncasecmp(typeStr, "float", 5) == 0)
					tc = B_FLOAT_TYPE;
				else if (strncasecmp(typeStr, "double", 6) == 0)
					tc = B_DOUBLE_TYPE;
				else if (strncasecmp(typeStr, "point", 5) == 0)
					tc = B_POINT_TYPE;
				else if (strncasecmp(typeStr, "rect", 4) == 0)
					tc = B_RECT_TYPE;

				// remove the colon and stuff
				argStr = argStr.Truncate(colon - arg);
			} else
				// remove the equals and arg
				argStr = argStr.Truncate(equals - arg);
		}

		switch(tc) {
			case B_STRING_TYPE:
				fSendMsg.AddString(argStr.String(), value);
				break;

			case B_INT8_TYPE:
				fSendMsg.AddInt8(argStr.String(), (int8)atoi(value));
				break;

			case B_INT16_TYPE:
				fSendMsg.AddInt16(argStr.String(), (int16)atoi(value));
				break;

			case B_INT32_TYPE:
				fSendMsg.AddInt32(argStr.String(), (int32)atoi(value));
				break;

			case B_INT64_TYPE:
				fSendMsg.AddInt64(argStr.String(), (int64)atoi(value));
				break;

			case B_BOOL_TYPE:
				fSendMsg.AddBool(argStr.String(), ((value[0] == 't')
					|| (value[0] == 'T')));
				break;

			case B_FLOAT_TYPE:
				fSendMsg.AddFloat(argStr.String(), atof(value));
				break;

			case B_DOUBLE_TYPE:
				fSendMsg.AddDouble(argStr.String(), (double)atof(value));
				break;

			case B_POINT_TYPE:
			{
				float pts[2] = {0.0f, 0.0f};
				_ParseFloatArgs(pts, 2, value);
				fSendMsg.AddPoint(argStr.String(), BPoint(pts[0], pts[1]));
				break;
			}

			case B_RECT_TYPE:
			{
				float pts[4] = {0.0f, 0.0f, 0.0f, 0.0f};
				_ParseFloatArgs(pts, 4, value);
				fSendMsg.AddRect(argStr.String(),
					BRect(pts[0], pts[1], pts[2], pts[3]));
				break;
			}
		}
	}
}


void
SendMessageCommandActuator::_ParseFloatArgs(float* args, int maxArgs,
	const char* str) const
{
	const char* next = str;
	for (int i = 0; i < maxArgs; i++) {
		args[i] = atof(next);
		next = strchr(next, ',');
		if (next) next++;
		else break;
	}
}


SendMessageCommandActuator::SendMessageCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	const char* temp;

	if (from->FindString("signature", 0, &temp) == B_NO_ERROR)
		fSignature = temp;

	(void) from->FindMessage("sendmsg", &fSendMsg);
}


SendMessageCommandActuator::~SendMessageCommandActuator()
{
	// empty
}


status_t
SendMessageCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = CommandActuator::Archive(into, deep);
	into->AddString("signature", fSignature.String());
	into->AddMessage("sendmsg", &fSendMsg);
	return ret;
}


filter_result
SendMessageCommandActuator::KeyEvent(const BMessage* keyMsg, BList* outlist,
	void** setAsyncData, BMessage* lastMouseMove)
{
	if (IS_KEY_DOWN(keyMsg))
		// cause KeyEventAsync() to be called asynchronously
		*setAsyncData = (void*) true;

	return B_SKIP_MESSAGE;
}


void
SendMessageCommandActuator::KeyEventAsync(const BMessage* keyMsg,
	void* asyncData)
{
	if (be_roster) {
		BString str;
		BString str1("Shortcuts SendMessage error");
		if (fSignature.Length() == 0) {
			str << "SendMessage: Target application signature not specified";
			(new BAlert(str1.String(), str.String(), "OK"))->Go(NULL);
		} else {
			status_t error = B_OK;
			BMessenger msngr(fSignature.String(), -1, &error);

			if (error == B_OK)
				msngr.SendMessage(&fSendMsg);
		}
	}
}


BArchivable*
SendMessageCommandActuator ::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "SendMessageCommandActuator"))
		return new SendMessageCommandActuator(from);
	else
		return NULL;
}
