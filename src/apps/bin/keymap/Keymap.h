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
//  Description: keymap bin
//  Created :    July 30, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef KEYMAP_H
#define KEYMAP_H

#include <InterfaceDefs.h>
#include <Entry.h>

class Keymap 
{
public:
	void LoadCurrent();
	status_t Load(entry_ref &ref);
	status_t Save(entry_ref &ref);
	status_t LoadSource(entry_ref &ref);
	void Dump();
	bool IsModifierKey(uint32 keyCode);
	uint8 IsDeadKey(uint32 keyCode, uint32 modifiers);
	bool IsDeadSecondKey(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey);
	void GetChars(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey, char** chars, int32* numBytes);
	void RestoreSystemDefault();
	static void GetKey( char *chars, int32 offset, char* string);
private:
	void ComputeChars(const char *buffer, struct re_registers &regs, int i, int &offset);

	char *fChars;
	key_map fKeys;
	uint32 fCharsSize;
};


#endif //KEYMAP_H
