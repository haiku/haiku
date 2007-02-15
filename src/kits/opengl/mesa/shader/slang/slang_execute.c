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
 * \file slang_execute.c
 * intermediate code interpreter
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_compile.h"
#include "slang_execute.h"
#include "slang_library_noise.h"
#include "slang_library_texsample.h"

#define DEBUG_SLANG 0

GLvoid
slang_machine_ctr(slang_machine * self)
{
   slang_machine_init(self);
   self->infolog = NULL;
#if defined(USE_X86_ASM) || defined(SLANG_X86)
   self->x86.compiled_func = NULL;
#endif
}

GLvoid
slang_machine_dtr(slang_machine * self)
{
   if (self->infolog != NULL) {
      slang_info_log_destruct(self->infolog);
      slang_alloc_free(self->infolog);
   }
#if defined(USE_X86_ASM) || defined(SLANG_X86)
   if (self->x86.compiled_func != NULL)
      _mesa_exec_free(self->x86.compiled_func);
#endif
}


/**
 * Initialize the shader virtual machine.
 * NOTE: stack grows downward in memory.
 */
void
slang_machine_init(slang_machine * mach)
{
   mach->ip = 0;
   mach->sp = SLANG_MACHINE_STACK_SIZE;
   mach->bp = 0;
   mach->kill = GL_FALSE;
   mach->exit = GL_FALSE;
}

#if DEBUG_SLANG

