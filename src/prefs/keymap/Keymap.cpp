// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        Keymap.cpp
//  Author:      Sandor Vroemisse, Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 12, 2004
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
Keymap::GetChars(int32 keyCode, uint32 modifiers, char **chars) 
{
	int32 offset = 0;
	switch (modifiers & 0xff) {
		case 0: offset = fKeys.normal_map[keyCode]; break;
		case B_SHIFT_KEY: offset = fKeys.shift_map[keyCode]; break;
		case B_CAPS_LOCK: offset = fKeys.caps_map[keyCode]; break;
		case B_CAPS_LOCK|B_SHIFT_KEY: offset = fKeys.caps_shift_map[keyCode]; break;
		case B_OPTION_KEY: offset = fKeys.option_map[keyCode]; break;
		case B_OPTION_KEY|B_SHIFT_KEY: offset = fKeys.option_shift_map[keyCode]; break;
		case B_OPTION_KEY|B_CAPS_LOCK: offset = fKeys.option_caps_map[keyCode]; break;
		case B_OPTION_KEY|B_SHIFT_KEY|B_CAPS_LOCK: offset = fKeys.option_caps_shift_map[keyCode]; break;
		case B_CONTROL_KEY: offset = fKeys.control_map[keyCode]; break;
	}

	int size = fChars[offset++];
	
	switch( size ) {
	case 0:
		// Not mapped
		*chars = NULL; 
		break; 
	default:
		// 1-, 2-, 3-, or 4-byte UTF-8 character 
		{ 
			char *str = *chars = new char[size + 1]; 
			strncpy(str, &(fChars[offset]), size );
			str[size] = 0; 
		} 
		break; 
	}
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


status_t 
Keymap::Load(entry_ref &ref)
{
	status_t err;
	
	BFile file(&ref, B_READ_ONLY);
	if ((err = file.InitCheck()) != B_OK) {
		printf("error %s\n", strerror(err));
		return err;
	}
	
	if (file.Read(&fKeys, sizeof(fKeys)) < (ssize_t)sizeof(fKeys)) {
		return B_BAD_VALUE;
	}
	
	for (uint32 i=0; i<sizeof(fKeys)/4; i++)
		((uint32*)&fKeys)[i] = B_BENDIAN_TO_HOST_INT32(((uint32*)&fKeys)[i]);
	
	uint32 chars_size;
	if (file.Read(&chars_size, sizeof(uint32)) < (ssize_t)sizeof(uint32)) {
		return B_BAD_VALUE;
	}
	
	chars_size = B_BENDIAN_TO_HOST_INT32(chars_size);
	if (!fChars)
		delete[] fChars;
	fChars = new char[chars_size];
	file.Read(fChars, chars_size);
	
	return B_OK;
}
