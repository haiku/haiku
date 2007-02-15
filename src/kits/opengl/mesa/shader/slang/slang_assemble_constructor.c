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

/**
 * \file slang_assemble_constructor.c
 * slang constructor and vector swizzle assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_assemble.h"
#include "slang_storage.h"



/**
 * Checks if a field selector is a general swizzle (an r-value swizzle
 * with replicated components or an l-value swizzle mask) for a
 * vector.  Returns GL_TRUE if this is the case, <swz> is filled with
 * swizzle information.  Returns GL_FALSE otherwise.
 */
GLboolean
_slang_is_swizzle(const char *field, GLuint rows, slang_swizzle * swz)
{
   GLuint i;
   GLboolean xyzw = GL_FALSE, rgba = GL_FALSE, stpq = GL_FALSE;

   /* the swizzle can be at most 4-component long */
   swz->num_components = slang_string_length(field);
   if (swz->num_components > 4)
      return GL_FALSE;

   for (i = 0; i < swz->num_components; i++) {
      /* mark which swizzle group is used */
      switch (field[i]) {
      case 'x':
      case 'y':
      case 'z':
      case 'w':
         xyzw = GL_TRUE;
         break;
      case 'r':
      case 'g':
      case 'b':
      case 'a':
         rgba = GL_TRUE;
         break;
      case 's':
      case 't':
      case 'p':
      case 'q':
         stpq = GL_TRUE;
         break;
      default:
         return GL_FALSE;
      }

      /* collect swizzle component */
      switch (field[i]) {
      case 'x':
      case 'r':
      case 's':
         swz->swizzle[i] = 0;
         break;
      case 'y':
      case 'g':
      case 't':
         swz->swizzle[i] = 1;
         break;
      case 'z':
      case 'b':
      case 'p':
         swz->swizzle[i] = 2;
         break;
      case 'w':
      case 'a':
      case 'q':
         swz->swizzle[i] = 3;
         break;
      }

      /* check if the component is valid for given vector's row count */
      if (rows <= swz->swizzle[i])
         return GL_FALSE;
   }

   /* only one swizzle group can be used */
   if ((xyzw && rgba) || (xyzw && stpq) || (rgba && stpq))
      return GL_FALSE;

   return GL_TRUE;
}



/**
 * Checks if a general swizzle is an l-value swizzle - these swizzles
 * do not have duplicated fields.  Returns GL_TRUE if this is a
 * swizzle mask.  Returns GL_FALSE otherwise
 */
GLboolean
_slang_is_swizzle_mask(const slang_swizzle * swz, GLuint rows)
{
   GLuint i, c = 0;

   /* the swizzle may not be longer than the vector dim */
   if (swz->num_components > rows)
      return GL_FALSE;

   /* the swizzle components cannot be duplicated */
   for (i = 0; i < swz->num_components; i++) {
      if ((c & (1 << swz->swizzle[i])) != 0)
         return GL_FALSE;
      c |= 1 << swz->swizzle[i];
   }

   return GL_TRUE;
}



/**
 * Combines (multiplies) two swizzles to form single swizzle.
 * Example: "vec.wzyx.yx" --> "vec.zw".
 */
GLvoid
_slang_multiply_swizzles(slang_swizzle * dst, const slang_swizzle * left,
                         const slang_swizzle * right)
{
   GLuint i;

   dst->num_components = right->num_components;
   for (i = 0; i < right->num_components; i++)
      dst->swizzle[i] = left->swizzle[right->swizzle[i]];
}



static GLboolean
sizeof_argument(slang_assemble_ctx * A, GLuint * size, slang_operation * op)
{
   slang_assembly_typeinfo ti;
   GLboolean result = GL_FALSE;
   slang_storage_aggregate agg;

   if (!slang_assembly_typeinfo_construct(&ti))
      return GL_FALSE;
   if (!_slang_typeof_operation(A, op, &ti))
      goto end1;

   if (!slang_storage_aggregate_construct(&agg))
      goto end1;
   if (!_slang_aggregate_variable(&agg, &ti.spec, 0, A->space.funcs,
                                  A->space.structs, A->space.vars,
                                  A->mach, A->file, A->atoms))
      goto end;

   *size = _slang_sizeof_aggregate(&agg);
   result = GL_TRUE;

 end:
   slang_storage_aggregate_destruct(&agg);
 end1:
   slang_assembly_typeinfo_destruct(&ti);
   return result;
}


static GLboolean
constructor_aggregate(slang_assemble_ctx * A,
                      const slang_storage_aggregate * flat,
                      slang_operation * op, GLuint garbage_size)
{
   slang_assembly_typeinfo ti;
   GLboolean result = GL_FALSE;
   slang_storage_aggregate agg, flat_agg;

   if (!slang_assembly_typeinfo_construct(&ti))
      return GL_FALSE;
   if (!_slang_typeof_operation(A, op, &ti))
      goto end1;

   if (!slang_storage_aggregate_construct(&agg))
      goto end1;
   if (!_slang_aggregate_variable(&agg, &ti.spec, 0, A->space.funcs,
                                  A->space.structs, A->space.vars,
                                  A->mach, A->file, A->atoms))
      goto end2;

   if (!slang_storage_aggregate_construct(&flat_agg))
      goto end2;
   if (!_slang_flatten_aggregate(&flat_agg, &agg))
      goto end;

   if (!_slang_assemble_operation(A, op, slang_ref_forbid))
      goto end;

   /* TODO: convert (generic) elements */

   /* free the garbage */
   if (garbage_size != 0) {
      GLuint i;

      /* move the non-garbage part to the end of the argument */
      if (!slang_assembly_file_push_label(A->file, slang_asm_addr_push, 0))
         goto end;
      for (i = flat_agg.count * 4 - garbage_size; i > 0; i -= 4) {
         if (!slang_assembly_file_push_label2(A->file, slang_asm_float_move,
                                              garbage_size + i, i)) {
            goto end;
         }
      }
      if (!slang_assembly_file_push_label
          (A->file, slang_asm_local_free, garbage_size + 4))
         goto end;
   }

   result = GL_TRUE;
 end:
   slang_storage_aggregate_destruct(&flat_agg);
 end2:
   slang_storage_aggregate_destruct(&agg);
 end1:
   slang_assembly_typeinfo_destruct(&ti);
   return result;
}


