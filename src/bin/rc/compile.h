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

#ifndef COMPILE_H
#define COMPILE_H

#include <Resources.h>
#include <TypeConstants.h>

// Handle of file we're currently parsing.
extern FILE* yyin;

// Name of the file we're currently parsing.
extern char lexfile[];

// The line we're currently parsing.
extern int yylineno;

struct field_t;

// Describes a data type.
struct type_t {
	type_code		code;			// type code
	const char*		name;			// name of this type
	int32			count;			// how many fields
	field_t*		fields;			// field definitions
	int32			def_id;			// default resource ID
	const char*		def_name;		// default resource name
};

// Used by the lexer and parser to pass around resource data. The rdef
// format allows string literals to contain embedded '\0' chars, so we
// can't use strlen() to find their length; instead we look at the size
// field for that (size includes the final '\0' too).
struct data_t {
	type_t			type;			// data type
	char*			name;			// name (only if this is a field)
	size_t			size;			// byte size of data
	void*			ptr;			// the actual data
};

// Describes a data field in a user-defined type.
struct field_t {
	type_t			type;			// data type
	const char*		name;			// name of this field
	size_t			resize;			// if not 0, data will be resized
	data_t			data;			// default value
};

// Describes an array of data_t or field_t objects.
struct list_t {
	int32			count;
	void*			items;			// cast to data_t* or field_t*
};

// Used by the parser to pass around resource IDs.
struct res_id_t {
	bool			has_id;
	bool			has_name;
	int32			id;
	char*			name;
};

// Describes a symbolic constant.
struct define_t {
	const char*		name;
	int32			value;
};

// The output file we add resources to.
extern BResources rsrc;
extern const char* rsrc_file;

int yylex();
int yyparse();

void init_lexer();
void clean_up_lexer();

void init_parser();
void clean_up_parser();

void* alloc_mem(size_t size);
void free_mem(void* ptr);

// Returns the data type with the specified name.
type_t get_type(const char* name);

void abort_compile(status_t err, const char* format, ...);
void abort_compile();

#endif // COMPILE_H
