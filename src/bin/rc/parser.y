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

%{

#include <Message.h>

#include <map>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "rdef.h"
#include "compile.h"
#include "private.h"

using namespace std;

#define YYERROR_VERBOSE

static void yyerror(const char*);

struct ident_compare_t {  // allows the maps to compare identifier names
	bool
	operator()(const char* s1, const char* s2) const
	{
		return strcmp(s1, s2) < 0;
	}
};

typedef std::map<const char*, int32, ident_compare_t> sym_tab_t;
typedef sym_tab_t::iterator sym_iter_t;

typedef std::map<const char*, type_t, ident_compare_t> type_tab_t;
typedef type_tab_t::iterator type_iter_t;

typedef std::map<const char*, define_t, ident_compare_t> define_tab_t;
typedef define_tab_t::iterator define_iter_t;


static sym_tab_t symbol_table;  // symbol table for enums
static int32 enum_cnt;          // counter for enum symbols without id
static type_tab_t type_table;  // symbol table for data types
static define_tab_t define_table;  // symbol table for defines


static void add_user_type(res_id_t, type_code, const char*, list_t);
static void add_symbol(const char*, int32);
static int32 get_symbol(const char*);

static bool is_type(const char* name);
static define_t get_define(const char* name);

static data_t make_data(size_t, type_t);
static data_t make_bool(bool);
static data_t make_int(uint64);
static data_t make_float(double);

static data_t import_data(char*);
static data_t resize_data(data_t, size_t);

static BMessage* make_msg(list_t);
static data_t flatten_msg(BMessage*);

static data_t make_default(type_t);
static data_t make_type(char* name, list_t);

static list_t make_field_list(field_t);
static list_t concat_field_list(list_t, field_t);
static list_t make_data_list(data_t);
static list_t concat_data_list(list_t, data_t);
static data_t concat_data(data_t, data_t);

static data_t cast(type_t, data_t);

static data_t unary_expr(data_t, char);
static data_t binary_expr(data_t, data_t, char);

static void add_resource(res_id_t, type_code, data_t);


//------------------------------------------------------------------------------
%}

%expect 15

%union {
	bool b;
	uint64 i;
	double f;
	char* I;
	type_code t;
	res_id_t id;
	data_t d;
	list_t l;
	field_t F;
	type_t T;
}

%token ENUM RESOURCE ARCHIVE ARRAY MESSAGE RTYPE IMPORT

%token <b> BOOL
%token <i> INTEGER
%token <f> FLOAT
%token <d> STRING RAW
%token <I> IDENT
%token <t> TYPECODE

%type <i> integer
%type <f> float
%type <id> id
%type <d> archive array arrayfields data expr message msgfield
%type <d> type typefield type_or_define
%type <l> msgfields typefields typedeffields
%type <F> typedeffield
%type <T> datatype typecast

%left '|'
%left '^'
%left '&'
%left '+' '-'
%left '*' '/' '%'
%right FLIP

%%

script
	: /* empty */
	| script enum
	| script typedef
	| script resource
	;

enum
	: enumstart '{' '}' ';'
	| enumstart '{' symbols '}' ';'
	| enumstart '{' symbols ',' '}' ';'
	;

enumstart
	: ENUM { enum_cnt = 0; }
	;

symbols
	: symbols ',' symboldef
	| symboldef
	;

symboldef
	: IDENT
		{
			add_symbol($1, enum_cnt);
			++enum_cnt;
		}
	| IDENT '=' integer
		{
			int32 id = (int32) $3;
			add_symbol($1, id);
			enum_cnt = id + 1;
		}
	;

typedef
	: RTYPE id TYPECODE IDENT '{' typedeffields '}' ';'
		{
			add_user_type($2, $3, $4, $6);
		}
	| RTYPE id IDENT '{' typedeffields '}' ';'
		{
			add_user_type($2, B_RAW_TYPE, $3, $5);
		}
	;

typedeffields
	: typedeffields ',' typedeffield { $$ = concat_field_list($1, $3); }
	| typedeffield                   { $$ = make_field_list($1); }
	;

typedeffield
	: datatype IDENT
		{
			$$.type   = $1;
			$$.name   = $2;
			$$.resize = 0;
			$$.data   = make_default($1);
		}
	| datatype IDENT '=' expr
		{
			$$.type   = $1;
			$$.name   = $2;
			$$.resize = 0;
			$$.data   = cast($1, $4);
		}
	| datatype IDENT '[' INTEGER ']'
		{
			$$.type   = $1;
			$$.name   = $2;
			$$.resize = (size_t) $4;
			$$.data   = resize_data(make_default($1), $$.resize);
		}
	| datatype IDENT '[' INTEGER ']' '=' expr
		{
			$$.type   = $1;
			$$.name   = $2;
			$$.resize = (size_t) $4;
			$$.data   = resize_data(cast($1, $7), $$.resize);
		}
	;

resource
	: RESOURCE id expr ';'
		{
			add_resource($2, $3.type.code, $3);
		}
	| RESOURCE id TYPECODE expr ';'
		{
			add_resource($2, $3, $4);
		}
	| RESOURCE id '(' TYPECODE ')' expr ';'
		{
			add_resource($2, $4, $6);
		}
	;

id
	: /* empty */
		{
			$$.has_id = false; $$.has_name = false; $$.name = NULL;
		}
	| '(' ')'
		{
			$$.has_id = false; $$.has_name = false; $$.name = NULL;
		}
	| '(' integer ')'
		{
			$$.has_id = true; $$.id = (int32) $2;
			$$.has_name = false; $$.name = NULL;
		}
	| '(' integer ',' STRING ')'
		{
			$$.has_id = true; $$.id = (int32) $2;
			$$.has_name = true; $$.name = (char*) $4.ptr;
		}
	| '(' IDENT ')'
		{
			$$.has_id = true; $$.id = get_symbol($2);

			if (flags & RDEF_AUTO_NAMES)
			{
				$$.has_name = true; $$.name = $2;
			}
			else
			{
				$$.has_name = false; $$.name = NULL;
				free_mem($2);
			}
		}
	| '(' IDENT ',' STRING ')'
		{
			$$.has_id = true; $$.id = get_symbol($2);
			$$.has_name = true; $$.name = (char*) $4.ptr;
			free_mem($2);
		}
	| '(' STRING ')'
		{
			$$.has_id = false;
			$$.has_name = true; $$.name = (char*) $2.ptr;
		}
	;

