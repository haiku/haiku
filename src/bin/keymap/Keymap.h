/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de.
 */
#ifndef KEYMAP_H
#define KEYMAP_H


#include <Keymap.h>

#if (defined(__BEOS__) || defined(__HAIKU__))
#	include <TextView.h>
#endif

#include <stdio.h>
#define __USE_GNU
#include <regex.h>


class Keymap : public BKeymap {
public:
								Keymap();
								~Keymap();

			status_t			LoadSource(const char* name);
			status_t			LoadSource(FILE* file);
			status_t			SaveAsCurrent();
			status_t			Save(const char* name);
			status_t			SaveAsSource(const char* name);
			status_t			SaveAsSource(FILE* file);
			status_t			SaveAsCppHeader(const char* name,
									const char* mapName);

			status_t			Use();

			void				RestoreSystemDefault();

	static	bool				GetKey(const char* chars, int32 offset,
									char* buffer, size_t bufferSize);

private:
#if (defined(__BEOS__) || defined(__HAIKU__))
			void				_SaveSourceText(FILE* file,
									text_run_array** _textRuns = NULL);
#else
			void				_SaveSourceText(FILE* file);
#endif
			void				_ComputeChars(const char* buffer,
									struct re_registers& regs, int i,
									int& offset);
			void				_ComputeTables(const char* buffer,
									struct re_registers& regs, uint32& table);
};

#define KEYMAP_ERROR_UNKNOWN_VERSION	(B_ERRORS_END + 1)

#endif // KEYMAP_H
