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

/**
 * \file slang_execute_x86.c
 * x86 back end compiler
 * \author Michal Krol, Keith Whitwell
 */

#include "imports.h"
#include "slang_compile.h"
#include "slang_execute.h"
#include "slang_library_noise.h"
#include "slang_library_texsample.h"

#if defined(USE_X86_ASM) || defined(SLANG_X86)

#include "x86/rtasm/x86sse.h"

typedef struct
{
   GLuint index;
   GLubyte *csr;
} fixup;

typedef struct
{
   struct x86_function f;
   struct x86_reg r_eax;
   struct x86_reg r_ecx;
   struct x86_reg r_edx;
   struct x86_reg r_ebx;
   struct x86_reg r_esp;
   struct x86_reg r_ebp;
   struct x86_reg r_st0;
   struct x86_reg r_st1;
   struct x86_reg r_st2;
   struct x86_reg r_st3;
   struct x86_reg r_st4;
   fixup *fixups;
   GLuint fixup_count;
   GLubyte **labels;
   slang_machine *mach;
   GLubyte *l_discard;
   GLubyte *l_exit;
   GLshort fpucntl;
} codegen_ctx;

static GLvoid
add_fixup(codegen_ctx * G, GLuint index, GLubyte * csr)
{
   G->fixups =
      (fixup *) slang_alloc_realloc(G->fixups, G->fixup_count * sizeof(fixup),
                                    (G->fixup_count + 1) * sizeof(fixup));
   G->fixups[G->fixup_count].index = index;
   G->fixups[G->fixup_count].csr = csr;
   G->fixup_count++;
}

#ifdef NO_FAST_MATH
#define RESTORE_FPU (DEFAULT_X86_FPU)
#define RND_NEG_FPU (DEFAULT_X86_FPU | 0x400)
#else
#define RESTORE_FPU (FAST_X86_FPU)
#define RND_NEG_FPU (FAST_X86_FPU | 0x400)
#endif

#if 0

/*
 * XXX
 * These should produce a valid code that computes powers.
 * Unfortunately, it does not.
 */
static void
set_fpu_round_neg_inf(codegen_ctx * G)
{
   if (G->fpucntl != RND_NEG_FPU) {
      G->fpucntl = RND_NEG_FPU;
      x87_fnclex(&G->f);
      x86_mov_reg_imm(&G->f, G->r_eax,
                      (GLint) & G->mach->x86.fpucntl_rnd_neg);
      x87_fldcw(&G->f, x86_deref(G->r_eax));
   }
}

static void
emit_x87_ex2(codegen_ctx * G)
{
   set_fpu_round_neg_inf(G);

   x87_fld(&G->f, G->r_st0);    /* a a */
   x87_fprndint(&G->f);         /* int(a) a */
   x87_fld(&G->f, G->r_st0);    /* int(a) int(a) a */
   x87_fstp(&G->f, G->r_st3);   /* int(a) a int(a) */
   x87_fsubp(&G->f, G->r_st1);  /* frac(a) int(a) */
   x87_f2xm1(&G->f);            /* (2^frac(a))-1 int(a) */
   x87_fld1(&G->f);             /* 1 (2^frac(a))-1 int(a) */
   x87_faddp(&G->f, G->r_st1);  /* 2^frac(a) int(a) */
   x87_fscale(&G->f);           /* 2^a */
}

static void
emit_pow(codegen_ctx * G)
{
   x87_fld(&G->f, x86_deref(G->r_esp));
   x87_fld(&G->f, x86_make_disp(G->r_esp, 4));
   x87_fyl2x(&G->f);
   emit_x87_ex2(G);
}

#endif

static GLfloat
do_ceilf(GLfloat x)
{
   return CEILF(x);
}

static GLfloat
do_floorf(GLfloat x)
{
   return FLOORF(x);
}

static GLfloat
do_ftoi(GLfloat x)
{
   return (GLfloat) ((GLint) (x));
}

static GLfloat
do_powf(GLfloat y, GLfloat x)
{
   return (GLfloat) _mesa_pow((GLdouble) x, (GLdouble) y);
}

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

