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
 * \file slang_assemble.c
 * slang intermediate code assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_assemble.h"
#include "slang_compile.h"
#include "slang_storage.h"

/* slang_assembly */

static GLboolean
slang_assembly_construct(slang_assembly * assem)
{
   assem->type = slang_asm_none;
   return GL_TRUE;
}

static GLvoid
slang_assembly_destruct(slang_assembly * assem)
{
}

/*
 * slang_assembly_file
 */

GLvoid
_slang_assembly_file_ctr(slang_assembly_file * self)
{
   self->code = NULL;
   self->count = 0;
   self->capacity = 0;
}

GLvoid
slang_assembly_file_destruct(slang_assembly_file * file)
{
   GLuint i;

   for (i = 0; i < file->count; i++)
      slang_assembly_destruct(&file->code[i]);
   slang_alloc_free(file->code);
}

static GLboolean
push_new(slang_assembly_file * file)
{
   if (file->count == file->capacity) {
      GLuint n;

      if (file->capacity == 0)
         n = 256;
      else
         n = file->capacity * 2;
      file->code = (slang_assembly *)
         slang_alloc_realloc(file->code,
                             file->capacity * sizeof(slang_assembly),
                             n * sizeof(slang_assembly));
      if (file->code == NULL)
         return GL_FALSE;
      file->capacity = n;
   }
   if (!slang_assembly_construct(&file->code[file->count]))
      return GL_FALSE;
   file->count++;
   return GL_TRUE;
}

static GLboolean
push_gen(slang_assembly_file * file, slang_assembly_type type,
         GLfloat literal, GLuint label, GLuint size)
{
   slang_assembly *assem;

   if (!push_new(file))
      return GL_FALSE;
   assem = &file->code[file->count - 1];
   assem->type = type;
   assem->literal = literal;
   assem->param[0] = label;
   assem->param[1] = size;
   return GL_TRUE;
}

GLboolean
slang_assembly_file_push(slang_assembly_file * file, slang_assembly_type type)
{
   return push_gen(file, type, (GLfloat) 0, 0, 0);
}

GLboolean
slang_assembly_file_push_label(slang_assembly_file * file,
                               slang_assembly_type type, GLuint label)
{
   return push_gen(file, type, (GLfloat) 0, label, 0);
}

GLboolean
slang_assembly_file_push_label2(slang_assembly_file * file,
                                slang_assembly_type type, GLuint label1,
                                GLuint label2)
{
   return push_gen(file, type, (GLfloat) 0, label1, label2);
}

GLboolean
slang_assembly_file_push_literal(slang_assembly_file * file,
                                 slang_assembly_type type, GLfloat literal)
{
   return push_gen(file, type, literal, 0, 0);
}

#define PUSH slang_assembly_file_push
#define PLAB slang_assembly_file_push_label
#define PLAB2 slang_assembly_file_push_label2
#define PLIT slang_assembly_file_push_literal

/* slang_assembly_file_restore_point */

GLboolean
slang_assembly_file_restore_point_save(slang_assembly_file * file,
                                       slang_assembly_file_restore_point *
                                       point)
{
   point->count = file->count;
   return GL_TRUE;
}

GLboolean
slang_assembly_file_restore_point_load(slang_assembly_file * file,
                                       slang_assembly_file_restore_point *
                                       point)
{
   GLuint i;

   for (i = point->count; i < file->count; i++)
      slang_assembly_destruct(&file->code[i]);
   file->count = point->count;
   return GL_TRUE;
}

/* utility functions */

static GLboolean
sizeof_variable(slang_assemble_ctx * A, slang_type_specifier * spec,
                slang_type_qualifier qual, GLuint array_len, GLuint * size)
{
   slang_storage_aggregate agg;

   /* calculate the size of the variable's aggregate */
   if (!slang_storage_aggregate_construct(&agg))
      return GL_FALSE;
   if (!_slang_aggregate_variable
       (&agg, spec, array_len, A->space.funcs, A->space.structs,
        A->space.vars, A->mach, A->file, A->atoms)) {
      slang_storage_aggregate_destruct(&agg);
      return GL_FALSE;
   }
   *size += _slang_sizeof_aggregate(&agg);
   slang_storage_aggregate_destruct(&agg);

   /* for reference variables consider the additional address overhead */
   if (qual == slang_qual_out || qual == slang_qual_inout)
      *size += 4;

   return GL_TRUE;
}

static GLboolean
sizeof_variable2(slang_assemble_ctx * A, slang_variable * var, GLuint * size)
{
   var->address = *size;
   if (var->type.qualifier == slang_qual_out
       || var->type.qualifier == slang_qual_inout)
      var->address += 4;
   return sizeof_variable(A, &var->type.specifier, var->type.qualifier,
                          var->array_len, size);
}

static GLboolean
sizeof_variables(slang_assemble_ctx * A, slang_variable_scope * vars,
                 GLuint start, GLuint stop, GLuint * size)
{
   GLuint i;

   for (i = start; i < stop; i++)
      if (!sizeof_variable2(A, &vars->variables[i], size))
         return GL_FALSE;
   return GL_TRUE;
}

static GLboolean
collect_locals(slang_assemble_ctx * A, slang_operation * op, GLuint * size)
{
   GLuint i;

   if (!sizeof_variables(A, op->locals, 0, op->locals->num_variables, size))
      return GL_FALSE;
   for (i = 0; i < op->num_children; i++)
      if (!collect_locals(A, &op->children[i], size))
         return GL_FALSE;
   return GL_TRUE;
}

/* _slang_locate_function() */

