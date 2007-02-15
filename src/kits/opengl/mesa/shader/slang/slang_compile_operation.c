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
 * \file slang_compile_operation.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_compile.h"


/**
 * Init a slang_operation object
 */
GLboolean
slang_operation_construct(slang_operation * oper)
{
   oper->type = slang_oper_none;
   oper->children = NULL;
   oper->num_children = 0;
   oper->literal = (float) 0;
   oper->a_id = SLANG_ATOM_NULL;
   oper->locals =
      (slang_variable_scope *)
      slang_alloc_malloc(sizeof(slang_variable_scope));
   if (oper->locals == NULL)
      return GL_FALSE;
   _slang_variable_scope_ctr(oper->locals);
   return GL_TRUE;
}

void
slang_operation_destruct(slang_operation * oper)
{
   GLuint i;

   for (i = 0; i < oper->num_children; i++)
      slang_operation_destruct(oper->children + i);
   slang_alloc_free(oper->children);
   slang_variable_scope_destruct(oper->locals);
   slang_alloc_free(oper->locals);
}

/**
 * Recursively copy a slang_operation node.
 * \return GL_TRUE for success, GL_FALSE if failure
 */
GLboolean
slang_operation_copy(slang_operation * x, const slang_operation * y)
{
   slang_operation z;
   GLuint i;

   if (!slang_operation_construct(&z))
      return GL_FALSE;
   z.type = y->type;
   z.children = (slang_operation *)
      slang_alloc_malloc(y->num_children * sizeof(slang_operation));
   if (z.children == NULL) {
      slang_operation_destruct(&z);
      return GL_FALSE;
   }
   for (z.num_children = 0; z.num_children < y->num_children;
        z.num_children++) {
      if (!slang_operation_construct(&z.children[z.num_children])) {
         slang_operation_destruct(&z);
         return GL_FALSE;
      }
   }
   for (i = 0; i < z.num_children; i++) {
      if (!slang_operation_copy(&z.children[i], &y->children[i])) {
         slang_operation_destruct(&z);
         return GL_FALSE;
      }
   }
   z.literal = y->literal;
   z.a_id = y->a_id;
   if (!slang_variable_scope_copy(z.locals, y->locals)) {
      slang_operation_destruct(&z);
      return GL_FALSE;
   }
   slang_operation_destruct(x);
   *x = z;
   return GL_TRUE;
}


slang_operation *
slang_operation_new(GLuint count)
{
   return (slang_operation *) _mesa_calloc(count * sizeof(slang_operation));
}
