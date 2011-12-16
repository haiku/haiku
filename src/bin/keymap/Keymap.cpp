/*
 * Copyright 2004-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include "Keymap.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <ByteOrder.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>


#define CHARS_TABLE_MAXSIZE  10000


extern status_t _restore_key_map_();


// i couldn't put patterns and pattern bufs on the stack without segfaulting
// regexp patterns
const char versionPattern[]
	= "Version[[:space:]]+=[[:space:]]+\\([[:digit:]]+\\)";
const char capslockPattern[]
	= "CapsLock[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char scrolllockPattern[]
	= "ScrollLock[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char numlockPattern[]
	= "NumLock[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char lshiftPattern[] = "LShift[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char rshiftPattern[] = "RShift[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char lcommandPattern[]
	= "LCommand[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char rcommandPattern[]
	= "RCommand[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char lcontrolPattern[]
	= "LControl[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char rcontrolPattern[]
	= "RControl[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char loptionPattern[]
	= "LOption[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char roptionPattern[]
	= "ROption[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char menuPattern[] = "Menu[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\)";
const char locksettingsPattern[]
	= "LockSettings[[:space:]]+=[[:space:]]+\\([[:alnum:]]*\\)"
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


const char acutePattern[] = "Acute[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
	"[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
const char gravePattern[] = "Grave[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
	"[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
const char circumflexPattern[] = "Circumflex[[:space:]]+\\([[:alnum:]]"
	"+\\|'.*'\\)[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
const char diaeresisPattern[] = "Diaeresis[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
	"[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
const char tildePattern[] = "Tilde[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)"
	"[[:space:]]+=[[:space:]]+\\([[:alnum:]]+\\|'.*'\\)[[:space:]]+";
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


void
dump_map(FILE* file, const char* name, int32* map)
{
	fprintf(file, "\t%s:{\n", name);

	for (uint32 i = 0; i < 16; i++) {
		fprintf(file, "\t\t");
		for (uint32 j = 0; j < 8; j++) {
			fprintf(file, "0x%04" B_PRIx32 ",%s", map[i * 8 + j],
				j < 7 ? " " : "");
		}
		fprintf(file, "\n");
	}
	fprintf(file, "\t},\n");
}


void
dump_keys(FILE* file, const char* name, int32* keys)
{
	fprintf(file, "\t%s:{\n", name);

	for (uint32 i = 0; i < 4; i++) {
		fprintf(file, "\t\t");
		for (uint32 j = 0; j < 8; j++) {
			fprintf(file, "0x%04" B_PRIx32 ",%s", keys[i * 8 + j],
				j < 7 ? " " : "");
		}
		fprintf(file, "\n");
	}
	fprintf(file, "\t},\n");
}


//	#pragma mark -


Keymap::Keymap()
{
}


Keymap::~Keymap()
{
}


status_t
Keymap::LoadSource(const char* name)
{
	FILE* file = fopen(name, "r");
	if (file == NULL)
		return errno;

	status_t status = LoadSource(file);
	fclose(file);

	return status;
}


status_t
Keymap::LoadSource(FILE* file)
{
	// Setup regexp parser

	reg_syntax_t syntax = RE_CHAR_CLASSES;
	re_set_syntax(syntax);

	const char* error = re_compile_pattern(versionPattern,
		strlen(versionPattern), &versionBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(capslockPattern, strlen(capslockPattern),
		&capslockBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(scrolllockPattern, strlen(scrolllockPattern),
		&scrolllockBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(numlockPattern, strlen(numlockPattern),
		&numlockBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(lshiftPattern, strlen(lshiftPattern), &lshiftBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(rshiftPattern, strlen(rshiftPattern), &rshiftBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(lcommandPattern, strlen(lcommandPattern),
		&lcommandBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(rcommandPattern, strlen(rcommandPattern),
		&rcommandBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(lcontrolPattern, strlen(lcontrolPattern),
		&lcontrolBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(rcontrolPattern, strlen(rcontrolPattern),
		&rcontrolBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(loptionPattern, strlen(loptionPattern),
		&loptionBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(roptionPattern, strlen(roptionPattern),
		&roptionBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(menuPattern, strlen(menuPattern), &menuBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(locksettingsPattern, strlen(locksettingsPattern),
		&locksettingsBuf);
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
	error = re_compile_pattern(circumflexPattern, strlen(circumflexPattern),
		&circumflexBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(diaeresisPattern, strlen(diaeresisPattern),
		&diaeresisBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(tildePattern, strlen(tildePattern), &tildeBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(acutetabPattern, strlen(acutetabPattern),
		&acutetabBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(gravetabPattern, strlen(gravetabPattern),
		&gravetabBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(circumflextabPattern,
		strlen(circumflextabPattern), &circumflextabBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(diaeresistabPattern, strlen(diaeresistabPattern),
		&diaeresistabBuf);
	if (error)
		fprintf(stderr, error);
	error = re_compile_pattern(tildetabPattern, strlen(tildetabPattern),
		&tildetabBuf);
	if (error)
		fprintf(stderr, error);

	// Read file

	delete[] fChars;
	fChars = new char[CHARS_TABLE_MAXSIZE];
	fCharsSize = CHARS_TABLE_MAXSIZE;
	int offset = 0;
	int acuteOffset = 0;
	int graveOffset = 0;
	int circumflexOffset = 0;
	int diaeresisOffset = 0;
	int tildeOffset = 0;

	int32* maps[] = {
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

	char buffer[1024];

	while (fgets(buffer, sizeof(buffer) - 1, file) != NULL) {
		if (buffer[0] == '#' || buffer[0] == '\n')
			continue;

		size_t length = strlen(buffer);

		struct re_registers regs;
		if (re_search(&versionBuf, buffer, length, 0, length, &regs) >= 0) {
			sscanf(buffer + regs.start[1], "%" B_SCNu32, &fKeys.version);
		} else if (re_search(&capslockBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32, &fKeys.caps_key);
		} else if (re_search(&scrolllockBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32, &fKeys.scroll_key);
		} else if (re_search(&numlockBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32, &fKeys.num_key);
		} else if (re_search(&lshiftBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32,
				&fKeys.left_shift_key);
		} else if (re_search(&rshiftBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32,
				&fKeys.right_shift_key);
		} else if (re_search(&lcommandBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32,
				&fKeys.left_command_key);
		} else if (re_search(&rcommandBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32,
				&fKeys.right_command_key);
		} else if (re_search(&lcontrolBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32,
				&fKeys.left_control_key);
		} else if (re_search(&rcontrolBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32,
				&fKeys.right_control_key);
		} else if (re_search(&loptionBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32,
				&fKeys.left_option_key);
		} else if (re_search(&roptionBuf, buffer, length, 0, length, &regs)
				>= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32,
				&fKeys.right_option_key);
		} else if (re_search(&menuBuf, buffer, length, 0, length, &regs) >= 0) {
			sscanf(buffer + regs.start[1], "0x%" B_SCNx32, &fKeys.menu_key);
		} else if (re_search(&locksettingsBuf, buffer, length, 0, length, &regs)
				>= 0) {
			fKeys.lock_settings = 0;
			for (int32 i = 1; i <= 3; i++) {
				const char* start = buffer + regs.start[i];
				length = regs.end[i] - regs.start[i];
				if (length == 0)
					break;

				if (!strncmp(start, "CapsLock", length))
					fKeys.lock_settings |= B_CAPS_LOCK;
				else if (!strncmp(start, "NumLock", length))
					fKeys.lock_settings |= B_NUM_LOCK;
				else if (!strncmp(start, "ScrollLock", length))
					fKeys.lock_settings |= B_SCROLL_LOCK;
			}
		} else if (re_search(&keyBuf, buffer, length, 0, length, &regs) >= 0) {
			uint32 keyCode;
			if (sscanf(buffer + regs.start[1], "0x%" B_SCNx32, &keyCode) > 0) {
				for (int i = 2; i <= 10; i++) {
					maps[i - 2][keyCode] = offset;
					_ComputeChars(buffer, regs, i, offset);
				}
			}
		} else if (re_search(&acuteBuf, buffer, length, 0, length, &regs) >= 0) {
			for (int i = 1; i <= 2; i++) {
				fKeys.acute_dead_key[acuteOffset++] = offset;
				_ComputeChars(buffer, regs, i, offset);
			}
		} else if (re_search(&graveBuf, buffer, length, 0, length, &regs) >= 0) {
			for (int i = 1; i <= 2; i++) {
				fKeys.grave_dead_key[graveOffset++] = offset;
				_ComputeChars(buffer, regs, i, offset);
			}
		} else if (re_search(&circumflexBuf, buffer, length, 0, length, &regs)
				>= 0) {
			for (int i = 1; i <= 2; i++) {
				fKeys.circumflex_dead_key[circumflexOffset++] = offset;
				_ComputeChars(buffer, regs, i, offset);
			}
		} else if (re_search(&diaeresisBuf, buffer, length, 0, length, &regs)
				>= 0) {
			for (int i = 1; i <= 2; i++) {
				fKeys.dieresis_dead_key[diaeresisOffset++] = offset;
				_ComputeChars(buffer, regs, i, offset);
			}
		} else if (re_search(&tildeBuf, buffer, length, 0, length, &regs) >= 0) {
			for (int i = 1; i <= 2; i++) {
				fKeys.tilde_dead_key[tildeOffset++] = offset;
				_ComputeChars(buffer, regs, i, offset);
			}
		} else if (re_search(&acutetabBuf, buffer, length, 0, length, &regs)
				>= 0) {
			_ComputeTables(buffer, regs, fKeys.acute_tables);
		} else if (re_search(&gravetabBuf, buffer, length, 0, length, &regs)
				>= 0) {
			_ComputeTables(buffer, regs, fKeys.grave_tables);
		} else if (re_search(&circumflextabBuf, buffer, length, 0, length, &regs)
				>= 0) {
			_ComputeTables(buffer, regs, fKeys.circumflex_tables);
		} else if (re_search(&diaeresistabBuf, buffer, length, 0, length, &regs)
				>= 0) {
			_ComputeTables(buffer, regs, fKeys.dieresis_tables);
		} else if (re_search(&tildetabBuf, buffer, length, 0, length, &regs)
				>= 0) {
			_ComputeTables(buffer, regs, fKeys.tilde_tables);
		}
	}

	fCharsSize = offset;

	if (fKeys.version != 3)
		return KEYMAP_ERROR_UNKNOWN_VERSION;

	return B_OK;
}


status_t
Keymap::SaveAsCurrent()
{
#if (defined(__BEOS__) || defined(__HAIKU__))
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (status != B_OK)
		return status;

	path.Append("Key_map");

	status = Save(path.Path());
	if (status != B_OK)
		return status;

	Use();
	return B_OK;
#else	// ! __BEOS__
	fprintf(stderr, "Unsupported operation on this platform!\n");
	exit(1);
#endif	// ! __BEOS__
}


//! Save a binary keymap to a file.
status_t
Keymap::Save(const char* name)
{
	BFile file;
	status_t status = file.SetTo(name,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	// convert to big endian
	for (uint32 i = 0; i < sizeof(fKeys) / sizeof(uint32); i++) {
		((uint32*)&fKeys)[i] = B_HOST_TO_BENDIAN_INT32(((uint32*)&fKeys)[i]);
	}

	ssize_t bytesWritten = file.Write(&fKeys, sizeof(fKeys));

	// convert endian back
	for (uint32 i = 0; i < sizeof(fKeys) / sizeof(uint32); i++) {
		((uint32*)&fKeys)[i] = B_BENDIAN_TO_HOST_INT32(((uint32*)&fKeys)[i]);
	}

	if (bytesWritten < (ssize_t)sizeof(fKeys))
		return B_ERROR;
	if (bytesWritten < B_OK)
		return bytesWritten;

	uint32 charSize = B_HOST_TO_BENDIAN_INT32(fCharsSize);

	bytesWritten = file.Write(&charSize, sizeof(uint32));
	if (bytesWritten < (ssize_t)sizeof(uint32))
		return B_ERROR;
	if (bytesWritten < B_OK)
		return bytesWritten;

	bytesWritten = file.Write(fChars, fCharsSize);
	if (bytesWritten < (ssize_t)fCharsSize)
		return B_ERROR;
	if (bytesWritten < B_OK)
		return bytesWritten;

	return B_OK;
}


status_t
Keymap::SaveAsSource(const char* name)
{
	FILE* file = fopen(name, "w");
	if (file == NULL)
		return errno;

#if (defined(__BEOS__) || defined(__HAIKU__))
	text_run_array* textRuns;
	_SaveSourceText(file, &textRuns);

	if (textRuns != NULL) {
		ssize_t dataSize;
		void* data = BTextView::FlattenRunArray(textRuns, &dataSize);
		if (data != NULL) {
			BNode node(name);
			node.WriteAttr("styles", B_RAW_TYPE, 0, data, dataSize);

			free(data);
		}

		BTextView::FreeRunArray(textRuns);
	}
#else
	_SaveSourceText(file);
#endif

	fclose(file);

	return B_OK;
}


status_t
Keymap::SaveAsSource(FILE* file)
{
	_SaveSourceText(file);
	return B_OK;
}


/*!	Save a keymap as C source file - this is used to get the default keymap
	into the input_server, for example.
	\a mapName is usually the path of the input keymap, and is used as the
	name of the keymap (the path will be removed, as well as its suffix).
*/
status_t
Keymap::SaveAsCppHeader(const char* fileName, const char* mapName)
{
	BString name = mapName;

	// cut off path
	int32 index = name.FindLast('/');
	if (index > 0)
		name.Remove(0, index + 1);

	// prune ".keymap"
	index = name.FindLast('.');
	if (index > 0)
		name.Truncate(index);

	FILE* file = fopen(fileName, "w");
	if (file == NULL)
		return errno;

	fprintf(file, "/*\n"
		" * Haiku Keymap\n"
		" * This file is generated automatically. Don't edit!\n"
		" */\n\n");
	fprintf(file, "#include <InterfaceDefs.h>\n\n");
	fprintf(file, "const char *kSystemKeymapName = \"%s\";\n\n", name.String());
	fprintf(file, "const key_map kSystemKeymap = {\n");
	fprintf(file, "\tversion:%" B_PRIu32 ",\n", fKeys.version);
	fprintf(file, "\tcaps_key:0x%" B_PRIx32 ",\n", fKeys.caps_key);
	fprintf(file, "\tscroll_key:0x%" B_PRIx32 ",\n", fKeys.scroll_key);
	fprintf(file, "\tnum_key:0x%" B_PRIx32 ",\n", fKeys.num_key);
	fprintf(file, "\tleft_shift_key:0x%" B_PRIx32 ",\n", fKeys.left_shift_key);
	fprintf(file, "\tright_shift_key:0x%" B_PRIx32 ",\n",
		fKeys.right_shift_key);
	fprintf(file, "\tleft_command_key:0x%" B_PRIx32 ",\n",
		fKeys.left_command_key);
	fprintf(file, "\tright_command_key:0x%" B_PRIx32 ",\n",
		fKeys.right_command_key);
	fprintf(file, "\tleft_control_key:0x%" B_PRIx32 ",\n",
		fKeys.left_control_key);
	fprintf(file, "\tright_control_key:0x%" B_PRIx32 ",\n",
		fKeys.right_control_key);
	fprintf(file, "\tleft_option_key:0x%" B_PRIx32 ",\n",
		fKeys.left_option_key);
	fprintf(file, "\tright_option_key:0x%" B_PRIx32 ",\n",
		fKeys.right_option_key);
	fprintf(file, "\tmenu_key:0x%" B_PRIx32 ",\n", fKeys.menu_key);
	fprintf(file, "\tlock_settings:0x%" B_PRIx32 ",\n", fKeys.lock_settings);

	dump_map(file, "control_map", fKeys.control_map);
	dump_map(file, "option_caps_shift_map", fKeys.option_caps_shift_map);
	dump_map(file, "option_caps_map", fKeys.option_caps_map);
	dump_map(file, "option_shift_map", fKeys.option_shift_map);
	dump_map(file, "option_map", fKeys.option_map);
	dump_map(file, "caps_shift_map", fKeys.caps_shift_map);
	dump_map(file, "caps_map", fKeys.caps_map);
	dump_map(file, "shift_map", fKeys.shift_map);
	dump_map(file, "normal_map", fKeys.normal_map);

	dump_keys(file, "acute_dead_key", fKeys.acute_dead_key);
	dump_keys(file, "grave_dead_key", fKeys.grave_dead_key);

	dump_keys(file, "circumflex_dead_key", fKeys.circumflex_dead_key);
	dump_keys(file, "dieresis_dead_key", fKeys.dieresis_dead_key);
	dump_keys(file, "tilde_dead_key", fKeys.tilde_dead_key);

	fprintf(file, "\tacute_tables:0x%" B_PRIx32 ",\n", fKeys.acute_tables);
	fprintf(file, "\tgrave_tables:0x%" B_PRIx32 ",\n", fKeys.grave_tables);
	fprintf(file, "\tcircumflex_tables:0x%" B_PRIx32 ",\n",
		fKeys.circumflex_tables);
	fprintf(file, "\tdieresis_tables:0x%" B_PRIx32 ",\n",
		fKeys.dieresis_tables);
	fprintf(file, "\ttilde_tables:0x%" B_PRIx32 ",\n", fKeys.tilde_tables);

	fprintf(file, "};\n\n");

	fprintf(file, "const char kSystemKeyChars[] = {\n");
	for (uint32 i = 0; i < fCharsSize; i++) {
		if (i % 10 == 0) {
			if (i > 0)
				fprintf(file, "\n");
			fprintf(file, "\t");
		} else
			fprintf(file, " ");

		fprintf(file, "0x%02x,", (uint8)fChars[i]);
	}
	fprintf(file, "\n};\n\n");

	fprintf(file, "const uint32 kSystemKeyCharsSize = %" B_PRIu32 ";\n",
		fCharsSize);
	fclose(file);

	return B_OK;
}