array
	: ARRAY '{' arrayfields '}' { $$ = $3; }
	| ARRAY '{' '}'             { $$ = make_data(0, get_type("raw")); }
	| ARRAY                     { $$ = make_data(0, get_type("raw")); }
	| ARRAY IMPORT STRING       { $$ = import_data((char*) $3.ptr); }
	| IMPORT STRING             { $$ = import_data((char*) $2.ptr); }
	;

arrayfields
	: arrayfields ',' expr { $$ = concat_data($1, $3); }
	| expr                 { $$ = $1; $$.type = get_type("raw"); }
	;

message
	: MESSAGE '(' integer ')' '{' msgfields '}'
		{
			BMessage* msg = make_msg($6);
			msg->what = (int32) $3;
			$$ = flatten_msg(msg);
		}
	| MESSAGE '(' integer ')' '{' '}'
		{
			BMessage* msg = new BMessage;
			msg->what = (int32) $3;
			$$ = flatten_msg(msg);
		}
	| MESSAGE '(' integer ')'
		{
			BMessage* msg = new BMessage;
			msg->what = (int32) $3;
			$$ = flatten_msg(msg);
		}
	| MESSAGE '{' msgfields '}' { $$ = flatten_msg(make_msg($3)); }
	| MESSAGE '{' '}'           { $$ = flatten_msg(new BMessage); }
	| MESSAGE                   { $$ = flatten_msg(new BMessage); }
	;

msgfields
	: msgfields ',' msgfield { $$ = concat_data_list($1, $3); }
	| msgfield               { $$ = make_data_list($1); }
	;

msgfield
	: STRING '=' expr
		{
			$$ = $3;
			$$.name = (char*) $1.ptr;
		}
	| datatype STRING '=' expr
		{
			$$ = cast($1, $4);
			$$.name = (char*) $2.ptr;
		}
	| TYPECODE STRING '=' expr
		{
			$$ = $4;
			$$.type.code = $1;
			$$.name = (char*) $2.ptr;
		}
	| TYPECODE datatype STRING '=' expr
		{
			$$ = cast($2, $5);
			$$.type.code = $1;
			$$.name = (char*) $3.ptr;
		}
	;

archive
	: ARCHIVE IDENT '{' msgfields '}'
		{
			BMessage* msg = make_msg($4);
			msg->AddString("class", $2);
			free_mem($2);
			$$ = flatten_msg(msg);
		}
	| ARCHIVE '(' STRING ')' IDENT '{' msgfields '}'
		{
			BMessage* msg = make_msg($7);
			msg->AddString("class", $5);
			msg->AddString("add_on", (char*) $3.ptr);
			free_mem($5);
			free_mem($3.ptr);
			$$ = flatten_msg(msg);
		}
	| ARCHIVE '(' ',' integer ')' IDENT '{' msgfields '}'
		{
			BMessage* msg = make_msg($8);
			msg->what = (int32) $4;
			msg->AddString("class", $6);
			free_mem($6);
			$$ = flatten_msg(msg);
		}
	| ARCHIVE '(' STRING ',' integer ')' IDENT '{' msgfields '}'
		{
			BMessage* msg = make_msg($9);
			msg->what = (int32) $5;
			msg->AddString("class", $7);
			msg->AddString("add_on", (char*) $3.ptr);
			free_mem($7);
			free_mem($3.ptr);
			$$ = flatten_msg(msg);
		}
	;

type
	: IDENT '{' typefields '}' { $$ = make_type($1, $3); }
	| IDENT '{' '}'
		{
			list_t list; list.count = 0; list.items = NULL;
			$$ = make_type($1, list);
		}
	| IDENT expr
		{
			$$ = make_type($1, make_data_list($2));
		}
	| type_or_define { $$ = $1; }
	;

type_or_define
	: IDENT
		{
			if (is_type($1))
			{
				list_t list; list.count = 0; list.items = NULL;
				$$ = make_type($1, list);
			}
			else
			{
				define_t define = get_define($1);
				$$ = cast(get_type("int32"), make_int(define.value));
				free_mem($1);
			}
		}
	;

typefields
	: typefields ',' typefield { $$ = concat_data_list($1, $3); }
	| typefield                { $$ = make_data_list($1); }
	;

typefield
	: IDENT '=' expr { $$ = $3; $$.name = $1; }
	| expr           { $$ = $1; }
	;

expr
	: expr '+' expr         { $$ = binary_expr($1, $3, '+'); }
	| expr '-' expr         { $$ = binary_expr($1, $3, '-'); }
	| expr '*' expr         { $$ = binary_expr($1, $3, '*'); }
	| expr '/' expr         { $$ = binary_expr($1, $3, '/'); }
	| expr '%' expr         { $$ = binary_expr($1, $3, '%'); }
	| expr '|' expr         { $$ = binary_expr($1, $3, '|'); }
	| expr '^' expr         { $$ = binary_expr($1, $3, '^'); }
	| expr '&' expr         { $$ = binary_expr($1, $3, '&'); }
	| '~' expr %prec FLIP   { $$ = unary_expr($2, '~'); }
	| data                  { $$ = $1; }
	;

