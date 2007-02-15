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
 * \file slang_assemble_typeinfo.c
 * slang type info
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_assemble.h"
#include "slang_compile.h"

/*
 * slang_type_specifier
 */

GLvoid
slang_type_specifier_ctr(slang_type_specifier * self)
{
   self->type = slang_spec_void;
   self->_struct = NULL;
   self->_array = NULL;
}

GLvoid
slang_type_specifier_dtr(slang_type_specifier * self)
{
   if (self->_struct != NULL) {
      slang_struct_destruct(self->_struct);
      slang_alloc_free(self->_struct);
   }
   if (self->_array != NULL) {
      slang_type_specifier_dtr(self->_array);
      slang_alloc_free(self->_array);
   }
}

GLboolean
slang_type_specifier_copy(slang_type_specifier * x,
                          const slang_type_specifier * y)
{
   slang_type_specifier z;

   slang_type_specifier_ctr(&z);
   z.type = y->type;
   if (z.type == slang_spec_struct) {
      z._struct = (slang_struct *) slang_alloc_malloc(sizeof(slang_struct));
      if (z._struct == NULL) {
         slang_type_specifier_dtr(&z);
         return GL_FALSE;
      }
      if (!slang_struct_construct(z._struct)) {
         slang_alloc_free(z._struct);
         slang_type_specifier_dtr(&z);
         return GL_FALSE;
      }
      if (!slang_struct_copy(z._struct, y->_struct)) {
         slang_type_specifier_dtr(&z);
         return GL_FALSE;
      }
   }
   else if (z.type == slang_spec_array) {
      z._array =
         (slang_type_specifier *)
         slang_alloc_malloc(sizeof(slang_type_specifier));
      if (z._array == NULL) {
         slang_type_specifier_dtr(&z);
         return GL_FALSE;
      }
      slang_type_specifier_ctr(z._array);
      if (!slang_type_specifier_copy(z._array, y->_array)) {
         slang_type_specifier_dtr(&z);
         return GL_FALSE;
      }
   }
   slang_type_specifier_dtr(x);
   *x = z;
   return GL_TRUE;
}

GLboolean
slang_type_specifier_equal(const slang_type_specifier * x,
                           const slang_type_specifier * y)
{
   if (x->type != y->type)
      return 0;
   if (x->type == slang_spec_struct)
      return slang_struct_equal(x->_struct, y->_struct);
   if (x->type == slang_spec_array)
      return slang_type_specifier_equal(x->_array, y->_array);
   return 1;
}

/* slang_assembly_typeinfo */

GLboolean
slang_assembly_typeinfo_construct(slang_assembly_typeinfo * ti)
{
   slang_type_specifier_ctr(&ti->spec);
   ti->array_len = 0;
   return GL_TRUE;
}

GLvoid
slang_assembly_typeinfo_destruct(slang_assembly_typeinfo * ti)
{
   slang_type_specifier_dtr(&ti->spec);
}

/* _slang_typeof_operation() */

/**
 * Determine the return type of a function.
 * \param name  name of the function
 * \param params  array of function parameters
 * \param num_params  number of parameters
 * \param space  namespace to use
 * \param spec  returns the function's type
 * \param atoms  atom pool
 * \return GL_TRUE for success, GL_FALSE if failure
 */
static GLboolean
typeof_existing_function(const char *name, const slang_operation * params,
                         GLuint num_params,
                         const slang_assembly_name_space * space,
                         slang_type_specifier * spec,
                         slang_atom_pool * atoms)
{
   slang_atom atom;
   GLboolean exists;

   atom = slang_atom_pool_atom(atoms, name);
   if (!_slang_typeof_function(atom, params, num_params, space, spec,
                               &exists, atoms))
      return GL_FALSE;
   return exists;
}

GLboolean
_slang_typeof_operation(const slang_assemble_ctx * A,
                        const slang_operation * op,
                        slang_assembly_typeinfo * ti)
{
   return _slang_typeof_operation_(op, &A->space, ti, A->atoms);
}


/**
 * Determine the return type of an operation.
 * \param op  the operation node
 * \param space  the namespace to use
 * \param ti  the returned type
 * \param atoms  atom pool
 * \return GL_TRUE for success, GL_FALSE if failure
 */
