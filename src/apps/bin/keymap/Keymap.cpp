// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        Keymap.cpp
//  Author:      Jérôme Duval
//  Description: keymap bin
//  Created :    July 29, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include "Keymap.h"
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <ByteOrder.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

#define CHARS_TABLE_MAXSIZE  10000

void 
Keymap::GetKey( char *chars, int32 offset, char* string) 
{
	int size = chars[offset++];
	char str[32];
	memset(str, 0, 32);
	memset(string, 0, 32);
	
	switch( size ) {
	case 0:
		// Not mapped 
		sprintf(str, "''");
		break; 
	
	case 1:
		// 1-byte UTF-8/ASCII character
		if ((uint8)chars[offset] < 0x20
			|| (uint8)chars[offset] > 0x7e)
			sprintf(str, "0x%02x", (uint8)chars[offset]);
		else
			sprintf(str, "'%s%c'", 
				(chars[offset] == '\\' || chars[offset] == '\'') ? "\\" : "", chars[offset]);
		break; 
	
	default:
		// n-byte UTF-8/ASCII character
		sprintf(str, "0x");
		for (int i=0; i<size; i++)
			sprintf(str + 2*(i+1), "%02x", (uint8)chars[offset+i]);
		break;
	}
	strncpy(string, str, (strlen(str) < 12) ? strlen(str) : 12);
}