slang_function *
_slang_locate_function(const slang_function_scope * funcs, slang_atom a_name,
                       const slang_operation * params, GLuint num_params,
                       const slang_assembly_name_space * space,
                       slang_atom_pool * atoms)
{
   GLuint i;

   for (i = 0; i < funcs->num_functions; i++) {
      GLuint j;
      slang_function *f = &funcs->functions[i];

      if (a_name != f->header.a_name)
         continue;
      if (f->param_count != num_params)
         continue;
      for (j = 0; j < num_params; j++) {
         slang_assembly_typeinfo ti;

         if (!slang_assembly_typeinfo_construct(&ti))
            return NULL;
         if (!_slang_typeof_operation_(&params[j], space, &ti, atoms)) {
            slang_assembly_typeinfo_destruct(&ti);
            return NULL;
         }
         if (!slang_type_specifier_equal
             (&ti.spec, &f->parameters->variables[j].type.specifier)) {
            slang_assembly_typeinfo_destruct(&ti);
            break;
         }
         slang_assembly_typeinfo_destruct(&ti);

         /* "out" and "inout" formal parameter requires the actual parameter to be l-value */
         if (!ti.can_be_referenced &&
             (f->parameters->variables[j].type.qualifier == slang_qual_out ||
              f->parameters->variables[j].type.qualifier == slang_qual_inout))
            break;
      }
      if (j == num_params)
         return f;
   }
   if (funcs->outer_scope != NULL)
      return _slang_locate_function(funcs->outer_scope, a_name, params,
                                    num_params, space, atoms);
   return NULL;
}

/* _slang_assemble_function() */

GLboolean
_slang_assemble_function(slang_assemble_ctx * A, slang_function * fun)
{
   GLuint param_size, local_size;
   GLuint skip, cleanup;

   fun->address = A->file->count;

   if (fun->body == NULL) {
      /* jump to the actual function body - we do not know it, so add
       * the instruction to fixup table
       */
      if (!slang_fixup_save(&fun->fixups, fun->address))
         return GL_FALSE;
      if (!PUSH(A->file, slang_asm_jump))
         return GL_FALSE;
      return GL_TRUE;
   }
   else {
      /* resolve all fixup table entries and delete it */
      GLuint i;
      for (i = 0; i < fun->fixups.count; i++)
         A->file->code[fun->fixups.table[i]].param[0] = fun->address;
      slang_fixup_table_free(&fun->fixups);
   }

   /* At this point traverse function formal parameters and code to calculate
    * total memory size to be allocated on the stack.
    * During this process the variables will be assigned local addresses to
    * reference them in the code.
    * No storage optimizations are performed so exclusive scopes are not
    * detected and shared.
    */

   /* calculate return value size */
   param_size = 0;
   if (fun->header.type.specifier.type != slang_spec_void)
      if (!sizeof_variable
          (A, &fun->header.type.specifier, slang_qual_none, 0, &param_size))
         return GL_FALSE;
   A->local.ret_size = param_size;

   /* calculate formal parameter list size */
   if (!sizeof_variables
       (A, fun->parameters, 0, fun->param_count, &param_size))
      return GL_FALSE;

   /* calculate local variables size - take into account the four-byte
    * return address and temporaries for various tasks (4 for addr and
    * 16 for swizzle temporaries).  these include variables from the
    * formal parameter scope and from the code
    */
   A->local.addr_tmp = param_size + 4;
   A->local.swizzle_tmp = param_size + 4 + 4;
   local_size = param_size + 4 + 4 + 16;
   if (!sizeof_variables
       (A, fun->parameters, fun->param_count, fun->parameters->num_variables,
        &local_size))
      return GL_FALSE;
   if (!collect_locals(A, fun->body, &local_size))
      return GL_FALSE;

   /* allocate local variable storage */
   if (!PLAB(A->file, slang_asm_local_alloc, local_size - param_size - 4))
      return GL_FALSE;

   /* mark a new frame for function variable storage */
   if (!PLAB(A->file, slang_asm_enter, local_size))
      return GL_FALSE;

   /* jump directly to the actual code */
   skip = A->file->count;
   if (!push_new(A->file))
      return GL_FALSE;
   A->file->code[skip].type = slang_asm_jump;

   /* all "return" statements will be directed here */
   A->flow.function_end = A->file->count;
   cleanup = A->file->count;
   if (!push_new(A->file))
      return GL_FALSE;
   A->file->code[cleanup].type = slang_asm_jump;

   /* execute the function body */
   A->file->code[skip].param[0] = A->file->count;
   if (!_slang_assemble_operation
       (A, fun->body, /*slang_ref_freelance */ slang_ref_forbid))
      return GL_FALSE;

   /* this is the end of the function - restore the old function frame */
   A->file->code[cleanup].param[0] = A->file->count;
   if (!PUSH(A->file, slang_asm_leave))
      return GL_FALSE;

   /* free local variable storage */
   if (!PLAB(A->file, slang_asm_local_free, local_size - param_size - 4))
      return GL_FALSE;

   /* return from the function */
   if (!PUSH(A->file, slang_asm_return))
      return GL_FALSE;

   return GL_TRUE;
}

GLboolean
_slang_cleanup_stack(slang_assemble_ctx * A, slang_operation * op)
{
   slang_assembly_typeinfo ti;
   GLuint size = 0;

   /* get type info of the operation and calculate its size */
   if (!slang_assembly_typeinfo_construct(&ti))
      return GL_FALSE;
   if (!_slang_typeof_operation(A, op, &ti)) {
      slang_assembly_typeinfo_destruct(&ti);
      return GL_FALSE;
   }
   if (ti.spec.type != slang_spec_void) {
      if (A->ref == slang_ref_force) {
         size = 4;
      }
      else if (!sizeof_variable(A, &ti.spec, slang_qual_none, 0, &size)) {
         slang_assembly_typeinfo_destruct(&ti);
         return GL_FALSE;
      }
   }
   slang_assembly_typeinfo_destruct(&ti);

   /* if nonzero, free it from the stack */
   if (size != 0) {
      if (!PLAB(A->file, slang_asm_local_free, size))
         return GL_FALSE;
   }

   return GL_TRUE;
}

/* _slang_assemble_operation() */

