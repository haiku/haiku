// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        Keymap.h
//  Author:      Jérôme Duval
//  Description: Keyboard input server addon
//  Created :    September 7, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include "Keymap.h"
#include <stdio.h>
#include <string.h>
#include <ByteOrder.h>
#include <File.h>

static void 
print_key( char *chars, int32 offset ) 
{
	int size = chars[offset++];
	
	switch( size ) {
	case 0:
		// Not mapped 
		printf( "N/A" ); 
		break; 
	
	case 1:
		// 1-byte UTF-8/ASCII character 
		printf( "%c", chars[offset] ); 
		break; 
	
	default:
		// 2-, 3-, or 4-byte UTF-8 character 
		{ 
			char *str = new char[size + 1]; 
			strncpy( str, &(chars[offset]), size );
			str[size] = 0; 
			printf( "%s", str ); 
			delete [] str; 
		} 
		break; 
	} 
	
	printf( "\t" ); 
}


void
Keymap::DumpKeymap()
{
	// Print a chart of the normal, shift, option, and option+shift 
	// keys. 
	printf( "Key #\tNormal\tShift\tCaps\tC+S\tOption\tO+S\tO+C\tO+C+S\tControl\n" ); 
	for( int idx = 0; idx < 128; idx++ ) { 
		printf( " 0x%x\t", idx ); 
		print_key( fChars, fKeys.normal_map[idx] ); 
		print_key( fChars, fKeys.shift_map[idx] ); 
		print_key( fChars, fKeys.caps_map[idx] ); 
		print_key( fChars, fKeys.caps_shift_map[idx] ); 
		print_key( fChars, fKeys.option_map[idx] ); 
		print_key( fChars, fKeys.option_shift_map[idx] ); 
		print_key( fChars, fKeys.option_caps_map[idx] ); 
		print_key( fChars, fKeys.option_caps_shift_map[idx] ); 
		print_key( fChars, fKeys.control_map[idx] ); 
		printf( "\n" ); 
	} 

}


Keymap::Keymap() : 
	fChars(NULL)
{
	key_map *keys;
	get_key_map(&keys, &fChars);
	
	if (keys)
		memcpy(&fKeys, keys, sizeof(key_map));
}


/* we need to know if a key is a modifier key to choose 
	a valid key when several are pressed together
*/
bool 
Keymap::IsModifierKey(uint32 keyCode)
{
	if ((keyCode == fKeys.caps_key)
		|| (keyCode == fKeys.num_key)
		|| (keyCode == fKeys.scroll_key)
		|| (keyCode == fKeys.left_shift_key)
		|| (keyCode == fKeys.right_shift_key)
		|| (keyCode == fKeys.left_command_key)
		|| (keyCode == fKeys.right_command_key)
		|| (keyCode == fKeys.left_control_key)
		|| (keyCode == fKeys.right_control_key)
		|| (keyCode == fKeys.left_option_key)
		|| (keyCode == fKeys.right_option_key)
		|| (keyCode == fKeys.menu_key))
			return true;
	return false;
}


/* we need to know a modifier for a key
*/
uint32 
Keymap::Modifier(uint32 keyCode)
{
	if (keyCode == fKeys.caps_key)
		return B_CAPS_LOCK;
	if (keyCode == fKeys.num_key)
		return B_NUM_LOCK;
	if (keyCode == fKeys.scroll_key)
		return B_SCROLL_LOCK;
	if (keyCode == fKeys.left_shift_key)
		return B_LEFT_SHIFT_KEY | B_SHIFT_KEY;
	if (keyCode == fKeys.right_shift_key)
		return B_RIGHT_SHIFT_KEY | B_SHIFT_KEY;
	if (keyCode == fKeys.left_command_key)
		return B_LEFT_COMMAND_KEY | B_COMMAND_KEY;
	if (keyCode == fKeys.right_command_key)
		return B_RIGHT_COMMAND_KEY | B_COMMAND_KEY;
	if (keyCode == fKeys.left_control_key)
		return B_LEFT_CONTROL_KEY | B_CONTROL_KEY;
	if (keyCode == fKeys.right_control_key)
		return B_RIGHT_CONTROL_KEY | B_CONTROL_KEY;
	if (keyCode == fKeys.left_option_key)
		return B_LEFT_OPTION_KEY | B_OPTION_KEY;
	if (keyCode == fKeys.right_option_key)
		return B_RIGHT_OPTION_KEY | B_OPTION_KEY;
	if (keyCode == fKeys.menu_key)
		return B_MENU_KEY;
	return 0;
}