data
	: BOOL                  { $$ = cast(get_type("bool"), make_bool($1)); }
	| integer               { $$ = cast(get_type("int32"), make_int($1)); }
	| float                 { $$ = cast(get_type("float"), make_float($1)); }
	| STRING                { $$ = cast($1.type, $1); }
	| RAW                   { $$ = cast($1.type, $1); }
	| array                 { $$ = cast($1.type, $1); }
	| message               { $$ = cast($1.type, $1); }
	| archive               { $$ = cast($1.type, $1); }
	| type                  { $$ = cast($1.type, $1); }
	| '(' expr ')'          { $$ = $2; }
	| typecast BOOL         { $$ = cast($1, make_bool($2)); }
	| typecast integer      { $$ = cast($1, make_int($2)); }
	| typecast float        { $$ = cast($1, make_float($2)); }
	| typecast STRING       { $$ = cast($1, $2); }
	| typecast RAW          { $$ = cast($1, $2); }
	| typecast array        { $$ = cast($1, $2); }
	| typecast message      { $$ = cast($1, $2); }
	| typecast archive      { $$ = cast($1, $2); }
	| typecast type         { $$ = cast($1, $2); }
	| typecast '(' expr ')' { $$ = cast($1, $3); }
	;

typecast
	: '(' ARRAY ')'         { $$ = get_type("raw"); }
	| '(' MESSAGE ')'       { $$ = get_type("message"); }
	| '(' ARCHIVE IDENT ')' { $$ = get_type("message"); free_mem($3); }
	| '(' IDENT ')'         { $$ = get_type($2); free_mem($2); }
	;

datatype
	: ARRAY         { $$ = get_type("raw"); }
	| MESSAGE       { $$ = get_type("message"); }
	| ARCHIVE IDENT { $$ = get_type("message"); free_mem($2); }
	| IDENT         { $$ = get_type($1); free_mem($1); }
	;

integer
	: INTEGER     { $$ = $1; }
	| '-' INTEGER { $$ = -($2); }
	;

float
	: FLOAT       { $$ = $1; }
	| '-' FLOAT   { $$ = -($2); }
	;

%%
//------------------------------------------------------------------------------


void
yyerror(const char* msg)
{
	// This function is called by the parser when it encounters
	// an error, after which it aborts parsing and returns from
	// yyparse(). We never call yyerror() directly.

	rdef_err = RDEF_COMPILE_ERR;
	rdef_err_line = yylineno;
	strcpy(rdef_err_file, lexfile);
	strcpy(rdef_err_msg, msg);
}


void
add_symbol(const char* name, int32 id)
{
	if (symbol_table.find(name) != symbol_table.end())
		abort_compile(RDEF_COMPILE_ERR, "duplicate symbol %s", name);

	symbol_table.insert(make_pair(name, id));
}


int32
get_symbol(const char* name)
{
	sym_iter_t i = symbol_table.find(name);

	if (i == symbol_table.end())
		abort_compile(RDEF_COMPILE_ERR, "unknown symbol %s", name);

	return i->second;
}


static void
add_builtin_type(type_code code, const char* name)
{
	type_t type;
	type.code     = code;
	type.name     = name;
	type.count    = 0;
	type.fields   = NULL;
	type.def_id   = 1;
	type.def_name = NULL;

	type_table.insert(make_pair(name, type));
}


void
add_user_type(res_id_t id, type_code code, const char* name, list_t list)
{
	if (type_table.find(name) != type_table.end())
		abort_compile(RDEF_COMPILE_ERR, "duplicate type %s", name);

	type_t type;
	type.code     = code;
	type.name     = name;
	type.count    = list.count;
	type.fields   = (field_t*) list.items;
	type.def_id   = 1;
	type.def_name = NULL;

	if (id.has_id)
		type.def_id = id.id;

	if (id.has_name)
		type.def_name = id.name;

	type_table.insert(make_pair(name, type));
}


static bool
is_builtin_type(type_t type)
{
	return type.count == 0;
}


static bool
same_type(type_t type1, type_t type2)
{
	return type1.name == type2.name;  // no need for strcmp
}


type_t
get_type(const char* name)
{
	type_iter_t i = type_table.find(name);

	if (i == type_table.end())
		abort_compile(RDEF_COMPILE_ERR, "unknown type %s", name);

	return i->second;
}


bool
is_type(const char* name)
{
	return type_table.find(name) != type_table.end();
}


define_t
get_define(const char* name)
{
	define_iter_t i = define_table.find(name);

	if (i == define_table.end())
		abort_compile(RDEF_COMPILE_ERR, "unknown define %s", name);

	return i->second;
}


data_t
make_data(size_t size, type_t type)
{
	data_t out;
	out.type = type;
	out.name = NULL;
	out.size = size;
	out.ptr  = alloc_mem(size);
	return out;
}


data_t
make_bool(bool b)
{
	data_t out = make_data(sizeof(bool), get_type("bool"));
	*((bool*)out.ptr) = b;
	return out;
}


data_t
make_int(uint64 i)
{
	data_t out = make_data(sizeof(uint64), get_type("uint64"));
	*((uint64*)out.ptr) = i;
	return out;
}


data_t
make_float(double f)
{
	data_t out = make_data(sizeof(double), get_type("double"));
	*((double*)out.ptr) = f;
	return out;
}


data_t
import_data(char* filename)
{
	data_t out;
	out.type = get_type("raw");
	out.name = NULL;

	char tmpname[B_PATH_NAME_LENGTH];
	if (open_file_from_include_dir(filename, tmpname)) {
		BFile file(tmpname, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			off_t size;
			if (file.GetSize(&size) == B_OK) {
				out.size = (size_t) size;
				out.ptr  = alloc_mem(size);

				if (file.Read(out.ptr, out.size) == (ssize_t) out.size) {
					free_mem(filename);
					return out;
				}
			}
		}
	}

	abort_compile(RDEF_COMPILE_ERR, "cannot import %s", filename);
	return out;
}


