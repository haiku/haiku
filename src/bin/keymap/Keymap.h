/*
 *	Copyright (c) 2004-2006, Haiku, Inc.
 *
 *  This software is part of the Haiku distribution and is covered 
 *  by the Haiku license.
 *
 *  Author: Jérôme Duval
 */
#ifndef KEYMAP_H
#define KEYMAP_H


#include <InterfaceDefs.h>
#include <Entry.h>

#include <stdio.h>


class Keymap {
	public:
		Keymap();
		~Keymap();

		status_t LoadCurrent();
		status_t Load(entry_ref &ref);
		status_t Save(entry_ref &ref);
		status_t LoadSource(FILE *f);
		status_t LoadSourceFromRef(entry_ref &ref);
		void SaveAsCurrent();
		void SaveAsHeader(entry_ref &ref);
		status_t Use();
		void Dump();
		bool IsModifierKey(uint32 keyCode);
		uint8 IsDeadKey(uint32 keyCode, uint32 modifiers);
		bool IsDeadSecondKey(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey);
		void GetChars(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey,
			char** chars, int32* numBytes);
		void RestoreSystemDefault();
		static void GetKey(char *chars, int32 offset, char* string);

	private:
		void ComputeChars(const char *buffer, struct re_registers &regs, int i, int &offset);
		void ComputeTables(const char *buffer, struct re_registers &regs, uint32 &table);

		char *fChars;
		key_map fKeys;
		uint32 fCharsSize;
};

#define KEYMAP_ERROR_UNKNOWN_VERSION	(B_ERRORS_END + 1)

#endif // KEYMAP_H
