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

#include <Mime.h>
#include <Resources.h>
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

static const char* rdef_name;
static char header_name[B_PATH_NAME_LENGTH + 1];

static FILE* rdef_file;
static FILE* header_file;

// Level of indentation (how many tabs).
static int32 tabs;

static void examine_data(const char*, type_code, const void*, size_t);

//------------------------------------------------------------------------------

static void indent()
{
	for (int32 t = 0; t < tabs; ++t)
	{
		fprintf(rdef_file, "\t");
	}
}

//------------------------------------------------------------------------------

void open_brace()
{
#ifdef PUSSY_CODER_STYLE
	fprintf(rdef_file, " {\n");
#else
	fprintf(rdef_file, "\n");
	indent();
	fprintf(rdef_file, "{\n");
#endif

	++tabs;
}

//------------------------------------------------------------------------------

void close_brace()
{
	--tabs;

	fprintf(rdef_file, "\n");
	indent();
	fprintf(rdef_file, "}");
}

//------------------------------------------------------------------------------

static bool make_code(uint32 value, char* code)
{
	code[0] = (value >> 24) & 0xFF;
	if (isprint(code[0]))
	{
		code[1] = (value >> 16) & 0xFF;
		if (isprint(code[1]))
		{
			code[2] = (value >> 8) & 0xFF;
			if (isprint(code[2]))
			{
				code[3] = value & 0xFF;
				if (isprint(code[3]))
				{
					code[4] = '\0';
					return true;
				}
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------------

static void write_code(uint32 value)
{
	char code[5];
	if (make_code(value, code))
	{
		fprintf(rdef_file, "'%s'", code);
	}
	else
	{
		fprintf(rdef_file, "%lu", value);
	}
}

//------------------------------------------------------------------------------

static void write_field_name(const char* name)
{
	// We call this from the write_xxx() functions to properly align a
	// field's type code (which goes to the left of the field name) and
	// the field's data (to right of the name). If we are not currently
	// writing a field (but the entire resource), name is NULL.

	if (name != NULL)
	{
		fprintf(rdef_file, "\"%s\" = ", name);
	}
}

//------------------------------------------------------------------------------

static bool is_ident(const char* name)
{
	if ((name[0] != '_') && !isalpha(name[0])) { return false; }

	for (size_t t = 1; t < strlen(name); ++t)
	{
		if ((name[t] != '_') && !isalnum(name[t])) { return false; } 
	}

	return true;
}

//------------------------------------------------------------------------------

static bool has_prefix(const char* name)
{
	size_t name_len = strlen(name);
	size_t prefix_len = strlen(PREFIX);

	if (name_len > prefix_len)
	{
		if (strncmp(name, PREFIX, prefix_len) == 0)
		{
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------

static void write_rsrc(type_code type, int32 id, const char* name)
{
	if (name[0] == '\0')
	{
		fprintf(rdef_file, "resource(%ld) ", id);
	}
	else if ((flags & RDEF_AUTO_NAMES) && is_ident(name))
	{
		char code[5];
		if (has_prefix(name))
		{
			fprintf(rdef_file, "resource(%s) ", name);
			fprintf(header_file, "\t%s = %ld,\n", name, id);
		}
		else if (make_code(type, code))
		{
			fprintf(rdef_file, "resource(%s%s_%s) ", PREFIX, code, name);
			fprintf(header_file, "\t%s%s_%s = %ld,\n", PREFIX, code, name, id);
		}
		else
		{
			fprintf(rdef_file, "resource(%s%ld_%s) ", PREFIX, type, name);
			fprintf(header_file, "\t%s%ld_%s = %ld,\n", PREFIX, type, name, id);
		}
	}
	else
	{
		fprintf(rdef_file, "resource(%ld, \"%s\") ", id, name);
	}
}

//------------------------------------------------------------------------------

static uint8* write_raw_line(uint8* ptr, uint8* end, size_t bytes_per_line)
{
	uint32 count = 0;

	fprintf(rdef_file, "$\"");

	while ((ptr < end) && (count < bytes_per_line))
	{
		fprintf(rdef_file, "%02X", *ptr++);
		++count;
	}

	fprintf(rdef_file, "\"");

	return ptr;
}

//------------------------------------------------------------------------------

static void write_raw(
	const char* name, type_code type, const void* data, size_t length, 
	size_t bytes_per_line = 32)
{
	uint8* ptr = (uint8*) data;
	uint8* end = ptr + length;

	if (length > bytes_per_line)
	{
		if (type != B_RAW_TYPE)
		{
			fprintf(rdef_file, "#");
			write_code(type);
			fprintf(rdef_file, " ");
		}

		write_field_name(name);
		fprintf(rdef_file, "array");

		open_brace();

		int32 item = 0;
		while (ptr < end)
		{
			if (item++ > 0) { fprintf(rdef_file, "\n"); }
			indent();
			ptr = write_raw_line(ptr, end, bytes_per_line);
		}

		close_brace();
	}
	else
	{
		if (type != B_RAW_TYPE)
		{
			fprintf(rdef_file, "#");
			write_code(type);
			fprintf(rdef_file, " ");
		}

		write_field_name(name);
		write_raw_line(ptr, end, bytes_per_line);
	}
}

//------------------------------------------------------------------------------

static void write_bool(const char* name, const void* data, size_t length)
{
	if (length != sizeof(bool))
	{
		write_raw(name, B_BOOL_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "%s", *((bool*) data) ? "true" : "false");
	}
}

//------------------------------------------------------------------------------

static void write_int8(const char* name, const void* data, size_t length)
{
	if (length != sizeof(int8))
	{
		write_raw(name, B_INT8_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(int8)%d", *((int8*) data));
	}
}

//------------------------------------------------------------------------------

static void write_int16(const char* name, const void* data, size_t length)
{
	if (length != sizeof(int16))
	{
		write_raw(name, B_INT16_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(int16)%d", *((int16*) data));
	}
}

//------------------------------------------------------------------------------

static void write_int32(const char* name, const void* data, size_t length)
{
	if (length != sizeof(int32))
	{
		write_raw(name, B_INT32_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "%ld", *((int32*) data));
	}
}

//------------------------------------------------------------------------------

static void write_int64(const char* name, const void* data, size_t length)
{
	if (length != sizeof(int64))
	{
		write_raw(name, B_INT64_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(int64)%lld", *((int64*) data));
	}
}

//------------------------------------------------------------------------------

static void write_uint8(const char* name, const void* data, size_t length)
{
	if (length != sizeof(uint8))
	{
		write_raw(name, B_UINT8_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(uint8)%u", *((uint8*) data));
	}
}

//------------------------------------------------------------------------------

static void write_uint16(const char* name, const void* data, size_t length)
{
	if (length != sizeof(uint16))
	{
		write_raw(name, B_UINT16_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(uint16)%u", *((uint16*) data));
	}
}

//------------------------------------------------------------------------------

static void write_uint32(const char* name, const void* data, size_t length)
{
	if (length != sizeof(uint32))
	{
		write_raw(name, B_UINT32_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(uint32)%lu", *((uint32*) data));
	}
}

//------------------------------------------------------------------------------

static void write_uint64(const char* name, const void* data, size_t length)
{
	if (length != sizeof(uint64))
	{
		write_raw(name, B_UINT64_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(uint64)%llu", *((uint64*) data));
	}
}

//------------------------------------------------------------------------------

static void write_float(const char* name, const void* data, size_t length)
{
	if (length != sizeof(float))
	{
		write_raw(name, B_FLOAT_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "%#g", *((float*) data));
	}
}

//------------------------------------------------------------------------------

static void write_double(const char* name, const void* data, size_t length)
{
	if (length != sizeof(double))
	{
		write_raw(name, B_DOUBLE_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(double)%#g", *((double*) data));
	}
}

//------------------------------------------------------------------------------

static void write_size(const char* name, const void* data, size_t length)
{
	if (length != sizeof(size_t))
	{
		write_raw(name, B_SIZE_T_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(size_t)%lu", *((size_t*) data));
	}
}

//------------------------------------------------------------------------------

static void write_ssize(const char* name, const void* data, size_t length)
{
	if (length != sizeof(ssize_t))
	{
		write_raw(name, B_SSIZE_T_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(ssize_t)%ld", *((ssize_t*) data));
	}
}

//------------------------------------------------------------------------------

static void write_off(const char* name, const void* data, size_t length)
{
	if (length != sizeof(off_t))
	{
		write_raw(name, B_OFF_T_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(off_t)%lld", *((off_t*) data));
	}
}

//------------------------------------------------------------------------------

static void write_time(const char* name, const void* data, size_t length)
{
	if (length != sizeof(time_t))
	{
		write_raw(name, B_TIME_TYPE, data, length);
	}
	else
	{
		write_field_name(name);
		fprintf(rdef_file, "(time_t)%ld", *((time_t*) data));
	}
}

//------------------------------------------------------------------------------

static void write_point(const char* name, const void* data)
{
	///TODO: using built-in type table

	write_field_name(name);
	float* f = (float*) data;
	fprintf(rdef_file, "point { %#g, %#g }", f[0], f[1]);
}

//------------------------------------------------------------------------------

static void write_rect(const char* name, const void* data)
{
	///TODO: using built-in type table

	write_field_name(name);
	float* f = (float*) data;
	fprintf(rdef_file, "rect { %#g, %#g, %#g, %#g }", f[0], f[1], f[2], f[3]);
}

//------------------------------------------------------------------------------

static void write_rgb(const char* name, const void* data)
{
	///TODO: using built-in type table

	write_field_name(name);
	uint8* b = (uint8*) data;

	fprintf(
		rdef_file, "rgb_color { 0x%02X, 0x%02X, 0x%02X, 0x%02X }", 
		b[0], b[1], b[2], b[3]);
}

//------------------------------------------------------------------------------

static char* write_string_line(char* ptr, char* end, size_t chars_per_line)
{
	uint32 count = 0;
	bool end_of_item = false;

	fprintf(rdef_file, "\"");

	while ((ptr < end) && (count < chars_per_line) && !end_of_item)
	{
		char c = *ptr++;

		switch (c)
		{
			case '\b': fprintf(rdef_file, "\\b");  count += 2; break;
			case '\f': fprintf(rdef_file, "\\f");  count += 2; break;
			case '\n': fprintf(rdef_file, "\\n");  count += 2; break;
			case '\r': fprintf(rdef_file, "\\r");  count += 2; break;
			case '\t': fprintf(rdef_file, "\\t");  count += 2; break;
			case '\v': fprintf(rdef_file, "\\v");  count += 2; break;
			case '\"': fprintf(rdef_file, "\\\""); count += 2; break; 
			case '\\': fprintf(rdef_file, "\\\\"); count += 2; break; 

			case '\0': end_of_item = true; break;

			default:
			{
				if ((((uint8) c) < 128) && !isprint(c))
				{
					fprintf(rdef_file, "\\0x%02X", (uint8) c); count += 5;
				}
				else
				{
					fprintf(rdef_file, "%c", c); ++count;
				}
			}
		}
	}

	fprintf(rdef_file, "\"");

	if (end_of_item && (ptr < end))
	{
		fprintf(rdef_file, ",");
	}

	return ptr;
}

//------------------------------------------------------------------------------

static void write_string(
	const char* name, type_code type, const void* data, size_t length)
{
	char* ptr = (char*) data;
	char* end = ptr + length;
	size_t chars_per_line = 64;

	// We write an "array" resource if the string has more than 64 
	// characters. A string resource may also be comprised of multiple 
	// substrings, each terminated by a '\0' char. In that case, we 
	// must write an "array" resource as well. Sneaky as we are, we use 
	// strlen() to check for that, because it also looks for a '\0'.

	if ((length > chars_per_line) || (strlen(ptr) < length - 1))
	{
		fprintf(rdef_file, "#");
		write_code(type);
		fprintf(rdef_file, " array");

		if (name != NULL)
		{
			fprintf(rdef_file, " ");
			write_field_name(name);
			fprintf(rdef_file, " array");
		}

		open_brace();

		int32 item = 0;
		while (ptr < end)
		{
			if (item++ > 0) { fprintf(rdef_file, "\n"); }
			indent();
			ptr = write_string_line(ptr, end, chars_per_line);
		}

		close_brace();
	}
	else
	{
		if (type != B_STRING_TYPE)
		{
			fprintf(rdef_file, "#");
			write_code(type);
			fprintf(rdef_file, " ");
		}

		write_field_name(name);
		write_string_line(ptr, end, chars_per_line);
	}
}

//------------------------------------------------------------------------------

static void write_fields(BMessage& msg)
{
	int32 t = 0;
	int32 item = 0;

	char* name;
	type_code type;
	int32 count;
	const void* data;
	size_t length;

	open_brace();

	while (msg.GetInfo(B_ANY_TYPE, t, &name, &type, &count) == B_OK)
	{
		for (int32 k = 0; k < count; ++k)
		{
			if (msg.FindData(name, type, k, &data, (ssize_t*) &length) == B_OK)
			{
				if (item++ > 0) { fprintf(rdef_file, ",\n"); }
				indent();
				examine_data(name, type, data, length);
			}
		}

		++t;
	}

	close_brace();
}

//------------------------------------------------------------------------------

static void write_message(const char* name, BMessage& msg, type_code type)
{	
	if (type != B_MESSAGE_TYPE)
	{
		fprintf(rdef_file, "#");
		write_code(type);
		fprintf(rdef_file, " ");
	}

	write_field_name(name);

	const char* class_;
	if (msg.FindString("class", &class_) == B_OK)
	{
		fprintf(rdef_file, "archive");

		const char* add_on;
		if (msg.FindString("add_on", &add_on) == B_OK)
		{
			fprintf(rdef_file, "(\"%s\"", add_on);
			if (msg.what != 0)
			{
				fprintf(rdef_file, ", ");
				write_code(msg.what);
			}
			fprintf(rdef_file, ")");

			msg.RemoveName("add_on");
		}
		else if (msg.what != 0)
		{
			fprintf(rdef_file, "(, ");
			write_code(msg.what);
			fprintf(rdef_file, ")");
		}

		fprintf(rdef_file, " %s", class_);
		msg.RemoveName("class");
	}
	else if (msg.what == 0)
	{
		fprintf(rdef_file, "message");
	}
	else
	{
		fprintf(rdef_file, "message(");
		write_code(msg.what);
		fprintf(rdef_file, ")");
	}

	if (msg.CountNames(B_ANY_TYPE) > 0)
	{
		write_fields(msg);
	}
}

//------------------------------------------------------------------------------

static bool is_string(const void* data, size_t length)
{
	// We consider the buffer a string if it contains only human readable
	// characters. The buffer should also end with a '\0'. Although the
	// compiler allows string literals to contain embedded '\0' chars as
	// well, we don't allow them here (because they may cause false hits).

	if (length == 0) { return false; }

	char* ptr = (char*) data;

	for (size_t t = 0; t < length - 1; ++t)
	{
		if (!isprint(*ptr++))
		{
			return false;
		}
	}

	return (*ptr == '\0');
}

//------------------------------------------------------------------------------

static void write_other(
	const char* name, type_code type, const void* data, size_t length)
{
	BMessage msg;
	if (msg.Unflatten((const char*) data) == B_OK)
	{
		write_message(name, msg, type);
	}
	else if (is_string(data, length))
	{
		write_string(name, type, data, length);
	}
	else
	{
		write_raw(name, type, data, length);
	}
}

//------------------------------------------------------------------------------

static void examine_data(
	const char* name, type_code type, const void* data, size_t length)
{
	switch (type)
	{
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

		case B_MIME_STRING_TYPE: write_string(name, type, data, length); break;
		case B_STRING_TYPE:      write_string(name, type, data, length); break;

		case B_POINTER_TYPE: write_raw(name, type, data, length);     break;
		case 'MICN':         write_raw(name, type, data, length, 16); break;
		case 'ICON':         write_raw(name, type, data, length);     break;

		default: write_other(name, type, data, length); break;
	}
}

//------------------------------------------------------------------------------

static void examine_file(char* file_name)
{
	BFile file(file_name, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
	{
		strcpy(rdef_err_file, file_name);
		rdef_err = RDEF_FILE_NOT_FOUND;
		return;
	}

	BResources res;
	if (res.SetTo(&file) != B_OK)
	{
		strcpy(rdef_err_file, file_name);
		rdef_err = RDEF_NO_RESOURCES;
		return;
	}

	int32 t = 0;
	type_code type;
	int32 id;
	const char* name;
	size_t length;
	const void* data;

	while (res.GetResourceInfo(t, &type, &id, &name, &length))
	{
		tabs = 0;

		data = res.LoadResource(type, id, NULL);
		if (data != NULL)
		{
			fprintf(rdef_file, "\n");
			write_rsrc(type, id, name);
			examine_data(NULL, type, data, length);
			fprintf(rdef_file, ";\n");
		}

		++t;
	}
}

//------------------------------------------------------------------------------

static status_t open_output_files()
{
	rdef_file = fopen(rdef_name, "w");
	if (rdef_file == NULL)
	{
		strcpy(rdef_err_msg, strerror(errno));
		strcpy(rdef_err_file, rdef_name);
		return RDEF_WRITE_ERR;
	}

	if (flags & RDEF_AUTO_NAMES)
	{
		header_file = fopen(header_name, "w");
		if (header_file == NULL)
		{
			strcpy(rdef_err_msg, strerror(errno));
			strcpy(rdef_err_file, header_name);
			fclose(rdef_file);
			return RDEF_WRITE_ERR;
		}

		fprintf(rdef_file, "\n#include \"%s\"\n", header_name);

#ifdef PUSSY_CODER_STYLE
		fprintf(header_file, "\nenum {\n");
#else
		fprintf(header_file, "\nenum\n{\n");
#endif
	}

	return B_OK;
}

//------------------------------------------------------------------------------

static void close_output_files()
{
	if (flags & RDEF_AUTO_NAMES)
	{
		fprintf(header_file, "};\n");
		fclose(header_file);

		if (rdef_err != B_OK)
		{
			unlink(header_name);
		}
	}

	fclose(rdef_file);

	if (rdef_err != B_OK)
	{
		unlink(rdef_name);
	}
}

//------------------------------------------------------------------------------

status_t rdef_decompile(const char* output_file)
{
	clear_error();

	if ((output_file == NULL) || (output_file[0] == '\0'))
	{
		rdef_err = B_BAD_VALUE;
		return rdef_err;
	}

	rdef_name = output_file;
	if (flags & RDEF_AUTO_NAMES)
	{
		sprintf(header_name, "%s.h", output_file);
	}

	rdef_err = open_output_files();
	if (rdef_err != B_OK)
	{
		return rdef_err;
	}

	for (ptr_iter_t i = input_files.begin();
			(i != input_files.end()) && (rdef_err == B_OK); ++i)
	{
		examine_file((char*) *i);
	}

	close_output_files();
	return rdef_err;
}

//------------------------------------------------------------------------------