data_t
resize_data(data_t data, size_t newSize)
{
	if (newSize == 0) {
		abort_compile(RDEF_COMPILE_ERR, "invalid size %lu", newSize);
	} else if (data.size != newSize) {
		void* newBuffer = alloc_mem(newSize);

		memset(newBuffer, 0, newSize);
		memcpy(newBuffer, data.ptr, min(data.size, newSize));

		if (data.type.code == B_STRING_TYPE)
			((char*)newBuffer)[newSize - 1] = '\0';

		free_mem(data.ptr);
		data.ptr  = newBuffer;
		data.size = newSize;
	}

	return data;
}


BMessage*
make_msg(list_t list)
{
	BMessage* msg = new BMessage;

	for (int32 t = 0; t < list.count; ++t) {
		data_t data = ((data_t*)list.items)[t];
		msg->AddData(data.name, data.type.code, data.ptr, data.size, false);
		free_mem(data.name);
		free_mem(data.ptr);
	}

	free_mem(list.items);
	return msg;
}


data_t
flatten_msg(BMessage* msg)
{
#ifndef B_BEOS_VERSION_DANO
	data_t out = make_data(msg->FlattenedSize(), get_type("message"));
	msg->Flatten((char*)out.ptr, out.size);
#else
	data_t out = make_data(msg->FlattenedSize(B_MESSAGE_VERSION_1),
		get_type("message"));
	msg->Flatten(B_MESSAGE_VERSION_1, (char*)out.ptr, out.size);
#endif
	delete msg;
	return out;
}


data_t
make_default(type_t type)
{
	data_t out;

	if (is_builtin_type(type)) {
		switch (type.code) {
			case B_BOOL_TYPE:
				out = make_data(sizeof(bool), type);
				*((bool*)out.ptr) = false;
				break;

			case B_INT8_TYPE:
			case B_UINT8_TYPE:
				out = make_data(sizeof(uint8), type);
				*((uint8*)out.ptr) = 0;
				break;

			case B_INT16_TYPE:
			case B_UINT16_TYPE:
				out = make_data(sizeof(uint16), type);
				*((uint16*)out.ptr) = 0;
				break;

			case B_INT32_TYPE:
			case B_UINT32_TYPE:
			case B_SIZE_T_TYPE:
			case B_SSIZE_T_TYPE:
			case B_TIME_TYPE:
				out = make_data(sizeof(uint32), type);
				*((uint32*)out.ptr) = 0;
				break;

			case B_INT64_TYPE:
			case B_UINT64_TYPE:
			case B_OFF_T_TYPE:
				out = make_data(sizeof(uint64), type);
				*((uint64*)out.ptr) = 0;
				break;

			case B_FLOAT_TYPE:
				out = make_data(sizeof(float), type);
				*((float*)out.ptr) = 0.0f;
				break;

			case B_DOUBLE_TYPE:
				out = make_data(sizeof(double), type);
				*((double*)out.ptr) = 0.0;
				break;

			case B_STRING_TYPE:
				out = make_data(sizeof(char), type);
				*((char*)out.ptr) = '\0';
				break;

			case B_RAW_TYPE:
				out = make_data(0, type);
				break;

			case B_MESSAGE_TYPE:
				out = flatten_msg(new BMessage);
				break;
		}
	} else {
		// For user-defined types, we copy the default values of the fields
		// into a new data_t object. There is no need to call resize_data()
		// here, because the default values were already resized to their
		// proper length when we added them to the type.

		size_t size = 0;
		for (int32 t = 0; t < type.count; ++t) {
			size += type.fields[t].data.size;
		}

		out = make_data(size, type);

		uint8* ptr = (uint8*) out.ptr;
		for (int32 t = 0; t < type.count; ++t) {
			data_t field_data = type.fields[t].data;
			memcpy(ptr, field_data.ptr, field_data.size);
			ptr += field_data.size;
		}
	}

	return out;
}


static data_t*
fill_slots(type_t type, list_t list)
{
	data_t* slots = (data_t*)alloc_mem(type.count * sizeof(data_t));
	memset(slots, 0, type.count * sizeof(data_t));

	for (int32 t = 0; t < list.count; ++t) {
		data_t data = ((data_t*)list.items)[t];

		if (data.name == NULL) {
			bool found = false;
			for (int32 k = 0; k < type.count; ++k) {
				if (slots[k].ptr == NULL) {
					slots[k] = cast(type.fields[k].type, data);
					found = true;
					break;
				}
			}

			if (!found)
				abort_compile(RDEF_COMPILE_ERR, "too many fields");
		} else {
			// named field
			bool found = false;
			for (int32 k = 0; k < type.count; ++k) {
				if (strcmp(type.fields[k].name, data.name) == 0) {
					if (slots[k].ptr != NULL)
						free_mem(slots[k].ptr);

					slots[k] = cast(type.fields[k].type, data);
					free_mem(data.name);
					found = true;
					break;
				}
			}

			if (!found)
				abort_compile(RDEF_COMPILE_ERR, "unknown field %s", data.name);
		}
	}

	return slots;
}


static data_t
convert_slots(type_t type, data_t* slots)
{
	size_t size = 0;
	for (int32 k = 0; k < type.count; ++k) {
		if (slots[k].ptr == NULL) {
			// default value
			size += type.fields[k].data.size;
		} else if (type.fields[k].resize != 0)
			size += type.fields[k].resize;
		else
			size += slots[k].size;
	}

	data_t out = make_data(size, type);
	uint8* ptr = (uint8*) out.ptr;

	for (int32 k = 0; k < type.count; ++k) {
		if (slots[k].ptr == NULL) {
			// default value
			memcpy(ptr, type.fields[k].data.ptr, type.fields[k].data.size);
			ptr += type.fields[k].data.size;
		} else if (type.fields[k].resize != 0) {
			data_t temp = resize_data(slots[k], type.fields[k].resize);
			memcpy(ptr, temp.ptr, temp.size);
			ptr += temp.size;
			free_mem(temp.ptr);
		} else {
			memcpy(ptr, slots[k].ptr, slots[k].size);
			ptr += slots[k].size;
			free_mem(slots[k].ptr);
		}
	}

	free_mem(slots);
	return out;
}


