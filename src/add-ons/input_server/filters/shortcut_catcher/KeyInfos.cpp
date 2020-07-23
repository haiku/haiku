/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#include "KeyInfos.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <InterfaceDefs.h>


#define NUM_KEYS 128
#define MAX_UTF8_LENGTH 5
	// up to 4 chars, plus a \0 terminator


struct KeyLabelMap {
	const char* fLabel;
	uint8 fKeyCode;
};


// This is a table of keys-codes that have special, hard-coded labels.
static const struct KeyLabelMap keyLabels[] = {
	{"<unset>",		0},
	{"Esc",			1},
	{"F1",			2},
	{"F2",			3},
	{"F3",			4},
	{"F4",			5},
	{"F5",			6},
	{"F6",			7},
	{"F7",			8},
	{"F8",			9},
	{"F9",			10},
	{"F10",			11},
	{"F11",			12},
	{"F12",			13},
	{"SysRq",		14},
	{"ScrlLck",		15},
	{"Pause",		16},
	{"Bcksp",		30},
	{"Insert",		31},
	{"Home",		32},
	{"PgUp",		33},
	{"Num Lock",	34},
	{"Kpd /",		35},
	{"Kpd *",		36},
	{"Kpd -",		37},
	{"Tab",			38},
	{"Delete",		52},
	{"End",			53},
	{"PgDn",		54},
	{"Kpd 7",		55},
	{"Kpd 8",		56},
	{"Kpd 9",		57},
	{"Kpd +",		58},
	{"Caps Lock",	59},
	{"Enter",		71},
	{"Kpd 4",		72},
	{"Kpd 5",		73},
	{"Kpd 6",		74},
	{"L.Shift",		75},
	{"R.Shift",		86},
	{"Up",			87},
	{"Kpd 1",		88},
	{"Kpd 2",		89},
	{"Kpd 3",		90},
	{"Kpd Entr",	91},
	{"L.Control",	92},
	{"L.Alt",		93},
	{"Space",		94},
	{"R.Alt",		95},
	{"R.Control",	96},
	{"Left",		97},
	{"Down",		98},
	{"Right",		99},
	{"Kpd 0",		100},
	{"Kpd .",		101},
	{"L.Command",	102},
	{"R.Command",	103},
	{"Menu",		104},
	{"PowerOn",		107},
};


// Key description strings (e.g. "A" or "Escape"). NULL if no description is
// available.
static const char* keyDescriptions[NUM_KEYS];


// series of optional up-to-(4+1)-byte terminated UTF-8 character strings...
static char utfDescriptions[NUM_KEYS * MAX_UTF8_LENGTH];


static const char*
FindSpecialKeyLabelFor(uint8 keyCode, uint32& last)
{
	while ((keyLabels[last].fKeyCode < keyCode)
		&& (last < (sizeof(keyLabels) / sizeof(struct KeyLabelMap)) - 1)) {
		last++;
	}

	if (keyLabels[last].fKeyCode == keyCode)
		return keyLabels[last].fLabel;
	else
		return NULL;
}


void
InitKeyIndices()
{
	uint32 nextSpecial = 0;
	key_map* map;
	char* keys;
	get_key_map(&map, &keys);

	if (map == NULL || keys == NULL)
		return;

	for (uint8 j = 0; j < NUM_KEYS; j++) {
		keyDescriptions[j] = NULL;
			// default

		const char* specialLabel = FindSpecialKeyLabelFor(j, nextSpecial);
		int32 keyCode = map->normal_map[j];

		if (keyCode >= 0) {
			const char* mapDesc = &keys[keyCode];
			uint8 length = *mapDesc;

			for (int m = 0; m < MAX_UTF8_LENGTH; m++)
				if (m < length)
					utfDescriptions[j * MAX_UTF8_LENGTH + m] = mapDesc[m + 1];
				else
					utfDescriptions[j * MAX_UTF8_LENGTH + m] = '\0';

			if (specialLabel != NULL)
				keyDescriptions[j] = specialLabel;
			else {
				// If it's an ASCII letter, capitalize it.
				char& c = utfDescriptions[j * MAX_UTF8_LENGTH];

				if ((length == 1) && (isalpha(c)))
					c = toupper(c);

				if (length > 1 || (length == 1 && c > ' '))
					keyDescriptions[j] = &c;
			}
		} else
			utfDescriptions[j * MAX_UTF8_LENGTH] = 0x00;
	}

	free(keys);
	free(map);
}


const char*
GetKeyUTF8(uint8 keyIndex)
{
	return &utfDescriptions[keyIndex * MAX_UTF8_LENGTH];
}


const char*
GetKeyName(uint8 keyIndex)
{
	return keyDescriptions[keyIndex];
}


int
GetNumKeyIndices()
{
	return NUM_KEYS;
}


uint8
FindKeyCode(const char* keyName)
{
	for (uint8 i = 0; i < NUM_KEYS; i++) {
		if ((keyDescriptions[i])
			&& (strcasecmp(keyName, keyDescriptions[i]) == 0)) {
			return i;
		}
	}

	return 0;
		// default to sentinel value
}
