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

#ifndef KEYMAP_H
#define KEYMAP_H

#include <InterfaceDefs.h>
#include <Entry.h>

class Keymap 
{
public:
	Keymap();
	void DumpKeymap();
	bool IsModifierKey(uint32 keyCode);
	uint8 IsDeadKey(uint32 keyCode, uint32 modifiers);
	bool IsDeadSecondKey(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey);
	void GetChars(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey, char** chars, int32* numBytes);
private:
	char *fChars;
	key_map fKeys;
	//uint32 fCharsSize;
};


#endif //KEYMAP_H