void
Keymap::Dump()
{
	printf(
"#!/bin/keymap -l\n"
"#\n"
"#\tRaw key numbering for 101 keyboard...\n"
"#                                                                                        [sys]       [brk]\n"
"#                                                                                         0x7e        0x7f\n"
"# [esc]       [ f1] [ f2] [ f3] [ f4] [ f5] [ f6] [ f7] [ f8] [ f9] [f10] [f11] [f12]    [prn] [scr] [pau]\n"
"#  0x01        0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0a  0x0b  0x0c  0x0d     0x0e  0x0f  0x10     K E Y P A D   K E Y S\n"
"#\n"
"# [ ` ] [ 1 ] [ 2 ] [ 3 ] [ 4 ] [ 5 ] [ 6 ] [ 7 ] [ 8 ] [ 9 ] [ 0 ] [ - ] [ = ] [bck]    [ins] [hme] [pup]    [num] [ / ] [ * ] [ - ]\n"
"#  0x11  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1a  0x1b  0x1c  0x1d  0x1e     0x1f  0x20  0x21     0x22  0x23  0x24  0x25\n"
"#\n"
"# [tab] [ q ] [ w ] [ e ] [ r ] [ t ] [ y ] [ u ] [ i ] [ o ] [ p ] [ [ ] [ ] ] [ \\ ]    [del] [end] [pdn]    [ 7 ] [ 8 ] [ 9 ] [ + ]\n"
"#  0x26  0x27  0x28  0x29  0x2a  0x2b  0x2c  0x2d  0x2e  0x2f  0x30  0x31  0x32  0x33     0x34  0x35  0x36     0x37  0x38  0x39  0x3a\n"
"#\n"
"# [cap] [ a ] [ s ] [ d ] [ f ] [ g ] [ h ] [ j ] [ k ] [ l ] [ ; ] [ ' ] [  enter  ]                         [ 4 ] [ 5 ] [ 6 ]\n"
"#  0x3b  0x3c  0x3d  0x3e  0x3f  0x40  0x41  0x42  0x43  0x44  0x45  0x46     0x47                             0x48  0x49  0x4a\n"
"#\n"
"# [shift]     [ z ] [ x ] [ c ] [ v ] [ b ] [ n ] [ m ] [ , ] [ . ] [ / ]     [shift]          [ up]          [ 1 ] [ 2 ] [ 3 ] [ent]\n"
"#   0x4b       0x4c  0x4d  0x4e  0x4f  0x50  0x51  0x52  0x53  0x54  0x55       0x56            0x57           0x58  0x59  0x5a  0x5b\n"
"#\n"
"# [ctr]             [cmd]             [  space  ]             [cmd]             [ctr]    [lft] [dwn] [rgt]    [ 0 ] [ . ]\n"
"#  0x5c              0x5d                 0x5e                 0x5f              0x60     0x61  0x62  0x63     0x64  0x65\n"
"#\n"
"#\tNOTE: On a Microsoft Natural Keyboard:\n"
"#\t\t\tleft option  = 0x66\n"
"#\t\t\tright option = 0x67\n"
"#\t\t\tmenu key     = 0x68\n"
"#\tNOTE: On an Apple Extended Keyboard:\n"
"#\t\t\tleft option  = 0x66\n"
"#\t\t\tright option = 0x67\n"
"#\t\t\tkeypad '='   = 0x6a\n"
"#\t\t\tpower key    = 0x6b\n");

	printf("Version = %ld\n", fKeys.version);
	printf("CapsLock = 0x%02lx\n", fKeys.caps_key);
	printf("ScrollLock = 0x%02lx\n", fKeys.scroll_key);
	printf("NumLock = 0x%02lx\n", fKeys.num_key);
	printf("LShift = 0x%02lx\n", fKeys.left_shift_key);
	printf("RShift = 0x%02lx\n", fKeys.right_shift_key);
	printf("LCommand = 0x%02lx\n", fKeys.left_command_key);
	printf("RCommand = 0x%02lx\n", fKeys.right_command_key);
	printf("LControl = 0x%02lx\n", fKeys.left_control_key);
	printf("RControl = 0x%02lx\n", fKeys.right_control_key);
	printf("LOption = 0x%02lx\n", fKeys.left_option_key);
	printf("ROption = 0x%02lx\n", fKeys.right_option_key);
	printf("Menu = 0x%02lx\n", fKeys.menu_key);
	printf(
"#\n"
"# Lock settings\n"
"# To set NumLock, do the following:\n"
"#   LockSettings = NumLock\n"
"#\n"
"# To set everything, do the following:\n"
"#   LockSettings = CapsLock NumLock ScrollLock\n"
"#\n");
	printf("LockSettings = ");
	if (fKeys.lock_settings & B_CAPS_LOCK)
		printf("CapsLock ");
	if (fKeys.lock_settings & B_NUM_LOCK)
		printf("NumLock ");
	if (fKeys.lock_settings & B_SCROLL_LOCK)
		printf("ScrollLock ");
	printf("\n");
	printf(
"# Legend:\n"
"#   n = Normal\n"
"#   s = Shift\n"
"#   c = Control\n"
"#   C = CapsLock\n"
"#   o = Option\n"
"# Key      n        s        c        o        os       C        Cs       Co       Cos     \n");
	
	for( int idx = 0; idx < 128; idx++ ) {
		char normalKey[32];
		char shiftKey[32];
		char controlKey[32];
		char optionKey[32];
		char optionShiftKey[32];
		char capsKey[32];
		char capsShiftKey[32];
		char optionCapsKey[32];
		char optionCapsShiftKey[32];
		
		GetKey( fChars, fKeys.normal_map[idx], normalKey);
		GetKey( fChars, fKeys.shift_map[idx], shiftKey); 
		GetKey( fChars, fKeys.control_map[idx], controlKey); 
		GetKey( fChars, fKeys.option_map[idx], optionKey); 
		GetKey( fChars, fKeys.option_shift_map[idx], optionShiftKey); 
		GetKey( fChars, fKeys.caps_map[idx], capsKey); 
		GetKey( fChars, fKeys.caps_shift_map[idx], capsShiftKey); 
		GetKey( fChars, fKeys.option_caps_map[idx], optionCapsKey); 
		GetKey( fChars, fKeys.option_caps_shift_map[idx], optionCapsShiftKey);
		
		printf("Key 0x%02x = %-9s%-9s%-9s%-9s%-9s%-9s%-9s%-9s%-9s\n", idx, normalKey, shiftKey, controlKey,
			optionKey, optionShiftKey, capsKey, capsShiftKey, optionCapsKey, optionCapsShiftKey); 
	}
	
	int32* deadOffsets[] = {
		fKeys.acute_dead_key,
		fKeys.grave_dead_key,
		fKeys.circumflex_dead_key,
		fKeys.dieresis_dead_key,
		fKeys.tilde_dead_key
	};
	
	char labels[][12] = {
		"Acute",
		"Grave",
		"Circumflex",
		"Diaeresis",
		"Tilde"
	};
	
	uint32 deadTables[] = {
		fKeys.acute_tables,
		fKeys.grave_tables,
		fKeys.circumflex_tables,
		fKeys.dieresis_tables,
		fKeys.tilde_tables
	};
	
	for (int i = 0; i<5; i++) {
		for (int idx = 0; idx < 32; idx++ ) {
			char deadKey[32];
			char secondKey[32];
			GetKey( fChars, deadOffsets[i][idx++], deadKey);
			GetKey( fChars, deadOffsets[i][idx], secondKey); 
			printf("%s %-9s = %-9s\n", labels[i], deadKey, secondKey);
		}
		
		printf("%sTab = ", labels[i]);
		
		if (deadTables[i] & B_NORMAL_TABLE)
			printf("Normal ");
		if (deadTables[i] & B_SHIFT_TABLE)
			printf("Shift ");
		if (deadTables[i] & B_CONTROL_TABLE)
			printf("Control ");
		if (deadTables[i] & B_OPTION_TABLE)
			printf("Option ");
		if (deadTables[i] & B_OPTION_SHIFT_TABLE)
			printf("Option-Shift ");
		if (deadTables[i] & B_CAPS_TABLE)
			printf("CapsLock ");
		if (deadTables[i] & B_CAPS_SHIFT_TABLE)
			printf("CapsLock-Shift ");
		if (deadTables[i] & B_OPTION_CAPS_TABLE)
			printf("CapsLock-Option ");
		if (deadTables[i] & B_OPTION_CAPS_SHIFT_TABLE)
			printf("CapsLock-Option-Shift ");
		printf("\n");
	}

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

/*
	file format in big endian :
	struct key_map	
	uint32 size of following charset
	charset (offsets go into this with size of character followed by character)
*/
// we load a map from a file
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
	
	if (file.Read(&fCharsSize, sizeof(uint32)) < (ssize_t)sizeof(uint32)) {
		return B_BAD_VALUE;
	}
	
	fCharsSize = B_BENDIAN_TO_HOST_INT32(fCharsSize);
	if (!fChars)
		delete[] fChars;
	fChars = new char[fCharsSize];
	if (file.Read(fChars, fCharsSize) != fCharsSize)
		return B_BAD_VALUE;
	
	return B_OK;
}


