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
 * \file slang_assemble_assignment.c
 * slang assignment expressions assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_assemble.h"
#include "slang_storage.h"

/*
 * _slang_assemble_assignment()
 *
 * Copies values on the stack (<component 0> to <component N-1>) to a memory
 * location pointed by <addr of variable>.
 *
 * in:
 *      +------------------+
 *      | addr of variable |
 *      +------------------+
 *      | component N-1    |
 *      | ...              |
 *      | component 0      |
 *      +------------------+
 *
 * out:
 *      +------------------+
 *      | addr of variable |
 *      +------------------+
 */



static GLboolean
assign_basic(slang_assemble_ctx * A, slang_storage_type type, GLuint * index,
             GLuint size)
{
   GLuint dst_offset, dst_addr_loc;
   slang_assembly_type ty;

   /* Calculate the offset within destination variable to write. */
   if (A->swz.num_components != 0)
      dst_offset = A->swz.swizzle[*index / 4] * 4;
   else
      dst_offset = *index;

   switch (type) {
   case slang_stor_bool:
      ty = slang_asm_bool_copy;
      break;
   case slang_stor_int:
      ty = slang_asm_int_copy;
      break;
   case slang_stor_float:
      ty = slang_asm_float_copy;
      break;
#if defined(USE_X86_ASM) || defined(SLANG_X86)
   case slang_stor_vec4:
      ty = slang_asm_vec4_copy;
      break;
#endif
   default:
      _mesa_problem(NULL, "Unexpected arr->type in assign_basic");
      ty = slang_asm_none;
   }

   /* Calculate the distance from top of the stack to the destination
    * address. As the copy operation progresses, components of the
    * source are being successively popped off the stack by the amount
    * of *index increase step.
    */
   dst_addr_loc = size - *index;

   if (!slang_assembly_file_push_label2
       (A->file, ty, dst_addr_loc, dst_offset))
      return GL_FALSE;
   *index += _slang_sizeof_type(type);

   return GL_TRUE;
}


static GLboolean
assign_aggregate(slang_assemble_ctx * A, const slang_storage_aggregate * agg,
                 GLuint * index, GLuint size)
{
   GLuint i;

   for (i = 0; i < agg->count; i++) {
      const slang_storage_array *arr = &agg->arrays[i];
      GLuint j;

      for (j = 0; j < arr->length; j++) {
         if (arr->type == slang_stor_aggregate) {
            if (!assign_aggregate(A, arr->aggregate, index, size))
               return GL_FALSE;
         }
         else {
            /* When the destination is swizzled, we are forced to do
             * float_copy, even if vec4 extension is enabled with
             * vec4_copy operation.
             */
            if (A->swz.num_components != 0 && arr->type == slang_stor_vec4) {
               if (!assign_basic(A, slang_stor_float, index, size))
                  return GL_FALSE;
               if (!assign_basic(A, slang_stor_float, index, size))
                  return GL_FALSE;
               if (!assign_basic(A, slang_stor_float, index, size))
                  return GL_FALSE;
               if (!assign_basic(A, slang_stor_float, index, size))
                  return GL_FALSE;
            }
            else {
               if (!assign_basic(A, arr->type, index, size))
                  return GL_FALSE;
            }
         }
      }
   }

   return GL_TRUE;
}


GLboolean
_slang_assemble_assignment(slang_assemble_ctx * A, const slang_operation * op)
{
   slang_assembly_typeinfo ti;
   GLboolean result = GL_FALSE;
   slang_storage_aggregate agg;
   GLuint index, size;

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

   index = 0;
   size = _slang_sizeof_aggregate(&agg);
   result = assign_aggregate(A, &agg, &index, size);

 end1:
   slang_storage_aggregate_destruct(&agg);
 end:
   slang_assembly_typeinfo_destruct(&ti);
   return result;
}


/**
 * Performs unary (pre ++ and --) or binary (=, +=, -=, *=, /=)
 * assignment on the operation's children.
 */
GLboolean
_slang_assemble_assign(slang_assemble_ctx * A, slang_operation * op,
                       const char *oper, slang_ref_type ref)
{
   slang_swizzle swz;

   if (ref == slang_ref_forbid) {
      if (!slang_assembly_file_push_label2
          (A->file, slang_asm_local_addr, A->local.addr_tmp, 4))
         return GL_FALSE;
   }

   if (slang_string_compare("=", oper) == 0) {
      if (!_slang_assemble_operation(A, &op->children[0], slang_ref_force))
         return GL_FALSE;
      swz = A->swz;
      if (!_slang_assemble_operation(A, &op->children[1], slang_ref_forbid))
         return GL_FALSE;
      A->swz = swz;
      if (!_slang_assemble_assignment(A, op->children))
         return GL_FALSE;
   }
   else {
      if (!_slang_assemble_function_call_name
          (A, oper, op->children, op->num_children, GL_TRUE))
         return GL_FALSE;
   }

   if (ref == slang_ref_forbid) {
      if (!slang_assembly_file_push(A->file, slang_asm_addr_copy))
         return GL_FALSE;
      if (!slang_assembly_file_push_label(A->file, slang_asm_local_free, 4))
         return GL_FALSE;
      if (!_slang_dereference(A, op->children))
         return GL_FALSE;
   }

   return GL_TRUE;
}