// tell if a key is a dead key, needed for draw a dead key
uint8
Keymap::IsDeadKey(uint32 keyCode, uint32 modifiers)
{
	int32 offset;
	uint32 tableMask = 0;
	
	switch (modifiers & 0xcf) {
		case B_SHIFT_KEY: offset = fKeys.shift_map[keyCode]; tableMask = B_SHIFT_TABLE; break;
		case B_CAPS_LOCK: offset = fKeys.caps_map[keyCode]; tableMask = B_CAPS_TABLE; break;
		case B_CAPS_LOCK|B_SHIFT_KEY: offset = fKeys.caps_shift_map[keyCode]; tableMask = B_CAPS_SHIFT_TABLE; break;
		case B_OPTION_KEY: offset = fKeys.option_map[keyCode]; tableMask = B_OPTION_TABLE; break;
		case B_OPTION_KEY|B_SHIFT_KEY: offset = fKeys.option_shift_map[keyCode]; tableMask = B_OPTION_SHIFT_TABLE; break;
		case B_OPTION_KEY|B_CAPS_LOCK: offset = fKeys.option_caps_map[keyCode]; tableMask = B_OPTION_CAPS_TABLE; break;
		case B_OPTION_KEY|B_SHIFT_KEY|B_CAPS_LOCK: offset = fKeys.option_caps_shift_map[keyCode]; tableMask = B_OPTION_CAPS_SHIFT_TABLE; break;
		case B_CONTROL_KEY: offset = fKeys.control_map[keyCode]; tableMask = B_CONTROL_TABLE; break;
		default: offset = fKeys.normal_map[keyCode]; tableMask = B_NORMAL_TABLE; break;
	}
	
	if (offset<=0)
		return 0;	
	uint32 numBytes = fChars[offset];
	
	if (!numBytes)
		return 0;
		
	char chars[4];	
	strncpy(chars, &(fChars[offset+1]), numBytes );
	chars[numBytes] = 0; 
	
	int32 deadOffsets[] = {
		fKeys.acute_dead_key[1],
		fKeys.grave_dead_key[1],
		fKeys.circumflex_dead_key[1],
		fKeys.dieresis_dead_key[1],
		fKeys.tilde_dead_key[1]
	}; 
	
	uint32 deadTables[] = {
		fKeys.acute_tables,
		fKeys.grave_tables,
		fKeys.circumflex_tables,
		fKeys.dieresis_tables,
		fKeys.tilde_tables
	};
	
	for (int32 i=0; i<5; i++) {
		if ((deadTables[i] & tableMask) == 0)
			continue;
		
		if (offset == deadOffsets[i])
			return i+1;
			
		uint32 deadNumBytes = fChars[deadOffsets[i]];
		
		if (!deadNumBytes)
			continue;
			
		if (strncmp(chars, &(fChars[deadOffsets[i]+1]), deadNumBytes ) == 0) {
			return i+1;
		}
	}
	return 0;
}


// tell if a key is a dead second key, needed for draw a dead second key
bool
Keymap::IsDeadSecondKey(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey)
{
	if (!activeDeadKey)
		return false;
	
	int32 offset;
	
	switch (modifiers & 0xcf) {
		case B_SHIFT_KEY: offset = fKeys.shift_map[keyCode]; break;
		case B_CAPS_LOCK: offset = fKeys.caps_map[keyCode]; break;
		case B_CAPS_LOCK|B_SHIFT_KEY: offset = fKeys.caps_shift_map[keyCode]; break;
		case B_OPTION_KEY: offset = fKeys.option_map[keyCode]; break;
		case B_OPTION_KEY|B_SHIFT_KEY: offset = fKeys.option_shift_map[keyCode]; break;
		case B_OPTION_KEY|B_CAPS_LOCK: offset = fKeys.option_caps_map[keyCode]; break;
		case B_OPTION_KEY|B_SHIFT_KEY|B_CAPS_LOCK: offset = fKeys.option_caps_shift_map[keyCode]; break;
		case B_CONTROL_KEY: offset = fKeys.control_map[keyCode]; break;
		default: offset = fKeys.normal_map[keyCode]; break;
	}
	
	uint32 numBytes = fChars[offset];
	
	if (!numBytes)
		return false;
	
	int32* deadOffsets[] = {
		fKeys.acute_dead_key,
		fKeys.grave_dead_key,
		fKeys.circumflex_dead_key,
		fKeys.dieresis_dead_key,
		fKeys.tilde_dead_key
	};
	
	int32 *deadOffset = deadOffsets[activeDeadKey-1]; 
	
	for (int32 i=0; i<32; i++) {
		if (offset == deadOffset[i])
			return true;
			
		uint32 deadNumBytes = fChars[deadOffset[i]];
		
		if (!deadNumBytes)
			continue;
			
		if (strncmp(&(fChars[offset+1]), &(fChars[deadOffset[i]+1]), deadNumBytes ) == 0)
			return true;
		i++;
	}
	return false;
}