void
Keymap::ComputeChars(const char *buffer, struct re_registers &regs, int i, int &offset)
{
	char *current = &fChars[offset+1];
	char hexChars[12];
	uint32 length = 0;
	if (strncmp(buffer + regs.start[i], "''", regs.end[i] - regs.start[i]) == 0)
		length = 0;
	else if (sscanf(buffer + regs.start[i], "'%s'", current) > 0) {
		if (current[0] == '\\')
			current[0] = current[1];
		else if (current[0] == '\'')
			current[0] = ' ';
		length = 1;
	} else if (sscanf(buffer + regs.start[i], "0x%s", hexChars) > 0) {
		length = strlen(hexChars) / 2;
		for (uint32 j=0; j<length; j++)
			sscanf(hexChars + 2*j, "%02x", (uint8*)current + j);
	}
	fChars[offset] = length;
	offset += length + 1;
}


void
Keymap::ComputeTables(const char *buffer, struct re_registers &regs, uint32 &table)
{
	for (int32 i=1; i<=9; i++) {
		if (regs.end[i] - regs.start[i] <= 0)
			break;
		if (strncmp(buffer + regs.start[i], "Normal", regs.end[i] - regs.start[i]) == 0)
			table |= B_NORMAL_TABLE;
		else if (strncmp(buffer + regs.start[i], "Shift", regs.end[i] - regs.start[i]) == 0)
			table |= B_SHIFT_TABLE;
		else if (strncmp(buffer + regs.start[i], "Control", regs.end[i] - regs.start[i]) == 0)
			table |= B_CONTROL_TABLE;
		else if (strncmp(buffer + regs.start[i], "Option", regs.end[i] - regs.start[i]) == 0)
			table |= B_OPTION_TABLE;
		else if (strncmp(buffer + regs.start[i], "Option-Shift", regs.end[i] - regs.start[i]) == 0)
			table |= B_OPTION_SHIFT_TABLE;
		else if (strncmp(buffer + regs.start[i], "CapsLock", regs.end[i] - regs.start[i]) == 0)
			table |= B_CAPS_TABLE;
		else if (strncmp(buffer + regs.start[i], "CapsLock-Shift", regs.end[i] - regs.start[i]) == 0)
			table |= B_CAPS_SHIFT_TABLE;
		else if (strncmp(buffer + regs.start[i], "CapsLock-Option", regs.end[i] - regs.start[i]) == 0)
			table |= B_OPTION_CAPS_TABLE;
		else if (strncmp(buffer + regs.start[i], "CapsLock-Option-Shift", regs.end[i] - regs.start[i]) == 0)
			table |= B_OPTION_CAPS_SHIFT_TABLE;
	}
}