data_t
make_type(char* name, list_t list)
{
	// Some explanation is in order. The "list" contains zero or more data_t
	// items. Each of these items corresponds to a data field that the user
	// specified, but not necessarily to a field from the type definition.
	// So here we have to figure out which data item goes where. It is fairly
	// obvious where names items should go, but for items without a name we
	// simply use the first available slot. For any fields that the user did
	// not fill in we use the default value from the type definition. This
	// algorithm allows for variable size fields, such as strings and arrays.

	type_t type = get_type(name);

	data_t* slots = fill_slots(type, list);
	data_t out = convert_slots(type, slots);

	free_mem(name);
	free_mem(list.items);
	return out;
}


list_t
make_field_list(field_t field)
{
	list_t out;
	out.count = 1;
	out.items = alloc_mem(sizeof(field_t));
	*((field_t*)out.items) = field;
	return out;
}


list_t
concat_field_list(list_t list, field_t field)
{
	list_t out;
	out.count = list.count + 1;
	out.items = alloc_mem(out.count * sizeof(field_t));

	memcpy(out.items, list.items, list.count * sizeof(field_t));
	memcpy((field_t*)out.items + list.count, &field, sizeof(field_t));

	free_mem(list.items);
	return out;
}


list_t
make_data_list(data_t data)
{
	list_t out;
	out.count = 1;
	out.items = alloc_mem(sizeof(data_t));
	*((data_t*)out.items) = data;
	return out;
}


list_t
concat_data_list(list_t list, data_t data)
{
	list_t out;
	out.count = list.count + 1;
	out.items = (data_t*)alloc_mem(out.count * sizeof(data_t));

	memcpy(out.items, list.items, list.count * sizeof(data_t));
	memcpy((data_t*)out.items + list.count, &data, sizeof(data_t));

	free_mem(list.items);
	return out;
}


data_t
concat_data(data_t data1, data_t data2)
{
	data_t out = make_data(data1.size + data2.size, get_type("raw"));

	memcpy(out.ptr, data1.ptr, data1.size);
	memcpy((uint8*)out.ptr + data1.size, data2.ptr, data2.size);

	free_mem(data1.ptr);
	free_mem(data2.ptr);
	return out;
}


static data_t
cast_to_uint8(type_t new_type, data_t data)
{
	data_t out = make_data(sizeof(uint8), new_type);

	switch (data.type.code) {
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			*((uint8*)out.ptr) = *(uint8*)data.ptr;
			break;

		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			*((uint8*)out.ptr) = (uint8)*(uint16*)data.ptr;
			break;

		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
			*((uint8*)out.ptr) = (uint8)*(uint32*)data.ptr;
			break;

		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_OFF_T_TYPE:
			*((uint8*)out.ptr) = (uint8)*(uint64*)data.ptr;
			break;

		default:
			abort_compile(RDEF_COMPILE_ERR, "cannot cast to this type");
	}

	free_mem(data.ptr);
	return out;
}


static data_t
cast_to_uint16(type_t new_type, data_t data)
{
	data_t out = make_data(sizeof(uint16), new_type);

	switch (data.type.code) {
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			*((uint16*)out.ptr) = (uint16)*(uint8*)data.ptr;
			break;

		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			*((uint16*)out.ptr) = *(uint16*)data.ptr;
			break;

		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
			*((uint16*)out.ptr) = (uint16)*(uint32*)data.ptr;
			break;

		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_OFF_T_TYPE:
			*((uint16*)out.ptr) = (uint16)*(uint64*)data.ptr;
			break;

		default:
			abort_compile(RDEF_COMPILE_ERR, "cannot cast to this type");
	}

	free_mem(data.ptr);
	return out;
}


static data_t
cast_to_uint32(type_t new_type, data_t data)
{
	data_t out = make_data(sizeof(uint32), new_type);

	switch (data.type.code) {
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			*((uint32*)out.ptr) = (uint32)*(uint8*)data.ptr;
			break;

		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			*((uint32*)out.ptr) = (uint32)*(uint16*)data.ptr;
			break;

		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
			*((uint32*)out.ptr) = *(uint32*)data.ptr;
			break;

		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_OFF_T_TYPE:
			*((uint32*)out.ptr) = (uint32)*(uint64*)data.ptr;
			break;

		default:
			abort_compile(RDEF_COMPILE_ERR, "cannot cast to this type");
	}

	free_mem(data.ptr);
	return out;
}


static data_t
cast_to_uint64(type_t new_type, data_t data)
{
	data_t out = make_data(sizeof(uint64), new_type);

	switch (data.type.code) {
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			*((uint64*)out.ptr) = (uint64)*(uint8*)data.ptr;
			break;

		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			*((uint64*)out.ptr) = (uint64)*(uint16*)data.ptr;
			break;

		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
			*((uint64*)out.ptr) = (uint64)*(uint32*)data.ptr;
			break;

		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_OFF_T_TYPE:
			*((uint64*)out.ptr) = *(uint64*)data.ptr;
			break;

		default:
			abort_compile(RDEF_COMPILE_ERR, "cannot cast to this type");
	}

	free_mem(data.ptr);
	return out;
}


static data_t
cast_to_float(type_t new_type, data_t data)
{
	data_t out = make_data(sizeof(float), new_type);

	switch (data.type.code) {
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			*((float*)out.ptr) = (float)*((uint8*)data.ptr);
			break;

		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			*((float*)out.ptr) = (float)*((uint16*)data.ptr);
			break;

		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
			*((float*)out.ptr) = (float)*((uint32*)data.ptr);
			break;

		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_OFF_T_TYPE:
			*((float*)out.ptr) = (float)*((uint64*)data.ptr);
			break;

		case B_DOUBLE_TYPE:
			*((float*)out.ptr) = (float)*((double*)data.ptr);
			break;

		default:
			abort_compile(RDEF_COMPILE_ERR, "cannot cast to this type");
	}

	free_mem(data.ptr);
	return out;
}


