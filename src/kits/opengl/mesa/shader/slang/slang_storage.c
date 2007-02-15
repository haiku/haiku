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

/**
 * \file slang_storage.c
 * slang variable storage
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_storage.h"

/* slang_storage_array */

GLboolean slang_storage_array_construct (slang_storage_array *arr)
{
	arr->type = slang_stor_aggregate;
	arr->aggregate = NULL;
	arr->length = 0;
	return GL_TRUE;
}

GLvoid slang_storage_array_destruct (slang_storage_array *arr)
{
	if (arr->aggregate != NULL)
	{
		slang_storage_aggregate_destruct (arr->aggregate);
		slang_alloc_free (arr->aggregate);
	}
}

/* slang_storage_aggregate */

GLboolean slang_storage_aggregate_construct (slang_storage_aggregate *agg)
{
	agg->arrays = NULL;
	agg->count = 0;
	return GL_TRUE;
}

GLvoid slang_storage_aggregate_destruct (slang_storage_aggregate *agg)
{
	GLuint i;

	for (i = 0; i < agg->count; i++)
		slang_storage_array_destruct (agg->arrays + i);
	slang_alloc_free (agg->arrays);
}

static slang_storage_array *slang_storage_aggregate_push_new (slang_storage_aggregate *agg)
{
	slang_storage_array *arr = NULL;

	agg->arrays = (slang_storage_array *) slang_alloc_realloc (agg->arrays, agg->count * sizeof (
		slang_storage_array), (agg->count + 1) * sizeof (slang_storage_array));
	if (agg->arrays != NULL)
	{
		arr = agg->arrays + agg->count;
		if (!slang_storage_array_construct (arr))
			return NULL;
		agg->count++;
	}
	return arr;
}

/* _slang_aggregate_variable() */

static GLboolean aggregate_vector (slang_storage_aggregate *agg, slang_storage_type basic_type,
	GLuint row_count)
{
	slang_storage_array *arr = slang_storage_aggregate_push_new (agg);
	if (arr == NULL)
		return GL_FALSE;
	arr->type = basic_type;
	arr->length = row_count;
	return GL_TRUE;
}

static GLboolean aggregate_matrix (slang_storage_aggregate *agg, slang_storage_type basic_type,
	GLuint dimension)
{
	slang_storage_array *arr = slang_storage_aggregate_push_new (agg);
	if (arr == NULL)
		return GL_FALSE;
	arr->type = slang_stor_aggregate;
	arr->length = dimension;
	arr->aggregate = (slang_storage_aggregate *) slang_alloc_malloc (sizeof (slang_storage_aggregate));
	if (arr->aggregate == NULL)
		return GL_FALSE;
	if (!slang_storage_aggregate_construct (arr->aggregate))
	{
		slang_alloc_free (arr->aggregate);
		arr->aggregate = NULL;
		return GL_FALSE;
	}
	if (!aggregate_vector (arr->aggregate, basic_type, dimension))
		return GL_FALSE;
	return GL_TRUE;
}

static GLboolean aggregate_variables (slang_storage_aggregate *agg, slang_variable_scope *vars,
	slang_function_scope *funcs, slang_struct_scope *structs, slang_variable_scope *globals,
	slang_machine *mach, slang_assembly_file *file, slang_atom_pool *atoms)
{
	GLuint i;

	for (i = 0; i < vars->num_variables; i++)
		if (!_slang_aggregate_variable (agg, &vars->variables[i].type.specifier,
				vars->variables[i].array_len, funcs, structs, globals, mach, file, atoms))
			return GL_FALSE;
	return GL_TRUE;
}

GLboolean _slang_evaluate_int (slang_assembly_file *file, slang_machine *pmach,
	slang_assembly_name_space *space, slang_operation *array_size, GLuint *pint,
	slang_atom_pool *atoms)
{
	slang_assembly_file_restore_point point;
	slang_machine mach;
	slang_assemble_ctx A;

	A.file = file;
	A.mach = pmach;
	A.atoms = atoms;
	A.space = *space;
	A.local.ret_size = 0;
	A.local.addr_tmp = 0;
	A.local.swizzle_tmp = 4;

	/* save the current assembly */
	if (!slang_assembly_file_restore_point_save (file, &point))
		return GL_FALSE;

	/* setup the machine */
	mach = *pmach;
	mach.ip = file->count;

	/* allocate local storage for expression */
	if (!slang_assembly_file_push_label (file, slang_asm_local_alloc, 20))
		return GL_FALSE;
	if (!slang_assembly_file_push_label (file, slang_asm_enter, 20))
		return GL_FALSE;

	/* insert the actual expression */
	if (!_slang_assemble_operation (&A, array_size, slang_ref_forbid))
		return GL_FALSE;
	if (!slang_assembly_file_push (file, slang_asm_exit))
		return GL_FALSE;

	/* execute the expression */
	if (!_slang_execute2 (file, &mach))
		return GL_FALSE;

	/* the evaluated expression is on top of the stack */
	*pint = (GLuint) mach.mem[mach.sp + SLANG_MACHINE_GLOBAL_SIZE]._float;

	/* restore the old assembly */
	if (!slang_assembly_file_restore_point_load (file, &point))
		return GL_FALSE;

	return GL_TRUE;
}