GLboolean
_slang_typeof_operation_(const slang_operation * op,
                         const slang_assembly_name_space * space,
                         slang_assembly_typeinfo * ti,
                         slang_atom_pool * atoms)
{
   ti->can_be_referenced = GL_FALSE;
   ti->is_swizzled = GL_FALSE;

   switch (op->type) {
   case slang_oper_block_no_new_scope:
   case slang_oper_block_new_scope:
   case slang_oper_variable_decl:
   case slang_oper_asm:
   case slang_oper_break:
   case slang_oper_continue:
   case slang_oper_discard:
   case slang_oper_return:
   case slang_oper_if:
   case slang_oper_while:
   case slang_oper_do:
   case slang_oper_for:
   case slang_oper_void:
      ti->spec.type = slang_spec_void;
      break;
   case slang_oper_expression:
   case slang_oper_assign:
   case slang_oper_addassign:
   case slang_oper_subassign:
   case slang_oper_mulassign:
   case slang_oper_divassign:
   case slang_oper_preincrement:
   case slang_oper_predecrement:
      if (!_slang_typeof_operation_(op->children, space, ti, atoms))
         return 0;
      break;
   case slang_oper_literal_bool:
   case slang_oper_logicalor:
   case slang_oper_logicalxor:
   case slang_oper_logicaland:
   case slang_oper_equal:
   case slang_oper_notequal:
   case slang_oper_less:
   case slang_oper_greater:
   case slang_oper_lessequal:
   case slang_oper_greaterequal:
   case slang_oper_not:
      ti->spec.type = slang_spec_bool;
      break;
   case slang_oper_literal_int:
      ti->spec.type = slang_spec_int;
      break;
   case slang_oper_literal_float:
      ti->spec.type = slang_spec_float;
      break;
   case slang_oper_identifier:
      {
         slang_variable *var;

         var = _slang_locate_variable(op->locals, op->a_id, GL_TRUE);
         if (var == NULL)
            return GL_FALSE;
         if (!slang_type_specifier_copy(&ti->spec, &var->type.specifier))
            return GL_FALSE;
         ti->can_be_referenced = GL_TRUE;
         ti->array_len = var->array_len;
      }
      break;
   case slang_oper_sequence:
      /* TODO: check [0] and [1] if they match */
      if (!_slang_typeof_operation_(&op->children[1], space, ti, atoms))
         return GL_FALSE;
      ti->can_be_referenced = GL_FALSE;
      ti->is_swizzled = GL_FALSE;
      break;
      /*case slang_oper_modassign: */
      /*case slang_oper_lshassign: */
      /*case slang_oper_rshassign: */
      /*case slang_oper_orassign: */
      /*case slang_oper_xorassign: */
      /*case slang_oper_andassign: */
   case slang_oper_select:
      /* TODO: check [1] and [2] if they match */
      if (!_slang_typeof_operation_(&op->children[1], space, ti, atoms))
         return GL_FALSE;
      ti->can_be_referenced = GL_FALSE;
      ti->is_swizzled = GL_FALSE;
      break;
      /*case slang_oper_bitor: */
      /*case slang_oper_bitxor: */
      /*case slang_oper_bitand: */
      /*case slang_oper_lshift: */
      /*case slang_oper_rshift: */
   case slang_oper_add:
      if (!typeof_existing_function("+", op->children, 2, space,
                                    &ti->spec, atoms))
         return GL_FALSE;
      break;
   case slang_oper_subtract:
      if (!typeof_existing_function("-", op->children, 2, space,
                                    &ti->spec, atoms))
         return GL_FALSE;
      break;
   case slang_oper_multiply:
      if (!typeof_existing_function("*", op->children, 2, space,
                                    &ti->spec, atoms))
         return GL_FALSE;
      break;
   case slang_oper_divide:
      if (!typeof_existing_function("/", op->children, 2, space,
                                    &ti->spec, atoms))
         return GL_FALSE;
      break;
      /*case slang_oper_modulus: */
   case slang_oper_plus:
      if (!_slang_typeof_operation_(op->children, space, ti, atoms))
         return GL_FALSE;
      ti->can_be_referenced = GL_FALSE;
      ti->is_swizzled = GL_FALSE;
      break;
   case slang_oper_minus:
      if (!typeof_existing_function
          ("-", op->children, 1, space, &ti->spec, atoms))
         return GL_FALSE;
      break;
      /*case slang_oper_complement: */
   case slang_oper_subscript:
      {
         slang_assembly_typeinfo _ti;

         if (!slang_assembly_typeinfo_construct(&_ti))
            return GL_FALSE;
         if (!_slang_typeof_operation_(op->children, space, &_ti, atoms)) {
            slang_assembly_typeinfo_destruct(&_ti);
            return GL_FALSE;
         }
         ti->can_be_referenced = _ti.can_be_referenced;
         if (_ti.spec.type == slang_spec_array) {
            if (!slang_type_specifier_copy(&ti->spec, _ti.spec._array)) {
               slang_assembly_typeinfo_destruct(&_ti);
               return GL_FALSE;
            }
         }
         else {
            if (!_slang_type_is_vector(_ti.spec.type)
                && !_slang_type_is_matrix(_ti.spec.type)) {
               slang_assembly_typeinfo_destruct(&_ti);
               return GL_FALSE;
            }
            ti->spec.type = _slang_type_base(_ti.spec.type);
         }
         slang_assembly_typeinfo_destruct(&_ti);
      }
      break;
   case slang_oper_call:
      {
         GLboolean exists;

         if (!_slang_typeof_function(op->a_id, op->children, op->num_children,
                                     space, &ti->spec, &exists, atoms))
            return GL_FALSE;
         if (!exists) {
            slang_struct *s =
               slang_struct_scope_find(space->structs, op->a_id, GL_TRUE);
            if (s != NULL) {
               ti->spec.type = slang_spec_struct;
               ti->spec._struct =
                  (slang_struct *) slang_alloc_malloc(sizeof(slang_struct));
               if (ti->spec._struct == NULL)
                  return GL_FALSE;
               if (!slang_struct_construct(ti->spec._struct)) {
                  slang_alloc_free(ti->spec._struct);
                  ti->spec._struct = NULL;
                  return GL_FALSE;
               }
               if (!slang_struct_copy(ti->spec._struct, s))
                  return GL_FALSE;
            }
            else {
               const char *name;
               slang_type_specifier_type type;

               name = slang_atom_pool_id(atoms, op->a_id);
               type = slang_type_specifier_type_from_string(name);
               if (type == slang_spec_void)
                  return GL_FALSE;
               ti->spec.type = type;
            }
         }
      }
      break;
   case slang_oper_field:
      {
         slang_assembly_typeinfo _ti;

         if (!slang_assembly_typeinfo_construct(&_ti))
            return GL_FALSE;
         if (!_slang_typeof_operation_(op->children, space, &_ti, atoms)) {
            slang_assembly_typeinfo_destruct(&_ti);
            return GL_FALSE;
         }
         if (_ti.spec.type == slang_spec_struct) {
            slang_variable *field;

            field =
               _slang_locate_variable(_ti.spec._struct->fields, op->a_id,
                                      GL_FALSE);
            if (field == NULL) {
               slang_assembly_typeinfo_destruct(&_ti);
               return GL_FALSE;
            }
            if (!slang_type_specifier_copy(&ti->spec, &field->type.specifier)) {
               slang_assembly_typeinfo_destruct(&_ti);
               return GL_FALSE;
            }
            ti->can_be_referenced = _ti.can_be_referenced;
         }
         else {
            GLuint rows;
            const char *swizzle;
            slang_type_specifier_type base;

            /* determine the swizzle of the field expression */
            if (!_slang_type_is_vector(_ti.spec.type)) {
               slang_assembly_typeinfo_destruct(&_ti);
               return GL_FALSE;
            }
            rows = _slang_type_dim(_ti.spec.type);
            swizzle = slang_atom_pool_id(atoms, op->a_id);
            if (!_slang_is_swizzle(swizzle, rows, &ti->swz)) {
               slang_assembly_typeinfo_destruct(&_ti);
               return GL_FALSE;
            }
            ti->is_swizzled = GL_TRUE;
            ti->can_be_referenced = _ti.can_be_referenced
               && _slang_is_swizzle_mask(&ti->swz, rows);
            if (_ti.is_swizzled) {
               slang_swizzle swz;

               /* swizzle the swizzle */
               _slang_multiply_swizzles(&swz, &_ti.swz, &ti->swz);
               ti->swz = swz;
            }
            base = _slang_type_base(_ti.spec.type);
            switch (ti->swz.num_components) {
            case 1:
               ti->spec.type = base;
               break;
            case 2:
               switch (base) {
               case slang_spec_float:
                  ti->spec.type = slang_spec_vec2;
                  break;
               case slang_spec_int:
                  ti->spec.type = slang_spec_ivec2;
                  break;
               case slang_spec_bool:
                  ti->spec.type = slang_spec_bvec2;
                  break;
               default:
                  break;
               }
               break;
            case 3:
               switch (base) {
               case slang_spec_float:
                  ti->spec.type = slang_spec_vec3;
                  break;
               case slang_spec_int:
                  ti->spec.type = slang_spec_ivec3;
                  break;
               case slang_spec_bool:
                  ti->spec.type = slang_spec_bvec3;
                  break;
               default:
                  break;
               }
               break;
            case 4:
               switch (base) {
               case slang_spec_float:
                  ti->spec.type = slang_spec_vec4;
                  break;
               case slang_spec_int:
                  ti->spec.type = slang_spec_ivec4;
                  break;
               case slang_spec_bool:
                  ti->spec.type = slang_spec_bvec4;
                  break;
               default:
                  break;
               }
               break;
            default:
               break;
            }
         }
         slang_assembly_typeinfo_destruct(&_ti);
      }
      break;
   case slang_oper_postincrement:
   case slang_oper_postdecrement:
      if (!_slang_typeof_operation_(op->children, space, ti, atoms))
         return GL_FALSE;
      ti->can_be_referenced = GL_FALSE;
      ti->is_swizzled = GL_FALSE;
      break;
   default:
      return GL_FALSE;
   }

   return GL_TRUE;
}