static GLboolean
dereference_basic(slang_assemble_ctx * A, slang_storage_type type,
                  GLuint * size, slang_swizzle * swz, GLboolean is_swizzled)
{
   GLuint src_offset;
   slang_assembly_type ty;

   *size -= _slang_sizeof_type(type);

   /* If swizzling is taking place, we are forced to use scalar
    * operations, even if we have vec4 instructions enabled (this
    * should be actually done with special vec4 shuffle instructions).
    * Adjust the size and calculate the offset within source variable
    * to read.
    */
   if (is_swizzled)
      src_offset = swz->swizzle[*size / 4] * 4;
   else
      src_offset = *size;

   /* dereference data slot of a basic type */
   if (!PLAB2(A->file, slang_asm_local_addr, A->local.addr_tmp, 4))
      return GL_FALSE;
   if (!PUSH(A->file, slang_asm_addr_deref))
      return GL_FALSE;
   if (src_offset != 0) {
      if (!PLAB(A->file, slang_asm_addr_push, src_offset))
         return GL_FALSE;
      if (!PUSH(A->file, slang_asm_addr_add))
         return GL_FALSE;
   }

   switch (type) {
   case slang_stor_bool:
      ty = slang_asm_bool_deref;
      break;
   case slang_stor_int:
      ty = slang_asm_int_deref;
      break;
   case slang_stor_float:
      ty = slang_asm_float_deref;
      break;
#if defined(USE_X86_ASM) || defined(SLANG_X86)
   case slang_stor_vec4:
      ty = slang_asm_vec4_deref;
      break;
#endif
   default:
      _mesa_problem(NULL, "Unexpected arr->type in dereference_basic");
      ty = slang_asm_none;
   }

   return PUSH(A->file, ty);
}

static GLboolean
dereference_aggregate(slang_assemble_ctx * A,
                      const slang_storage_aggregate * agg, GLuint * size,
                      slang_swizzle * swz, GLboolean is_swizzled)
{
   GLuint i;

   for (i = agg->count; i > 0; i--) {
      const slang_storage_array *arr = &agg->arrays[i - 1];
      GLuint j;

      for (j = arr->length; j > 0; j--) {
         if (arr->type == slang_stor_aggregate) {
            if (!dereference_aggregate
                (A, arr->aggregate, size, swz, is_swizzled))
               return GL_FALSE;
         }
         else {
            if (is_swizzled && arr->type == slang_stor_vec4) {
               if (!dereference_basic
                   (A, slang_stor_float, size, swz, is_swizzled))
                  return GL_FALSE;
               if (!dereference_basic
                   (A, slang_stor_float, size, swz, is_swizzled))
                  return GL_FALSE;
               if (!dereference_basic
                   (A, slang_stor_float, size, swz, is_swizzled))
                  return GL_FALSE;
               if (!dereference_basic
                   (A, slang_stor_float, size, swz, is_swizzled))
                  return GL_FALSE;
            }
            else {
               if (!dereference_basic(A, arr->type, size, swz, is_swizzled))
                  return GL_FALSE;
            }
         }
      }
   }

   return GL_TRUE;
}

GLboolean
_slang_dereference(slang_assemble_ctx * A, slang_operation * op)
{
   slang_assembly_typeinfo ti;
   GLboolean result = GL_FALSE;
   slang_storage_aggregate agg;
   GLuint size;

   /* get type information of the given operation */
   if (!slang_assembly_typeinfo_construct(&ti))
      return GL_FALSE;
   if (!_slang_typeof_operation(A, op, &ti))
      goto end1;

   /* construct aggregate from the type info */
   if (!slang_storage_aggregate_construct(&agg))
      goto end1;
   if (!_slang_aggregate_variable
       (&agg, &ti.spec, ti.array_len, A->space.funcs, A->space.structs,
        A->space.vars, A->mach, A->file, A->atoms))
      goto end;

   /* dereference the resulting aggregate */
   size = _slang_sizeof_aggregate(&agg);
   result = dereference_aggregate(A, &agg, &size, &ti.swz, ti.is_swizzled);

 end:
   slang_storage_aggregate_destruct(&agg);
 end1:
   slang_assembly_typeinfo_destruct(&ti);
   return result;
}

GLboolean
_slang_assemble_function_call(slang_assemble_ctx * A, slang_function * fun,
                              slang_operation * params, GLuint param_count,
                              GLboolean assignment)
{
   GLuint i;
   slang_swizzle p_swz[64];
   slang_ref_type p_ref[64];

   /* TODO: fix this, allocate dynamically */
   if (param_count > 64)
      return GL_FALSE;

   /* make room for the return value, if any */
   if (fun->header.type.specifier.type != slang_spec_void) {
      GLuint ret_size = 0;

      if (!sizeof_variable
          (A, &fun->header.type.specifier, slang_qual_none, 0, &ret_size))
         return GL_FALSE;
      if (!PLAB(A->file, slang_asm_local_alloc, ret_size))
         return GL_FALSE;
   }

   /* push the actual parameters on the stack */
   for (i = 0; i < param_count; i++) {
      if (fun->parameters->variables[i].type.qualifier == slang_qual_inout ||
          fun->parameters->variables[i].type.qualifier == slang_qual_out) {
         if (!PLAB2(A->file, slang_asm_local_addr, A->local.addr_tmp, 4))
            return GL_FALSE;
         /* TODO: optimize the "out" parameter case */
         if (!_slang_assemble_operation(A, &params[i], slang_ref_force))
            return GL_FALSE;
         p_swz[i] = A->swz;
         p_ref[i] = A->ref;
         if (!PUSH(A->file, slang_asm_addr_copy))
            return GL_FALSE;
         if (!PUSH(A->file, slang_asm_addr_deref))
            return GL_FALSE;
         if (i == 0 && assignment) {
            /* duplicate the resulting address */
            if (!PLAB2(A->file, slang_asm_local_addr, A->local.addr_tmp, 4))
               return GL_FALSE;
            if (!PUSH(A->file, slang_asm_addr_deref))
               return GL_FALSE;
         }
         if (!_slang_dereference(A, &params[i]))
            return GL_FALSE;
      }
      else {
         if (!_slang_assemble_operation(A, &params[i], slang_ref_forbid))
            return GL_FALSE;
         p_swz[i] = A->swz;
         p_ref[i] = A->ref;
      }
   }

   /* call the function */
   if (!PLAB(A->file, slang_asm_call, fun->address))
      return GL_FALSE;

   /* pop the parameters from the stack */
   for (i = param_count; i > 0; i--) {
      GLuint j = i - 1;

      A->swz = p_swz[j];
      A->ref = p_ref[j];
      if (fun->parameters->variables[j].type.qualifier == slang_qual_inout ||
          fun->parameters->variables[j].type.qualifier == slang_qual_out) {
         /* for output parameter copy the contents of the formal parameter
          * back to the original actual parameter
          */
         if (!_slang_assemble_assignment(A, &params[j]))
            return GL_FALSE;
         /* pop the actual parameter's address */
         if (!PLAB(A->file, slang_asm_local_free, 4))
            return GL_FALSE;
      }
      else {
         /* pop the value of the parameter */
         if (!_slang_cleanup_stack(A, &params[j]))
            return GL_FALSE;
      }
   }

   return GL_TRUE;
}