status_t 
Keymap::LoadSourceFromRef(entry_ref &ref)
{
	status_t err;
	
	BFile file(&ref, B_READ_ONLY);
	if ((err = file.InitCheck()) != B_OK) {
		printf("error %s\n", strerror(err));
		return err;
	}
	
	int fd = file.Dup();
	FILE * f = fdopen(fd, "r");
	
	return LoadSource(f);
}


// i couldn't put patterns and pattern bufs on the stack without segfaulting
// regexp patterns
const char versionPattern[] = "Version[[:space:]]+=[[:space:]]+\\([[:digit:]]+\\)";
const char capslockPattern[] = "CapsLock[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char scrolllockPattern[] = "ScrollLock[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char numlockPattern[] = "NumLock[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char lshiftPattern[] = "LShift[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char rshiftPattern[] = "RShift[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char lcommandPattern[] = "LCommand[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char rcommandPattern[] = "RCommand[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char lcontrolPattern[] = "LControl[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char rcontrolPattern[] = "RControl[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char loptionPattern[] = "LOption[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char roptionPattern[] = "ROption[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char menuPattern[] = "Menu[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char locksettingsPattern[] = "LockSettings[[:space:]]+=[[:space:]]+\\([[:alnum:]]*\\)"
						"[[:space:]]*\\([[:alnum:]]*\\)"
						"[[:space:]]*\\([[:alnum:]]*\\)[[:space:]]*";
const char keyPattern[] = "Key[[:space:]]+\\([[:alnum:]]+\\)[[:space:]]+="
						"[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
						"[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
						"[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
						"[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
						"[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
						"[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
						"[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
						"[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
						"[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
						"[[:space:]]+";


const char acutePattern[] = "Acute[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
const char gravePattern[] = "Grave[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
const char circumflexPattern[] = "Circumflex[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
const char diaeresisPattern[] = "Diaeresis[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
const char tildePattern[] = "Tilde[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
const char acutetabPattern[] = "AcuteTab[[:space:]]+="
						"[[:space:]]+\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)[[:space:]]*" ;
const char gravetabPattern[] = "GraveTab[[:space:]]+="
						"[[:space:]]+\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)[[:space:]]*" ;
const char circumflextabPattern[] = "CircumflexTab[[:space:]]+="
						"[[:space:]]+\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)[[:space:]]*" ;
const char diaeresistabPattern[] = "DiaeresisTab[[:space:]]+="
						"[[:space:]]+\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)[[:space:]]*" ;
const char tildetabPattern[] = "TildeTab[[:space:]]+="
						"[[:space:]]+\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)"
						"[[:space:]]*\\([[:alnum:]-]*\\)[[:space:]]*" ;


// re_pattern_buffer buffers
struct re_pattern_buffer versionBuf;
struct re_pattern_buffer capslockBuf;
struct re_pattern_buffer scrolllockBuf;
struct re_pattern_buffer numlockBuf;
struct re_pattern_buffer lshiftBuf;
struct re_pattern_buffer rshiftBuf;
struct re_pattern_buffer lcommandBuf;
struct re_pattern_buffer rcommandBuf;
struct re_pattern_buffer lcontrolBuf;
struct re_pattern_buffer rcontrolBuf;
struct re_pattern_buffer loptionBuf;
struct re_pattern_buffer roptionBuf;
struct re_pattern_buffer menuBuf;
struct re_pattern_buffer locksettingsBuf;
struct re_pattern_buffer keyBuf;
struct re_pattern_buffer acuteBuf;
struct re_pattern_buffer graveBuf;
struct re_pattern_buffer circumflexBuf;
struct re_pattern_buffer diaeresisBuf;
struct re_pattern_buffer tildeBuf;
struct re_pattern_buffer acutetabBuf;
struct re_pattern_buffer gravetabBuf;
struct re_pattern_buffer circumflextabBuf;
struct re_pattern_buffer diaeresistabBuf;
struct re_pattern_buffer tildetabBuf;
	
status_t
Keymap::LoadSource(FILE * f)
{
	reg_syntax_t syntax = RE_CHAR_CLASSES;
	re_set_syntax(syntax);
	
	const char* error; 
	error = re_compile_pattern(versionPattern, strlen(versionPattern), &versionBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(capslockPattern, strlen(capslockPattern), &capslockBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(scrolllockPattern, strlen(scrolllockPattern), &scrolllockBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(numlockPattern, strlen(numlockPattern), &numlockBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(lshiftPattern, strlen(lshiftPattern), &lshiftBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(rshiftPattern, strlen(rshiftPattern), &rshiftBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(lcommandPattern, strlen(lcommandPattern), &lcommandBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(rcommandPattern, strlen(rcommandPattern), &rcommandBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(lcontrolPattern, strlen(lcontrolPattern), &lcontrolBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(rcontrolPattern, strlen(rcontrolPattern), &rcontrolBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(loptionPattern, strlen(loptionPattern), &loptionBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(roptionPattern, strlen(roptionPattern), &roptionBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(menuPattern, strlen(menuPattern), &menuBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(locksettingsPattern, strlen(locksettingsPattern), &locksettingsBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(keyPattern, strlen(keyPattern), &keyBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(acutePattern, strlen(acutePattern), &acuteBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(gravePattern, strlen(gravePattern), &graveBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(circumflexPattern, strlen(circumflexPattern), &circumflexBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(diaeresisPattern, strlen(diaeresisPattern), &diaeresisBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(tildePattern, strlen(tildePattern), &tildeBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(acutetabPattern, strlen(acutetabPattern), &acutetabBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(gravetabPattern, strlen(gravetabPattern), &gravetabBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(circumflextabPattern, strlen(circumflextabPattern), &circumflextabBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(diaeresistabPattern, strlen(diaeresistabPattern), &diaeresistabBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(tildetabPattern, strlen(tildetabPattern), &tildetabBuf);
	if (error)
		fprintf(stderr, error);
	
	
	char buffer[1024];
	
	delete [] fChars;
	fChars = new char[CHARS_TABLE_MAXSIZE];
	fCharsSize = CHARS_TABLE_MAXSIZE;
	int offset = 0;
	int acuteOffset = 0;
	int graveOffset = 0;
	int circumflexOffset = 0;
	int diaeresisOffset = 0;
	int tildeOffset = 0;
		
	int32 *maps[] = {
		fKeys.normal_map,
		fKeys.shift_map,
		fKeys.control_map,
		fKeys.option_map,
		fKeys.option_shift_map,
		fKeys.caps_map,
		fKeys.caps_shift_map,
		fKeys.option_caps_map,
		fKeys.option_caps_shift_map
	};
	
	while (fgets(buffer, 1024-1, f)!=NULL) {
		if (buffer[0]== '#' || buffer[0]== '\n')
			continue;
		struct re_registers regs;
		if (re_search(&versionBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "%ld", &fKeys.version);
		} else if (re_search(&capslockBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.caps_key);
		} else if (re_search(&scrolllockBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.scroll_key);
		} else if (re_search(&numlockBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.num_key);
		} else if (re_search(&lshiftBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.left_shift_key);
		} else if (re_search(&rshiftBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.right_shift_key);
		} else if (re_search(&lcommandBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.left_command_key);
		} else if (re_search(&rcommandBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.right_command_key);
		} else if (re_search(&lcontrolBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.left_control_key);
		} else if (re_search(&rcontrolBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.right_control_key);
		} else if (re_search(&loptionBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.left_option_key);
		} else if (re_search(&roptionBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.right_option_key);
		} else if (re_search(&menuBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%lx", &fKeys.menu_key);
		} else if (re_search(&locksettingsBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			fKeys.lock_settings = 0;
			for (int32 i=1; i<=3; i++) {
				if (regs.end[i] - regs.start[i] <= 0)
					break;
				if (strncmp(buffer + regs.start[i], "CapsLock", regs.end[i] - regs.start[i]) == 0)
					fKeys.lock_settings |= B_CAPS_LOCK;
				else if (strncmp(buffer + regs.start[i], "NumLock", regs.end[i] - regs.start[i]) == 0)
					fKeys.lock_settings |= B_NUM_LOCK;
				else if (strncmp(buffer + regs.start[i], "ScrollLock", regs.end[i] - regs.start[i]) == 0)
					fKeys.lock_settings |= B_SCROLL_LOCK;
			}
		} else if (re_search(&keyBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			uint32 keyCode; 
			if (sscanf(buffer + regs.start[1], "0x%lx", &keyCode) > 0) {
				
				for (int i=2; i<=10; i++) {
					maps[i-2][keyCode] = offset;
					ComputeChars(buffer, regs, i, offset);
				}
			}
		} else if (re_search(&acuteBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			for (int i=1; i<=2; i++) {
				fKeys.acute_dead_key[acuteOffset++] = offset;		
				ComputeChars(buffer, regs, i, offset);
			}			
		} else if (re_search(&graveBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			for (int i=1; i<=2; i++) {
				fKeys.grave_dead_key[graveOffset++] = offset;		
				ComputeChars(buffer, regs, i, offset);
			}			
		} else if (re_search(&circumflexBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			for (int i=1; i<=2; i++) {
				fKeys.circumflex_dead_key[circumflexOffset++] = offset;		
				ComputeChars(buffer, regs, i, offset);
			}			
		} else if (re_search(&diaeresisBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			for (int i=1; i<=2; i++) {
				fKeys.dieresis_dead_key[diaeresisOffset++] = offset;		
				ComputeChars(buffer, regs, i, offset);
			}			
		} else if (re_search(&tildeBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			for (int i=1; i<=2; i++) {
				fKeys.tilde_dead_key[tildeOffset++] = offset;		
				ComputeChars(buffer, regs, i, offset);
			}			
		} else if (re_search(&acutetabBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			ComputeTables(buffer, regs, fKeys.acute_tables);
		} else if (re_search(&gravetabBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			ComputeTables(buffer, regs, fKeys.grave_tables);
		} else if (re_search(&circumflextabBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			ComputeTables(buffer, regs, fKeys.circumflex_tables);
		} else if (re_search(&diaeresistabBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			ComputeTables(buffer, regs, fKeys.dieresis_tables);
		} else if (re_search(&tildetabBuf, buffer, strlen(buffer), 0, strlen(buffer), &regs) >= 0) {
			ComputeTables(buffer, regs, fKeys.tilde_tables);
		}
	}
	
	fCharsSize = offset;
	
	return B_OK;
}
	

// we save a map to a file
status_t 
Keymap::Save(entry_ref &ref)
{
	status_t err;
	
	BFile file(&ref, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE );
	if ((err = file.InitCheck()) != B_OK) {
		printf("error %s\n", strerror(err));
		return err;
	}
	
	for (uint32 i=0; i<sizeof(fKeys)/4; i++)
		((uint32*)&fKeys)[i] = B_HOST_TO_BENDIAN_INT32(((uint32*)&fKeys)[i]);
		
	if ((err = file.Write(&fKeys, sizeof(fKeys))) < (ssize_t)sizeof(fKeys)) {
		return err;
	}
	
	for (uint32 i=0; i<sizeof(fKeys)/4; i++)
		((uint32*)&fKeys)[i] = B_BENDIAN_TO_HOST_INT32(((uint32*)&fKeys)[i]);
	
	fCharsSize = B_HOST_TO_BENDIAN_INT32(fCharsSize);
	
	if ((err = file.Write(&fCharsSize, sizeof(uint32))) < (ssize_t)sizeof(uint32)) {
		return B_BAD_VALUE;
	}
	
	fCharsSize = B_BENDIAN_TO_HOST_INT32(fCharsSize);
	
	if ((err = file.Write(fChars, fCharsSize)) < (ssize_t)fCharsSize)
		return err;
	
	return B_OK;
}


const char header_header[] =
"/*\tHaiku \t*/\n"
"/*\n"
" This file is generated automatically. Don't edit ! \n"
"*/\n\n";

void
Keymap::SaveAsHeader(entry_ref &ref)
{
	status_t err;

	BFile file(&ref, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE );
	if ((err = file.InitCheck()) != B_OK) {
	        printf("error %s\n", strerror(err));
	        return;
	}

	int fd = file.Dup();
	FILE * f = fdopen(fd, "w");

	fprintf(f, "%s", header_header);
	fprintf(f, "#include <InterfaceDefs.h>\n\n");
	fprintf(f, "const key_map sSystemKeymap = {\n");
	fprintf(f, "\tversion:%ld,\n", fKeys.version);
	fprintf(f, "\tcaps_key:0x%lx,\n", fKeys.caps_key);
	fprintf(f, "\tscroll_key:0x%lx,\n", fKeys.scroll_key);
	fprintf(f, "\tnum_key:0x%lx,\n", fKeys.num_key);
	fprintf(f, "\tleft_shift_key:0x%lx,\n", fKeys.left_shift_key);
	fprintf(f, "\tright_shift_key:0x%lx,\n", fKeys.right_shift_key);
	fprintf(f, "\tleft_command_key:0x%lx,\n", fKeys.left_command_key);
	fprintf(f, "\tright_command_key:0x%lx,\n", fKeys.right_command_key);
	fprintf(f, "\tleft_control_key:0x%lx,\n", fKeys.left_control_key);
	fprintf(f, "\tright_control_key:0x%lx,\n", fKeys.right_control_key);
	fprintf(f, "\tleft_option_key:0x%lx,\n", fKeys.left_option_key);
	fprintf(f, "\tright_option_key:0x%lx,\n", fKeys.right_option_key);
	fprintf(f, "\tmenu_key:0x%lx,\n", fKeys.menu_key);
	fprintf(f, "\tlock_settings:0x%lx,\n", fKeys.lock_settings);
	
	fprintf(f, "\tcontrol_map:{\n");
	for (uint32 i=0; i<128; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.control_map[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\toption_caps_shift_map:{\n");
	for (uint32 i=0; i<128; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.option_caps_shift_map[i]);
	fprintf(f, "\t},\n");
		
	fprintf(f, "\toption_caps_map:{\n");
	for (uint32 i=0; i<128; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.option_caps_map[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\toption_shift_map:{\n");
	for (uint32 i=0; i<128; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.option_shift_map[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\toption_map:{\n");
	for (uint32 i=0; i<128; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.option_map[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\tcaps_shift_map:{\n");
	for (uint32 i=0; i<128; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.caps_shift_map[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\tcaps_map:{\n");
	for (uint32 i=0; i<128; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.caps_map[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\tshift_map:{\n");
	for (uint32 i=0; i<128; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.shift_map[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\tnormal_map:{\n");
	for (uint32 i=0; i<128; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.normal_map[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\tacute_dead_key:{\n");
	for (uint32 i=0; i<32; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.acute_dead_key[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\tgrave_dead_key:{\n");
	for (uint32 i=0; i<32; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.grave_dead_key[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\tcircumflex_dead_key:{\n");
	for (uint32 i=0; i<32; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.circumflex_dead_key[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\tdieresis_dead_key:{\n");
	for (uint32 i=0; i<32; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.dieresis_dead_key[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\ttilde_dead_key:{\n");
	for (uint32 i=0; i<32; i++)
		fprintf(f, "\t\t%ld,\n", fKeys.tilde_dead_key[i]);
	fprintf(f, "\t},\n");
	
	fprintf(f, "\tacute_tables:0x%lx,\n", fKeys.acute_tables);
	fprintf(f, "\tgrave_tables:0x%lx,\n", fKeys.grave_tables);
	fprintf(f, "\tcircumflex_tables:0x%lx,\n", fKeys.circumflex_tables);
	fprintf(f, "\tdieresis_tables:0x%lx,\n", fKeys.dieresis_tables);
	fprintf(f, "\ttilde_tables:0x%lx,\n", fKeys.tilde_tables);
	
	fprintf(f, "};\n\n");
	
	fprintf(f, "const char sSystemKeyChars[] = {\n");
	for (uint32 i=0; i<fCharsSize; i++)
		fprintf(f, "\t%hhd,\n", fChars[i]);
	fprintf(f, "};\n\n");
	
	fprintf(f, "const uint32 sSystemKeyCharsSize = %ld;\n", fCharsSize);
}


/* we need to know if a key is a modifier key to choose 
	a valid key when several are pressed together
*/
bool 
Keymap::IsModifierKey(uint32 keyCode)
{
	if ((keyCode == fKeys.caps_key)
		|| (keyCode == fKeys.num_key)
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

status_t _restore_key_map_();


void
Keymap::RestoreSystemDefault()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)!=B_OK)
		return;
	
	path.Append("Key_map");

	BEntry ref(path.Path());
	ref.Remove();	
	
	_restore_key_map_();
}


void
Keymap::SaveAsCurrent()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)!=B_OK)
		return;
	
	path.Append("Key_map");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);

	status_t err;
	if ((err = Save(ref)) != B_OK) {
		printf("error when saving : %s", strerror(err));
		return;
	}
	Use();
}


// we make our input server use the map in /boot/home/config/settings/Keymap
status_t
Keymap::Use()
{
	return _restore_key_map_();
}
