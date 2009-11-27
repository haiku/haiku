/*
 * Copyright (c) 2003 Matthijs Hollemans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include <AppFileInfo.h>
#include <Mime.h>
#include <Resources.h>
#include <Roster.h>
#include <TypeConstants.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rdef.h"
#include "private.h"

// What we add to the front of enum symbols.
#define PREFIX  "R_"


static FILE *sOutputFile;
static FILE *sHeaderFile;

// Level of indentation (how many tabs).
static int32 sTabs;
static bool sBraceOnNextLine = false;


static void write_generic_data(const char *name, type_code type,
	const void *data, size_t length);


static void
indent()
{
	for (int32 t = 0; t < sTabs; ++t) {
		fprintf(sOutputFile, "\t");
	}
}


void
open_brace()
{
	if (sBraceOnNextLine) {
		fprintf(sOutputFile, "\n");
		indent();
		fprintf(sOutputFile, "{\n");
	} else
		fprintf(sOutputFile, " {\n");

	++sTabs;
}


void
close_brace()
{
	--sTabs;

	fprintf(sOutputFile, "\n");
	indent();
	fprintf(sOutputFile, "}");
}


static bool
make_code(uint32 value, char *code)
{
	code[0] = (value >> 24) & 0xFF;
	if (isprint(code[0])) {
		code[1] = (value >> 16) & 0xFF;
		if (isprint(code[1])) {
			code[2] = (value >> 8) & 0xFF;
			if (isprint(code[2])) {
				code[3] = value & 0xFF;
				if (isprint(code[3])) {
					code[4] = '\0';
					return true;
				}
			}
		}
	}

	return false;
}


static void
write_code(uint32 value)
{
	char code[5];
	if (make_code(value, code))
		fprintf(sOutputFile, "'%s'", code);
	else
		fprintf(sOutputFile, "%" B_PRIu32, value);
}


static void
write_field_name(const char *name)
{
	// We call this from the write_xxx() functions to properly align a
	// field's type code (which goes to the left of the field name) and
	// the field's data (to right of the name). If we are not currently
	// writing a field (but the entire resource), name is NULL.

	if (name != NULL)
		fprintf(sOutputFile, "\"%s\" = ", name);
}


static bool
is_ident(const char *name)
{
	if (name[0] != '_' && !isalpha(name[0]))
		return false;

	for (size_t t = 1; t < strlen(name); ++t) {
		if (name[t] != '_' && !isalnum(name[t]))
			return false;
	}

	return true;
}


static bool
has_prefix(const char *name)
{
	size_t name_len = strlen(name);
	size_t prefix_len = strlen(PREFIX);

	if (name_len > prefix_len) {
		if (strncmp(name, PREFIX, prefix_len) == 0)
			return true;
	}

	return false;
}


static bool
is_string(const void *data, size_t length)
{
	// We consider the buffer a string if it contains only human readable
	// characters. The buffer should also end with a '\0'. Although the
	// compiler allows string literals to contain embedded '\0' chars as
	// well, we don't allow them here (because they may cause false hits).

	if (length == 0)
		return false;

	char *ptr = (char *)data;

	for (size_t t = 0; t < length - 1; ++t) {
		if (!isprint(*ptr++))
			return false;
	}

	return (*ptr == '\0');
}


static void
write_rsrc(type_code type, int32 id, const char *name)
{
	if (name[0] == '\0') {
		fprintf(sOutputFile, "resource(%" B_PRId32 ") ", id);
	} else if ((flags & RDEF_AUTO_NAMES) != 0&& is_ident(name)) {
		char code[5];
		if (has_prefix(name)) {
			fprintf(sOutputFile, "resource(%s) ", name);
			fprintf(sHeaderFile, "\t%s = %" B_PRId32 ",\n", name, id);
		} else if (make_code(type, code)) {
			fprintf(sOutputFile, "resource(%s%s_%s) ", PREFIX, code, name);
			fprintf(sHeaderFile, "\t%s%s_%s = %" B_PRId32 ",\n", PREFIX, code,
				name, id);
		} else {
			fprintf(sOutputFile, "resource(%s%" B_PRIu32 "_%s) ", PREFIX,
				(uint32)type, name);
			fprintf(sHeaderFile, "\t%s%" B_PRIu32 "_%s = %" B_PRId32 ",\n",
				PREFIX, (uint32)type, name, id);
		}
	} else {
		fprintf(sOutputFile, "resource(%" B_PRId32 ", \"%s\") ", id, name);
	}
}


//	#pragma mark - generic types


static uint8 *
write_raw_line(uint8 *ptr, uint8 *end, size_t bytesPerLine)
{
	uint32 count = 0;

	fprintf(sOutputFile, "$\"");

	while (ptr < end && count < bytesPerLine) {
		fprintf(sOutputFile, "%02X", *ptr++);
		++count;
	}

	fprintf(sOutputFile, "\"");

	return ptr;
}


static void
write_raw(const char *name, type_code type, const void *data,
	size_t length, size_t bytesPerLine = 32)
{
	uint8 *ptr = (uint8 *)data;
	uint8 *end = ptr + length;

	if (length > bytesPerLine) {
		if (type != B_RAW_TYPE) {
			fprintf(sOutputFile, "#");
			write_code(type);
			fprintf(sOutputFile, " ");
		}

		write_field_name(name);
		fprintf(sOutputFile, "array");

		open_brace();

		int32 item = 0;
		while (ptr < end) {
			if (item++ > 0)
				fprintf(sOutputFile, "\n");

			indent();
			ptr = write_raw_line(ptr, end, bytesPerLine);
		}

		close_brace();
	} else {
		if (type != B_RAW_TYPE) {
			fprintf(sOutputFile, "#");
			write_code(type);
			fprintf(sOutputFile, " ");
		}

		write_field_name(name);
		write_raw_line(ptr, end, bytesPerLine);
	}
}


static void
write_bool(const char *name, const void *data, size_t length)
{
	if (length != sizeof(bool)) {
		write_raw(name, B_BOOL_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "%s", *(bool *)data ? "true" : "false");
	}
}


static void
write_int8(const char *name, const void *data, size_t length)
{
	if (length != sizeof(int8)) {
		write_raw(name, B_INT8_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(int8)%d", *(int8 *)data);
	}
}


static void
write_int16(const char *name, const void *data, size_t length)
{
	if (length != sizeof(int16)) {
		write_raw(name, B_INT16_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(int16)%d", *(int16 *)data);
	}
}


static void
write_int32(const char *name, const void *data, size_t length)
{
	if (length != sizeof(int32)) {
		write_raw(name, B_INT32_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "%" B_PRId32, *(int32 *)data);
	}
}


static void
write_int64(const char *name, const void *data, size_t length)
{
	if (length != sizeof(int64)) {
		write_raw(name, B_INT64_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(int64)%" B_PRId64, *(int64 *)data);
	}
}


static void
write_uint8(const char *name, const void *data, size_t length)
{
	if (length != sizeof(uint8)) {
		write_raw(name, B_UINT8_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(uint8)%u", *(uint8 *)data);
	}
}


static void
write_uint16(const char *name, const void *data, size_t length)
{
	if (length != sizeof(uint16)) {
		write_raw(name, B_UINT16_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(uint16)%u", *(uint16 *)data);
	}
}


static void
write_uint32(const char *name, const void *data, size_t length)
{
	if (length != sizeof(uint32)) {
		write_raw(name, B_UINT32_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(uint32)%" B_PRIu32, *(uint32 *)data);
	}
}


static void
write_uint64(const char *name, const void *data, size_t length)
{
	if (length != sizeof(uint64)) {
		write_raw(name, B_UINT64_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(uint64)%" B_PRIu64, *(uint64 *)data);
	}
}


static void
write_float(const char *name, const void *data, size_t length)
{
	if (length != sizeof(float)) {
		write_raw(name, B_FLOAT_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "%#g", *(float *)data);
	}
}


static void
write_double(const char *name, const void *data, size_t length)
{
	if (length != sizeof(double)) {
		write_raw(name, B_DOUBLE_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(double)%#g", *(double *)data);
	}
}


static void
write_size(const char *name, const void *data, size_t length)
{
	if (length != sizeof(size_t)) {
		write_raw(name, B_SIZE_T_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(size_t)%lu", (unsigned long)*(size_t *)data);
	}
}


static void
write_ssize(const char *name, const void *data, size_t length)
{
	if (length != sizeof(ssize_t)) {
		write_raw(name, B_SSIZE_T_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(ssize_t)%ld", (long)*(ssize_t *)data);
	}
}


static void
write_off(const char *name, const void *data, size_t length)
{
	if (length != sizeof(off_t)) {
		write_raw(name, B_OFF_T_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(off_t)%" B_PRIdOFF, *(off_t *)data);
	}
}


static void
write_time(const char *name, const void *data, size_t length)
{
	if (length != sizeof(time_t)) {
		write_raw(name, B_TIME_TYPE, data, length);
	} else {
		write_field_name(name);
		fprintf(sOutputFile, "(time_t)%ld", (long)*(time_t *)data);
	}
}


static void
write_point(const char *name, const void *data)
{
	///TODO: using built-in type table

	write_field_name(name);
	float *f = (float *)data;
	fprintf(sOutputFile, "point { %#g, %#g }", f[0], f[1]);
}


static void
write_rect(const char *name, const void *data)
{
	///TODO: using built-in type table

	write_field_name(name);
	float *f = (float *)data;
	fprintf(sOutputFile, "rect { %#g, %#g, %#g, %#g }", f[0], f[1], f[2], f[3]);
}


static void
write_rgb(const char *name, const void *data)
{
	///TODO: using built-in type table

	write_field_name(name);
	uint8 *b = (uint8 *)data;

	fprintf(sOutputFile, "rgb_color { 0x%02X, 0x%02X, 0x%02X, 0x%02X }",
		b[0], b[1], b[2], b[3]);
}


static const char *
write_string_line(const char *ptr, const char *end, size_t charsPerLine)
{
	uint32 count = 0;
	bool end_of_item = false;

	fprintf(sOutputFile, "\"");

	while (ptr < end && count < charsPerLine && !end_of_item) {
		char c = *ptr++;

		switch (c) {
			case '\b': fprintf(sOutputFile, "\\b");  count += 2; break;
			case '\f': fprintf(sOutputFile, "\\f");  count += 2; break;
			case '\n': fprintf(sOutputFile, "\\n");  count += 2; break;
			case '\r': fprintf(sOutputFile, "\\r");  count += 2; break;
			case '\t': fprintf(sOutputFile, "\\t");  count += 2; break;
			case '\v': fprintf(sOutputFile, "\\v");  count += 2; break;
			case '\"': fprintf(sOutputFile, "\\\""); count += 2; break;
			case '\\': fprintf(sOutputFile, "\\\\"); count += 2; break;

			case '\0': end_of_item = true; break;

			default:
			{
				if ((uint8)c < 128 && !isprint(c)) {
					fprintf(sOutputFile, "\\0x%02X", (uint8)c); count += 5;
				} else {
					fprintf(sOutputFile, "%c", c); ++count;
				}
			}
		}
	}

	fprintf(sOutputFile, "\"");

	if (end_of_item && ptr < end)
		fprintf(sOutputFile, ",");

	return ptr;
}


static void
write_string(const char *name, type_code type,
	const void *data, size_t length)
{
	const char *ptr = (const char *)data;
	const char *end = ptr + length;
	size_t charsPerLine = 64;

	// We write an "array" resource if the string has more than 64
	// characters. A string resource may also be comprised of multiple
	// substrings, each terminated by a '\0' char. In that case, we
	// must write an "array" resource as well. Sneaky as we are, we use
	// strlen() to check for that, because it also looks for a '\0'.

	if (length > charsPerLine || strlen(ptr) < length - 1) {
		fprintf(sOutputFile, "#");
		write_code(type);
		fprintf(sOutputFile, " array");

		if (name != NULL) {
			fprintf(sOutputFile, " ");
			write_field_name(name);
			fprintf(sOutputFile, " array");
		}

		open_brace();

		int32 item = 0;
		while (ptr < end) {
			if (item++ > 0)
				fprintf(sOutputFile, "\n");

			indent();
			ptr = write_string_line(ptr, end, charsPerLine);
		}

		close_brace();
	} else {
		if (type != B_STRING_TYPE) {
			fprintf(sOutputFile, "#");
			write_code(type);
			fprintf(sOutputFile, " ");
		}

		write_field_name(name);
		write_string_line(ptr, end, charsPerLine);
	}
}


static void
write_fields(BMessage &msg)
{
	int32 t = 0;
	int32 item = 0;

#ifdef B_BEOS_VERSION_DANO
	const char *name;
#else
	char *name;
#endif
	type_code type;
	int32 count;
	const void *data;
	size_t length;

	open_brace();

	while (msg.GetInfo(B_ANY_TYPE, t, &name, &type, &count) == B_OK) {
		for (int32 k = 0; k < count; ++k) {
			if (msg.FindData(name, type, k, &data, (ssize_t*) &length) == B_OK) {
				if (item++ > 0)
					fprintf(sOutputFile, ",\n");

				indent();
				write_generic_data(name, type, data, length);
			}
		}

		++t;
	}

	close_brace();
}


static void
write_message(const char *name, BMessage &msg, type_code type)
{
	if (type != B_MESSAGE_TYPE) {
		fprintf(sOutputFile, "#");
		write_code(type);
		fprintf(sOutputFile, " ");
	}

	write_field_name(name);

	const char *class_;
	if (msg.FindString("class", &class_) == B_OK) {
		fprintf(sOutputFile, "archive");

		const char *add_on;
		if (msg.FindString("add_on", &add_on) == B_OK) {
			fprintf(sOutputFile, "(\"%s\"", add_on);
			if (msg.what != 0) {
				fprintf(sOutputFile, ", ");
				write_code(msg.what);
			}
			fprintf(sOutputFile, ")");

			msg.RemoveName("add_on");
		} else if (msg.what != 0) {
			fprintf(sOutputFile, "(, ");
			write_code(msg.what);
			fprintf(sOutputFile, ")");
		}

		fprintf(sOutputFile, " %s", class_);
		msg.RemoveName("class");
	} else if (msg.what == 0) {
		fprintf(sOutputFile, "message");
	} else {
		fprintf(sOutputFile, "message(");
		write_code(msg.what);
		fprintf(sOutputFile, ")");
	}

	if (msg.CountNames(B_ANY_TYPE) > 0)
		write_fields(msg);
}


static void
write_other(const char *name, type_code type,
	const void *data, size_t length)
{
	BMessage msg;
	if (msg.Unflatten((const char *)data) == B_OK)
		write_message(name, msg, type);
	else if (is_string(data, length))
		write_string(name, type, data, length);
	else
		write_raw(name, type, data, length);
}


//	#pragma mark - special types


static void
write_app_signature(const void *data, size_t length)
{
	fprintf(sOutputFile, "resource app_signature ");
	write_string_line((const char *)data, (const char *)data + length, length * 2);
}


static void
write_app_flags(const void *data, size_t length)
{
	fprintf(sOutputFile, "resource app_flags ");

	uint32 flags = *(uint32 *)data;
	switch (flags & B_LAUNCH_MASK) {
		case B_SINGLE_LAUNCH:
			fputs("B_SINGLE_LAUNCH", sOutputFile);
			break;
		case B_MULTIPLE_LAUNCH:
			fputs("B_MULTIPLE_LAUNCH", sOutputFile);
			break;
		case B_EXCLUSIVE_LAUNCH:
			fputs("B_EXCLUSIVE_LAUNCH", sOutputFile);
			break;
	}

	if (flags & B_BACKGROUND_APP)
		fputs(" | B_BACKGROUND_APP", sOutputFile);
	if (flags & B_ARGV_ONLY)
		fputs(" | B_ARGV_ONLY", sOutputFile);
}


static void
write_app_icon(uint32 which, const void *data, size_t length)
{
	int32 lineWidth = 32;
	const char* type = "";
	switch (which) {
		case B_MINI_ICON:
			type = "mini";
			lineWidth = 16;
			break;
		case B_LARGE_ICON:
			type = "large";
			break;
		case 'VICN':
			type = "vector";
			break;
		case 'PNG ':
			type = "png";
			break;
		default:
			fprintf(stderr, "write_app_icon() called with invalid type!\n");
			break;
	}
	fprintf(sOutputFile, "resource %s_icon ", type);
	write_raw(NULL, B_RAW_TYPE, data, length, lineWidth);
}


static void
write_app_file_types(const void *data, size_t length)
{
	fputs("resource file_types ", sOutputFile);
	write_other(NULL, B_MESSAGE_TYPE, data, length);
}


static void
write_app_version(const void *data, size_t length)
{
	const version_info *version = (const version_info *)data;
	//const version_info *systemVersion = version + 1;

	fputs("resource app_version", sOutputFile);
	open_brace();

	fprintf(sOutputFile, "\tmajor  = %" B_PRIu32 ",\n"
		"\tmiddle = %" B_PRIu32 ",\n"
		"\tminor  = %" B_PRIu32 ",\n\n", version->major, version->middle,
		version->minor);

	const char *variety = "B_APPV_DEVELOPMENT";
	switch (version->variety) {
		case 1:
			variety = "B_APPV_ALPHA";
			break;
		case 2:
			variety = "B_APPV_BETA";
			break;
		case 3:
			variety = "B_APPV_GAMMA";
			break;
		case 4:
			variety = "B_APPV_GOLDEN_MASTER";
			break;
		case 5:
			variety = "B_APPV_FINAL";
			break;
	}
	fprintf(sOutputFile, "\tvariety = %s,\n"
		"\tinternal = %" B_PRIu32 ",\n\n", variety, version->internal);

	fprintf(sOutputFile, "\tshort_info = ");
	write_string(NULL, B_STRING_TYPE, version->short_info, strlen(version->short_info));

	fprintf(sOutputFile, ",\n\tlong_info = ");
	write_string(NULL, B_STRING_TYPE, version->long_info, strlen(version->long_info));

	close_brace();
}


//	#pragma mark - file examination


static void
write_generic_data(const char *name, type_code type,
	const void *data, size_t length)
{
	switch (type) {
		case B_BOOL_TYPE:    write_bool(name, data, length);   break;
		case B_INT8_TYPE:    write_int8(name, data, length);   break;
		case B_INT16_TYPE:   write_int16(name, data, length);  break;
		case B_INT32_TYPE:   write_int32(name, data, length);  break;
		case B_INT64_TYPE:   write_int64(name, data, length);  break;
		case B_UINT8_TYPE:   write_uint8(name, data, length);  break;
		case B_UINT16_TYPE:  write_uint16(name, data, length); break;
		case B_UINT32_TYPE:  write_uint32(name, data, length); break;
		case B_UINT64_TYPE:  write_uint64(name, data, length); break;
		case B_FLOAT_TYPE:   write_float(name, data, length);  break;
		case B_DOUBLE_TYPE:  write_double(name, data, length); break;
		case B_SIZE_T_TYPE:  write_size(name, data, length);   break;
		case B_SSIZE_T_TYPE: write_ssize(name, data, length);  break;
		case B_OFF_T_TYPE:   write_off(name, data, length);    break;
		case B_TIME_TYPE:    write_time(name, data, length);   break;

		case B_POINT_TYPE:     write_point(name, data);  break;
		case B_RECT_TYPE:      write_rect(name, data);   break;
		case B_RGB_COLOR_TYPE: write_rgb(name, data);    break;

		case B_MIME_STRING_TYPE:
		case B_STRING_TYPE:
			write_string(name, type, data, length);
			break;

		case 'MICN':
			write_raw(name, type, data, length, 16);
			break;
		case B_POINTER_TYPE:
		case 'ICON':
		case 'VICN':
			write_raw(name, type, data, length);
			break;

		default:
			write_other(name, type, data, length);
			break;
	}
}


static void
write_data(int32 id, const char *name, type_code type,
	const void *data, size_t length)
{
	// check for special types

	switch (type) {
		case B_MIME_STRING_TYPE:
			if (!strcmp(name, "BEOS:APP_SIG")) {
				write_app_signature(data, length);
				return;
			}
			break;

		case 'VICN':
			if (!strcmp(name, "BEOS:ICON")) {
				write_app_icon('VICN', data, length);
				return;
			}
			break;

		case 'PNG ':
			if (!strcmp(name, "BEOS:ICON")) {
				write_app_icon('PNG ', data, length);
				return;
			}
			break;

		case 'MICN':
			if (!strcmp(name, "BEOS:M:STD_ICON")) {
				write_app_icon(B_MINI_ICON, data, length);
				return;
			}
			break;

		case 'ICON':
			if (!strcmp(name, "BEOS:L:STD_ICON")) {
				write_app_icon(B_LARGE_ICON, data, length);
				return;
			}
			break;

		case B_MESSAGE_TYPE:
			if (!strcmp(name, "BEOS:FILE_TYPES")) {
				write_app_file_types(data, length);
				return;
			}
			break;

		case 'APPF':
			if (!strcmp(name, "BEOS:APP_FLAGS") && length == 4) {
				write_app_flags(data, length);
				return;
			}
			break;

		case 'APPV':
			if (!strcmp(name, "BEOS:APP_VERSION") && length == sizeof(version_info) * 2) {
				write_app_version(data, length);
				return;
			}
			break;
	}

	// write generic types

	write_rsrc(type, id, name);
	write_generic_data(NULL, type, data, length);
}


static void
examine_file(char *fileName)
{
	BFile file(fileName, B_READ_ONLY);
	if (file.InitCheck() != B_OK) {
		strcpy(rdef_err_file, fileName);
		rdef_err = RDEF_FILE_NOT_FOUND;
		return;
	}

	BResources res;
	if (res.SetTo(&file) != B_OK) {
		strcpy(rdef_err_file, fileName);
		rdef_err = RDEF_NO_RESOURCES;
		return;
	}

	int32 t = 0;
	type_code type;
	int32 id;
	const char *name;
	size_t length;
	const void *data;

	while (res.GetResourceInfo(t, &type, &id, &name, &length)) {
		sTabs = 0;

		data = res.LoadResource(type, id, NULL);
		if (data != NULL) {
			fprintf(sOutputFile, "\n");
			write_data(id, name, type, data, length);
			fprintf(sOutputFile, ";\n");
		}

		++t;
	}
}


static status_t
open_output_files(const char *fileName, const char *headerName)
{
	sOutputFile = fopen(fileName, "w");
	if (sOutputFile == NULL) {
		strcpy(rdef_err_msg, strerror(errno));
		strcpy(rdef_err_file, fileName);
		return RDEF_WRITE_ERR;
	}

	if (flags & RDEF_AUTO_NAMES) {
		sHeaderFile = fopen(headerName, "w");
		if (sHeaderFile == NULL) {
			strcpy(rdef_err_msg, strerror(errno));
			strcpy(rdef_err_file, headerName);
			fclose(sOutputFile);
			return RDEF_WRITE_ERR;
		}

		fprintf(sOutputFile, "\n#include \"%s\"\n", headerName);

		if (sBraceOnNextLine)
			fprintf(sHeaderFile, "\nenum\n{\n");
		else
			fprintf(sHeaderFile, "\nenum {\n");
	}

	return B_OK;
}


static void
close_output_files(const char *fileName, const char *headerName)
{
	if (flags & RDEF_AUTO_NAMES) {
		fprintf(sHeaderFile, "};\n");
		fclose(sHeaderFile);

		if (rdef_err != B_OK)
			unlink(headerName);
	}

	fclose(sOutputFile);

	if (rdef_err != B_OK)
		unlink(fileName);
}


status_t
rdef_decompile(const char *fileName)
{
	clear_error();

	if (fileName == NULL || fileName[0] == '\0') {
		rdef_err = B_BAD_VALUE;
		return rdef_err;
	}

	char headerName[B_PATH_NAME_LENGTH + 1];
	if ((flags & RDEF_AUTO_NAMES) != 0)
		sprintf(headerName, "%s.h", fileName);

	rdef_err = open_output_files(fileName, headerName);
	if (rdef_err != B_OK)
		return rdef_err;

	for (ptr_iter_t i = input_files.begin();
			(i != input_files.end()) && (rdef_err == B_OK); ++i) {
		examine_file((char *)*i);
	}

	close_output_files(fileName, headerName);
	return rdef_err;
}