GLboolean
_slang_assemble_function_call_name(slang_assemble_ctx * A, const char *name,
                                   slang_operation * params,
                                   GLuint param_count, GLboolean assignment)
{
   slang_atom atom;
   slang_function *fun;

   atom = slang_atom_pool_atom(A->atoms, name);
   if (atom == SLANG_ATOM_NULL)
      return GL_FALSE;
   fun =
      _slang_locate_function(A->space.funcs, atom, params, param_count,
                             &A->space, A->atoms);
   if (fun == NULL)
      return GL_FALSE;
   return _slang_assemble_function_call(A, fun, params, param_count,
                                        assignment);
}

static GLboolean
assemble_function_call_name_dummyint(slang_assemble_ctx * A, const char *name,
                                     slang_operation * params)
{
   slang_operation p[2];
   GLboolean result;

   p[0] = params[0];
   if (!slang_operation_construct(&p[1]))
      return GL_FALSE;
   p[1].type = slang_oper_literal_int;
   result = _slang_assemble_function_call_name(A, name, p, 2, GL_FALSE);
   slang_operation_destruct(&p[1]);
   return result;
}

static const struct
{
   const char *name;
   slang_assembly_type code1, code2;
} inst[] = {
   /* core */
   {"float_add", slang_asm_float_add, slang_asm_float_copy},
   {"float_multiply", slang_asm_float_multiply, slang_asm_float_copy},
   {"float_divide", slang_asm_float_divide, slang_asm_float_copy},
   {"float_negate", slang_asm_float_negate, slang_asm_float_copy},
   {"float_less", slang_asm_float_less, slang_asm_bool_copy},
   {"float_equal", slang_asm_float_equal_exp, slang_asm_bool_copy},
   {"float_to_int", slang_asm_float_to_int, slang_asm_int_copy},
   {"float_sine", slang_asm_float_sine, slang_asm_float_copy},
   {"float_arcsine", slang_asm_float_arcsine, slang_asm_float_copy},
   {"float_arctan", slang_asm_float_arctan, slang_asm_float_copy},
   {"float_power", slang_asm_float_power, slang_asm_float_copy},
   {"float_log2", slang_asm_float_log2, slang_asm_float_copy},
   {"float_floor", slang_asm_float_floor, slang_asm_float_copy},
   {"float_ceil", slang_asm_float_ceil, slang_asm_float_copy},
   {"float_noise1", slang_asm_float_noise1, slang_asm_float_copy},
   {"float_noise2", slang_asm_float_noise2, slang_asm_float_copy},
   {"float_noise3", slang_asm_float_noise3, slang_asm_float_copy},
   {"float_noise4", slang_asm_float_noise4, slang_asm_float_copy},
   {"int_to_float", slang_asm_int_to_float, slang_asm_float_copy},
   {"vec4_tex1d", slang_asm_vec4_tex1d, slang_asm_none},
   {"vec4_tex2d", slang_asm_vec4_tex2d, slang_asm_none},
   {"vec4_tex3d", slang_asm_vec4_tex3d, slang_asm_none},
   {"vec4_texcube", slang_asm_vec4_texcube, slang_asm_none},
   {"vec4_shad1d", slang_asm_vec4_shad1d, slang_asm_none},
   {"vec4_shad2d", slang_asm_vec4_shad2d, slang_asm_none},
    /* GL_MESA_shader_debug */
   {"float_print", slang_asm_float_deref, slang_asm_float_print},
   {"int_print", slang_asm_int_deref, slang_asm_int_print},
   {"bool_print", slang_asm_bool_deref, slang_asm_bool_print},
   /* vec4 */
   {"float_to_vec4", slang_asm_float_to_vec4, slang_asm_none},
   {"vec4_add", slang_asm_vec4_add, slang_asm_none},
   {"vec4_subtract", slang_asm_vec4_subtract, slang_asm_none},
   {"vec4_multiply", slang_asm_vec4_multiply, slang_asm_none},
   {"vec4_divide", slang_asm_vec4_divide, slang_asm_none},
   {"vec4_negate", slang_asm_vec4_negate, slang_asm_none},
   {"vec4_dot", slang_asm_vec4_dot, slang_asm_none},
   {NULL, slang_asm_none, slang_asm_none}
};

static GLboolean
call_asm_instruction(slang_assemble_ctx * A, slang_atom a_name)
{
   const char *id;
   GLuint i;

   id = slang_atom_pool_id(A->atoms, a_name);

   for (i = 0; inst[i].name != NULL; i++)
      if (slang_string_compare(id, inst[i].name) == 0)
         break;
   if (inst[i].name == NULL)
      return GL_FALSE;

   if (!PLAB2(A->file, inst[i].code1, 4, 0))
      return GL_FALSE;
   if (inst[i].code2 != slang_asm_none)
      if (!PLAB2(A->file, inst[i].code2, 4, 0))
         return GL_FALSE;

   /* clean-up the stack from the remaining dst address */
   if (!PLAB(A->file, slang_asm_local_free, 4))
      return GL_FALSE;

   return GL_TRUE;
}

