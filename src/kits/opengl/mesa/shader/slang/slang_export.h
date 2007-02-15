/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
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

#if !defined SLANG_EXPORT_H
#define SLANG_EXPORT_H

#include "slang_utility.h"

#if defined __cplusplus
extern "C" {
#endif

/*
 * Basic data quantity to transfer between application and assembly.
 * The <size> is the actual size of the data quantity including padding, if any. It is
 * used to calculate offsets from the beginning of the data.
 * If the <array_len> is not 0, the data quantity is an array of <array_len> size.
 * If the <structure> is not NULL, the data quantity is a struct. The <basic_type> is
 * invalid and the <field_count> holds the size of the <structure> array.
 * The <basic_type> values match those of <type> parameter for glGetActiveUniformARB.
 */

typedef struct slang_export_data_quant_
{
	slang_atom name;
	GLuint size;
	GLuint array_len;
	struct slang_export_data_quant_ *structure;
	union
	{
		GLenum basic_type;
		GLuint field_count;
	} u;
} slang_export_data_quant;

GLvoid slang_export_data_quant_ctr (slang_export_data_quant *);
GLvoid slang_export_data_quant_dtr (slang_export_data_quant *);
slang_export_data_quant *slang_export_data_quant_add_field (slang_export_data_quant *);

/*
 * Returns GL_FALSE if the quant is not an array.
 */
GLboolean slang_export_data_quant_array (slang_export_data_quant *);

/*
 * Returns GL_FALSE if the quant is not a structure.
 */
GLboolean slang_export_data_quant_struct (slang_export_data_quant *);

/*
 * Returns GL_TRUE if the quant is neither an array nor a structure.
 */
GLboolean slang_export_data_quant_simple (slang_export_data_quant *);

/*
 * Returns basic type of the quant. It must not be a structure.
 */
GLenum slang_export_data_quant_type (slang_export_data_quant *);

/*
 * Returns number of fields in the quant that is a structure.
 */
GLuint slang_export_data_quant_fields (slang_export_data_quant *);

/*
 * Return number of elements in the quant.
 * For arrays, return the size of the array.
 * Otherwise, return 1.
 */
GLuint slang_export_data_quant_elements (slang_export_data_quant *);

/*
 * Returns total number of components withing the quant element.
 */
GLuint slang_export_data_quant_components (slang_export_data_quant *);

/*
 * Returns size of the quant element.
 */
GLuint slang_export_data_quant_size (slang_export_data_quant *);

/*
 * Data access pattern. Specifies how data is accessed at what frequency.
 */

typedef enum
{
	slang_exp_uniform,
	slang_exp_varying,
	slang_exp_attribute
} slang_export_data_access;

/*
 * Data export entry. Holds the data type information, access pattern and base address.
 */

typedef struct
{
	slang_export_data_quant quant;
	slang_export_data_access access;
	GLuint address;
} slang_export_data_entry;

GLvoid slang_export_data_entry_ctr (slang_export_data_entry *);
GLvoid slang_export_data_entry_dtr (slang_export_data_entry *);

/*
 * Data export table.
 */

typedef struct
{
	slang_export_data_entry *entries;
	GLuint count;
	slang_atom_pool *atoms;
} slang_export_data_table;

GLvoid slang_export_data_table_ctr (slang_export_data_table *);
GLvoid slang_export_data_table_dtr (slang_export_data_table *);
slang_export_data_entry *slang_export_data_table_add (slang_export_data_table *);

/*
 * Code export entry. Contains label name and its entry point (label, address).
 */

typedef struct
{
	slang_atom name;
	GLuint address;
} slang_export_code_entry;

/*
 * Code export table.
 */

typedef struct
{
	slang_export_code_entry *entries;
	GLuint count;
	slang_atom_pool *atoms;
} slang_export_code_table;

GLvoid slang_export_code_table_ctr (slang_export_code_table *);
GLvoid slang_export_code_table_dtr (slang_export_code_table *);
slang_export_code_entry *slang_export_code_table_add (slang_export_code_table *);

/*
 * _slang_find_exported_data()
 *
 * Parses the name string and returns corresponding data entry, data quantity and offset.
 * Returns GL_TRUE if the data is found, returns GL_FALSE otherwise.
 */

GLboolean _slang_find_exported_data (slang_export_data_table *, const char *,
	slang_export_data_entry **, slang_export_data_quant **, GLuint *);

#ifdef __cplusplus
}
#endif

#endif