static data_t
cast_to_double(type_t new_type, data_t data)
{
	data_t out = make_data(sizeof(double), new_type);

	switch (data.type.code) {
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			*((double*)out.ptr) = (double)*((uint8*)data.ptr);
			break;

		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			*((double*)out.ptr) = (double)*((uint16*)data.ptr);
			break;

		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
			*((double*)out.ptr) = (double)*((uint32*)data.ptr);
			break;

		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_OFF_T_TYPE:
			*((double*)out.ptr) = (double)*((uint64*)data.ptr);
			break;

		case B_FLOAT_TYPE:
			*((double*)out.ptr) = (double)*((float*)data.ptr);
			break;

		default:
			abort_compile(RDEF_COMPILE_ERR, "cannot cast to this type");
	}

	free_mem(data.ptr);
	return out;
}


data_t
cast(type_t newType, data_t data)
{
	if (same_type(newType, data.type)) {
		// you can't cast bool, string,
		// message, or user-defined type
		// to another type, only to same
		return data;
	}

	if (is_builtin_type(newType)) {
		switch (newType.code) {
			case B_INT8_TYPE:
			case B_UINT8_TYPE:
				return cast_to_uint8(newType, data);

			case B_INT16_TYPE:
			case B_UINT16_TYPE:
				return cast_to_uint16(newType, data);

			case B_INT32_TYPE:
			case B_UINT32_TYPE:
			case B_SIZE_T_TYPE:
			case B_SSIZE_T_TYPE:
			case B_TIME_TYPE:
				return cast_to_uint32(newType, data);

			case B_INT64_TYPE:
			case B_UINT64_TYPE:
			case B_OFF_T_TYPE:
				return cast_to_uint64(newType, data);

			case B_FLOAT_TYPE:
				return cast_to_float(newType, data);

			case B_DOUBLE_TYPE:
				return cast_to_double(newType, data);

			case B_RAW_TYPE:
				// you can always cast anything to raw
				data.type = newType;
				return data;
		}
	}

	abort_compile(RDEF_COMPILE_ERR, "cannot cast to this type");
	return data;
}


data_t
unary_expr(data_t data, char oper)
{
	data_t op = cast_to_uint32(get_type("int32"), data);
	int32 i = *((int32*)op.ptr);
	data_t out;

	switch (oper) {
		case '~':
			out = make_int(~i);
			break;
	}

	free_mem(op.ptr);

	return cast(get_type("int32"), out);
}


data_t
binary_expr(data_t data1, data_t data2, char oper)
{
	data_t op1 = cast_to_uint32(get_type("int32"), data1);
	data_t op2 = cast_to_uint32(get_type("int32"), data2);
	int32 i1 = *((int32*) op1.ptr);
	int32 i2 = *((int32*) op2.ptr);
	data_t out;

	switch (oper) {
		case '+':
			out = make_int(i1 + i2);
			break;
		case '-':
			out = make_int(i1 - i2);
			break;
		case '*':
			out = make_int(i1 * i2);
			break;

		case '/':
			if (i2 == 0)
				abort_compile(RDEF_COMPILE_ERR, "division by zero");
			else
				 out = make_int(i1 / i2);
			break;

		case '%':
			if (i2 == 0)
				abort_compile(RDEF_COMPILE_ERR, "division by zero");
			else
				out = make_int(i1 % i2);
			break;

		case '|':
			out = make_int(i1 | i2);
			break;
		case '^':
			out = make_int(i1 ^ i2);
			break;
		case '&':
			out = make_int(i1 & i2);
			break;
	}

	free_mem(op1.ptr);
	free_mem(op2.ptr);

	return cast(get_type("int32"), out);
}


void
add_resource(res_id_t id, type_code code, data_t data)
{
	if (!id.has_id)
		id.id = data.type.def_id;

	if (!id.has_name)
		id.name = (char*)data.type.def_name;

	if (!(flags & RDEF_MERGE_RESOURCES) && rsrc.HasResource(code, id.id))
		abort_compile(RDEF_COMPILE_ERR, "duplicate resource");

	status_t err = rsrc.AddResource(code, id.id, data.ptr, data.size, id.name);
	if (err != B_OK) {
		rdef_err = RDEF_WRITE_ERR;
		rdef_err_line = 0;
		strcpy(rdef_err_file, rsrc_file);
		sprintf(rdef_err_msg, "cannot add resource (%s)", strerror(err));
		abort_compile();
	}

	if (id.has_name)
		free_mem(id.name);

	free_mem(data.ptr);
}


static void
add_point_type()
{
	field_t* fields  = (field_t*)alloc_mem(2 * sizeof(field_t));
	fields[0].type   = get_type("float");
	fields[0].name   = "x";
	fields[0].resize = 0;
	fields[0].data   = make_default(fields[0].type);
	fields[1].type   = get_type("float");
	fields[1].name   = "y";
	fields[1].resize = 0;
	fields[1].data   = make_default(fields[1].type);

	type_t type;
	type.code     = B_POINT_TYPE;
	type.name     = "point";
	type.fields   = fields;
	type.count    = 2;
	type.def_id   = 1;
	type.def_name = NULL;

	type_table.insert(make_pair(type.name, type));
}


