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
 * \file slang_compile_function.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_compile.h"

/* slang_fixup_table */

void
slang_fixup_table_init(slang_fixup_table * fix)
{
   fix->table = NULL;
   fix->count = 0;
}

void
slang_fixup_table_free(slang_fixup_table * fix)
{
   slang_alloc_free(fix->table);
   slang_fixup_table_init(fix);
}

/**
 * Add a new fixup address to the table.
 */
GLboolean
slang_fixup_save(slang_fixup_table *fixups, GLuint address)
{
   fixups->table = (GLuint *)
      slang_alloc_realloc(fixups->table,
                          fixups->count * sizeof(GLuint),
                          (fixups->count + 1) * sizeof(GLuint));
   if (fixups->table == NULL)
      return GL_FALSE;
   fixups->table[fixups->count] = address;
   fixups->count++;
   return GL_TRUE;
}



/* slang_function */

int
slang_function_construct(slang_function * func)
{
   func->kind = slang_func_ordinary;
   if (!slang_variable_construct(&func->header))
      return 0;

   func->parameters = (slang_variable_scope *)
      slang_alloc_malloc(sizeof(slang_variable_scope));
   if (func->parameters == NULL) {
      slang_variable_destruct(&func->header);
      return 0;
   }

   _slang_variable_scope_ctr(func->parameters);
   func->param_count = 0;
   func->body = NULL;
   func->address = ~0;
   slang_fixup_table_init(&func->fixups);
   return 1;
}

void
slang_function_destruct(slang_function * func)
{
   slang_variable_destruct(&func->header);
   slang_variable_scope_destruct(func->parameters);
   slang_alloc_free(func->parameters);
   if (func->body != NULL) {
      slang_operation_destruct(func->body);
      slang_alloc_free(func->body);
   }
   slang_fixup_table_free(&func->fixups);
}

/*
 * slang_function_scope
 */

GLvoid
_slang_function_scope_ctr(slang_function_scope * self)
{
   self->functions = NULL;
   self->num_functions = 0;
   self->outer_scope = NULL;
}

void
slang_function_scope_destruct(slang_function_scope * scope)
{
   unsigned int i;

   for (i = 0; i < scope->num_functions; i++)
      slang_function_destruct(scope->functions + i);
   slang_alloc_free(scope->functions);
}


/**
 * Search a list of functions for a particular function by name.
 * \param funcs  the list of functions to search
 * \param a_name  the name to search for
 * \param all_scopes  if non-zero, search containing scopes too.
 * \return pointer to found function, or NULL.
 */
int
slang_function_scope_find_by_name(slang_function_scope * funcs,
                                  slang_atom a_name, int all_scopes)
{
   unsigned int i;

   for (i = 0; i < funcs->num_functions; i++)
      if (a_name == funcs->functions[i].header.a_name)
         return 1;
   if (all_scopes && funcs->outer_scope != NULL)
      return slang_function_scope_find_by_name(funcs->outer_scope, a_name, 1);
   return 0;
}


/**
 * Search a list of functions for a particular function (for implementing
 * function calls.  Matching is done by first comparing the function's name,
 * then the function's parameter list.
 *
 * \param funcs  the list of functions to search
 * \param fun  the function to search for
 * \param all_scopes  if non-zero, search containing scopes too.
 * \return pointer to found function, or NULL.
 */
slang_function *
slang_function_scope_find(slang_function_scope * funcs, slang_function * fun,
                          int all_scopes)
{
   unsigned int i;

   for (i = 0; i < funcs->num_functions; i++) {
      slang_function *f = &funcs->functions[i];
      unsigned int j;

      if (fun->header.a_name != f->header.a_name)
         continue;
      if (fun->param_count != f->param_count)
         continue;
      for (j = 0; j < fun->param_count; j++) {
         if (!slang_type_specifier_equal
             (&fun->parameters->variables[j].type.specifier,
              &f->parameters->variables[j].type.specifier))
            break;
      }
      if (j == fun->param_count)
         return f;
   }
   if (all_scopes && funcs->outer_scope != NULL)
      return slang_function_scope_find(funcs->outer_scope, fun, 1);
   return NULL;
}

/*
 * _slang_build_export_code_table()
 */

GLboolean
_slang_build_export_code_table(slang_export_code_table * tbl,
                               slang_function_scope * funs,
                               slang_code_unit * unit)
{
   slang_atom mainAtom;
   GLuint i;

   mainAtom = slang_atom_pool_atom(tbl->atoms, "main");
   if (mainAtom == SLANG_ATOM_NULL)
      return GL_FALSE;

   for (i = 0; i < funs->num_functions; i++) {
      if (funs->functions[i].header.a_name == mainAtom) {
         slang_function *fun = &funs->functions[i];
         slang_export_code_entry *e;
         slang_assemble_ctx A;

         e = slang_export_code_table_add(tbl);
         if (e == NULL)
            return GL_FALSE;
         e->address = unit->object->assembly.count;
         e->name = slang_atom_pool_atom(tbl->atoms, "@main");
         if (e->name == SLANG_ATOM_NULL)
            return GL_FALSE;

         A.file = &unit->object->assembly;
         A.mach = &unit->object->machine;
         A.atoms = &unit->object->atompool;
         A.space.funcs = &unit->funs;
         A.space.structs = &unit->structs;
         A.space.vars = &unit->vars;
         slang_assembly_file_push_label(&unit->object->assembly,
                                        slang_asm_local_alloc, 20);
         slang_assembly_file_push_label(&unit->object->assembly,
                                        slang_asm_enter, 20);
         _slang_assemble_function_call(&A, fun, NULL, 0, GL_FALSE);
         slang_assembly_file_push(&unit->object->assembly, slang_asm_exit);
      }
   }
   return GL_TRUE;
}