GLboolean
_slang_assemble_constructor(slang_assemble_ctx * A, const slang_operation * op)
{
   slang_assembly_typeinfo ti;
   GLboolean result = GL_FALSE;
   slang_storage_aggregate agg, flat;
   GLuint size, i;
   GLuint arg_sums[2];

   /* get typeinfo of the constructor (the result of constructor expression) */
   if (!slang_assembly_typeinfo_construct(&ti))
      return GL_FALSE;
   if (!_slang_typeof_operation(A, op, &ti))
      goto end1;

   /* create an aggregate of the constructor */
   if (!slang_storage_aggregate_construct(&agg))
      goto end1;
   if (!_slang_aggregate_variable(&agg, &ti.spec, 0, A->space.funcs,
                                  A->space.structs, A->space.vars,
                                  A->mach, A->file, A->atoms))
      goto end2;

   /* calculate size of the constructor */
   size = _slang_sizeof_aggregate(&agg);

   /* flatten the constructor */
   if (!slang_storage_aggregate_construct(&flat))
      goto end2;
   if (!_slang_flatten_aggregate(&flat, &agg))
      goto end;

   /* collect the last two constructor's argument size sums */
   arg_sums[0] = 0;       /* will hold all but the last argument's size sum */
   arg_sums[1] = 0;       /* will hold all argument's size sum */
   for (i = 0; i < op->num_children; i++) {
      GLuint arg_size = 0;

      if (!sizeof_argument(A, &arg_size, &op->children[i]))
         goto end;
      if (i > 0)
         arg_sums[0] = arg_sums[1];
      arg_sums[1] += arg_size;
   }

   /* check if there are too many arguments */
   if (arg_sums[0] >= size) {
      /* TODO: info log: too many arguments in constructor list */
      goto end;
   }

   /* check if there are too few arguments */
   if (arg_sums[1] < size) {
      /* TODO: info log: too few arguments in constructor list */
      goto end;
   }

   /* traverse the children that form the constructor expression */
   for (i = op->num_children; i > 0; i--) {
      GLuint garbage_size;

      /* the last argument may be too big - calculate the unnecessary
       * data size
       */
      if (i == op->num_children)
         garbage_size = arg_sums[1] - size;
      else
         garbage_size = 0;

      if (!constructor_aggregate
          (A, &flat, &op->children[i - 1], garbage_size))
         goto end;
   }

   result = GL_TRUE;
 end:
   slang_storage_aggregate_destruct(&flat);
 end2:
   slang_storage_aggregate_destruct(&agg);
 end1:
   slang_assembly_typeinfo_destruct(&ti);
   return result;
}



GLboolean
_slang_assemble_constructor_from_swizzle(slang_assemble_ctx * A,
                                         const slang_swizzle * swz,
                                         const slang_type_specifier * spec,
                                         const slang_type_specifier * master_spec)
{
   const GLuint master_rows = _slang_type_dim(master_spec->type);
   GLuint i;

   for (i = 0; i < master_rows; i++) {
      switch (_slang_type_base(master_spec->type)) {
      case slang_spec_bool:
         if (!slang_assembly_file_push_label2(A->file, slang_asm_bool_copy,
                                              (master_rows - i) * 4, i * 4))
            return GL_FALSE;
         break;
      case slang_spec_int:
         if (!slang_assembly_file_push_label2(A->file, slang_asm_int_copy,
                                              (master_rows - i) * 4, i * 4))
            return GL_FALSE;
         break;
      case slang_spec_float:
         if (!slang_assembly_file_push_label2(A->file, slang_asm_float_copy,
                                              (master_rows - i) * 4, i * 4))
            return GL_FALSE;
         break;
      default:
         break;
      }
   }

   if (!slang_assembly_file_push_label(A->file, slang_asm_local_free, 4))
      return GL_FALSE;

   for (i = swz->num_components; i > 0; i--) {
      const GLuint n = i - 1;

      if (!slang_assembly_file_push_label2(A->file, slang_asm_local_addr,
                                           A->local.swizzle_tmp, 16))
         return GL_FALSE;
      if (!slang_assembly_file_push_label(A->file, slang_asm_addr_push,
                                          swz->swizzle[n] * 4))
         return GL_FALSE;
      if (!slang_assembly_file_push(A->file, slang_asm_addr_add))
         return GL_FALSE;

      switch (_slang_type_base(master_spec->type)) {
      case slang_spec_bool:
         if (!slang_assembly_file_push(A->file, slang_asm_bool_deref))
            return GL_FALSE;
         break;
      case slang_spec_int:
         if (!slang_assembly_file_push(A->file, slang_asm_int_deref))
            return GL_FALSE;
         break;
      case slang_spec_float:
         if (!slang_assembly_file_push(A->file, slang_asm_float_deref))
            return GL_FALSE;
         break;
      default:
         break;
      }
   }

   return GL_TRUE;
}