static GLboolean
equality_aggregate(slang_assemble_ctx * A,
                   const slang_storage_aggregate * agg, GLuint * index,
                   GLuint size, GLuint z_label)
{
   GLuint i;

   for (i = 0; i < agg->count; i++) {
      const slang_storage_array *arr = &agg->arrays[i];
      GLuint j;

      for (j = 0; j < arr->length; j++) {
         if (arr->type == slang_stor_aggregate) {
            if (!equality_aggregate(A, arr->aggregate, index, size, z_label))
               return GL_FALSE;
         }
         else {
#if defined(USE_X86_ASM) || defined(SLANG_X86)
            if (arr->type == slang_stor_vec4) {
               if (!PLAB2
                   (A->file, slang_asm_vec4_equal_int, size + *index, *index))
                  return GL_FALSE;
            }
            else
#endif
            if (!PLAB2
                   (A->file, slang_asm_float_equal_int, size + *index,
                       *index))
               return GL_FALSE;

            *index += _slang_sizeof_type(arr->type);
            if (!PLAB(A->file, slang_asm_jump_if_zero, z_label))
               return GL_FALSE;
         }
      }
   }

   return GL_TRUE;
}

static GLboolean
equality(slang_assemble_ctx * A, slang_operation * op, GLboolean equal)
{
   slang_assembly_typeinfo ti;
   GLboolean result = GL_FALSE;
   slang_storage_aggregate agg;
   GLuint index, size;
   GLuint skip_jump, true_label, true_jump, false_label, false_jump;

   /* get type of operation */
   if (!slang_assembly_typeinfo_construct(&ti))
      return GL_FALSE;
   if (!_slang_typeof_operation(A, op, &ti))
      goto end1;

   /* convert it to an aggregate */
   if (!slang_storage_aggregate_construct(&agg))
      goto end1;
   if (!_slang_aggregate_variable
       (&agg, &ti.spec, 0, A->space.funcs, A->space.structs, A->space.vars,
        A->mach, A->file, A->atoms))
      goto end;

   /* compute the size of the agregate - there are two such aggregates on the stack */
   size = _slang_sizeof_aggregate(&agg);

   /* jump to the actual data-comparison code */
   skip_jump = A->file->count;
   if (!PUSH(A->file, slang_asm_jump))
      goto end;

   /* pop off the stack the compared data and push 1 */
   true_label = A->file->count;
   if (!PLAB(A->file, slang_asm_local_free, size * 2))
      goto end;
   if (!PLIT(A->file, slang_asm_bool_push, (GLfloat) 1))
      goto end;
   true_jump = A->file->count;
   if (!PUSH(A->file, slang_asm_jump))
      goto end;

   false_label = A->file->count;
   if (!PLAB(A->file, slang_asm_local_free, size * 2))
      goto end;
   if (!PLIT(A->file, slang_asm_bool_push, (GLfloat) 0))
      goto end;
   false_jump = A->file->count;
   if (!PUSH(A->file, slang_asm_jump))
      goto end;

   A->file->code[skip_jump].param[0] = A->file->count;

   /* compare the data on stack, it will eventually jump either to true or false label */
   index = 0;
   if (!equality_aggregate
       (A, &agg, &index, size, equal ? false_label : true_label))
      goto end;
   if (!PLAB(A->file, slang_asm_jump, equal ? true_label : false_label))
      goto end;

   A->file->code[true_jump].param[0] = A->file->count;
   A->file->code[false_jump].param[0] = A->file->count;

   result = GL_TRUE;
 end:
   slang_storage_aggregate_destruct(&agg);
 end1:
   slang_assembly_typeinfo_destruct(&ti);
   return result;
}

static GLboolean
handle_subscript(slang_assemble_ctx * A, slang_assembly_typeinfo * tie,
                 slang_assembly_typeinfo * tia, slang_operation * op,
                 slang_ref_type ref)
{
   GLuint asize = 0, esize = 0;

   /* get type info of the master expression (matrix, vector or an array */
   if (!_slang_typeof_operation(A, &op->children[0], tia))
      return GL_FALSE;
   if (!sizeof_variable
       (A, &tia->spec, slang_qual_none, tia->array_len, &asize))
      return GL_FALSE;

   /* get type info of the result (matrix column, vector row or array element) */
   if (!_slang_typeof_operation(A, op, tie))
      return GL_FALSE;
   if (!sizeof_variable(A, &tie->spec, slang_qual_none, 0, &esize))
      return GL_FALSE;

   /* assemble the master expression */
   if (!_slang_assemble_operation(A, &op->children[0], ref))
      return GL_FALSE;

   /* when indexing an l-value swizzle, push the swizzle_tmp */
   if (ref == slang_ref_force && tia->is_swizzled)
      if (!PLAB2(A->file, slang_asm_local_addr, A->local.swizzle_tmp, 16))
         return GL_FALSE;

   /* assemble the subscript expression */
   if (!_slang_assemble_operation(A, &op->children[1], slang_ref_forbid))
      return GL_FALSE;

   if (ref == slang_ref_force && tia->is_swizzled) {
      GLuint i;

      /* copy the swizzle indexes to the swizzle_tmp */
      for (i = 0; i < tia->swz.num_components; i++) {
         if (!PLAB2(A->file, slang_asm_local_addr, A->local.swizzle_tmp, 16))
            return GL_FALSE;
         if (!PLAB(A->file, slang_asm_addr_push, i * 4))
            return GL_FALSE;
         if (!PUSH(A->file, slang_asm_addr_add))
            return GL_FALSE;
         if (!PLAB(A->file, slang_asm_addr_push, tia->swz.swizzle[i]))
            return GL_FALSE;
         if (!PUSH(A->file, slang_asm_addr_copy))
            return GL_FALSE;
         if (!PLAB(A->file, slang_asm_local_free, 4))
            return GL_FALSE;
      }

      /* offset the pushed swizzle_tmp address and dereference it */
      if (!PUSH(A->file, slang_asm_int_to_addr))
         return GL_FALSE;
      if (!PLAB(A->file, slang_asm_addr_push, 4))
         return GL_FALSE;
      if (!PUSH(A->file, slang_asm_addr_multiply))
         return GL_FALSE;
      if (!PUSH(A->file, slang_asm_addr_add))
         return GL_FALSE;
      if (!PUSH(A->file, slang_asm_addr_deref))
         return GL_FALSE;
   }
   else {
      /* convert the integer subscript to a relative address */
      if (!PUSH(A->file, slang_asm_int_to_addr))
         return GL_FALSE;
   }

   if (!PLAB(A->file, slang_asm_addr_push, esize))
      return GL_FALSE;
   if (!PUSH(A->file, slang_asm_addr_multiply))
      return GL_FALSE;

   if (ref == slang_ref_force) {
      /* offset the base address with the relative address */
      if (!PUSH(A->file, slang_asm_addr_add))
         return GL_FALSE;
   }
   else {
      GLuint i;

      /* move the selected element to the beginning of the master expression */
      for (i = 0; i < esize; i += 4)
         if (!PLAB2
             (A->file, slang_asm_float_move, asize - esize + i + 4, i + 4))
            return GL_FALSE;
      if (!PLAB(A->file, slang_asm_local_free, 4))
         return GL_FALSE;

      /* free the rest of the master expression */
      if (!PLAB(A->file, slang_asm_local_free, asize - esize))
         return GL_FALSE;
   }

   return GL_TRUE;
}