GLboolean _slang_aggregate_variable (slang_storage_aggregate *agg, slang_type_specifier *spec,
	GLuint array_len, slang_function_scope *funcs, slang_struct_scope *structs,
	slang_variable_scope *vars, slang_machine *mach, slang_assembly_file *file,
	slang_atom_pool *atoms)
{
	switch (spec->type)
	{
	case slang_spec_bool:
		return aggregate_vector (agg, slang_stor_bool, 1);
	case slang_spec_bvec2:
		return aggregate_vector (agg, slang_stor_bool, 2);
	case slang_spec_bvec3:
		return aggregate_vector (agg, slang_stor_bool, 3);
	case slang_spec_bvec4:
		return aggregate_vector (agg, slang_stor_bool, 4);
	case slang_spec_int:
		return aggregate_vector (agg, slang_stor_int, 1);
	case slang_spec_ivec2:
		return aggregate_vector (agg, slang_stor_int, 2);
	case slang_spec_ivec3:
		return aggregate_vector (agg, slang_stor_int, 3);
	case slang_spec_ivec4:
		return aggregate_vector (agg, slang_stor_int, 4);
	case slang_spec_float:
		return aggregate_vector (agg, slang_stor_float, 1);
	case slang_spec_vec2:
		return aggregate_vector (agg, slang_stor_float, 2);
	case slang_spec_vec3:
		return aggregate_vector (agg, slang_stor_float, 3);
   case slang_spec_vec4:
#if defined(USE_X86_ASM) || defined(SLANG_X86)
      return aggregate_vector (agg, slang_stor_vec4, 1);
#else
      return aggregate_vector (agg, slang_stor_float, 4);
#endif
	case slang_spec_mat2:
		return aggregate_matrix (agg, slang_stor_float, 2);
	case slang_spec_mat3:
		return aggregate_matrix (agg, slang_stor_float, 3);
   case slang_spec_mat4:
#if defined(USE_X86_ASM) || defined(SLANG_X86)
      return aggregate_vector (agg, slang_stor_vec4, 4);
#else
      return aggregate_matrix (agg, slang_stor_float, 4);
#endif
	case slang_spec_sampler1D:
	case slang_spec_sampler2D:
	case slang_spec_sampler3D:
	case slang_spec_samplerCube:
	case slang_spec_sampler1DShadow:
	case slang_spec_sampler2DShadow:
		return aggregate_vector (agg, slang_stor_int, 1);
	case slang_spec_struct:
		return aggregate_variables (agg, spec->_struct->fields, funcs, structs, vars, mach,
			file, atoms);
	case slang_spec_array:
		{
			slang_storage_array *arr;

			arr = slang_storage_aggregate_push_new (agg);
			if (arr == NULL)
				return GL_FALSE;
			arr->type = slang_stor_aggregate;
			arr->aggregate = (slang_storage_aggregate *) slang_alloc_malloc (sizeof (slang_storage_aggregate));
			if (arr->aggregate == NULL)
				return GL_FALSE;
			if (!slang_storage_aggregate_construct (arr->aggregate))
			{
				slang_alloc_free (arr->aggregate);
				arr->aggregate = NULL;
				return GL_FALSE;
			}
			if (!_slang_aggregate_variable (arr->aggregate, spec->_array, 0, funcs, structs,
					vars, mach, file, atoms))
				return GL_FALSE;
			arr->length = array_len;
			/* TODO: check if 0 < arr->length <= 65535 */
		}
		return GL_TRUE;
	default:
		return GL_FALSE;
	}
}

/* _slang_sizeof_type() */

GLuint
_slang_sizeof_type (slang_storage_type type)
{
   if (type == slang_stor_aggregate)
      return 0;
   if (type == slang_stor_vec4)
      return 4 * sizeof (GLfloat);
   return sizeof (GLfloat);
}

/* _slang_sizeof_aggregate() */

GLuint _slang_sizeof_aggregate (const slang_storage_aggregate *agg)
{
   GLuint i, size = 0;

   for (i = 0; i < agg->count; i++) {
      slang_storage_array *arr = &agg->arrays[i];
      GLuint element_size;

      if (arr->type == slang_stor_aggregate)
         element_size = _slang_sizeof_aggregate (arr->aggregate);
      else
         element_size = _slang_sizeof_type (arr->type);
      size += element_size * arr->length;
   }
   return size;
}

/* _slang_flatten_aggregate () */

GLboolean
_slang_flatten_aggregate (slang_storage_aggregate *flat, const slang_storage_aggregate *agg)
{
   GLuint i;

   for (i = 0; i < agg->count; i++) {
      GLuint j;

      for (j = 0; j < agg->arrays[i].length; j++) {
         if (agg->arrays[i].type == slang_stor_aggregate) {
            if (!_slang_flatten_aggregate (flat, agg->arrays[i].aggregate))
               return GL_FALSE;
         }
         else {
            GLuint k, count;
            slang_storage_type type;

            if (agg->arrays[i].type == slang_stor_vec4) {
               count = 4;
               type = slang_stor_float;
            }
            else {
               count = 1;
               type = agg->arrays[i].type;
            }

            for (k = 0; k < count; k++) {
               slang_storage_array *arr;

               arr = slang_storage_aggregate_push_new (flat);
               if (arr == NULL)
                  return GL_FALSE;
               arr->type = type;
               arr->length = 1;
            }
         }
      }
   }
   return GL_TRUE;
}