static GLvoid
do_print_float(slang_info_log ** infolog, GLfloat x)
{
   _mesa_printf("slang print: %f\n", x);
   ensure_infolog_created(infolog);
   slang_info_log_print(*infolog, "%f", x);
}

static GLvoid
do_print_int(slang_info_log ** infolog, GLfloat x)
{
   _mesa_printf("slang print: %d\n", (GLint) (x));
   ensure_infolog_created(infolog);
   slang_info_log_print(*infolog, "%d", (GLint) (x));
}

static GLvoid
do_print_bool(slang_info_log ** infolog, GLfloat x)
{
   _mesa_printf("slang print: %s\n", (GLint) (x) ? "true" : "false");
   ensure_infolog_created(infolog);
   slang_info_log_print(*infolog, "%s", (GLint) (x) ? "true" : "false");
}

#define FLOAT_ONE 0x3f800000
#define FLOAT_ZERO 0

static GLvoid
codegen_assem(codegen_ctx * G, slang_assembly * a, slang_info_log ** infolog)
{
   GLint disp, i;

   switch (a->type) {
   case slang_asm_none:
      break;
   case slang_asm_float_copy:
   case slang_asm_int_copy:
   case slang_asm_bool_copy:
      x86_mov(&G->f, G->r_eax, x86_make_disp(G->r_esp, a->param[0]));
      x86_pop(&G->f, G->r_ecx);
      x86_mov(&G->f, x86_make_disp(G->r_eax, a->param[1]), G->r_ecx);
      break;
   case slang_asm_float_move:
   case slang_asm_int_move:
   case slang_asm_bool_move:
      x86_lea(&G->f, G->r_eax, x86_make_disp(G->r_esp, a->param[1]));
      x86_add(&G->f, G->r_eax, x86_deref(G->r_esp));
      x86_mov(&G->f, G->r_eax, x86_deref(G->r_eax));
      x86_mov(&G->f, x86_make_disp(G->r_esp, a->param[0]), G->r_eax);
      break;
   case slang_asm_float_push:
   case slang_asm_int_push:
   case slang_asm_bool_push:
      /* TODO: use push imm32 */
      x86_mov_reg_imm(&G->f, G->r_eax, *((GLint *) & a->literal));
      x86_push(&G->f, G->r_eax);
      break;
   case slang_asm_float_deref:
   case slang_asm_int_deref:
   case slang_asm_bool_deref:
   case slang_asm_addr_deref:
      x86_mov(&G->f, G->r_eax, x86_deref(G->r_esp));
      x86_mov(&G->f, G->r_eax, x86_deref(G->r_eax));
      x86_mov(&G->f, x86_deref(G->r_esp), G->r_eax);
      break;
   case slang_asm_float_add:
      x87_fld(&G->f, x86_make_disp(G->r_esp, 4));
      x87_fld(&G->f, x86_deref(G->r_esp));
      x87_faddp(&G->f, G->r_st1);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_multiply:
      x87_fld(&G->f, x86_make_disp(G->r_esp, 4));
      x87_fld(&G->f, x86_deref(G->r_esp));
      x87_fmulp(&G->f, G->r_st1);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_divide:
      x87_fld(&G->f, x86_make_disp(G->r_esp, 4));
      x87_fld(&G->f, x86_deref(G->r_esp));
      x87_fdivp(&G->f, G->r_st1);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_negate:
      x87_fld(&G->f, x86_deref(G->r_esp));
      x87_fchs(&G->f);
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_less:
      x87_fld(&G->f, x86_make_disp(G->r_esp, 4));
      x87_fcomp(&G->f, x86_deref(G->r_esp));
      x87_fnstsw(&G->f, G->r_eax);
      /* TODO: use test r8,imm8 */
      x86_mov_reg_imm(&G->f, G->r_ecx, 0x100);
      x86_test(&G->f, G->r_eax, G->r_ecx);
      {
         GLubyte *lab0, *lab1;
         /* TODO: use jcc rel8 */
         lab0 = x86_jcc_forward(&G->f, cc_E);
         x86_mov_reg_imm(&G->f, G->r_ecx, FLOAT_ONE);
         /* TODO: use jmp rel8 */
         lab1 = x86_jmp_forward(&G->f);
         x86_fixup_fwd_jump(&G->f, lab0);
         x86_mov_reg_imm(&G->f, G->r_ecx, FLOAT_ZERO);
         x86_fixup_fwd_jump(&G->f, lab1);
         x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
         x86_mov(&G->f, x86_deref(G->r_esp), G->r_ecx);
      }
      break;
   case slang_asm_float_equal_exp:
      x87_fld(&G->f, x86_make_disp(G->r_esp, 4));
      x87_fcomp(&G->f, x86_deref(G->r_esp));
      x87_fnstsw(&G->f, G->r_eax);
      /* TODO: use test r8,imm8 */
      x86_mov_reg_imm(&G->f, G->r_ecx, 0x4000);
      x86_test(&G->f, G->r_eax, G->r_ecx);
      {
         GLubyte *lab0, *lab1;
         /* TODO: use jcc rel8 */
         lab0 = x86_jcc_forward(&G->f, cc_E);
         x86_mov_reg_imm(&G->f, G->r_ecx, FLOAT_ONE);
         /* TODO: use jmp rel8 */
         lab1 = x86_jmp_forward(&G->f);
         x86_fixup_fwd_jump(&G->f, lab0);
         x86_mov_reg_imm(&G->f, G->r_ecx, FLOAT_ZERO);
         x86_fixup_fwd_jump(&G->f, lab1);
         x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
         x86_mov(&G->f, x86_deref(G->r_esp), G->r_ecx);
      }
      break;
   case slang_asm_float_equal_int:
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, -4));
      x87_fld(&G->f, x86_make_disp(G->r_esp, a->param[0] + 4));
      x87_fcomp(&G->f, x86_make_disp(G->r_esp, a->param[1] + 4));
      x87_fnstsw(&G->f, G->r_eax);
      /* TODO: use test r8,imm8 */
      x86_mov_reg_imm(&G->f, G->r_ecx, 0x4000);
      x86_test(&G->f, G->r_eax, G->r_ecx);
      {
         GLubyte *lab0, *lab1;
         /* TODO: use jcc rel8 */
         lab0 = x86_jcc_forward(&G->f, cc_E);
         x86_mov_reg_imm(&G->f, G->r_ecx, FLOAT_ONE);
         /* TODO: use jmp rel8 */
         lab1 = x86_jmp_forward(&G->f);
         x86_fixup_fwd_jump(&G->f, lab0);
         x86_mov_reg_imm(&G->f, G->r_ecx, FLOAT_ZERO);
         x86_fixup_fwd_jump(&G->f, lab1);
         x86_mov(&G->f, x86_deref(G->r_esp), G->r_ecx);
      }
      break;
   case slang_asm_float_to_int:
      /* TODO: use fistp without rounding */
      x86_call(&G->f, (GLubyte *) (do_ftoi));
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_sine:
      /* TODO: use fsin */
      x86_call(&G->f, (GLubyte *) _mesa_sinf);
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_arcsine:
      /* TODO: use fpatan (?) */
      x86_call(&G->f, (GLubyte *) _mesa_asinf);
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_arctan:
      /* TODO: use fpatan */
      x86_call(&G->f, (GLubyte *) _mesa_atanf);
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_power:
      /* TODO: use emit_pow() */
      x86_call(&G->f, (GLubyte *) do_powf);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_log2:
      x87_fld1(&G->f);
      x87_fld(&G->f, x86_deref(G->r_esp));
      x87_fyl2x(&G->f);
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_floor:
      x86_call(&G->f, (GLubyte *) do_floorf);
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_ceil:
      x86_call(&G->f, (GLubyte *) do_ceilf);
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_noise1:
      x86_call(&G->f, (GLubyte *) _slang_library_noise1);
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_noise2:
      x86_call(&G->f, (GLubyte *) _slang_library_noise2);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_noise3:
      x86_call(&G->f, (GLubyte *) _slang_library_noise4);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 8));
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_float_noise4:
      x86_call(&G->f, (GLubyte *) _slang_library_noise4);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 12));
      x87_fstp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_int_to_float:
      break;
   case slang_asm_int_to_addr:
      x87_fld(&G->f, x86_deref(G->r_esp));
      x87_fistp(&G->f, x86_deref(G->r_esp));
      break;
   case slang_asm_addr_copy:
      x86_pop(&G->f, G->r_eax);
      x86_mov(&G->f, G->r_ecx, x86_deref(G->r_esp));
      x86_mov(&G->f, x86_deref(G->r_ecx), G->r_eax);
      break;
   case slang_asm_addr_push:
      /* TODO: use push imm32 */
      x86_mov_reg_imm(&G->f, G->r_eax, (GLint) a->param[0]);
      x86_push(&G->f, G->r_eax);
      break;
   case slang_asm_addr_add:
      x86_pop(&G->f, G->r_eax);
      x86_add(&G->f, x86_deref(G->r_esp), G->r_eax);
      break;
   case slang_asm_addr_multiply:
      x86_pop(&G->f, G->r_ecx);
      x86_mov(&G->f, G->r_eax, x86_deref(G->r_esp));
      x86_mul(&G->f, G->r_ecx);
      x86_mov(&G->f, x86_deref(G->r_esp), G->r_eax);
      break;
   case slang_asm_vec4_tex1d:
      x86_call(&G->f, (GLubyte *) _slang_library_tex1d);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 12));
      break;
   case slang_asm_vec4_tex2d:
      x86_call(&G->f, (GLubyte *) _slang_library_tex2d);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 16));
      break;
   case slang_asm_vec4_tex3d:
      x86_call(&G->f, (GLubyte *) _slang_library_tex3d);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 20));
      break;
   case slang_asm_vec4_texcube:
      x86_call(&G->f, (GLubyte *) _slang_library_texcube);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 20));
      break;
   case slang_asm_vec4_shad1d:
      x86_call(&G->f, (GLubyte *) _slang_library_shad1d);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 20));
      break;
   case slang_asm_vec4_shad2d:
      x86_call(&G->f, (GLubyte *) _slang_library_shad2d);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 20));
      break;
   case slang_asm_jump:
      add_fixup(G, a->param[0], x86_jmp_forward(&G->f));
      break;
   case slang_asm_jump_if_zero:
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      x86_xor(&G->f, G->r_eax, G->r_eax);
      x86_cmp(&G->f, G->r_eax, x86_make_disp(G->r_esp, -4));
      {
         GLubyte *lab0;
         /* TODO: use jcc rel8 */
         lab0 = x86_jcc_forward(&G->f, cc_NE);
         add_fixup(G, a->param[0], x86_jmp_forward(&G->f));
         x86_fixup_fwd_jump(&G->f, lab0);
      }
      break;
   case slang_asm_enter:
      /* FIXME: x86_make_disp(esp, 0) + x86_lea() generates bogus code */
      assert(a->param[0] != 0);
      x86_push(&G->f, G->r_ebp);
      x86_lea(&G->f, G->r_ebp, x86_make_disp(G->r_esp, (GLint) a->param[0]));
      break;
   case slang_asm_leave:
      x86_pop(&G->f, G->r_ebp);
      break;
   case slang_asm_local_alloc:
      /* FIXME: x86_make_disp(esp, 0) + x86_lea() generates bogus code */
      assert(a->param[0] != 0);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, -(GLint) a->param[0]));
      break;
   case slang_asm_local_free:
      /* FIXME: x86_make_disp(esp, 0) + x86_lea() generates bogus code */
      assert(a->param[0] != 0);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, (GLint) a->param[0]));
      break;
   case slang_asm_local_addr:
      disp = -(GLint) (a->param[0] + a->param[1]) + 4;
      if (disp != 0) {
         x86_lea(&G->f, G->r_eax, x86_make_disp(G->r_ebp, disp));
         x86_push(&G->f, G->r_eax);
      }
      else
         x86_push(&G->f, G->r_ebp);
      break;
   case slang_asm_global_addr:
      /* TODO: use push imm32 */
      x86_mov_reg_imm(&G->f, G->r_eax, (GLint) & G->mach->mem + a->param[0]);
      x86_push(&G->f, G->r_eax);
      break;
   case slang_asm_call:
      add_fixup(G, a->param[0], x86_call_forward(&G->f));
      break;
   case slang_asm_return:
      x86_ret(&G->f);
      break;
   case slang_asm_discard:
      x86_jmp(&G->f, G->l_discard);
      break;
   case slang_asm_exit:
      x86_jmp(&G->f, G->l_exit);
      break;
      /* GL_MESA_shader_debug */
   case slang_asm_float_print:
      /* TODO: use push imm32 */
      x86_mov_reg_imm(&G->f, G->r_eax, (GLint) (infolog));
      x86_push(&G->f, G->r_eax);
      x86_call(&G->f, (GLubyte *) (do_print_float));
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      break;
   case slang_asm_int_print:
      /* TODO: use push imm32 */
      x86_mov_reg_imm(&G->f, G->r_eax, (GLint) (infolog));
      x86_push(&G->f, G->r_eax);
      x86_call(&G->f, (GLubyte *) do_print_int);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      break;
   case slang_asm_bool_print:
      /* TODO: use push imm32 */
      x86_mov_reg_imm(&G->f, G->r_eax, (GLint) (infolog));
      x86_push(&G->f, G->r_eax);
      x86_call(&G->f, (GLubyte *) do_print_bool);
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      break;
      /* vec4 */
   case slang_asm_float_to_vec4:
      /* [vec4] | float > [vec4] */
      x87_fld(&G->f, x86_deref(G->r_esp));
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 4));
      x86_mov(&G->f, G->r_eax, x86_deref(G->r_esp));
      x87_fst(&G->f, x86_make_disp(G->r_eax, 12));
      x87_fst(&G->f, x86_make_disp(G->r_eax, 8));
      x87_fst(&G->f, x86_make_disp(G->r_eax, 4));
      x87_fstp(&G->f, x86_deref(G->r_eax));
      break;
   case slang_asm_vec4_add:
      /* [vec4] | vec4 > [vec4] */
      x86_mov(&G->f, G->r_eax, x86_make_disp(G->r_esp, 16));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_eax, i * 4));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_esp, i * 4));
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 16));
      for (i = 0; i < 4; i++)
         x87_faddp(&G->f, G->r_st4);
      for (i = 0; i < 4; i++)
         x87_fstp(&G->f, x86_make_disp(G->r_eax, 12 - i * 4));
      break;
   case slang_asm_vec4_subtract:
      /* [vec4] | vec4 > [vec4] */
      x86_mov(&G->f, G->r_eax, x86_make_disp(G->r_esp, 16));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_eax, i * 4));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_esp, i * 4));
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 16));
      for (i = 0; i < 4; i++)
         x87_fsubp(&G->f, G->r_st4);
      for (i = 0; i < 4; i++)
         x87_fstp(&G->f, x86_make_disp(G->r_eax, 12 - i * 4));
      break;
   case slang_asm_vec4_multiply:
      /* [vec4] | vec4 > [vec4] */
      x86_mov(&G->f, G->r_eax, x86_make_disp(G->r_esp, 16));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_eax, i * 4));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_esp, i * 4));
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 16));
      for (i = 0; i < 4; i++)
         x87_fmulp(&G->f, G->r_st4);
      for (i = 0; i < 4; i++)
         x87_fstp(&G->f, x86_make_disp(G->r_eax, 12 - i * 4));
      break;
   case slang_asm_vec4_divide:
      /* [vec4] | vec4 > [vec4] */
      x86_mov(&G->f, G->r_eax, x86_make_disp(G->r_esp, 16));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_eax, i * 4));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_esp, i * 4));
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 16));
      for (i = 0; i < 4; i++)
         x87_fdivp(&G->f, G->r_st4);
      for (i = 0; i < 4; i++)
         x87_fstp(&G->f, x86_make_disp(G->r_eax, 12 - i * 4));
      break;
   case slang_asm_vec4_negate:
      /* [vec4] > [vec4] */
      x86_mov(&G->f, G->r_eax, x86_deref(G->r_esp));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_eax, i * 4));
      for (i = 0; i < 4; i++) {
         x87_fchs(&G->f);
         x87_fstp(&G->f, x86_make_disp(G->r_eax, 12 - i * 4));
      }
      break;
   case slang_asm_vec4_dot:
      /* [vec4] | vec4 > [float] */
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_esp, i * 4));
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, 16));
      x86_mov(&G->f, G->r_eax, x86_deref(G->r_esp));
      for (i = 0; i < 4; i++)
         x87_fld(&G->f, x86_make_disp(G->r_eax, i * 4));
      for (i = 0; i < 4; i++)
         x87_fmulp(&G->f, G->r_st4);
      for (i = 0; i < 3; i++)
         x87_faddp(&G->f, G->r_st1);
      x87_fstp(&G->f, x86_deref(G->r_eax));
      break;
   case slang_asm_vec4_copy:
      /* [vec4] | vec4 > [vec4] */
      x86_mov(&G->f, G->r_eax, x86_make_disp(G->r_esp, a->param[0]));
      x86_pop(&G->f, G->r_ecx);
      x86_pop(&G->f, G->r_edx);
      x86_mov(&G->f, x86_make_disp(G->r_eax, a->param[1]), G->r_ecx);
      x86_pop(&G->f, G->r_ebx);
      x86_mov(&G->f, x86_make_disp(G->r_eax, a->param[1] + 4), G->r_edx);
      x86_pop(&G->f, G->r_ecx);
      x86_mov(&G->f, x86_make_disp(G->r_eax, a->param[1] + 8), G->r_ebx);
      x86_mov(&G->f, x86_make_disp(G->r_eax, a->param[1] + 12), G->r_ecx);
      break;
   case slang_asm_vec4_deref:
      /* [vec4] > vec4 */
      x86_mov(&G->f, G->r_eax, x86_deref(G->r_esp));
      x86_mov(&G->f, G->r_ecx, x86_make_disp(G->r_eax, 12));
      x86_mov(&G->f, G->r_edx, x86_make_disp(G->r_eax, 8));
      x86_mov(&G->f, x86_deref(G->r_esp), G->r_ecx);
      x86_mov(&G->f, G->r_ebx, x86_make_disp(G->r_eax, 4));
      x86_push(&G->f, G->r_edx);
      x86_mov(&G->f, G->r_ecx, x86_deref(G->r_eax));
      x86_push(&G->f, G->r_ebx);
      x86_push(&G->f, G->r_ecx);
      break;
   case slang_asm_vec4_equal_int:
      x86_lea(&G->f, G->r_esp, x86_make_disp(G->r_esp, -4));
      x86_mov_reg_imm(&G->f, G->r_edx, 0x4000);
      for (i = 0; i < 4; i++) {
         x87_fld(&G->f, x86_make_disp(G->r_esp, a->param[0] + 4 + i * 4));
         x87_fcomp(&G->f, x86_make_disp(G->r_esp, a->param[1] + 4 + i * 4));
         x87_fnstsw(&G->f, G->r_eax);
         x86_and(&G->f, G->r_edx, G->r_eax);
      }
      /* TODO: use test r8,imm8 */
      x86_mov_reg_imm(&G->f, G->r_ecx, 0x4000);
      x86_test(&G->f, G->r_edx, G->r_ecx);
      {
         GLubyte *lab0, *lab1;

         /* TODO: use jcc rel8 */
         lab0 = x86_jcc_forward(&G->f, cc_E);
         x86_mov_reg_imm(&G->f, G->r_ecx, FLOAT_ONE);
         /* TODO: use jmp rel8 */
         lab1 = x86_jmp_forward(&G->f);
         x86_fixup_fwd_jump(&G->f, lab0);
         x86_mov_reg_imm(&G->f, G->r_ecx, FLOAT_ZERO);
         x86_fixup_fwd_jump(&G->f, lab1);
         x86_mov(&G->f, x86_deref(G->r_esp), G->r_ecx);
      }
      break;
   default:
      _mesa_problem(NULL, "Unexpected switch case in codegen_assem");
   }
}