static GLboolean
handle_field(slang_assemble_ctx * A, slang_assembly_typeinfo * tia,
             slang_assembly_typeinfo * tib, slang_operation * op,
             slang_ref_type ref)
{
   /* get type info of the result (field or swizzle) */
   if (!_slang_typeof_operation(A, op, tia))
      return GL_FALSE;

   /* get type info of the master expression being accessed (struct or vector) */
   if (!_slang_typeof_operation(A, &op->children[0], tib))
      return GL_FALSE;

   /* if swizzling a vector in-place, the swizzle temporary is needed */
   if (ref == slang_ref_forbid && tia->is_swizzled)
      if (!PLAB2(A->file, slang_asm_local_addr, A->local.swizzle_tmp, 16))
         return GL_FALSE;

   /* assemble the master expression */
   if (!_slang_assemble_operation(A, &op->children[0], ref))
      return GL_FALSE;

   /* assemble the field expression */
   if (tia->is_swizzled) {
      if (ref == slang_ref_force) {
#if 0
         if (tia->swz.num_components == 1) {
            /* simple case - adjust the vector's address to point to
             * the selected component
             */
            if (!PLAB(file, slang_asm_addr_push, tia->swz.swizzle[0] * 4))
               return 0;
            if (!PUSH(file, slang_asm_addr_add))
               return 0;
         }
         else
#endif
         {
            /* two or more vector components are being referenced -
             * the so-called write mask must be passed to the upper
             * operations and applied when assigning value to this swizzle
             */
            A->swz = tia->swz;
         }
      }
      else {
         /* swizzle the vector in-place using the swizzle temporary */
         if (!_slang_assemble_constructor_from_swizzle
             (A, &tia->swz, &tia->spec, &tib->spec))
            return GL_FALSE;
      }
   }
   else {
      GLuint i, struct_size = 0, field_offset = 0, field_size = 0;

      /*
       * Calculate struct size, field offset and field size.
       */
      for (i = 0; i < tib->spec._struct->fields->num_variables; i++) {
         slang_variable *field;
         slang_storage_aggregate agg;
         GLuint size;

         field = &tib->spec._struct->fields->variables[i];
         if (!slang_storage_aggregate_construct(&agg))
            return GL_FALSE;
         if (!_slang_aggregate_variable
             (&agg, &field->type.specifier, field->array_len, A->space.funcs,
              A->space.structs, A->space.vars, A->mach, A->file, A->atoms)) {
            slang_storage_aggregate_destruct(&agg);
            return GL_FALSE;
         }
         size = _slang_sizeof_aggregate(&agg);
         slang_storage_aggregate_destruct(&agg);

         if (op->a_id == field->a_name) {
            field_size = size;
            field_offset = struct_size;
         }
         struct_size += size;
      }

      if (ref == slang_ref_force) {
         GLboolean shift;

         /*
          * OPTIMIZATION: If selecting first field, no address shifting
          * is needed.
          */
         shift = (field_offset != 0);

         if (shift) {
            if (!PLAB(A->file, slang_asm_addr_push, field_offset))
               return GL_FALSE;
            if (!PUSH(A->file, slang_asm_addr_add))
               return GL_FALSE;
         }
      }
      else {
         GLboolean relocate, shrink;
         GLuint free_b = 0;

         /*
          * OPTIMIZATION: If selecting last field, no relocation is needed.
          */
         relocate = (field_offset != (struct_size - field_size));

         /*
          * OPTIMIZATION: If field and struct sizes are equal, no partial
          * free is needed.
          */
         shrink = (field_size != struct_size);

         if (relocate) {
            GLuint i;

            /*
             * Move the selected element to the end of the master expression.
             * Do it in reverse order to avoid overwriting itself.
             */
            if (!PLAB(A->file, slang_asm_addr_push, field_offset))
               return GL_FALSE;
            for (i = field_size; i > 0; i -= 4)
               if (!PLAB2
                   (A->file, slang_asm_float_move,
                    struct_size - field_size + i, i))
                  return GL_FALSE;
            free_b += 4;
         }

         if (shrink) {
            /* free the rest of the master expression */
            free_b += struct_size - field_size;
         }

         if (free_b) {
            if (!PLAB(A->file, slang_asm_local_free, free_b))
               return GL_FALSE;
         }
      }
   }

   return GL_TRUE;
}

