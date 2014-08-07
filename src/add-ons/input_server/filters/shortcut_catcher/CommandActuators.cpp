/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Fredrik Modéen
 */


#include "CommandActuators.h"


#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

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


// factory function
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


//	#pragma mark - CommandActuator


CommandActuator::CommandActuator(int32 argc, char** argv)
{
}


CommandActuator::CommandActuator(BMessage* from)
	:
	BArchivable(from)
{
}


status_t
CommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);
	return ret;
}


//	#pragma mark - LaunchCommandActuator


LaunchCommandActuator::LaunchCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv),
	fArgv(CloneArgv(argv)),
	fArgc(argc)
{
}


LaunchCommandActuator::LaunchCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	BList argList;
	const char* temp;
	int idx = 0;
	while (from->FindString("largv", idx++, &temp) == B_OK) {
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
LaunchCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage)) {
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
LaunchCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "LaunchCommandActuator"))
		return new LaunchCommandActuator(from);
	else
		return NULL;
}


void
LaunchCommandActuator::KeyEventAsync(const BMessage* keyMessage,
	void* asyncData)
{
	if (be_roster == NULL)
		return;

	status_t result = B_OK;
	BString string;
	if (fArgc < 1)
		string << "You didn't specify a command for this hotkey.";
	else if ((result = LaunchCommand(fArgv, fArgc)) != B_OK) {
		string << "Can't launch " << fArgv[0];
		string << ", no such file exists.";
		string << " Please check your Shortcuts settings.";
	}

	if (fArgc < 1 || result != B_OK) {
		BAlert* alert = new BAlert("Shortcuts launcher error",
			string.String(), "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	}
}


//	#pragma mark - MouseCommandActuator


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
	const BMessage* keyMessage, BList* outList, BMessage* mouseMessage)
{
	BMessage* fakeMouse = new BMessage(*mouseMessage);
	fakeMouse->what = mouseDown ? B_MOUSE_DOWN : B_MOUSE_UP;

	// Update the buttons to reflect which mouse buttons we are faking
	fakeMouse->RemoveName("buttons");

	if (mouseDown)
		fakeMouse->AddInt32("buttons", fWhichButtons);

	// Trey sez you gotta keep then "when"'s increasing if you want
	// click & drag to work!
	int64 when;

	const BMessage* lastMessage;
	if (outList->CountItems() > 0) {
		int32 last = outList->CountItems() - 1;
		lastMessage = (const BMessage*)outList->ItemAt(last);
	} else
		lastMessage = keyMessage;

	if (lastMessage->FindInt64("when", &when) == B_OK) {
		when++;
		fakeMouse->RemoveName("when");
		fakeMouse->AddInt64("when", when);
	}

	outList->AddItem(fakeMouse);
}


//	#pragma mark - MouseDownCommandActuator


MouseDownCommandActuator::MouseDownCommandActuator(int32 argc, char** argv)
	:
	MouseCommandActuator(argc, argv)
{
}


MouseDownCommandActuator::MouseDownCommandActuator(BMessage* from)
	:
	MouseCommandActuator(from)
{
}


MouseDownCommandActuator::~MouseDownCommandActuator()
{
}


filter_result
MouseDownCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage))
		_GenerateMouseButtonEvent(true, keyMessage, outList, mouseMessage);

	return B_DISPATCH_MESSAGE;
}


status_t
MouseDownCommandActuator::Archive(BMessage* into, bool deep) const
{
	return MouseCommandActuator::Archive(into, deep);
}


BArchivable*
MouseDownCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MouseDownCommandActuator"))
		return new MouseDownCommandActuator(from);
	else
		return NULL;
}


//	#pragma mark - MouseUpCommandActuator


MouseUpCommandActuator::MouseUpCommandActuator(int32 argc, char** argv)
	:
	MouseCommandActuator(argc, argv)
{
}


MouseUpCommandActuator::MouseUpCommandActuator(BMessage* from)
	:
	MouseCommandActuator(from)
{
}


MouseUpCommandActuator::~MouseUpCommandActuator()
{
}


filter_result
MouseUpCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage))
		_GenerateMouseButtonEvent(false, keyMessage, outList, mouseMessage);

	return B_DISPATCH_MESSAGE;
}


status_t
MouseUpCommandActuator::Archive(BMessage* into, bool deep) const
{
	return MouseCommandActuator::Archive(into, deep);
}


BArchivable*
MouseUpCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MouseUpCommandActuator"))
		return new MouseUpCommandActuator(from);
	else
		return NULL;
}