//! We make our input server use the map in /boot/home/config/settings/Keymap
status_t
Keymap::Use()
{
#if (defined(__BEOS__) || defined(__HAIKU__))
	return _restore_key_map_();

#else	// ! __BEOS__
	fprintf(stderr, "Unsupported operation on this platform!\n");
	exit(1);
#endif	// ! __BEOS__
}


void
Keymap::RestoreSystemDefault()
{
#if (defined(__BEOS__) || defined(__HAIKU__))
	// work-around to get rid of this stupid find_directory_r() on Zeta
#	ifdef find_directory
#		undef find_directory
#	endif
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("Key_map");

	BEntry entry(path.Path());
	entry.Remove();

	_restore_key_map_();
#else	// ! __BEOS__
	fprintf(stderr, "Unsupported operation on this platform!\n");
	exit(1);
#endif	// ! __BEOS__
}


/*static*/ bool
Keymap::GetKey(const char* chars, int32 offset, char* buffer, size_t bufferSize)
{
	uint8 size = (uint8)chars[offset++];
	char string[1024];

	switch (size) {
		case 0:
			// Not mapped
			strlcpy(buffer, "''", bufferSize);
			return false;

		case 1:
			// 1-byte UTF-8/ASCII character
			if ((uint8)chars[offset] < 0x20 || (uint8)chars[offset] > 0x7e)
				sprintf(string, "0x%02x", (uint8)chars[offset]);
			else {
				sprintf(string, "'%s%c'",
					(chars[offset] == '\\' || chars[offset] == '\'') ? "\\" : "",
					chars[offset]);
			}
			break;

		default:
			// multi-byte UTF-8 character
			sprintf(string, "0x");
			for (int i = 0; i < size; i++) {
				sprintf(string + 2 * (i + 1), "%02x", (uint8)chars[offset + i]);
			}
			break;
	}

	strlcpy(buffer, string, bufferSize);
	return true;
}