static void
dump_instruction(FILE * f, slang_assembly * a, unsigned int i)
{
   fprintf(f, "%.5u:\t", i);

   switch (a->type) {
      /* core */
   case slang_asm_none:
      fprintf(f, "none");
      break;
   case slang_asm_float_copy:
      fprintf(f, "float_copy\t%d, %d", a->param[0], a->param[1]);
      break;
   case slang_asm_float_move:
      fprintf(f, "float_move\t%d, %d", a->param[0], a->param[1]);
      break;
   case slang_asm_float_push:
      fprintf(f, "float_push\t%f", a->literal);
      break;
   case slang_asm_float_deref:
      fprintf(f, "float_deref");
      break;
   case slang_asm_float_add:
      fprintf(f, "float_add");
      break;
   case slang_asm_float_multiply:
      fprintf(f, "float_multiply");
      break;
   case slang_asm_float_divide:
      fprintf(f, "float_divide");
      break;
   case slang_asm_float_negate:
      fprintf(f, "float_negate");
      break;
   case slang_asm_float_less:
      fprintf(f, "float_less");
      break;
   case slang_asm_float_equal_exp:
      fprintf(f, "float_equal");
      break;
   case slang_asm_float_equal_int:
      fprintf(f, "float_equal\t%d, %d", a->param[0], a->param[1]);
      break;
   case slang_asm_float_to_int:
      fprintf(f, "float_to_int");
      break;
   case slang_asm_float_sine:
      fprintf(f, "float_sine");
      break;
   case slang_asm_float_arcsine:
      fprintf(f, "float_arcsine");
      break;
   case slang_asm_float_arctan:
      fprintf(f, "float_arctan");
      break;
   case slang_asm_float_power:
      fprintf(f, "float_power");
      break;
   case slang_asm_float_log2:
      fprintf(f, "float_log2");
      break;
   case slang_asm_float_floor:
      fprintf(f, "float_floor");
      break;
   case slang_asm_float_ceil:
      fprintf(f, "float_ceil");
      break;
   case slang_asm_float_noise1:
      fprintf(f, "float_noise1");
      break;
   case slang_asm_float_noise2:
      fprintf(f, "float_noise2");
      break;
   case slang_asm_float_noise3:
      fprintf(f, "float_noise3");
      break;
   case slang_asm_float_noise4:
      fprintf(f, "float_noise4");
      break;
   case slang_asm_int_copy:
      fprintf(f, "int_copy\t%d, %d", a->param[0], a->param[1]);
      break;
   case slang_asm_int_move:
      fprintf(f, "int_move\t%d, %d", a->param[0], a->param[1]);
      break;
   case slang_asm_int_push:
      fprintf(f, "int_push\t%d", (GLint) a->literal);
      break;
   case slang_asm_int_deref:
      fprintf(f, "int_deref");
      break;
   case slang_asm_int_to_float:
      fprintf(f, "int_to_float");
      break;
   case slang_asm_int_to_addr:
      fprintf(f, "int_to_addr");
      break;
   case slang_asm_bool_copy:
      fprintf(f, "bool_copy\t%d, %d", a->param[0], a->param[1]);
      break;
   case slang_asm_bool_move:
      fprintf(f, "bool_move\t%d, %d", a->param[0], a->param[1]);
      break;
   case slang_asm_bool_push:
      fprintf(f, "bool_push\t%d", a->literal != 0.0f);
      break;
   case slang_asm_bool_deref:
      fprintf(f, "bool_deref");
      break;
   case slang_asm_addr_copy:
      fprintf(f, "addr_copy");
      break;
   case slang_asm_addr_push:
      fprintf(f, "addr_push\t%u", a->param[0]);
      break;
   case slang_asm_addr_deref:
      fprintf(f, "addr_deref");
      break;
   case slang_asm_addr_add:
      fprintf(f, "addr_add");
      break;
   case slang_asm_addr_multiply:
      fprintf(f, "addr_multiply");
      break;
   case slang_asm_vec4_tex1d:
      fprintf(f, "vec4_tex1d");
      break;
   case slang_asm_vec4_tex2d:
      fprintf(f, "vec4_tex2d");
      break;
   case slang_asm_vec4_tex3d:
      fprintf(f, "vec4_tex3d");
      break;
   case slang_asm_vec4_texcube:
      fprintf(f, "vec4_texcube");
      break;
   case slang_asm_vec4_shad1d:
      fprintf(f, "vec4_shad1d");
      break;
   case slang_asm_vec4_shad2d:
      fprintf(f, "vec4_shad2d");
      break;
   case slang_asm_jump:
      fprintf(f, "jump\t%u", a->param[0]);
      break;
   case slang_asm_jump_if_zero:
      fprintf(f, "jump_if_zero\t%u", a->param[0]);
      break;
   case slang_asm_enter:
      fprintf(f, "enter\t%u", a->param[0]);
      break;
   case slang_asm_leave:
      fprintf(f, "leave");
      break;
   case slang_asm_local_alloc:
      fprintf(f, "local_alloc\t%u", a->param[0]);
      break;
   case slang_asm_local_free:
      fprintf(f, "local_free\t%u", a->param[0]);
      break;
   case slang_asm_local_addr:
      fprintf(f, "local_addr\t%u, %u", a->param[0], a->param[1]);
      break;
   case slang_asm_global_addr:
      fprintf(f, "global_addr\t%u", a->param[0]);
      break;
   case slang_asm_call:
      fprintf(f, "call\t%u", a->param[0]);
      break;
   case slang_asm_return:
      fprintf(f, "return");
      break;
   case slang_asm_discard:
      fprintf(f, "discard");
      break;
   case slang_asm_exit:
      fprintf(f, "exit");
      break;
      /* GL_MESA_shader_debug */
   case slang_asm_float_print:
      fprintf(f, "float_print");
      break;
   case slang_asm_int_print:
      fprintf(f, "int_print");
      break;
   case slang_asm_bool_print:
      fprintf(f, "bool_print");
      break;
      /* vec4 */
   case slang_asm_float_to_vec4:
      fprintf(f, "float_to_vec4");
      break;
   case slang_asm_vec4_add:
      fprintf(f, "vec4_add");
      break;
   case slang_asm_vec4_subtract:
      fprintf(f, "vec4_subtract");
      break;
   case slang_asm_vec4_multiply:
      fprintf(f, "vec4_multiply");
      break;
   case slang_asm_vec4_divide:
      fprintf(f, "vec4_divide");
      break;
   case slang_asm_vec4_negate:
      fprintf(f, "vec4_negate");
      break;
   case slang_asm_vec4_dot:
      fprintf(f, "vec4_dot");
      break;
   case slang_asm_vec4_copy:
      fprintf(f, "vec4_copy");
      break;
   case slang_asm_vec4_deref:
      fprintf(f, "vec4_deref");
      break;
   case slang_asm_vec4_equal_int:
      fprintf(f, "vec4_equal");
      break;
   default:
      break;
   }

   fprintf(f, "\n");
}

static void
dump(const slang_assembly_file * file)
{
   unsigned int i;
   static unsigned int counter = 0;
   FILE *f;
   char filename[256];

   counter++;
   _mesa_sprintf(filename, "~mesa-slang-assembly-dump-(%u).txt", counter);
   f = fopen(filename, "w");
   if (f == NULL)
      return;

   for (i = 0; i < file->count; i++)
      dump_instruction(f, file->code + i, i);

   fclose(f);
}

#endif

