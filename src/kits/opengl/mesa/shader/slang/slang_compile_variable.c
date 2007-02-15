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
 * \file slang_compile_variable.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_compile.h"

/* slang_type_specifier_type */

typedef struct
{
   const char *name;
   slang_type_specifier_type type;
} type_specifier_type_name;

static const type_specifier_type_name type_specifier_type_names[] = {
   {"void", slang_spec_void},
   {"bool", slang_spec_bool},
   {"bvec2", slang_spec_bvec2},
   {"bvec3", slang_spec_bvec3},
   {"bvec4", slang_spec_bvec4},
   {"int", slang_spec_int},
   {"ivec2", slang_spec_ivec2},
   {"ivec3", slang_spec_ivec3},
   {"ivec4", slang_spec_ivec4},
   {"float", slang_spec_float},
   {"vec2", slang_spec_vec2},
   {"vec3", slang_spec_vec3},
   {"vec4", slang_spec_vec4},
   {"mat2", slang_spec_mat2},
   {"mat3", slang_spec_mat3},
   {"mat4", slang_spec_mat4},
   {"sampler1D", slang_spec_sampler1D},
   {"sampler2D", slang_spec_sampler2D},
   {"sampler3D", slang_spec_sampler3D},
   {"samplerCube", slang_spec_samplerCube},
   {"sampler1DShadow", slang_spec_sampler1DShadow},
   {"sampler2DShadow", slang_spec_sampler2DShadow},
   {NULL, slang_spec_void}
};

slang_type_specifier_type
slang_type_specifier_type_from_string(const char *name)
{
   const type_specifier_type_name *p = type_specifier_type_names;
   while (p->name != NULL) {
      if (slang_string_compare(p->name, name) == 0)
         break;
      p++;
   }
   return p->type;
}

const char *
slang_type_specifier_type_to_string(slang_type_specifier_type type)
{
   const type_specifier_type_name *p = type_specifier_type_names;
   while (p->name != NULL) {
      if (p->type == type)
         break;
      p++;
   }
   return p->name;
}

/* slang_fully_specified_type */

int
slang_fully_specified_type_construct(slang_fully_specified_type * type)
{
   type->qualifier = slang_qual_none;
   slang_type_specifier_ctr(&type->specifier);
   return 1;
}

void
slang_fully_specified_type_destruct(slang_fully_specified_type * type)
{
   slang_type_specifier_dtr(&type->specifier);
}

int
slang_fully_specified_type_copy(slang_fully_specified_type * x,
                                const slang_fully_specified_type * y)
{
   slang_fully_specified_type z;

   if (!slang_fully_specified_type_construct(&z))
      return 0;
   z.qualifier = y->qualifier;
   if (!slang_type_specifier_copy(&z.specifier, &y->specifier)) {
      slang_fully_specified_type_destruct(&z);
      return 0;
   }
   slang_fully_specified_type_destruct(x);
   *x = z;
   return 1;
}

/*
 * slang_variable_scope
 */

GLvoid
_slang_variable_scope_ctr(slang_variable_scope * self)
{
   self->variables = NULL;
   self->num_variables = 0;
   self->outer_scope = NULL;
}

void
slang_variable_scope_destruct(slang_variable_scope * scope)
{
   unsigned int i;

   if (!scope)
      return;
   for (i = 0; i < scope->num_variables; i++)
      slang_variable_destruct(scope->variables + i);
   slang_alloc_free(scope->variables);
   /* do not free scope->outer_scope */
}

int
slang_variable_scope_copy(slang_variable_scope * x,
                          const slang_variable_scope * y)
{
   slang_variable_scope z;
   unsigned int i;

   _slang_variable_scope_ctr(&z);
   z.variables = (slang_variable *)
      slang_alloc_malloc(y->num_variables * sizeof(slang_variable));
   if (z.variables == NULL) {
      slang_variable_scope_destruct(&z);
      return 0;
   }
   for (z.num_variables = 0; z.num_variables < y->num_variables;
        z.num_variables++) {
      if (!slang_variable_construct(&z.variables[z.num_variables])) {
         slang_variable_scope_destruct(&z);
         return 0;
      }
   }
   for (i = 0; i < z.num_variables; i++) {
      if (!slang_variable_copy(&z.variables[i], &y->variables[i])) {
         slang_variable_scope_destruct(&z);
         return 0;
      }
   }
   z.outer_scope = y->outer_scope;
   slang_variable_scope_destruct(x);
   *x = z;
   return 1;
}


/**
 * Grow the variable list by one.
 * \return  pointer to space for the new variable (will be initialized)
 */
slang_variable *
slang_variable_scope_grow(slang_variable_scope *scope)
{
   const int n = scope->num_variables;
   scope->variables = (slang_variable *)
         slang_alloc_realloc(scope->variables,
                             n * sizeof(slang_variable),
                             (n + 1) * sizeof(slang_variable));
   if (!scope->variables)
      return NULL;

   scope->num_variables++;

   if (!slang_variable_construct(scope->variables + n))
      return NULL;

   return scope->variables + n;
}



/* slang_variable */

int
slang_variable_construct(slang_variable * var)
{
   if (!slang_fully_specified_type_construct(&var->type))
      return 0;
   var->a_name = SLANG_ATOM_NULL;
   var->array_len = 0;
   var->initializer = NULL;
   var->address = ~0;
   var->address2 = 0;
   var->size = 0;
   var->global = GL_FALSE;
   return 1;
}

void
slang_variable_destruct(slang_variable * var)
{
   slang_fully_specified_type_destruct(&var->type);
   if (var->initializer != NULL) {
      slang_operation_destruct(var->initializer);
      slang_alloc_free(var->initializer);
   }
}