static void
add_rect_type()
{
	field_t* fields  = (field_t*)alloc_mem(4 * sizeof(field_t));
	fields[0].type   = get_type("float");
	fields[0].name   = "left";
	fields[0].resize = 0;
	fields[0].data   = make_default(fields[0].type);
	fields[1].type   = get_type("float");
	fields[1].name   = "top";
	fields[1].resize = 0;
	fields[1].data   = make_default(fields[1].type);
	fields[2].type   = get_type("float");
	fields[2].name   = "right";
	fields[2].resize = 0;
	fields[2].data   = make_default(fields[2].type);
	fields[3].type   = get_type("float");
	fields[3].name   = "bottom";
	fields[3].resize = 0;
	fields[3].data   = make_default(fields[3].type);

	type_t type;
	type.code     = B_RECT_TYPE;
	type.name     = "rect";
	type.fields   = fields;
	type.count    = 4;
	type.def_id   = 1;
	type.def_name = NULL;

	type_table.insert(make_pair(type.name, type));
}


static void
add_rgb_color_type()
{
	field_t* fields  = (field_t*)alloc_mem(4 * sizeof(field_t));
	fields[0].type   = get_type("uint8");
	fields[0].name   = "red";
	fields[0].resize = 0;
	fields[0].data   = make_default(fields[0].type);
	fields[1].type   = get_type("uint8");
	fields[1].name   = "green";
	fields[1].resize = 0;
	fields[1].data   = make_default(fields[1].type);
	fields[2].type   = get_type("uint8");
	fields[2].name   = "blue";
	fields[2].resize = 0;
	fields[2].data   = make_default(fields[2].type);
	fields[3].type   = get_type("uint8");
	fields[3].name   = "alpha";
	fields[3].resize = 0;
	fields[3].data   = make_default(fields[3].type);

	*((uint8*)fields[3].data.ptr) = 255;

	type_t type;
	type.code     = B_RGB_COLOR_TYPE;
	type.name     = "rgb_color";
	type.fields   = fields;
	type.count    = 4;
	type.def_id   = 1;
	type.def_name = NULL;

	type_table.insert(make_pair(type.name, type));
}


static void
add_app_signature_type()
{
	field_t* fields  = (field_t*)alloc_mem(1 * sizeof(field_t));
	fields[0].type   = get_type("string");
	fields[0].name   = "signature";
	fields[0].resize = 0;
	fields[0].data   = make_default(fields[0].type);

	type_t type;
	type.code     = 'MIMS';
	type.name     = "app_signature";
	type.fields   = fields;
	type.count    = 1;
	type.def_id   = 1;
	type.def_name = "BEOS:APP_SIG";

	type_table.insert(make_pair(type.name, type));
}


static void
add_app_name_catalog_entry()
{
	field_t* fields  = (field_t*)alloc_mem(1 * sizeof(field_t));
	fields[0].type   = get_type("string");
	fields[0].name   = "catalog_entry";
	fields[0].resize = 0;
	fields[0].data   = make_default(fields[0].type);

	type_t type;
	type.code     = B_STRING_TYPE;
	type.name     = "app_name_catalog_entry";
	type.fields   = fields;
	type.count    = 1;
	type.def_id   = 1;
	type.def_name = "SYS:NAME";

	type_table.insert(make_pair(type.name, type));
}


static void
add_app_flags()
{
	field_t* fields  = (field_t*)alloc_mem(1 * sizeof(field_t));
	fields[0].type   = get_type("uint32");
	fields[0].name   = "flags";
	fields[0].resize = 0;
	fields[0].data   = make_default(fields[0].type);

	type_t type;
	type.code     = 'APPF';
	type.name     = "app_flags";
	type.fields   = fields;
	type.count    = 1;
	type.def_id   = 1;
	type.def_name = "BEOS:APP_FLAGS";

	type_table.insert(make_pair(type.name, type));
}


static void
add_app_version()
{
	field_t* fields  = (field_t*)alloc_mem(7 * sizeof(field_t));
	fields[0].type   = get_type("uint32");
	fields[0].name   = "major";
	fields[0].resize = 0;
	fields[0].data   = make_default(fields[0].type);
	fields[1].type   = get_type("uint32");
	fields[1].name   = "middle";
	fields[1].resize = 0;
	fields[1].data   = make_default(fields[1].type);
	fields[2].type   = get_type("uint32");
	fields[2].name   = "minor";
	fields[2].resize = 0;
	fields[2].data   = make_default(fields[2].type);
	fields[3].type   = get_type("uint32");
	fields[3].name   = "variety";
	fields[3].resize = 0;
	fields[3].data   = make_default(fields[3].type);
	fields[4].type   = get_type("uint32");
	fields[4].name   = "internal";
	fields[4].resize = 0;
	fields[4].data   = make_default(fields[4].type);
	fields[5].type   = get_type("string");
	fields[5].name   = "short_info";
	fields[5].resize = 64;
	fields[5].data   = make_data(fields[5].resize, fields[5].type);
	fields[6].type   = get_type("string");
	fields[6].name   = "long_info";
	fields[6].resize = 256;
	fields[6].data   = make_data(fields[6].resize, fields[6].type);

	memset(fields[5].data.ptr, '\0', fields[5].data.size);
	memset(fields[6].data.ptr, '\0', fields[6].data.size);

	type_t type;
	type.code     = 'APPV';
	type.name     = "app_version";
	type.fields   = fields;
	type.count    = 7;
	type.def_id   = 1;
	type.def_name = "BEOS:APP_VERSION";

	type_table.insert(make_pair(type.name, type));
}


static void
add_png_icon()
{
	field_t* fields  = (field_t*)alloc_mem(1 * sizeof(field_t));
	fields[0].type   = get_type("raw");
	fields[0].name   = "icon";
	fields[0].resize = 0;
	fields[0].data   = make_data(fields[0].resize, fields[0].type);

	type_t type;
	type.code     = 'PNG ';
	type.name     = "png_icon";
	type.fields   = fields;
	type.count    = 1;
	type.def_id   = 101;
	type.def_name = "BEOS:ICON";

	type_table.insert(make_pair(type.name, type));
}


