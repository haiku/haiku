/*
 * Copyright 2004-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de.
 */
#ifndef KEYMAP_H
#define KEYMAP_H


#include <InterfaceDefs.h>

#if (defined(__BEOS__) || defined(__HAIKU__))
#	include <TextView.h>
#endif

#include <stdio.h>
#include <regex.h>


class Keymap {
public:
							Keymap();
							~Keymap();

			status_t		LoadCurrent();
			status_t		Load(const char* name);
			status_t		Load(FILE* file);
			status_t		LoadSource(const char* name);
			status_t		LoadSource(FILE* file);
			status_t		SaveAsCurrent();
			status_t		Save(const char* name);
			status_t		SaveAsSource(const char* name);
			status_t		SaveAsSource(FILE* file);
			status_t		SaveAsCppHeader(const char* name,
								const char* mapName);

			status_t		Use();

			bool			IsModifierKey(uint32 keyCode);
			uint8			IsDeadKey(uint32 keyCode, uint32 modifiers);
			bool			IsDeadSecondKey(uint32 keyCode, uint32 modifiers,
								uint8 activeDeadKey);
			void			GetChars(uint32 keyCode, uint32 modifiers,
								uint8 activeDeadKey, char** chars,
								int32* numBytes);
			void			RestoreSystemDefault();
	static	bool			GetKey(const char* chars, int32 offset,
								char* buffer, size_t bufferSize);

private:
#if (defined(__BEOS__) || defined(__HAIKU__))
			void			_SaveSourceText(FILE* file,
								text_run_array** _textRuns = NULL);
#else
			void			_SaveSourceText(FILE* file);
#endif
			void			_ComputeChars(const char* buffer,
								struct re_registers& regs, int i, int& offset);
			void			_ComputeTables(const char* buffer,
								struct re_registers& regs, uint32& table);

			char*			fChars;
			key_map			fKeys;
			uint32			fCharsSize;
};

#define KEYMAP_ERROR_UNKNOWN_VERSION	(B_ERRORS_END + 1)

#endif // KEYMAP_H