//	#pragma mark - MouseButtonCommandActuator


MouseButtonCommandActuator::MouseButtonCommandActuator(int32 argc, char** argv)
	:
	MouseCommandActuator(argc, argv),
	fKeyDown(false)
{
}


MouseButtonCommandActuator::MouseButtonCommandActuator(BMessage* from)
	:
	MouseCommandActuator(from),
	fKeyDown(false)
{
}


MouseButtonCommandActuator::~MouseButtonCommandActuator()
{
}


filter_result
MouseButtonCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage) != fKeyDown) {
		_GenerateMouseButtonEvent(IS_KEY_DOWN(keyMessage), keyMessage, outList,
			mouseMessage);
		fKeyDown = IS_KEY_DOWN(keyMessage);

		return B_DISPATCH_MESSAGE;
	} else {
		// This will handle key-repeats, which we don't want turned into lots
		// of B_MOUSE_DOWN messages.
		return B_SKIP_MESSAGE;
	}
}


status_t
MouseButtonCommandActuator::Archive(BMessage* into, bool deep) const
{
	return MouseCommandActuator::Archive(into, deep);
}


BArchivable*
MouseButtonCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MouseButtonCommandActuator"))
		return new MouseButtonCommandActuator(from);
	else
		return NULL;
}


//	#pragma mark - KeyStrokeSequenceCommandActuator


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
					if ((customMods & B_SHIFT_KEY) == 0)
						unicodeVal += 'a'-'A';
				} else if ((unicodeVal >= 'a') && (unicodeVal <= 'z')) {
					if ((customMods & B_SHIFT_KEY) != 0)
						unicodeVal -= 'a'-'A';
				}
			} else {
				unicodeVal = strtol(&(fSequence.String())[nextStart + 2],
					NULL, 0);
				customMods = (uint32) -1;
			}

			if (unicodeVal == 0)
				unicodeVal = ' ';

			BString newString = fSequence;
			newString.Truncate(nextStart);
			fOverrides.AddItem((void*)(addr_t)unicodeVal);
			fOverrideOffsets.AddItem((void*)(addr_t)newString.Length());
			fOverrideModifiers.AddItem((void*)(addr_t)customMods);
			fOverrideKeyCodes.AddItem((void*)(addr_t)customKey);
			newString.Append((unicodeVal > 0
				&& unicodeVal < 127) ? (char)unicodeVal : ' ', 1);
			newString.Append(&fSequence.String()[nextEnd + 2]);
			fSequence = newString;
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
	const char* sequence;
	if (from->FindString("sequence", 0, &sequence) == B_OK)
		fSequence = sequence;

	int32 temp;
	for (int32 i = 0; from->FindInt32("ooffsets", i, &temp) == B_OK; i++) {
		fOverrideOffsets.AddItem((void*)(addr_t)temp);

		if (from->FindInt32("overrides", i, &temp) != B_OK)
			temp = ' ';

		fOverrides.AddItem((void*)(addr_t)temp);

		if (from->FindInt32("omods", i, &temp) != B_OK)
			temp = -1;

		fOverrideModifiers.AddItem((void*)(addr_t)temp);

		if (from->FindInt32("okeys", i, &temp) != B_OK)
			temp = 0;

		fOverrideKeyCodes.AddItem((void*)(addr_t)temp);
	}

	_GenerateKeyCodes();
}