static GLvoid
ensure_infolog_created(slang_info_log ** infolog)
{
   if (*infolog == NULL) {
      *infolog = slang_alloc_malloc(sizeof(slang_info_log));
      if (*infolog == NULL)
         return;
      slang_info_log_construct(*infolog);
   }
}

GLboolean
_slang_execute2(const slang_assembly_file * file, slang_machine * mach)
{
   slang_machine_slot *stack;

#if DEBUG_SLANG
   static unsigned int counter = 0;
   char filename[256];
   FILE *f;
#endif

   /* assume 32-bit floats and uints; should work fine also on 64-bit platforms */
   static_assert(sizeof(GLfloat) == 4);
   static_assert(sizeof(GLuint) == 4);

#if DEBUG_SLANG
   dump(file);
   counter++;
   _mesa_sprintf(filename, "~mesa-slang-assembly-exec-(%u).txt", counter);
   f = fopen(filename, "w");
#endif

#if defined(USE_X86_ASM) || defined(SLANG_X86)
   if (mach->x86.compiled_func != NULL) {
      mach->x86.compiled_func(mach);
      return GL_TRUE;
   }
#endif

   stack = mach->mem + SLANG_MACHINE_GLOBAL_SIZE;

   while (!mach->exit) {
      const slang_assembly *a = &file->code[mach->ip];

#if DEBUG_SLANG
      if (f != NULL && a->type != slang_asm_none) {
         unsigned int i;

         dump_instruction(f, file->code + mach->ip, mach->ip);
         fprintf(f, "\t\tsp=%u bp=%u\n", mach->sp, mach->bp);
         for (i = mach->sp; i < SLANG_MACHINE_STACK_SIZE; i++)
            fprintf(f, "\t%.5u\t%6f\t%u\n", i, stack[i]._float,
                    stack[i]._addr);
         fflush(f);
      }
#endif

      mach->ip++;

      switch (a->type) {
         /* core */
      case slang_asm_none:
         break;
      case slang_asm_float_copy:
      case slang_asm_int_copy:
      case slang_asm_bool_copy:
         /* store top value on stack to memory */
         {
            GLuint address
               = (stack[mach->sp + a->param[0] / 4]._addr + a->param[1]) / 4;
            GLfloat value = stack[mach->sp]._float;
            mach->mem[address]._float = value;
         }
         mach->sp++;
         break;
      case slang_asm_float_move:
      case slang_asm_int_move:
      case slang_asm_bool_move:
         stack[mach->sp + a->param[0] / 4]._float =
            stack[mach->sp +
                  (stack[mach->sp]._addr + a->param[1]) / 4]._float;
         break;
      case slang_asm_float_push:
      case slang_asm_int_push:
      case slang_asm_bool_push:
         /* push float/int/bool literal onto stop of stack */
         mach->sp--;
         stack[mach->sp]._float = a->literal;
         break;
      case slang_asm_float_deref:
      case slang_asm_int_deref:
      case slang_asm_bool_deref:
         /* load value from memory, replace stop of stack with it */
         stack[mach->sp]._float = mach->mem[stack[mach->sp]._addr / 4]._float;
         break;
      case slang_asm_float_add:
         /* pop two top floats, push sum */
         stack[mach->sp + 1]._float += stack[mach->sp]._float;
         mach->sp++;
         break;
      case slang_asm_float_multiply:
         stack[mach->sp + 1]._float *= stack[mach->sp]._float;
         mach->sp++;
         break;
      case slang_asm_float_divide:
         stack[mach->sp + 1]._float /= stack[mach->sp]._float;
         mach->sp++;
         break;
      case slang_asm_float_negate:
         stack[mach->sp]._float = -stack[mach->sp]._float;
         break;
      case slang_asm_float_less:
         stack[mach->sp + 1]._float =
            (stack[mach->sp + 1]._float < stack[mach->sp]._float)
            ? (GLfloat) 1 : (GLfloat) 0;
         mach->sp++;
         break;
      case slang_asm_float_equal_exp:
         stack[mach->sp + 1]._float =
	    (stack[mach->sp + 1]._float == stack[mach->sp]._float)
	    ? (GLfloat) 1 : (GLfloat) 0;
         mach->sp++;
         break;
      case slang_asm_float_equal_int:
         /* pop top two values, compare, push 0 or 1 */
         mach->sp--;
         stack[mach->sp]._float =
            (stack[mach->sp + 1 + a->param[0] / 4]._float ==
	     stack[mach->sp + 1 + a->param[1] / 4]._float)
	    ? (GLfloat) 1 : (GLfloat) 0;
         break;
      case slang_asm_float_to_int:
         stack[mach->sp]._float = (GLfloat) (GLint) stack[mach->sp]._float;
         break;
      case slang_asm_float_sine:
         stack[mach->sp]._float = (GLfloat) _mesa_sin(stack[mach->sp]._float);
         break;
      case slang_asm_float_arcsine:
         stack[mach->sp]._float = _mesa_asinf(stack[mach->sp]._float);
         break;
      case slang_asm_float_arctan:
         stack[mach->sp]._float = _mesa_atanf(stack[mach->sp]._float);
         break;
      case slang_asm_float_power:
         stack[mach->sp + 1]._float = (GLfloat)
            _mesa_pow(stack[mach->sp + 1]._float, stack[mach->sp]._float);
         mach->sp++;
         break;
      case slang_asm_float_log2:
         stack[mach->sp]._float = LOG2(stack[mach->sp]._float);
         break;
      case slang_asm_float_floor:
         stack[mach->sp]._float = FLOORF(stack[mach->sp]._float);
         break;
      case slang_asm_float_ceil:
         stack[mach->sp]._float = CEILF(stack[mach->sp]._float);
         break;
      case slang_asm_float_noise1:
         stack[mach->sp]._float =
            _slang_library_noise1(stack[mach->sp]._float);
         break;
      case slang_asm_float_noise2:
         stack[mach->sp + 1]._float =
            _slang_library_noise2(stack[mach->sp]._float,
                                  stack[mach->sp + 1]._float);
         mach->sp++;
         break;
      case slang_asm_float_noise3:
         stack[mach->sp + 2]._float =
            _slang_library_noise3(stack[mach->sp]._float,
                                  stack[mach->sp + 1]._float,
                                  stack[mach->sp + 2]._float);
         mach->sp += 2;
         break;
      case slang_asm_float_noise4:
         stack[mach->sp + 3]._float =
            _slang_library_noise4(stack[mach->sp]._float,
                                  stack[mach->sp + 1]._float,
                                  stack[mach->sp + 2]._float,
                                  stack[mach->sp + 3]._float);
         mach->sp += 3;
         break;
      case slang_asm_int_to_float:
         break;
      case slang_asm_int_to_addr:
         stack[mach->sp]._addr = (GLuint) (GLint) stack[mach->sp]._float;
         break;
      case slang_asm_addr_copy:
         mach->mem[stack[mach->sp + 1]._addr / 4]._addr =
            stack[mach->sp]._addr;
         mach->sp++;
         break;
      case slang_asm_addr_push:
      case slang_asm_global_addr:
         mach->sp--;
         stack[mach->sp]._addr = a->param[0];
         break;
      case slang_asm_addr_deref:
         stack[mach->sp]._addr = mach->mem[stack[mach->sp]._addr / 4]._addr;
         break;
      case slang_asm_addr_add:
         stack[mach->sp + 1]._addr += stack[mach->sp]._addr;
         mach->sp++;
         break;
      case slang_asm_addr_multiply:
         stack[mach->sp + 1]._addr *= stack[mach->sp]._addr;
         mach->sp++;
         break;
      case slang_asm_vec4_tex1d:
         _slang_library_tex1d(stack[mach->sp]._float,
                              stack[mach->sp + 1]._float,
                              stack[mach->sp + 2]._float,
                              &mach->mem[stack[mach->sp + 3]._addr /
                                         4]._float);
         mach->sp += 3;
         break;
      case slang_asm_vec4_tex2d:
         _slang_library_tex2d(stack[mach->sp]._float,
                              stack[mach->sp + 1]._float,
                              stack[mach->sp + 2]._float,
                              stack[mach->sp + 3]._float,
                              &mach->mem[stack[mach->sp + 4]._addr /
                                         4]._float);
         mach->sp += 4;
         break;
      case slang_asm_vec4_tex3d:
         _slang_library_tex3d(stack[mach->sp]._float,
                              stack[mach->sp + 1]._float,
                              stack[mach->sp + 2]._float,
                              stack[mach->sp + 3]._float,
                              stack[mach->sp + 4]._float,
                              &mach->mem[stack[mach->sp + 5]._addr /
                                         4]._float);
         mach->sp += 5;
         break;
      case slang_asm_vec4_texcube:
         _slang_library_texcube(stack[mach->sp]._float,
                                stack[mach->sp + 1]._float,
                                stack[mach->sp + 2]._float,
                                stack[mach->sp + 3]._float,
                                stack[mach->sp + 4]._float,
                                &mach->mem[stack[mach->sp + 5]._addr /
                                           4]._float);
         mach->sp += 5;
         break;
      case slang_asm_vec4_shad1d:
         _slang_library_shad1d(stack[mach->sp]._float,
                               stack[mach->sp + 1]._float,
                               stack[mach->sp + 2]._float,
                               stack[mach->sp + 3]._float,
                               stack[mach->sp + 4]._float,
                               &mach->mem[stack[mach->sp + 5]._addr /
                                          4]._float);
         mach->sp += 5;
         break;
      case slang_asm_vec4_shad2d:
         _slang_library_shad2d(stack[mach->sp]._float,
                               stack[mach->sp + 1]._float,
                               stack[mach->sp + 2]._float,
                               stack[mach->sp + 3]._float,
                               stack[mach->sp + 4]._float,
                               &mach->mem[stack[mach->sp + 5]._addr /
                                          4]._float);
         mach->sp += 5;
         break;
      case slang_asm_jump:
         mach->ip = a->param[0];
         break;
      case slang_asm_jump_if_zero:
         if (stack[mach->sp]._float == 0.0f)
            mach->ip = a->param[0];
         mach->sp++;
         break;
      case slang_asm_enter:
         mach->sp--;
         stack[mach->sp]._addr = mach->bp;
         mach->bp = mach->sp + a->param[0] / 4;
         break;
      case slang_asm_leave:
         mach->bp = stack[mach->sp]._addr;
         mach->sp++;
         break;
      case slang_asm_local_alloc:
         mach->sp -= a->param[0] / 4;
         break;
      case slang_asm_local_free:
         mach->sp += a->param[0] / 4;
         break;
      case slang_asm_local_addr:
         mach->sp--;
         stack[mach->sp]._addr =
            SLANG_MACHINE_GLOBAL_SIZE * 4 + mach->bp * 4 - (a->param[0] +
                                                            a->param[1]) + 4;
         break;
      case slang_asm_call:
         mach->sp--;
         stack[mach->sp]._addr = mach->ip;
         mach->ip = a->param[0];
         break;
      case slang_asm_return:
         mach->ip = stack[mach->sp]._addr;
         mach->sp++;
         break;
      case slang_asm_discard:
         mach->kill = GL_TRUE;
         break;
      case slang_asm_exit:
         mach->exit = GL_TRUE;
         break;
         /* GL_MESA_shader_debug */
      case slang_asm_float_print:
         _mesa_printf("slang print: %f\n", stack[mach->sp]._float);
         ensure_infolog_created(&mach->infolog);
         slang_info_log_print(mach->infolog, "%f", stack[mach->sp]._float);
         break;
      case slang_asm_int_print:
         _mesa_printf("slang print: %d\n", (GLint) stack[mach->sp]._float);
         ensure_infolog_created(&mach->infolog);
         slang_info_log_print(mach->infolog, "%d",
                              (GLint) (stack[mach->sp]._float));
         break;
      case slang_asm_bool_print:
         _mesa_printf("slang print: %s\n",
                      (GLint) stack[mach->sp]._float ? "true" : "false");
         ensure_infolog_created(&mach->infolog);
         slang_info_log_print(mach->infolog, "%s",
                              (GLint) (stack[mach->sp].
                                       _float) ? "true" : "false");
         break;
         /* vec4 */
      case slang_asm_float_to_vec4:
         /* [vec4] | float > [vec4] */
         {
            GLuint da = stack[mach->sp + 1]._addr;
            mach->mem[da / 4]._float = stack[mach->sp]._float;
            mach->sp++;
         }
         break;
      case slang_asm_vec4_add:
         /* [vec4] | vec4 > [vec4] */
         {
            GLuint da = stack[mach->sp + 4]._addr;
            mach->mem[da / 4]._float += stack[mach->sp]._float;
            mach->mem[(da + 4) / 4]._float += stack[mach->sp + 1]._float;
            mach->mem[(da + 8) / 4]._float += stack[mach->sp + 2]._float;
            mach->mem[(da + 12) / 4]._float += stack[mach->sp + 3]._float;
            mach->sp += 4;
         }
         break;
      case slang_asm_vec4_subtract:
         /* [vec4] | vec4 > [vec4] */
         {
            GLuint da = stack[mach->sp + 4]._addr;
            mach->mem[da / 4]._float -= stack[mach->sp]._float;
            mach->mem[(da + 4) / 4]._float -= stack[mach->sp + 1]._float;
            mach->mem[(da + 8) / 4]._float -= stack[mach->sp + 2]._float;
            mach->mem[(da + 12) / 4]._float -= stack[mach->sp + 3]._float;
            mach->sp += 4;
         }
         break;
      case slang_asm_vec4_multiply:
         /* [vec4] | vec4 > [vec4] */
         {
            GLuint da = stack[mach->sp + 4]._addr;
            mach->mem[da / 4]._float *= stack[mach->sp]._float;
            mach->mem[(da + 4) / 4]._float *= stack[mach->sp + 1]._float;
            mach->mem[(da + 8) / 4]._float *= stack[mach->sp + 2]._float;
            mach->mem[(da + 12) / 4]._float *= stack[mach->sp + 3]._float;
            mach->sp += 4;
         }
         break;
      case slang_asm_vec4_divide:
         /* [vec4] | vec4 > [vec4] */
         {
            GLuint da = stack[mach->sp + 4]._addr;
            mach->mem[da / 4]._float /= stack[mach->sp]._float;
            mach->mem[(da + 4) / 4]._float /= stack[mach->sp + 1]._float;
            mach->mem[(da + 8) / 4]._float /= stack[mach->sp + 2]._float;
            mach->mem[(da + 12) / 4]._float /= stack[mach->sp + 3]._float;
            mach->sp += 4;
         }
         break;
      case slang_asm_vec4_negate:
         /* [vec4] > [vec4] */
         {
            GLuint da = stack[mach->sp]._addr;
            mach->mem[da / 4]._float = -mach->mem[da / 4]._float;
            mach->mem[(da + 4) / 4]._float = -mach->mem[(da + 4) / 4]._float;
            mach->mem[(da + 8) / 4]._float = -mach->mem[(da + 8) / 4]._float;
            mach->mem[(da + 12) / 4]._float =
               -mach->mem[(da + 12) / 4]._float;
         }
         break;
      case slang_asm_vec4_dot:
         /* [vec4] | vec4 > [float] */
         {
            GLuint da = stack[mach->sp + 4]._addr;
            mach->mem[da / 4]._float =
               mach->mem[da / 4]._float * stack[mach->sp]._float +
               mach->mem[(da + 4) / 4]._float * stack[mach->sp + 1]._float +
               mach->mem[(da + 8) / 4]._float * stack[mach->sp + 2]._float +
               mach->mem[(da + 12) / 4]._float * stack[mach->sp + 3]._float;
            mach->sp += 4;
         }
         break;
      case slang_asm_vec4_copy:
         /* [vec4] | vec4 > [vec4] */
         {
            GLuint da = stack[mach->sp + a->param[0] / 4]._addr + a->param[1];
            mach->mem[da / 4]._float = stack[mach->sp]._float;
            mach->mem[(da + 4) / 4]._float = stack[mach->sp + 1]._float;
            mach->mem[(da + 8) / 4]._float = stack[mach->sp + 2]._float;
            mach->mem[(da + 12) / 4]._float = stack[mach->sp + 3]._float;
            mach->sp += 4;
         }
         break;
      case slang_asm_vec4_deref:
         /* [vec4] > vec4 */
         {
            GLuint sa = stack[mach->sp]._addr;
            mach->sp -= 3;
            stack[mach->sp]._float = mach->mem[sa / 4]._float;
            stack[mach->sp + 1]._float = mach->mem[(sa + 4) / 4]._float;
            stack[mach->sp + 2]._float = mach->mem[(sa + 8) / 4]._float;
            stack[mach->sp + 3]._float = mach->mem[(sa + 12) / 4]._float;
         }
         break;
      case slang_asm_vec4_equal_int:
         {
            GLuint sp0 = mach->sp + a->param[0] / 4;
            GLuint sp1 = mach->sp + a->param[1] / 4;
            mach->sp--;
            if (stack[sp0]._float == stack[sp1]._float &&
                stack[sp0 + 1]._float == stack[sp1 + 1]._float &&
                stack[sp0 + 2]._float == stack[sp1 + 2]._float &&
                stack[sp0 + 3]._float == stack[sp1 + 3]._float) {
               stack[mach->sp]._float = 1.0f;
            }
            else {
               stack[mach->sp]._float = 0.0f;
            }
         }
         break;
      default:
         _mesa_problem(NULL, "bad slang opcode 0x%x", a->type);
         return GL_FALSE;
      }
   }

#if DEBUG_SLANG
   if (f != NULL)
      fclose(f);
#endif

   return GL_TRUE;
}