/**
 * Determine the return type of a function.
 * \param a_name  the function name
 * \param param  function parameters (overloading)
 * \param num_params  number of parameters to function
 * \param space  namespace to search
 * \param exists  returns GL_TRUE or GL_FALSE to indicate existance of function
 * \return GL_TRUE for success, GL_FALSE if failure (bad function name)
 */
GLboolean
_slang_typeof_function(slang_atom a_name, const slang_operation * params,
                       GLuint num_params,
                       const slang_assembly_name_space * space,
                       slang_type_specifier * spec, GLboolean * exists,
                       slang_atom_pool * atoms)
{
   slang_function *fun = _slang_locate_function(space->funcs, a_name, params,
                                                num_params, space, atoms);
   *exists = fun != NULL;
   if (!fun)
      return GL_TRUE;  /* yes, not false */
   return slang_type_specifier_copy(spec, &fun->header.type.specifier);
}



/**
 * Determine if a type is a matrix.
 * \return GL_TRUE if is a matrix, GL_FALSE otherwise.
 */
GLboolean
_slang_type_is_matrix(slang_type_specifier_type ty)
{
   switch (ty) {
   case slang_spec_mat2:
   case slang_spec_mat3:
   case slang_spec_mat4:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Determine if a type is a vector.
 * \return GL_TRUE if is a vector, GL_FALSE otherwise.
 */
GLboolean
_slang_type_is_vector(slang_type_specifier_type ty)
{
   switch (ty) {
   case slang_spec_vec2:
   case slang_spec_vec3:
   case slang_spec_vec4:
   case slang_spec_ivec2:
   case slang_spec_ivec3:
   case slang_spec_ivec4:
   case slang_spec_bvec2:
   case slang_spec_bvec3:
   case slang_spec_bvec4:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Given a vector type, return the type of the vector's elements
 */
slang_type_specifier_type
_slang_type_base(slang_type_specifier_type ty)
{
   switch (ty) {
   case slang_spec_float:
   case slang_spec_vec2:
   case slang_spec_vec3:
   case slang_spec_vec4:
      return slang_spec_float;
   case slang_spec_int:
   case slang_spec_ivec2:
   case slang_spec_ivec3:
   case slang_spec_ivec4:
      return slang_spec_int;
   case slang_spec_bool:
   case slang_spec_bvec2:
   case slang_spec_bvec3:
   case slang_spec_bvec4:
      return slang_spec_bool;
   case slang_spec_mat2:
      return slang_spec_vec2;
   case slang_spec_mat3:
      return slang_spec_vec3;
   case slang_spec_mat4:
      return slang_spec_vec4;
   default:
      return slang_spec_void;
   }
}


/**
 * Return the dimensionality of a vector or matrix type.
 */
GLuint
_slang_type_dim(slang_type_specifier_type ty)
{
   switch (ty) {
   case slang_spec_float:
   case slang_spec_int:
   case slang_spec_bool:
      return 1;
   case slang_spec_vec2:
   case slang_spec_ivec2:
   case slang_spec_bvec2:
   case slang_spec_mat2:
      return 2;
   case slang_spec_vec3:
   case slang_spec_ivec3:
   case slang_spec_bvec3:
   case slang_spec_mat3:
      return 3;
   case slang_spec_vec4:
   case slang_spec_ivec4:
   case slang_spec_bvec4:
   case slang_spec_mat4:
      return 4;
   default:
      return 0;
   }
}