KeyStrokeSequenceCommandActuator::~KeyStrokeSequenceCommandActuator()
{
	delete[] fKeyCodes;
	delete[] fModCodes;
	delete[] fStates;
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
		uint32 overrideKey = 0;
		uint32 overrideMods = (uint32)-1;
		for (int32 j = fOverrideOffsets.CountItems()-1; j >= 0; j--) {
			if ((int32)(addr_t)fOverrideOffsets.ItemAt(j) == i) {
				overrideKey= (uint32)(addr_t) fOverrideKeyCodes.ItemAt(j);
				overrideMods = (uint32)(addr_t) fOverrideModifiers.ItemAt(j);
				break;
			}
		}

		uint8* states = &fStates[i * 16];
		int32& modCode = fModCodes[i];
		if (overrideKey == 0) {
			// Gotta do reverse-lookups to find out the raw keycodes for a
			// given character. Expensive--there oughtta be a better way to do
			// this.
			char next = fSequence.ByteAt(i);
			int32 key = _LookupKeyCode(map, keys, map->normal_map, next,
				states, modCode, 0);
			if (key < 0) {
				key = _LookupKeyCode(map, keys, map->shift_map, next, states,
					modCode, B_LEFT_SHIFT_KEY | B_SHIFT_KEY);
			}

			if (key < 0) {
				key = _LookupKeyCode(map, keys, map->caps_map, next, states,
					modCode, B_CAPS_LOCK);
			}

			if (key < 0) {
				key = _LookupKeyCode(map, keys, map->caps_shift_map, next,
					states, modCode,
					B_LEFT_SHIFT_KEY | B_SHIFT_KEY | B_CAPS_LOCK);
			}

			if (key < 0) {
				key = _LookupKeyCode(map, keys, map->option_map, next, states,
					modCode, B_LEFT_OPTION_KEY | B_OPTION_KEY);
			}

			if (key < 0) {
				key = _LookupKeyCode(map, keys, map->option_shift_map, next,
					states, modCode, B_LEFT_OPTION_KEY | B_OPTION_KEY
						| B_LEFT_SHIFT_KEY | B_SHIFT_KEY);
			}

			if (key < 0) {
				key = _LookupKeyCode(map, keys, map->option_caps_map, next,
					states, modCode,
					B_LEFT_OPTION_KEY | B_OPTION_KEY | B_CAPS_LOCK);
			}

			if (key < 0) {
				key = _LookupKeyCode(map, keys, map->option_caps_shift_map,
					next, states, modCode, B_LEFT_OPTION_KEY | B_OPTION_KEY
						| B_CAPS_LOCK | B_LEFT_SHIFT_KEY | B_SHIFT_KEY);
			}

			if (key < 0) {
				key = _LookupKeyCode(map, keys, map->control_map, next, states,
					modCode, B_CONTROL_KEY);
			}

			fKeyCodes[i] = key >= 0 ? key : 0;
		}

		if (overrideMods != (uint32)-1) {
			modCode = (int32)overrideMods;

			// Clear any bits that might have been set by the lookups...
			_SetStateBit(states, map->caps_key, false);
			_SetStateBit(states, map->scroll_key, false);
			_SetStateBit(states, map->num_key, false);
			_SetStateBit(states, map->menu_key, false);
			_SetStateBit(states, map->left_shift_key, false);
			_SetStateBit(states, map->right_shift_key, false);
			_SetStateBit(states, map->left_command_key, false);
			_SetStateBit(states, map->right_command_key, false);
			_SetStateBit(states, map->left_control_key, false);
			_SetStateBit(states, map->right_control_key, false);
			_SetStateBit(states, map->left_option_key, false);
			_SetStateBit(states, map->right_option_key, false);

			// And then set any bits that were specified in our override.
			if (modCode & B_CAPS_LOCK)
				_SetStateBit(states, map->caps_key);

			if (modCode & B_SCROLL_LOCK)
				_SetStateBit(states, map->scroll_key);

			if (modCode & B_NUM_LOCK)
				_SetStateBit(states, map->num_key);

			if (modCode & B_MENU_KEY)
				_SetStateBit(states, map->menu_key);

			if (modCode & B_LEFT_SHIFT_KEY)
				_SetStateBit(states, map->left_shift_key);

			if (modCode & B_RIGHT_SHIFT_KEY)
				_SetStateBit(states, map->right_shift_key);

			if (modCode & B_LEFT_COMMAND_KEY)
				_SetStateBit(states, map->left_command_key);

			if (modCode & B_RIGHT_COMMAND_KEY)
				_SetStateBit(states, map->right_command_key);

			if (modCode & B_LEFT_CONTROL_KEY)
				_SetStateBit(states, map->left_control_key);

			if (modCode & B_RIGHT_CONTROL_KEY)
				_SetStateBit(states, map->right_control_key);

			if (modCode & B_LEFT_OPTION_KEY)
				_SetStateBit(states, map->left_option_key);

			if (modCode & B_RIGHT_OPTION_KEY)
				_SetStateBit(states, map->right_option_key);
		}

		if (overrideKey > 0) {
			if (overrideKey > 127) {
				// invalid value?
				overrideKey = 0;
			}

			fKeyCodes[i] = overrideKey;
			_SetStateBit(states, overrideKey);
		}
	}

	free(keys);
	free(map);
}


int32
KeyStrokeSequenceCommandActuator::_LookupKeyCode(key_map* map, char* keys,
	int32 offsets[128], char c, uint8* setStates, int32& setModifier,
	int32 setTo) const
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

			setModifier = setTo;

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
		setStates[key / 8] |= (0x80 >> (key % 8));
	else
		setStates[key / 8] &= ~(0x80 >> (key % 8));
}


