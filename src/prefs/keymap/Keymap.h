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
//  Description: Keymap Preferences
//  Created :    July 12, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef KEYMAP_H
#define KEYMAP_H

#include <InterfaceDefs.h>
#include <Entry.h>

class Keymap 
{
public:
	status_t Load(entry_ref &ref);
	status_t Save(entry_ref &ref);
	void DumpKeymap();
	void GetChars(uint32 keyCode, uint32 modifiers, char **chars, int32 *numBytes);
	bool IsModifierKey(uint32 keyCode);
	uint8 IsDeadKey(uint32 keyCode, uint32 modifiers);
	bool IsDeadSecondKey(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey);
	void DeadKey(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey, char** chars, int32* numBytes);
	status_t Use();
private:
	char *fChars;
	key_map fKeys;
	uint32 fCharsSize;
};


#endif //KEYMAP_H