GLboolean
_slang_x86_codegen(slang_machine * mach, slang_assembly_file * file,
                   GLuint start)
{
   codegen_ctx G;
   GLubyte *j_body, *j_exit;
   GLuint i;

   /* Free the old code - if any.
    */
   if (mach->x86.compiled_func != NULL) {
      _mesa_exec_free(mach->x86.compiled_func);
      mach->x86.compiled_func = NULL;
   }

   /*
    * We need as much as 1M because *all* assembly, including built-in library, is
    * being translated to x86.
    * The built-in library occupies 450K, so we can be safe for now.
    * It is going to change in the future, when we get assembly analysis running.
    */
   x86_init_func_size(&G.f, 1048576);
   G.r_eax = x86_make_reg(file_REG32, reg_AX);
   G.r_ecx = x86_make_reg(file_REG32, reg_CX);
   G.r_edx = x86_make_reg(file_REG32, reg_DX);
   G.r_ebx = x86_make_reg(file_REG32, reg_BX);
   G.r_esp = x86_make_reg(file_REG32, reg_SP);
   G.r_ebp = x86_make_reg(file_REG32, reg_BP);
   G.r_st0 = x86_make_reg(file_x87, 0);
   G.r_st1 = x86_make_reg(file_x87, 1);
   G.r_st2 = x86_make_reg(file_x87, 2);
   G.r_st3 = x86_make_reg(file_x87, 3);
   G.r_st4 = x86_make_reg(file_x87, 4);
   G.fixups = NULL;
   G.fixup_count = 0;
   G.labels =
      (GLubyte **) slang_alloc_malloc(file->count * sizeof(GLubyte *));
   G.mach = mach;
   G.fpucntl = RESTORE_FPU;

   mach->x86.fpucntl_rnd_neg = RND_NEG_FPU;
   mach->x86.fpucntl_restore = RESTORE_FPU;

   /* prepare stack and jump to start */
   x86_push(&G.f, G.r_ebp);
   x86_mov_reg_imm(&G.f, G.r_eax, (GLint) & mach->x86.esp_restore);
   x86_push(&G.f, G.r_esp);
   x86_pop(&G.f, G.r_ecx);
   x86_mov(&G.f, x86_deref(G.r_eax), G.r_ecx);
   j_body = x86_jmp_forward(&G.f);

   /* "discard" instructions jump to this label */
   G.l_discard = x86_get_label(&G.f);
   x86_mov_reg_imm(&G.f, G.r_eax, (GLint) & G.mach->kill);
   x86_mov_reg_imm(&G.f, G.r_ecx, 1);
   x86_mov(&G.f, x86_deref(G.r_eax), G.r_ecx);
   G.l_exit = x86_get_label(&G.f);
   j_exit = x86_jmp_forward(&G.f);

   for (i = 0; i < file->count; i++) {
      G.labels[i] = x86_get_label(&G.f);
      if (i == start)
         x86_fixup_fwd_jump(&G.f, j_body);
      codegen_assem(&G, &file->code[i], &mach->infolog);
   }

   /*
    * Restore stack and return.
    * This must be handled this way, because "discard" can be invoked from any
    * place in the code.
    */
   x86_fixup_fwd_jump(&G.f, j_exit);
   x86_mov_reg_imm(&G.f, G.r_eax, (GLint) & mach->x86.esp_restore);
   x86_mov(&G.f, G.r_esp, x86_deref(G.r_eax));
   x86_pop(&G.f, G.r_ebp);
   if (G.fpucntl != RESTORE_FPU) {
      x87_fnclex(&G.f);
      x86_mov_reg_imm(&G.f, G.r_eax, (GLint) & G.mach->x86.fpucntl_restore);
      x87_fldcw(&G.f, x86_deref(G.r_eax));
   }
   x86_ret(&G.f);

   /* fixup forward labels */
   for (i = 0; i < G.fixup_count; i++) {
      G.f.csr = G.labels[G.fixups[i].index];
      x86_fixup_fwd_jump(&G.f, G.fixups[i].csr);
   }

   slang_alloc_free(G.fixups);
   slang_alloc_free(G.labels);

   /* install new code */
   mach->x86.compiled_func = (GLvoid(*)(slang_machine *)) x86_get_func(&G.f);

   return GL_TRUE;
}

#endif