status_t
KeyStrokeSequenceCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t result = CommandActuator::Archive(into, deep);
	if (result != B_OK)
		return result;

	into->AddString("sequence", fSequence.String());
	int32 numOverrides = fOverrideOffsets.CountItems();

	status_t overridesResult = B_OK;
	for (int32 i = 0; i < numOverrides; i++) {
		result = into->AddInt32("ooffsets",
			(int32)(addr_t)fOverrideOffsets.ItemAt(i));
		if (result != B_OK)
			overridesResult = B_ERROR;

		result = into->AddInt32("overrides",
			(int32)(addr_t)fOverrides.ItemAt(i));
		if (result != B_OK)
			overridesResult = B_ERROR;

		result = into->AddInt32("omods",
			(int32)(addr_t)fOverrideModifiers.ItemAt(i));
		if (result != B_OK)
			overridesResult = B_ERROR;

		result = into->AddInt32("okeys",
			(int32)(addr_t)fOverrideKeyCodes.ItemAt(i));
	}

	return overridesResult == B_ERROR ? B_ERROR : result;
}


filter_result
KeyStrokeSequenceCommandActuator::KeyEvent(const BMessage* keyMessage,
	BList* outList, void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage)) {
		BMessage temp(*keyMessage);
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
				int32 offset = (int32)(addr_t) fOverrideOffsets.ItemAt(j);
				if (offset == i) {
					override = (int32)(addr_t) fOverrides.ItemAt(j);
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
			outList->AddItem(new BMessage(temp));
			temp.what = B_KEY_UP;
			outList->AddItem(new BMessage(temp));
		}

		return B_DISPATCH_MESSAGE;
	} else
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


//	#pragma mark - MIMEHandlerCommandActuator


MIMEHandlerCommandActuator::MIMEHandlerCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv),
	fMimeType((argc > 1) ? argv[1] : "")
{
}


MIMEHandlerCommandActuator::MIMEHandlerCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	const char* temp;
	if (from->FindString("mimeType", 0, &temp) == B_OK)
		fMimeType = temp;
}


MIMEHandlerCommandActuator::~MIMEHandlerCommandActuator()
{
}


status_t
MIMEHandlerCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = CommandActuator::Archive(into, deep);
	into->AddString("mimeType", fMimeType.String());
	return ret;
}


filter_result
MIMEHandlerCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage))
		// cause KeyEventAsync() to be called asynchronously
		*setAsyncData = (void*) true;
	return B_SKIP_MESSAGE;
}


void
MIMEHandlerCommandActuator::KeyEventAsync(const BMessage* keyMessage,
	void* asyncData)
{
	if (be_roster == NULL)
		return;

	BString string;
	status_t ret = be_roster->Launch(fMimeType.String());
	if ((ret != B_OK) && (ret != B_ALREADY_RUNNING)) {
		string << "Can't launch handler for ";
		string << ", no such MIME type exists. Please check your Shortcuts";
		string << " settings.";
		BAlert* alert = new BAlert("Shortcuts MIME launcher error",
			string.String(), "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	}
}


BArchivable* MIMEHandlerCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MIMEHandlerCommandActuator"))
		return new MIMEHandlerCommandActuator(from);
	else
		return NULL;
}


//	#pragma mark - BeepCommandActuator


BeepCommandActuator::BeepCommandActuator(int32 argc, char** argv)
	:
	CommandActuator(argc, argv)
{
}


BeepCommandActuator::BeepCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
}


BeepCommandActuator::~BeepCommandActuator()
{
}


status_t
BeepCommandActuator::Archive(BMessage* into, bool deep) const
{
	return CommandActuator::Archive(into, deep);
}


BArchivable*
BeepCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BeepCommandActuator"))
		return new BeepCommandActuator(from);
	else
		return NULL;
}


filter_result
BeepCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage))
		beep();

	return B_SKIP_MESSAGE;
}


//	#pragma mark - MultiCommandActuator


MultiCommandActuator::MultiCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	BMessage msg;
	for (int i = 0; from->FindMessage("subs", i, &msg) == B_OK; i++) {
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
	if (ret != B_OK)
		return ret;

	int numSubs = fSubActuators.CountItems();
	for (int i = 0; i < numSubs; i++) {
		BMessage msg;
		ret = ((CommandActuator*)fSubActuators.ItemAt(i))->Archive(&msg, deep);

		if (ret != B_OK)
			return ret;

		into->AddMessage("subs", &msg);
	}
	return B_OK;
}


