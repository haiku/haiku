/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
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

#ifndef SLANG_COMPILE_VARIABLE_H
#define SLANG_COMPILE_VARIABLE_H

#if defined __cplusplus
extern "C" {
#endif


typedef enum slang_type_qualifier_
{
   slang_qual_none,
   slang_qual_const,
   slang_qual_attribute,
   slang_qual_varying,
   slang_qual_uniform,
   slang_qual_out,
   slang_qual_inout,
   slang_qual_fixedoutput,      /* internal */
   slang_qual_fixedinput        /* internal */
} slang_type_qualifier;

extern slang_type_specifier_type
slang_type_specifier_type_from_string(const char *);

extern const char *
slang_type_specifier_type_to_string(slang_type_specifier_type);



typedef struct slang_fully_specified_type_
{
   slang_type_qualifier qualifier;
   slang_type_specifier specifier;
} slang_fully_specified_type;

extern int
slang_fully_specified_type_construct(slang_fully_specified_type *);

extern void
slang_fully_specified_type_destruct(slang_fully_specified_type *);

extern int
slang_fully_specified_type_copy(slang_fully_specified_type *,
				const slang_fully_specified_type *);


/**
 * A shading language program variable.
 */
typedef struct slang_variable_
{
   slang_fully_specified_type type; /**< Variable's data type */
   slang_atom a_name;               /**< The variable's name (char *) */
   GLuint array_len;                /**< only if type == slang_spec_array */
   struct slang_operation_ *initializer; /**< Optional initializer code */
   GLuint address;                  /**< Storage location */
   GLuint address2;                 /**< Storage location */
   GLuint size;                     /**< Variable's size in bytes */
   GLboolean global;                /**< A global var? */
   void *aux;                       /**< Used during code gen */
} slang_variable;


/**
 * Basically a list of variables, with a pointer to the parent scope.
 */
typedef struct slang_variable_scope_
{
   slang_variable *variables;  /**< Array [num_variables] */
   GLuint num_variables;
   struct slang_variable_scope_ *outer_scope;
} slang_variable_scope;

extern GLvoid
_slang_variable_scope_ctr(slang_variable_scope *);

extern void
slang_variable_scope_destruct(slang_variable_scope *);

extern int
slang_variable_scope_copy(slang_variable_scope *,
                          const slang_variable_scope *);

slang_variable *
slang_variable_scope_grow(slang_variable_scope *);

extern int
slang_variable_construct(slang_variable *);

extern void
slang_variable_destruct(slang_variable *);

extern int
slang_variable_copy(slang_variable *, const slang_variable *);

extern slang_variable *
_slang_locate_variable(const slang_variable_scope *, const slang_atom a_name,
                       GLboolean all);

extern GLboolean
_slang_build_export_data_table(slang_export_data_table *,
                               slang_variable_scope *);



#ifdef __cplusplus
}
#endif

#endif /* SLANG_COMPILE_VARIABLE_H */