#if (defined(__BEOS__) || defined(__HAIKU__))
void
Keymap::_SaveSourceText(FILE* file, text_run_array** _textRuns)
{
	text_run_array* runs = NULL;
	if (_textRuns != NULL) {
		runs = BTextView::AllocRunArray(8);
		*_textRuns = runs;
	}
#else
void
Keymap::_SaveSourceText(FILE* file)
{
#endif

	static const rgb_color kCommentColor = (rgb_color){200, 92, 92, 255};
	static const rgb_color kTextColor = (rgb_color){0, 0, 0, 255};

#if (defined(__BEOS__) || defined(__HAIKU__))
	BFont font = *be_fixed_font;

	if (runs != NULL) {
		runs->runs[0].offset = 0;
		runs->runs[0].font = font;
		runs->runs[0].color = kCommentColor;
	}
#endif

	int bytes = fprintf(file, "#!/bin/keymap -s\n"
		"#\n"
		"#\tRaw key numbering for 102-key keyboard...\n");

#if (defined(__BEOS__) || defined(__HAIKU__))
	if (runs != NULL) {
		runs->runs[1].offset = bytes;
		runs->runs[1].font = font;
		runs->runs[1].font.SetSize(9);
		runs->runs[1].color = kCommentColor;
	}
#endif

	bytes += fprintf(file, "#                                                                                          [sys]       [brk]\n"
		"#                                                                                           0x7e        0x7f\n"
		"# [esc]   [ f1] [ f2] [ f3] [ f4]    [ f5] [ f6] [ f7] [ f8]    [ f9] [f10] [f11] [f12]    [prn] [scr] [pau]\n"
		"#  0x01    0x02  0x03  0x04  0x05     0x06  0x07  0x08  0x09     0x0a  0x0b  0x0c  0x0d     0x0e  0x0f  0x10     K E Y P A D   K E Y S\n"
		"#\n"
		"# [ ` ] [ 1 ] [ 2 ] [ 3 ] [ 4 ] [ 5 ] [ 6 ] [ 7 ] [ 8 ] [ 9 ] [ 0 ] [ - ] [ = ] [ bck ]    [ins] [hme] [pup]    [num] [ / ] [ * ] [ - ]\n"
		"#  0x11  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1a  0x1b  0x1c  0x1d   0x1e      0x1f  0x20  0x21     0x22  0x23  0x24  0x25\n"
		"#\n"
		"# [ tab ] [ q ] [ w ] [ e ] [ r ] [ t ] [ y ] [ u ] [ i ] [ o ] [ p ] [ [ ] [ ] ] [ \\ ]    [del] [end] [pdn]    [ 7 ] [ 8 ] [ 9 ] [ + ]\n"
		"#   0x26   0x27  0x28  0x29  0x2a  0x2b  0x2c  0x2d  0x2e  0x2f  0x30  0x31  0x32  0x33     0x34  0x35  0x36     0x37  0x38  0x39  0x3a\n"
		"#\n"
		"# [ caps ] [ a ] [ s ] [ d ] [ f ] [ g ] [ h ] [ j ] [ k ] [ l ] [ ; ] [ ' ]  [ enter ]                         [ 4 ] [ 5 ] [ 6 ]\n"
		"#   0x3b    0x3c  0x3d  0x3e  0x3f  0x40  0x41  0x42  0x43  0x44  0x45  0x46     0x47                            0x48  0x49  0x4a\n"
		"#\n"
		"# [shft] [ \\ ] [ z ] [ x ] [ c ] [ v ] [ b ] [ n ] [ m ] [ , ] [ . ] [ / ]    [ shift ]          [ up]          [ 1 ] [ 2 ] [ 3 ] [ent]\n"
		"#  0x4b   0x69  0x4c  0x4d  0x4e  0x4f  0x50  0x51  0x52  0x53  0x54  0x55       0x56             0x57           0x58  0x59  0x5a  0x5b\n"
		"#\n"
		"# [ ctrl ]          [ cmd ]             [ space ]             [ cmd ]          [ ctrl ]    [lft] [dwn] [rgt]    [ 0 ] [ . ]\n"
		"#   0x5c              0x5d                 0x5e                 0x5f             0x60       0x61  0x62  0x63     0x64  0x65\n");

#if (defined(__BEOS__) || defined(__HAIKU__))
	if (runs != NULL) {
		runs->runs[2].offset = bytes;
		runs->runs[2].font = font;
		runs->runs[2].color = kCommentColor;
	}
#endif

	bytes += fprintf(file, "#\n"
		"#\tNOTE: Key 0x69 does not exist on US keyboards\n"
		"#\tNOTE: On a Microsoft Natural Keyboard:\n"
		"#\t\t\tleft option  = 0x66\n"
		"#\t\t\tright option = 0x67\n"
		"#\t\t\tmenu key     = 0x68\n"
		"#\tNOTE: On an Apple Extended Keyboard:\n"
		"#\t\t\tleft option  = 0x66\n"
		"#\t\t\tright option = 0x67\n"
		"#\t\t\tkeypad '='   = 0x6a\n"
		"#\t\t\tpower key    = 0x6b\n");

#if (defined(__BEOS__) || defined(__HAIKU__))
	if (runs != NULL) {
		runs->runs[3].offset = bytes;
		runs->runs[3].font = *be_fixed_font;
		runs->runs[3].color = kTextColor;
	}
#endif

	bytes += fprintf(file, "Version = %" B_PRIu32 "\n"
		"CapsLock = 0x%02" B_PRIx32 "\n"
		"ScrollLock = 0x%02" B_PRIx32 "\n"
		"NumLock = 0x%02" B_PRIx32 "\n"
		"LShift = 0x%02" B_PRIx32 "\n"
		"RShift = 0x%02" B_PRIx32 "\n"
		"LCommand = 0x%02" B_PRIx32 "\n"
		"RCommand = 0x%02" B_PRIx32 "\n"
		"LControl = 0x%02" B_PRIx32 "\n"
		"RControl = 0x%02" B_PRIx32 "\n"
		"LOption = 0x%02" B_PRIx32 "\n"
		"ROption = 0x%02" B_PRIx32 "\n"
		"Menu = 0x%02" B_PRIx32 "\n",
		fKeys.version, fKeys.caps_key, fKeys.scroll_key, fKeys.num_key,
		fKeys.left_shift_key, fKeys.right_shift_key, fKeys.left_command_key,
		fKeys.right_command_key, fKeys.left_control_key, fKeys.right_control_key,
		fKeys.left_option_key, fKeys.right_option_key, fKeys.menu_key);

#if (defined(__BEOS__) || defined(__HAIKU__))
	if (runs != NULL) {
		runs->runs[4].offset = bytes;
		runs->runs[4].font = *be_fixed_font;
		runs->runs[4].color = kCommentColor;
	}
#endif

	bytes += fprintf(file, "#\n"
		"# Lock settings\n"
		"# To set NumLock, do the following:\n"
		"#   LockSettings = NumLock\n"
		"#\n"
		"# To set everything, do the following:\n"
		"#   LockSettings = CapsLock NumLock ScrollLock\n"
		"#\n");

#if (defined(__BEOS__) || defined(__HAIKU__))
	if (runs != NULL) {
		runs->runs[5].offset = bytes;
		runs->runs[5].font = *be_fixed_font;
		runs->runs[5].color = kTextColor;
	}
#endif

	bytes += fprintf(file, "LockSettings = ");
	if ((fKeys.lock_settings & B_CAPS_LOCK) != 0)
		bytes += fprintf(file, "CapsLock ");
	if ((fKeys.lock_settings & B_NUM_LOCK) != 0)
		bytes += fprintf(file, "NumLock ");
	if ((fKeys.lock_settings & B_SCROLL_LOCK) != 0)
		bytes += fprintf(file, "ScrollLock ");
	bytes += fprintf(file, "\n");

#if (defined(__BEOS__) || defined(__HAIKU__))
	if (runs != NULL) {
		runs->runs[6].offset = bytes;
		runs->runs[6].font = *be_fixed_font;
		runs->runs[6].color = kCommentColor;
	}
#endif

	bytes += fprintf(file, "# Legend:\n"
		"#   n = Normal\n"
		"#   s = Shift\n"
		"#   c = Control\n"
		"#   C = CapsLock\n"
		"#   o = Option\n"
		"# Key      n        s        c        o        os       C        Cs       Co       Cos     \n");

#if (defined(__BEOS__) || defined(__HAIKU__))
	if (runs != NULL) {
		runs->runs[7].offset = bytes;
		runs->runs[7].font = *be_fixed_font;
		runs->runs[7].color = kTextColor;
	}
#endif

	for (int i = 0; i < 128; i++) {
		char normalKey[32];
		char shiftKey[32];
		char controlKey[32];
		char optionKey[32];
		char optionShiftKey[32];
		char capsKey[32];
		char capsShiftKey[32];
		char optionCapsKey[32];
		char optionCapsShiftKey[32];

		GetKey(fChars, fKeys.normal_map[i], normalKey, 32);
		GetKey(fChars, fKeys.shift_map[i], shiftKey, 32);
		GetKey(fChars, fKeys.control_map[i], controlKey, 32);
		GetKey(fChars, fKeys.option_map[i], optionKey, 32);
		GetKey(fChars, fKeys.option_shift_map[i], optionShiftKey, 32);
		GetKey(fChars, fKeys.caps_map[i], capsKey, 32);
		GetKey(fChars, fKeys.caps_shift_map[i], capsShiftKey, 32);
		GetKey(fChars, fKeys.option_caps_map[i], optionCapsKey, 32);
		GetKey(fChars, fKeys.option_caps_shift_map[i], optionCapsShiftKey, 32);

		fprintf(file, "Key 0x%02x = %-9s%-9s%-9s%-9s%-9s%-9s%-9s%-9s%-9s\n", i,
			normalKey, shiftKey, controlKey, optionKey, optionShiftKey, capsKey,
			capsShiftKey, optionCapsKey, optionCapsShiftKey);
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

	for (int i = 0; i < 5; i++) {
		for (int deadIndex = 0; deadIndex < 32; deadIndex++) {
			char deadKey[32];
			char secondKey[32];
			if (!GetKey(fChars, deadOffsets[i][deadIndex++], deadKey, 32))
				break;

			GetKey(fChars, deadOffsets[i][deadIndex], secondKey, 32);
			fprintf(file, "%s %-9s = %-9s\n", labels[i], deadKey, secondKey);
		}

		fprintf(file, "%sTab = ", labels[i]);

		if (deadTables[i] & B_NORMAL_TABLE)
			fprintf(file, "Normal ");
		if (deadTables[i] & B_SHIFT_TABLE)
			fprintf(file, "Shift ");
		if (deadTables[i] & B_CONTROL_TABLE)
			fprintf(file, "Control ");
		if (deadTables[i] & B_OPTION_TABLE)
			fprintf(file, "Option ");
		if (deadTables[i] & B_OPTION_SHIFT_TABLE)
			fprintf(file, "Option-Shift ");
		if (deadTables[i] & B_CAPS_TABLE)
			fprintf(file, "CapsLock ");
		if (deadTables[i] & B_CAPS_SHIFT_TABLE)
			fprintf(file, "CapsLock-Shift ");
		if (deadTables[i] & B_OPTION_CAPS_TABLE)
			fprintf(file, "CapsLock-Option ");
		if (deadTables[i] & B_OPTION_CAPS_SHIFT_TABLE)
			fprintf(file, "CapsLock-Option-Shift ");
		fprintf(file, "\n");
	}
}


void
Keymap::_ComputeChars(const char* buffer, struct re_registers& regs, int i,
	int& offset)
{
	char* current = &fChars[offset + 1];
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
		for (uint32 j = 0; j < length; j++)
			sscanf(hexChars + 2*j, "%02hhx", current + j);
	}

	fChars[offset] = length;
	offset += length + 1;
}


void
Keymap::_ComputeTables(const char* buffer, struct re_registers& regs,
	uint32& table)
{
	for (int32 i = 1; i <= 9; i++) {
		int32 length = regs.end[i] - regs.start[i];
		if (length <= 0)
			break;

		const char* start = buffer + regs.start[i];

		if (strncmp(start, "Normal", length) == 0)
			table |= B_NORMAL_TABLE;
		else if (strncmp(start, "Shift", length) == 0)
			table |= B_SHIFT_TABLE;
		else if (strncmp(start, "Control", length) == 0)
			table |= B_CONTROL_TABLE;
		else if (strncmp(start, "Option", length) == 0)
			table |= B_OPTION_TABLE;
		else if (strncmp(start, "Option-Shift", length) == 0)
			table |= B_OPTION_SHIFT_TABLE;
		else if (strncmp(start, "CapsLock", length) == 0)
			table |= B_CAPS_TABLE;
		else if (strncmp(start, "CapsLock-Shift", length) == 0)
			table |= B_CAPS_SHIFT_TABLE;
		else if (strncmp(start, "CapsLock-Option", length) == 0)
			table |= B_OPTION_CAPS_TABLE;
		else if (strncmp(start, "CapsLock-Option-Shift", length) == 0)
			table |= B_OPTION_CAPS_SHIFT_TABLE;
	}
}