GLboolean
_slang_assemble_operation(slang_assemble_ctx * A, slang_operation * op,
                          slang_ref_type ref)
{
   /* set default results */
   A->ref = /*(ref == slang_ref_freelance) ? slang_ref_force : */ ref;
   A->swz.num_components = 0;

   switch (op->type) {
   case slang_oper_block_no_new_scope:
   case slang_oper_block_new_scope:
      {
         GLuint i;

         for (i = 0; i < op->num_children; i++) {
            if (!_slang_assemble_operation
                (A, &op->children[i],
                 slang_ref_forbid /*slang_ref_freelance */ ))
               return GL_FALSE;
            if (!_slang_cleanup_stack(A, &op->children[i]))
               return GL_FALSE;
         }
      }
      break;
   case slang_oper_variable_decl:
      {
         GLuint i;
         slang_operation assign;
         GLboolean result;

         /* Construct assignment expression placeholder. */
         if (!slang_operation_construct(&assign))
            return GL_FALSE;
         assign.type = slang_oper_assign;
         assign.children =
            (slang_operation *) slang_alloc_malloc(2 *
                                                   sizeof(slang_operation));
         if (assign.children == NULL) {
            slang_operation_destruct(&assign);
            return GL_FALSE;
         }
         for (assign.num_children = 0; assign.num_children < 2;
              assign.num_children++)
            if (!slang_operation_construct
                (&assign.children[assign.num_children])) {
               slang_operation_destruct(&assign);
               return GL_FALSE;
            }

         result = GL_TRUE;
         for (i = 0; i < op->num_children; i++) {
            slang_variable *var;

            var =
               _slang_locate_variable(op->children[i].locals,
                                      op->children[i].a_id, GL_TRUE);
            if (var == NULL) {
               result = GL_FALSE;
               break;
            }
            if (var->initializer == NULL)
               continue;

            if (!slang_operation_copy(&assign.children[0], &op->children[i])
                || !slang_operation_copy(&assign.children[1],
                                         var->initializer)
                || !_slang_assemble_assign(A, &assign, "=", slang_ref_forbid)
                || !_slang_cleanup_stack(A, &assign)) {
               result = GL_FALSE;
               break;
            }
         }
         slang_operation_destruct(&assign);
         if (!result)
            return GL_FALSE;
      }
      break;
   case slang_oper_asm:
      {
         GLuint i;
         if (!_slang_assemble_operation(A, &op->children[0], slang_ref_force))
            return GL_FALSE;
         for (i = 1; i < op->num_children; i++)
            if (!_slang_assemble_operation
                (A, &op->children[i], slang_ref_forbid))
               return GL_FALSE;
         if (!call_asm_instruction(A, op->a_id))
            return GL_FALSE;
      }
      break;
   case slang_oper_break:
      if (!PLAB(A->file, slang_asm_jump, A->flow.loop_end))
         return GL_FALSE;
      break;
   case slang_oper_continue:
      if (!PLAB(A->file, slang_asm_jump, A->flow.loop_start))
         return GL_FALSE;
      break;
   case slang_oper_discard:
      if (!PUSH(A->file, slang_asm_discard))
         return GL_FALSE;
      if (!PUSH(A->file, slang_asm_exit))
         return GL_FALSE;
      break;
   case slang_oper_return:
      if (A->local.ret_size != 0) {
         /* push the result's address */
         if (!PLAB2(A->file, slang_asm_local_addr, 0, A->local.ret_size))
            return GL_FALSE;
         if (!_slang_assemble_operation
             (A, &op->children[0], slang_ref_forbid))
            return GL_FALSE;

         A->swz.num_components = 0;
         /* assign the operation to the function result (it was reserved on the stack) */
         if (!_slang_assemble_assignment(A, op->children))
            return GL_FALSE;

         if (!PLAB(A->file, slang_asm_local_free, 4))
            return GL_FALSE;
      }
      if (!PLAB(A->file, slang_asm_jump, A->flow.function_end))
         return GL_FALSE;
      break;
   case slang_oper_expression:
      if (ref == slang_ref_force)
         return GL_FALSE;
      if (!_slang_assemble_operation(A, &op->children[0], ref))
         return GL_FALSE;
      break;
   case slang_oper_if:
      if (!_slang_assemble_if(A, op))
         return GL_FALSE;
      break;
   case slang_oper_while:
      if (!_slang_assemble_while(A, op))
         return GL_FALSE;
      break;
   case slang_oper_do:
      if (!_slang_assemble_do(A, op))
         return GL_FALSE;
      break;
   case slang_oper_for:
      if (!_slang_assemble_for(A, op))
         return GL_FALSE;
      break;
   case slang_oper_void:
      break;
   case slang_oper_literal_bool:
      if (ref == slang_ref_force)
         return GL_FALSE;
      if (!PLIT(A->file, slang_asm_bool_push, op->literal))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_literal_int:
      if (ref == slang_ref_force)
         return GL_FALSE;
      if (!PLIT(A->file, slang_asm_int_push, op->literal))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_literal_float:
      if (ref == slang_ref_force)
         return GL_FALSE;
      if (!PLIT(A->file, slang_asm_float_push, op->literal))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_identifier:
      {
         slang_variable *var;
         GLuint size;

         /* find the variable and calculate its size */
         var = _slang_locate_variable(op->locals, op->a_id, GL_TRUE);
         if (var == NULL)
            return GL_FALSE;
         size = 0;
         if (!sizeof_variable
             (A, &var->type.specifier, slang_qual_none, var->array_len,
              &size))
            return GL_FALSE;

         /* prepare stack for dereferencing */
         if (ref == slang_ref_forbid)
            if (!PLAB2(A->file, slang_asm_local_addr, A->local.addr_tmp, 4))
               return GL_FALSE;

         /* push the variable's address */
         if (var->global) {
            if (!PLAB(A->file, slang_asm_global_addr, var->address))
               return GL_FALSE;
         }
         else {
            if (!PLAB2(A->file, slang_asm_local_addr, var->address, size))
               return GL_FALSE;
         }

         /* perform the dereference */
         if (ref == slang_ref_forbid) {
            if (!PUSH(A->file, slang_asm_addr_copy))
               return GL_FALSE;
            if (!PLAB(A->file, slang_asm_local_free, 4))
               return GL_FALSE;
            if (!_slang_dereference(A, op))
               return GL_FALSE;
         }
      }
      break;
   case slang_oper_sequence:
      if (ref == slang_ref_force)
         return GL_FALSE;
      if (!_slang_assemble_operation(A, &op->children[0],
                                     slang_ref_forbid /*slang_ref_freelance */ ))
         return GL_FALSE;
      if (!_slang_cleanup_stack(A, &op->children[0]))
         return GL_FALSE;
      if (!_slang_assemble_operation(A, &op->children[1], slang_ref_forbid))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_assign:
      if (!_slang_assemble_assign(A, op, "=", ref))
         return GL_FALSE;
      break;
   case slang_oper_addassign:
      if (!_slang_assemble_assign(A, op, "+=", ref))
         return GL_FALSE;
      A->ref = ref;
      break;
   case slang_oper_subassign:
      if (!_slang_assemble_assign(A, op, "-=", ref))
         return GL_FALSE;
      A->ref = ref;
      break;
   case slang_oper_mulassign:
      if (!_slang_assemble_assign(A, op, "*=", ref))
         return GL_FALSE;
      A->ref = ref;
      break;
      /*case slang_oper_modassign: */
      /*case slang_oper_lshassign: */
      /*case slang_oper_rshassign: */
      /*case slang_oper_orassign: */
      /*case slang_oper_xorassign: */
      /*case slang_oper_andassign: */
   case slang_oper_divassign:
      if (!_slang_assemble_assign(A, op, "/=", ref))
         return GL_FALSE;
      A->ref = ref;
      break;
   case slang_oper_select:
      if (!_slang_assemble_select(A, op))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_logicalor:
      if (!_slang_assemble_logicalor(A, op))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_logicaland:
      if (!_slang_assemble_logicaland(A, op))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_logicalxor:
      if (!_slang_assemble_function_call_name(A, "^^", op->children, 2, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
      /*case slang_oper_bitor: */
      /*case slang_oper_bitxor: */
      /*case slang_oper_bitand: */
   case slang_oper_less:
      if (!_slang_assemble_function_call_name(A, "<", op->children, 2, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_greater:
      if (!_slang_assemble_function_call_name(A, ">", op->children, 2, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_lessequal:
      if (!_slang_assemble_function_call_name(A, "<=", op->children, 2, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_greaterequal:
      if (!_slang_assemble_function_call_name(A, ">=", op->children, 2, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
      /*case slang_oper_lshift: */
      /*case slang_oper_rshift: */
   case slang_oper_add:
      if (!_slang_assemble_function_call_name(A, "+", op->children, 2, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_subtract:
      if (!_slang_assemble_function_call_name(A, "-", op->children, 2, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_multiply:
      if (!_slang_assemble_function_call_name(A, "*", op->children, 2, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
      /*case slang_oper_modulus: */
   case slang_oper_divide:
      if (!_slang_assemble_function_call_name(A, "/", op->children, 2, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_equal:
      if (!_slang_assemble_operation(A, &op->children[0], slang_ref_forbid))
         return GL_FALSE;
      if (!_slang_assemble_operation(A, &op->children[1], slang_ref_forbid))
         return GL_FALSE;
      if (!equality(A, op->children, GL_TRUE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_notequal:
      if (!_slang_assemble_operation(A, &op->children[0], slang_ref_forbid))
         return GL_FALSE;
      if (!_slang_assemble_operation(A, &op->children[1], slang_ref_forbid))
         return GL_FALSE;
      if (!equality(A, op->children, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_preincrement:
      if (!_slang_assemble_assign(A, op, "++", ref))
         return GL_FALSE;
      A->ref = ref;
      break;
   case slang_oper_predecrement:
      if (!_slang_assemble_assign(A, op, "--", ref))
         return GL_FALSE;
      A->ref = ref;
      break;
   case slang_oper_plus:
      if (!_slang_dereference(A, op))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_minus:
      if (!_slang_assemble_function_call_name
          (A, "-", op->children, 1, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
      /*case slang_oper_complement: */
   case slang_oper_not:
      if (!_slang_assemble_function_call_name
          (A, "!", op->children, 1, GL_FALSE))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_subscript:
      {
         slang_assembly_typeinfo ti_arr, ti_elem;

         if (!slang_assembly_typeinfo_construct(&ti_arr))
            return GL_FALSE;
         if (!slang_assembly_typeinfo_construct(&ti_elem)) {
            slang_assembly_typeinfo_destruct(&ti_arr);
            return GL_FALSE;
         }
         if (!handle_subscript(A, &ti_elem, &ti_arr, op, ref)) {
            slang_assembly_typeinfo_destruct(&ti_arr);
            slang_assembly_typeinfo_destruct(&ti_elem);
            return GL_FALSE;
         }
         slang_assembly_typeinfo_destruct(&ti_arr);
         slang_assembly_typeinfo_destruct(&ti_elem);
      }
      break;
   case slang_oper_call:
      {
         slang_function *fun;

         fun =
            _slang_locate_function(A->space.funcs, op->a_id, op->children,
                                   op->num_children, &A->space, A->atoms);
         if (fun == NULL) {
            if (!_slang_assemble_constructor(A, op))
               return GL_FALSE;
         }
         else {
            if (!_slang_assemble_function_call
                (A, fun, op->children, op->num_children, GL_FALSE))
               return GL_FALSE;
         }
         A->ref = slang_ref_forbid;
      }
      break;
   case slang_oper_field:
      {
         slang_assembly_typeinfo ti_after, ti_before;

         if (!slang_assembly_typeinfo_construct(&ti_after))
            return GL_FALSE;
         if (!slang_assembly_typeinfo_construct(&ti_before)) {
            slang_assembly_typeinfo_destruct(&ti_after);
            return GL_FALSE;
         }
         if (!handle_field(A, &ti_after, &ti_before, op, ref)) {
            slang_assembly_typeinfo_destruct(&ti_after);
            slang_assembly_typeinfo_destruct(&ti_before);
            return GL_FALSE;
         }
         slang_assembly_typeinfo_destruct(&ti_after);
         slang_assembly_typeinfo_destruct(&ti_before);
      }
      break;
   case slang_oper_postincrement:
      if (!assemble_function_call_name_dummyint(A, "++", op->children))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   case slang_oper_postdecrement:
      if (!assemble_function_call_name_dummyint(A, "--", op->children))
         return GL_FALSE;
      A->ref = slang_ref_forbid;
      break;
   default:
      return GL_FALSE;
   }

   return GL_TRUE;
}