BArchivable*
MultiCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MultiCommandActuator"))
		return new MultiCommandActuator(from);
	else
		return NULL;
}


filter_result
MultiCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** asyncData, BMessage* mouseMessage)
{
	BList* aDataList = NULL; // demand-allocated
	filter_result res = B_SKIP_MESSAGE;
	int numSubs = fSubActuators.CountItems();
	for (int i = 0; i < numSubs; i++) {
		void* aData = NULL;
		status_t next = ((CommandActuator*)fSubActuators.ItemAt(i))->
			KeyEvent(keyMessage, outList, &aData, mouseMessage);

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


//	#pragma mark - MoveMouseCommandActuator


MoveMouseCommandActuator::MoveMouseCommandActuator(BMessage* from)
	:
	CommandActuator(from)
{
	if (from->FindFloat("xPercent", &fXPercent) != B_OK)
		fXPercent = 0.0f;

	if (from->FindFloat("yPercent", &fYPercent) != B_OK)
		fYPercent = 0.0f;

	if (from->FindFloat("xPixels", &fXPixels) != B_OK)
		fXPixels = 0;

	if (from->FindFloat("yPixels", &fYPixels) != B_OK)
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
MoveMouseCommandActuator::CreateMouseMessage(const BMessage* original,
	BPoint where, BList* outList) const
{
	// Force where into the screen space
	{
		BScreen screen;
		where.ConstrainTo(screen.Frame());
	}

	BMessage* newMessage = new BMessage(B_MOUSE_MOVED);

	newMessage->AddPoint("where", where);

	int32 buttons = 0;
	(void)original->FindInt32("buttons", &buttons);

	if (buttons == 0)
		buttons = 1;

	newMessage->AddInt32("buttons", buttons);

	// Trey sez you gotta keep then "when"'s increasing if you want click&drag
	// to work!
	const BMessage* lastMessage;
	int32 last = outList->CountItems() - 1;

	if (outList->CountItems() > 0)
		lastMessage = (const BMessage*)outList->ItemAt(last);
	else
		lastMessage = original;

	int64 when;

	if (lastMessage->FindInt64("when", &when) == B_OK) {
		when++;
		newMessage->RemoveName("when");
		newMessage->AddInt64("when", when);
	}
	return newMessage;
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


//	#pragma mark - MoveMouseToCommandActuator


MoveMouseToCommandActuator::MoveMouseToCommandActuator(BMessage* from)
	:
	MoveMouseCommandActuator(from)
{
}


MoveMouseToCommandActuator::MoveMouseToCommandActuator(int32 argc, char** argv)
	:
	MoveMouseCommandActuator(argc, argv)
{
}


MoveMouseToCommandActuator::~MoveMouseToCommandActuator()
{
}


status_t
MoveMouseToCommandActuator::Archive(BMessage* into, bool deep) const
{
	return MoveMouseCommandActuator::Archive(into, deep);
}


BArchivable*
MoveMouseToCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MoveMouseToCommandActuator"))
		return new MoveMouseToCommandActuator(from);
	else
		return NULL;
}


filter_result
MoveMouseToCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage)) {
		float x, y;
		CalculateCoords(x, y);
		BPoint where(x, y);
		BMessage* newMessage = CreateMouseMessage(keyMessage, where, outList);
		*mouseMessage = *newMessage;
		outList->AddItem(newMessage);

		return B_DISPATCH_MESSAGE;
	}

	return B_SKIP_MESSAGE;
}


//	#pragma mark - MoveMouseByCommandActuator


MoveMouseByCommandActuator::MoveMouseByCommandActuator(BMessage* from)
	:
	MoveMouseCommandActuator(from)
{
}


MoveMouseByCommandActuator::MoveMouseByCommandActuator(int32 argc, char** argv)
	:
	MoveMouseCommandActuator(argc, argv)
{
}


MoveMouseByCommandActuator::~MoveMouseByCommandActuator()
{
}


status_t MoveMouseByCommandActuator::Archive(BMessage* into, bool deep) const
{
 status_t ret = MoveMouseCommandActuator::Archive(into, deep);
 return ret;
}


BArchivable*
MoveMouseByCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "MoveMouseByCommandActuator"))
		return new MoveMouseByCommandActuator(from);
	else
		return NULL;
}


