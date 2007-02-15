/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#if !defined SLANG_COMPILE_H
#define SLANG_COMPILE_H

#include "slang_export.h"
#include "slang_execute.h"
#include "slang_compile_variable.h"
#include "slang_compile_struct.h"
#include "slang_compile_operation.h"
#include "slang_compile_function.h"

#if defined __cplusplus
extern "C" {
#endif

typedef enum slang_unit_type_
{
	slang_unit_fragment_shader,
	slang_unit_vertex_shader,
	slang_unit_fragment_builtin,
	slang_unit_vertex_builtin
} slang_unit_type;

typedef struct slang_var_pool_
{
	GLuint next_addr;
} slang_var_pool;

typedef struct slang_code_unit_
{
   slang_variable_scope vars;
   slang_function_scope funs;
   slang_struct_scope structs;
   slang_unit_type type;
   struct slang_code_object_ *object;
} slang_code_unit;

extern GLvoid
_slang_code_unit_ctr (slang_code_unit *, struct slang_code_object_ *);

extern GLvoid
_slang_code_unit_dtr (slang_code_unit *);

#define SLANG_BUILTIN_CORE   0
#define SLANG_BUILTIN_COMMON 1
#define SLANG_BUILTIN_TARGET 2

#if defined(USE_X86_ASM) || defined(SLANG_X86)
#define SLANG_BUILTIN_VEC4   3
#define SLANG_BUILTIN_TOTAL  4
#else
#define SLANG_BUILTIN_TOTAL  3
#endif

typedef struct slang_code_object_
{
   slang_code_unit builtin[SLANG_BUILTIN_TOTAL];
   slang_code_unit unit;
   slang_assembly_file assembly;
   slang_machine machine;
   slang_var_pool varpool;
   slang_atom_pool atompool;
   slang_export_data_table expdata;
   slang_export_code_table expcode;
} slang_code_object;

extern GLvoid
_slang_code_object_ctr (slang_code_object *);

extern GLvoid
_slang_code_object_dtr (slang_code_object *);

typedef struct slang_info_log_
{
	char *text;
	int dont_free_text;
} slang_info_log;

void slang_info_log_construct (slang_info_log *);
void slang_info_log_destruct (slang_info_log *);
int slang_info_log_print (slang_info_log *, const char *, ...);
int slang_info_log_error (slang_info_log *, const char *, ...);
int slang_info_log_warning (slang_info_log *, const char *, ...);
void slang_info_log_memory (slang_info_log *);

extern GLboolean
_slang_compile (const char *, slang_code_object *, slang_unit_type, slang_info_log *);

#ifdef __cplusplus
}
#endif

#endif