int
slang_variable_copy(slang_variable * x, const slang_variable * y)
{
   slang_variable z;

   if (!slang_variable_construct(&z))
      return 0;
   if (!slang_fully_specified_type_copy(&z.type, &y->type)) {
      slang_variable_destruct(&z);
      return 0;
   }
   z.a_name = y->a_name;
   z.array_len = y->array_len;
   if (y->initializer != NULL) {
      z.initializer =
         (slang_operation *) slang_alloc_malloc(sizeof(slang_operation));
      if (z.initializer == NULL) {
         slang_variable_destruct(&z);
         return 0;
      }
      if (!slang_operation_construct(z.initializer)) {
         slang_alloc_free(z.initializer);
         slang_variable_destruct(&z);
         return 0;
      }
      if (!slang_operation_copy(z.initializer, y->initializer)) {
         slang_variable_destruct(&z);
         return 0;
      }
   }
   z.address = y->address;
   z.size = y->size;
   z.global = y->global;
   slang_variable_destruct(x);
   *x = z;
   return 1;
}

slang_variable *
_slang_locate_variable(const slang_variable_scope * scope,
                       const slang_atom a_name, GLboolean all)
{
   GLuint i;

   for (i = 0; i < scope->num_variables; i++)
      if (a_name == scope->variables[i].a_name)
         return &scope->variables[i];
   if (all && scope->outer_scope != NULL)
      return _slang_locate_variable(scope->outer_scope, a_name, 1);
   return NULL;
}

/*
 * _slang_build_export_data_table()
 */

static GLenum
gl_type_from_specifier(const slang_type_specifier * type)
{
   switch (type->type) {
   case slang_spec_bool:
      return GL_BOOL_ARB;
   case slang_spec_bvec2:
      return GL_BOOL_VEC2_ARB;
   case slang_spec_bvec3:
      return GL_BOOL_VEC3_ARB;
   case slang_spec_bvec4:
      return GL_BOOL_VEC4_ARB;
   case slang_spec_int:
      return GL_INT;
   case slang_spec_ivec2:
      return GL_INT_VEC2_ARB;
   case slang_spec_ivec3:
      return GL_INT_VEC3_ARB;
   case slang_spec_ivec4:
      return GL_INT_VEC4_ARB;
   case slang_spec_float:
      return GL_FLOAT;
   case slang_spec_vec2:
      return GL_FLOAT_VEC2_ARB;
   case slang_spec_vec3:
      return GL_FLOAT_VEC3_ARB;
   case slang_spec_vec4:
      return GL_FLOAT_VEC4_ARB;
   case slang_spec_mat2:
      return GL_FLOAT_MAT2_ARB;
   case slang_spec_mat3:
      return GL_FLOAT_MAT3_ARB;
   case slang_spec_mat4:
      return GL_FLOAT_MAT4_ARB;
   case slang_spec_sampler1D:
      return GL_SAMPLER_1D_ARB;
   case slang_spec_sampler2D:
      return GL_SAMPLER_2D_ARB;
   case slang_spec_sampler3D:
      return GL_SAMPLER_3D_ARB;
   case slang_spec_samplerCube:
      return GL_SAMPLER_CUBE_ARB;
   case slang_spec_sampler1DShadow:
      return GL_SAMPLER_1D_SHADOW_ARB;
   case slang_spec_sampler2DShadow:
      return GL_SAMPLER_2D_SHADOW_ARB;
   case slang_spec_array:
      return gl_type_from_specifier(type->_array);
   default:
      return GL_FLOAT;
   }
}

static GLboolean
build_quant(slang_export_data_quant * q, const slang_variable * var)
{
   const slang_type_specifier *spec = &var->type.specifier;

   q->name = var->a_name;
   q->size = var->size;
   if (spec->type == slang_spec_array) {
      q->array_len = var->array_len;
      q->size /= var->array_len;
      spec = spec->_array;
   }
   if (spec->type == slang_spec_struct) {
      GLuint i;

      q->u.field_count = spec->_struct->fields->num_variables;
      q->structure = (slang_export_data_quant *)
         slang_alloc_malloc(q->u.field_count
                            * sizeof(slang_export_data_quant));
      if (q->structure == NULL)
         return GL_FALSE;

      for (i = 0; i < q->u.field_count; i++)
         slang_export_data_quant_ctr(&q->structure[i]);
      for (i = 0; i < q->u.field_count; i++) {
         if (!build_quant(&q->structure[i],
                          &spec->_struct->fields->variables[i]))
            return GL_FALSE;
      }
   }
   else
      q->u.basic_type = gl_type_from_specifier(spec);
   return GL_TRUE;
}

GLboolean
_slang_build_export_data_table(slang_export_data_table * tbl,
                               slang_variable_scope * vars)
{
   GLuint i;

   for (i = 0; i < vars->num_variables; i++) {
      const slang_variable *var = &vars->variables[i];
      slang_export_data_entry *e;

      e = slang_export_data_table_add(tbl);
      if (e == NULL)
         return GL_FALSE;
      if (!build_quant(&e->quant, var))
         return GL_FALSE;
      if (var->type.qualifier == slang_qual_uniform)
         e->access = slang_exp_uniform;
      else if (var->type.qualifier == slang_qual_attribute)
         e->access = slang_exp_attribute;
      else
         e->access = slang_exp_varying;
      e->address = var->address;
   }

   if (vars->outer_scope != NULL)
      return _slang_build_export_data_table(tbl, vars->outer_scope);
   return GL_TRUE;
}