// get the char for a key given modifiers and active dead key
void
Keymap::GetChars(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey, char** chars, int32* numBytes)
{
	int32 offset;
	
	*numBytes = 0;
	*chars = NULL;
	
	// here we take NUMLOCK into account
	if (modifiers & B_NUM_LOCK)
		switch (keyCode) {
			case 0x37:
			case 0x38:
			case 0x39:
			case 0x48:
			case 0x49:
			case 0x4a:
			case 0x58:
			case 0x59:
			case 0x5a:
			case 0x64:
			case 0x65:
				modifiers ^= B_SHIFT_KEY;
		}	

	// here we choose the right map given the modifiers
	switch (modifiers & 0xcf) {
		case B_SHIFT_KEY: offset = fKeys.shift_map[keyCode]; break;
		case B_CAPS_LOCK: offset = fKeys.caps_map[keyCode]; break;
		case B_CAPS_LOCK|B_SHIFT_KEY: offset = fKeys.caps_shift_map[keyCode]; break;
		case B_OPTION_KEY: offset = fKeys.option_map[keyCode]; break;
		case B_OPTION_KEY|B_SHIFT_KEY: offset = fKeys.option_shift_map[keyCode]; break;
		case B_OPTION_KEY|B_CAPS_LOCK: offset = fKeys.option_caps_map[keyCode]; break;
		case B_OPTION_KEY|B_SHIFT_KEY|B_CAPS_LOCK: offset = fKeys.option_caps_shift_map[keyCode]; break;
		case B_CONTROL_KEY: offset = fKeys.control_map[keyCode]; break;
		default: offset = fKeys.normal_map[keyCode]; break;
	}
	
	// here we get the char size
	*numBytes = fChars[offset];
	
	if (!*numBytes)
		return;
	
	// here we take an potential active dead key
	int32 *dead_key;
	switch(activeDeadKey) {
		case 1: dead_key = fKeys.acute_dead_key; break;
		case 2: dead_key = fKeys.grave_dead_key; break;
		case 3: dead_key = fKeys.circumflex_dead_key; break;
		case 4: dead_key = fKeys.dieresis_dead_key; break;
		case 5: dead_key = fKeys.tilde_dead_key; break;
		default: 
		{
			// if not dead, we copy and return the char
			char *str = *chars = new char[*numBytes + 1]; 
			strncpy(str, &(fChars[offset+1]), *numBytes );
			str[*numBytes] = 0;
			return;
		}
	}

	// if dead key, we search for our current offset char in the dead key offset table
	// string comparison is needed
	for (int32 i=0; i<32; i++) {
		if (strncmp(&(fChars[offset+1]), &(fChars[dead_key[i]+1]), *numBytes ) == 0) {
			*numBytes = fChars[dead_key[i+1]];
		
			switch( *numBytes ) {
				case 0:
					// Not mapped
					*chars = NULL; 
					break; 
				default:
					// 1-, 2-, 3-, or 4-byte UTF-8 character 
				{ 
					char *str = *chars = new char[*numBytes + 1]; 
					strncpy(str, &fChars[dead_key[i+1]+1], *numBytes );
					str[*numBytes] = 0; 
				} 
					break; 
			}
			return;
		}		
		i++;
	}

	// if not found we return the current char mapped	
	*chars = new char[*numBytes + 1];
	strncpy(*chars, &(fChars[offset+1]), *numBytes );
	(*chars)[*numBytes] = 0; 	
	
}


status_t
Keymap::LoadCurrent()
{
	key_map *keys = NULL;
	get_key_map(&keys, &fChars);
	if (!keys) {
		fprintf(stderr, "error while getting current keymap!\n");
		return B_ERROR;
	}
	memcpy(&fKeys, keys, sizeof(fKeys));
	delete keys;
	return B_OK;
}