static void
add_vector_icon()
{
	field_t* fields  = (field_t*)alloc_mem(1 * sizeof(field_t));
	fields[0].type   = get_type("raw");
	fields[0].name   = "icon";
	fields[0].resize = 0;
	fields[0].data   = make_data(fields[0].resize, fields[0].type);

	type_t type;
	type.code     = 'VICN';
	type.name     = "vector_icon";
	type.fields   = fields;
	type.count    = 1;
	type.def_id   = 101;
	type.def_name = "BEOS:ICON";

	type_table.insert(make_pair(type.name, type));
}


static void
add_large_icon()
{
	field_t* fields  = (field_t*)alloc_mem(1 * sizeof(field_t));
	fields[0].type   = get_type("raw");
	fields[0].name   = "icon";
	fields[0].resize = 1024;
	fields[0].data   = make_data(fields[0].resize, fields[0].type);

	type_t type;
	type.code     = 'ICON';
	type.name     = "large_icon";
	type.fields   = fields;
	type.count    = 1;
	type.def_id   = 101;
	type.def_name = "BEOS:L:STD_ICON";

	type_table.insert(make_pair(type.name, type));
}


static void
add_mini_icon()
{
	field_t* fields  = (field_t*)alloc_mem(1 * sizeof(field_t));
	fields[0].type   = get_type("raw");
	fields[0].name   = "icon";
	fields[0].resize = 256;
	fields[0].data   = make_data(fields[0].resize, fields[0].type);

	type_t type;
	type.code     = 'MICN';
	type.name     = "mini_icon";
	type.fields   = fields;
	type.count    = 1;
	type.def_id   = 101;
	type.def_name = "BEOS:M:STD_ICON";

	type_table.insert(make_pair(type.name, type));
}


static void
add_file_types()
{
	field_t* fields  = (field_t*)alloc_mem(1 * sizeof(field_t));
	fields[0].type   = get_type("message");
	fields[0].name   = "types";
	fields[0].resize = 0;
	fields[0].data   = make_default(fields[0].type);

	type_t type;
	type.code     = 'MSGG';
	type.name     = "file_types";
	type.fields   = fields;
	type.count    = 1;
	type.def_id   = 1;
	type.def_name = "BEOS:FILE_TYPES";

	type_table.insert(make_pair(type.name, type));
}


static void
add_define(const char* name, int32 value)
{
	define_t define;
	define.name  = name;
	define.value = value;

	define_table.insert(make_pair(define.name, define));
}


void
init_parser()
{
	add_builtin_type(B_BOOL_TYPE,    "bool");
	add_builtin_type(B_INT8_TYPE,    "int8");
	add_builtin_type(B_UINT8_TYPE,   "uint8");
	add_builtin_type(B_INT16_TYPE,   "int16");
	add_builtin_type(B_UINT16_TYPE,  "uint16");
	add_builtin_type(B_INT32_TYPE,   "int32");
	add_builtin_type(B_UINT32_TYPE,  "uint32");
	add_builtin_type(B_SIZE_T_TYPE,  "size_t");
	add_builtin_type(B_SSIZE_T_TYPE, "ssize_t");
	add_builtin_type(B_TIME_TYPE,    "time_t");
	add_builtin_type(B_INT64_TYPE,   "int64");
	add_builtin_type(B_UINT64_TYPE,  "uint64");
	add_builtin_type(B_OFF_T_TYPE,   "off_t");
	add_builtin_type(B_FLOAT_TYPE,   "float");
	add_builtin_type(B_DOUBLE_TYPE,  "double");
	add_builtin_type(B_STRING_TYPE,  "string");
	add_builtin_type(B_RAW_TYPE,     "raw");
	add_builtin_type(B_RAW_TYPE,     "buffer");
	add_builtin_type(B_MESSAGE_TYPE, "message");

	add_point_type();
	add_rect_type();
	add_rgb_color_type();
	add_app_signature_type();
	add_app_name_catalog_entry();
	add_app_flags();
	add_app_version();
	add_large_icon();
	add_mini_icon();
	add_vector_icon();
	add_png_icon();
	add_file_types();

	add_define("B_SINGLE_LAUNCH",    0x0);
	add_define("B_MULTIPLE_LAUNCH",  0x1);
	add_define("B_EXCLUSIVE_LAUNCH", 0x2);
	add_define("B_BACKGROUND_APP",   0x4);
	add_define("B_ARGV_ONLY",        0x8);

	add_define("B_APPV_DEVELOPMENT",   0x0);
	add_define("B_APPV_ALPHA",         0x1);
	add_define("B_APPV_BETA",          0x2);
	add_define("B_APPV_GAMMA",         0x3);
	add_define("B_APPV_GOLDEN_MASTER", 0x4);
	add_define("B_APPV_FINAL",         0x5);
}


void
clean_up_parser()
{
	// The symbol table entries have several malloc'ed objects associated
	// with them (such as their name). They were allocated with alloc_mem(),
	// so we don't need to free them here; compile.cpp already does that
	// when it cleans up. However, we do need to remove the entries from
	// the tables, otherwise they will still be around the next time we are
	// asked to compile something.

#ifdef DEBUG
	// Note that in DEBUG mode, we _do_ free these items, so they don't show
	// up in the mem leak statistics. The names etc of builtin items are not
	// alloc_mem()'d but we still free_mem() them. Not entirely correct, but
	// it doesn't seem to hurt, and we only do it in DEBUG mode anyway.

	for (sym_iter_t i = symbol_table.begin(); i != symbol_table.end(); ++i) {
		free_mem((void*) i->first);
	}

	for (type_iter_t i = type_table.begin(); i != type_table.end(); ++i) {
		free_mem((void*) i->first);
		type_t type = i->second;

		for (int32 t = 0; t < type.count; ++t) {
			free_mem((void*) type.fields[t].name);
			free_mem((void*) type.fields[t].data.ptr);
		}
		free_mem((void*) type.fields);
		free_mem((void*) type.name);
		free_mem((void*) type.def_name);
	}
#endif

	symbol_table.clear();
	type_table.clear();
	define_table.clear();
}

