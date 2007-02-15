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

#ifndef SLANG_EXECUTE_H
#define SLANG_EXECUTE_H

#include "slang_assemble.h"

#if defined __cplusplus
extern "C" {
#endif


/**
 * A memory location
 */
typedef union slang_machine_slot_
{
   GLfloat _float;
   GLuint _addr;
} slang_machine_slot;

#define SLANG_MACHINE_GLOBAL_SIZE 3072
#define SLANG_MACHINE_STACK_SIZE 1024
#define SLANG_MACHINE_MEMORY_SIZE (SLANG_MACHINE_GLOBAL_SIZE + SLANG_MACHINE_STACK_SIZE)


#if defined(USE_X86_ASM) || defined(SLANG_X86)
/**
 * Extra machine state for x86 execution.
 */
typedef struct
{
   GLvoid(*compiled_func) (struct slang_machine_ *);
   GLuint esp_restore;
   GLshort fpucntl_rnd_neg;
   GLshort fpucntl_restore;
} slang_machine_x86;
#endif


/**
 * Runtime shader machine state.
 */
typedef struct slang_machine_
{
   GLuint ip;               /**< instruction pointer, for flow control */
   GLuint sp;               /**< stack pointer, for stack access */
   GLuint bp;               /**< base pointer, for local variable access */
   GLboolean kill;          /**< discard the fragment? */
   GLboolean exit;          /**< terminate the shader */
   /** Machine memory */
   slang_machine_slot mem[SLANG_MACHINE_MEMORY_SIZE];
   struct slang_info_log_ *infolog;     /**< printMESA() support */
#if defined(USE_X86_ASM) || defined(SLANG_X86)
   slang_machine_x86 x86;
#endif
} slang_machine;


extern GLvoid
slang_machine_ctr(slang_machine *);

extern GLvoid
slang_machine_dtr(slang_machine *);

extern void
slang_machine_init(slang_machine *);

extern GLboolean
_slang_execute2(const slang_assembly_file *, slang_machine *);


#if defined(USE_X86_ASM) || defined(SLANG_X86)
extern GLboolean
_slang_x86_codegen(slang_machine *, slang_assembly_file *, GLuint);
#endif


#ifdef __cplusplus
}
#endif

#endif