filter_result
MoveMouseByCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage)) {
		// Get the current mouse position
		BPoint where;
		if (mouseMessage->FindPoint("where", &where) == B_OK) {
			// Get the desired offset
			BPoint diff;
			CalculateCoords(diff.x, diff.y);
			where += diff;
			BMessage* newMessage = CreateMouseMessage(keyMessage, where, outList);
			*mouseMessage = *newMessage;
			outList->AddItem(newMessage);
			return B_DISPATCH_MESSAGE;
		}
	}
	return B_SKIP_MESSAGE;
}


//	#pragma mark - SendMessageCommandActuator


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
			fSendMessage.what = 0;
			uint32 mult = 1;
			for (int i = 0; i < 4; i++) {
				fSendMessage.what += ((uint32)(whatStr[4 - i]))* mult;
				mult <<= 8;
			}
		} else if (strncmp(whatStr, "0x", 2) == 0)
			// translate hex string to decimal
			fSendMessage.what = strtoul(&whatStr[2], NULL, 16);
		else
			fSendMessage.what = atoi(whatStr);
	} else
		fSendMessage.what = 0;

	for (int i = 3; i < argc; i++) {
		type_code tc = B_BOOL_TYPE;// default type when no value is present
		const char* arg = argv[i];
		BString argString(arg);
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
				argString = argString.Truncate(colon - arg);
			} else
				// remove the equals and arg
				argString = argString.Truncate(equals - arg);
		}

		switch(tc) {
			case B_STRING_TYPE:
				fSendMessage.AddString(argString.String(), value);
				break;

			case B_INT8_TYPE:
				fSendMessage.AddInt8(argString.String(), (int8)atoi(value));
				break;

			case B_INT16_TYPE:
				fSendMessage.AddInt16(argString.String(), (int16)atoi(value));
				break;

			case B_INT32_TYPE:
				fSendMessage.AddInt32(argString.String(), (int32)atoi(value));
				break;

			case B_INT64_TYPE:
				fSendMessage.AddInt64(argString.String(), (int64)atoi(value));
				break;

			case B_BOOL_TYPE:
				fSendMessage.AddBool(argString.String(), ((value[0] == 't')
					|| (value[0] == 'T')));
				break;

			case B_FLOAT_TYPE:
				fSendMessage.AddFloat(argString.String(), atof(value));
				break;

			case B_DOUBLE_TYPE:
				fSendMessage.AddDouble(argString.String(), (double)atof(value));
				break;

			case B_POINT_TYPE:
			{
				float pts[2] = {0.0f, 0.0f};
				_ParseFloatArgs(pts, 2, value);
				fSendMessage.AddPoint(argString.String(), BPoint(pts[0], pts[1]));
				break;
			}

			case B_RECT_TYPE:
			{
				float pts[4] = {0.0f, 0.0f, 0.0f, 0.0f};
				_ParseFloatArgs(pts, 4, value);
				fSendMessage.AddRect(argString.String(),
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

	if (from->FindString("signature", 0, &temp) == B_OK)
		fSignature = temp;

	(void) from->FindMessage("sendmsg", &fSendMessage);
}


SendMessageCommandActuator::~SendMessageCommandActuator()
{
}


status_t
SendMessageCommandActuator::Archive(BMessage* into, bool deep) const
{
	status_t ret = CommandActuator::Archive(into, deep);
	into->AddString("signature", fSignature.String());
	into->AddMessage("sendmsg", &fSendMessage);
	return ret;
}


filter_result
SendMessageCommandActuator::KeyEvent(const BMessage* keyMessage, BList* outList,
	void** setAsyncData, BMessage* mouseMessage)
{
	if (IS_KEY_DOWN(keyMessage))
		// cause KeyEventAsync() to be called asynchronously
		*setAsyncData = (void*) true;

	return B_SKIP_MESSAGE;
}


void
SendMessageCommandActuator::KeyEventAsync(const BMessage* keyMessage,
	void* asyncData)
{
	if (be_roster == NULL)
		return;

	BString string;
	if (fSignature.Length() == 0) {
		string << "SendMessage: Target application signature not specified";
		BAlert* alert = new BAlert("Shortcuts SendMessage error",
			string.String(), "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	} else {
		status_t result = B_OK;
		BMessenger messenger(fSignature.String(), -1, &result);
		if (result == B_OK)
			messenger.SendMessage(&fSendMessage);
	}
}


BArchivable*
SendMessageCommandActuator::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "SendMessageCommandActuator"))
		return new SendMessageCommandActuator(from);
	else
		return NULL;
}
