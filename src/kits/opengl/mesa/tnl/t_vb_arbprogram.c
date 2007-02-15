/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * \file t_arb_program.c
 * Compile vertex programs to an intermediate representation.
 * Execute vertex programs over a buffer of vertices.
 * \author Keith Whitwell, Brian Paul
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "arbprogparse.h"
#include "light.h"
#include "program.h"
#include "math/m_matrix.h"
#include "t_context.h"
#include "t_pipeline.h"
#include "t_vb_arbprogram.h"
#include "tnl.h"
#include "program_instruction.h"


#define DISASSEM 0


struct compilation {
   GLuint reg_active;
   union instruction *csr;
};


#define ARB_VP_MACHINE(stage) ((struct arb_vp_machine *)(stage->privatePtr))

#define PUFF(x) ((x)[1] = (x)[2] = (x)[3] = (x)[0])



/* Lower precision functions for the EXP, LOG and LIT opcodes.  The
 * LOG2() implementation is probably not accurate enough, and the
 * attempted optimization for Exp2 is definitely not accurate
 * enough - it discards all of t's fractional bits!
 */
static GLfloat RoughApproxLog2(GLfloat t)
{
   return LOG2(t);
}

static GLfloat RoughApproxExp2(GLfloat t)
{   
#if 0
   fi_type fi;
   fi.i = (GLint) t;
   fi.i = (fi.i << 23) + 0x3f800000;
   return fi.f;
#else
   return (GLfloat) _mesa_pow(2.0, t);
#endif
}

static GLfloat RoughApproxPower(GLfloat x, GLfloat y)
{
   if (x == 0.0 && y == 0.0)
      return 1.0;  /* spec requires this */
   else
      return RoughApproxExp2(y * RoughApproxLog2(x));
}


/* Higher precision functions for the EX2, LG2 and POW opcodes:
 */
static GLfloat ApproxLog2(GLfloat t)
{
   return (GLfloat) (LOGF(t) * 1.442695F);
}

static GLfloat ApproxExp2(GLfloat t)
{   
   return (GLfloat) _mesa_pow(2.0, t);
}

static GLfloat ApproxPower(GLfloat x, GLfloat y)
{
   return (GLfloat) _mesa_pow(x, y);
}


/**
 * Perform a reduced swizzle:
 */
static void do_RSW( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.rsw.dst];
   const GLfloat *arg0 = m->File[op.rsw.file0][op.rsw.idx0];
   const GLuint swz = op.rsw.swz;
   const GLuint neg = op.rsw.neg;
   GLfloat tmp[4];

   /* Need a temporary to be correct in the case where result == arg0.
    */
   COPY_4V(tmp, arg0);

   result[0] = tmp[GET_SWZ(swz, 0)];
   result[1] = tmp[GET_SWZ(swz, 1)];
   result[2] = tmp[GET_SWZ(swz, 2)];
   result[3] = tmp[GET_SWZ(swz, 3)];

   if (neg) {
      if (neg & 0x1) result[0] = -result[0];
      if (neg & 0x2) result[1] = -result[1];
      if (neg & 0x4) result[2] = -result[2];
      if (neg & 0x8) result[3] = -result[3];
   }
}

/**
 * Perform a full swizzle
 */
static void do_SWZ( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.rsw.dst];
   const GLfloat *arg0 = m->File[op.rsw.file0][op.rsw.idx0];
   const GLuint swz = op.rsw.swz;
   const GLuint neg = op.rsw.neg;
   GLfloat tmp[6];
   tmp[4] = 0.0;
   tmp[5] = 1.0;

   /* Need a temporary to be correct in the case where result == arg0.
    */
   COPY_4V(tmp, arg0);

   result[0] = tmp[GET_SWZ(swz, 0)];
   result[1] = tmp[GET_SWZ(swz, 1)];
   result[2] = tmp[GET_SWZ(swz, 2)];
   result[3] = tmp[GET_SWZ(swz, 3)];

   if (neg) {
      if (neg & 0x1) result[0] = -result[0];
      if (neg & 0x2) result[1] = -result[1];
      if (neg & 0x4) result[2] = -result[2];
      if (neg & 0x8) result[3] = -result[3];
   }
}

/* Used to implement write masking.  To make things easier for the sse
 * generator I've gone back to a 1 argument version of this function
 * (dst.msk = arg), rather than the semantically cleaner (dst = SEL
 * arg0, arg1, msk)
 *
 * That means this is the only instruction which doesn't write a full
 * 4 dwords out.  This would make such a program harder to analyse,
 * but it looks like analysis is going to take place on a higher level
 * anyway.
 */
static void do_MSK( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *dst = m->File[0][op.msk.dst];
   const GLfloat *arg = m->File[op.msk.file][op.msk.idx];
 
   if (op.msk.mask & WRITEMASK_X) dst[0] = arg[0];
   if (op.msk.mask & WRITEMASK_Y) dst[1] = arg[1];
   if (op.msk.mask & WRITEMASK_Z) dst[2] = arg[2];
   if (op.msk.mask & WRITEMASK_W) dst[3] = arg[3];
}


static void do_PRT( struct arb_vp_machine *m, union instruction op )
{
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   
   _mesa_printf("%d: %f %f %f %f\n", m->vtx_nr, 
		arg0[0], arg0[1], arg0[2], arg0[3]);
}


/**
 * The traditional ALU and texturing instructions.  All operate on
 * internal registers and ignore write masks and swizzling issues.
 */

static void do_ABS( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = (arg0[0] < 0.0) ? -arg0[0] : arg0[0];
   result[1] = (arg0[1] < 0.0) ? -arg0[1] : arg0[1];
   result[2] = (arg0[2] < 0.0) ? -arg0[2] : arg0[2];
   result[3] = (arg0[3] < 0.0) ? -arg0[3] : arg0[3];
}

static void do_ADD( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = arg0[0] + arg1[0];
   result[1] = arg0[1] + arg1[1];
   result[2] = arg0[2] + arg1[2];
   result[3] = arg0[3] + arg1[3];
}


static void do_DP3( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] * arg1[0] + 
		arg0[1] * arg1[1] + 
		arg0[2] * arg1[2]);

   PUFF(result);
}



static void do_DP4( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] * arg1[0] + 
		arg0[1] * arg1[1] + 
		arg0[2] * arg1[2] + 
		arg0[3] * arg1[3]);

   PUFF(result);
}

static void do_DPH( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] * arg1[0] + 
		arg0[1] * arg1[1] + 
		arg0[2] * arg1[2] + 
		1.0     * arg1[3]);
   
   PUFF(result);
}

static void do_DST( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   /* This should be ok even if result == arg0 or result == arg1.
    */
   result[0] = 1.0F;
   result[1] = arg0[1] * arg1[1];
   result[2] = arg0[2];
   result[3] = arg1[3];
}


/* Intended to be high precision:
 */
static void do_EX2( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = (GLfloat)ApproxExp2(arg0[0]);
   PUFF(result);
}


/* Allowed to be lower precision:
 */
static void do_EXP( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat tmp = arg0[0];
   const GLfloat flr_tmp = FLOORF(tmp);
   const GLfloat frac_tmp = tmp - flr_tmp;

   result[0] = LDEXPF(1.0, (int)flr_tmp);
   result[1] = frac_tmp;
   result[2] = RoughApproxExp2(tmp);
   result[3] = 1.0F;
}

static void do_FLR( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = FLOORF(arg0[0]);
   result[1] = FLOORF(arg0[1]);
   result[2] = FLOORF(arg0[2]);
   result[3] = FLOORF(arg0[3]);
}

static void do_FRC( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = arg0[0] - FLOORF(arg0[0]);
   result[1] = arg0[1] - FLOORF(arg0[1]);
   result[2] = arg0[2] - FLOORF(arg0[2]);
   result[3] = arg0[3] - FLOORF(arg0[3]);
}

/* High precision log base 2:
 */
static void do_LG2( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = ApproxLog2(arg0[0]);
   PUFF(result);
}



static void do_LIT( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   GLfloat tmp[4]; /* use temp in case arg0 == result register */

   tmp[0] = 1.0;
   tmp[1] = arg0[0];
   if (arg0[0] > 0.0) {
      tmp[2] = RoughApproxPower(arg0[1], arg0[3]);
   }
   else {
      tmp[2] = 0.0;
   }
   tmp[3] = 1.0;

   COPY_4V(result, tmp);
}


/* Intended to allow a lower precision than required for LG2 above.
 */
static void do_LOG( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat tmp = FABSF(arg0[0]);
   int exponent;
   const GLfloat mantissa = FREXPF(tmp, &exponent);

   result[0] = (GLfloat) (exponent - 1);
   result[1] = 2.0 * mantissa; /* map [.5, 1) -> [1, 2) */
   result[2] = exponent + LOG2(mantissa);
   result[3] = 1.0;
}

static void do_MAX( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] > arg1[0]) ? arg0[0] : arg1[0];
   result[1] = (arg0[1] > arg1[1]) ? arg0[1] : arg1[1];
   result[2] = (arg0[2] > arg1[2]) ? arg0[2] : arg1[2];
   result[3] = (arg0[3] > arg1[3]) ? arg0[3] : arg1[3];
}


static void do_MIN( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] < arg1[0]) ? arg0[0] : arg1[0];
   result[1] = (arg0[1] < arg1[1]) ? arg0[1] : arg1[1];
   result[2] = (arg0[2] < arg1[2]) ? arg0[2] : arg1[2];
   result[3] = (arg0[3] < arg1[3]) ? arg0[3] : arg1[3];
}

static void do_MOV( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = arg0[0];
   result[1] = arg0[1];
   result[2] = arg0[2];
   result[3] = arg0[3];
}

static void do_MUL( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = arg0[0] * arg1[0];
   result[1] = arg0[1] * arg1[1];
   result[2] = arg0[2] * arg1[2];
   result[3] = arg0[3] * arg1[3];
}


/* Intended to be "high" precision
 */
static void do_POW( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (GLfloat)ApproxPower(arg0[0], arg1[0]);
   PUFF(result);
}

static void do_REL( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLuint idx = (op.alu.idx0 + (GLint)m->File[0][REG_ADDR][0]) & (MAX_NV_VERTEX_PROGRAM_PARAMS-1);
   const GLfloat *arg0 = m->File[op.alu.file0][idx];

   result[0] = arg0[0];
   result[1] = arg0[1];
   result[2] = arg0[2];
   result[3] = arg0[3];
}

static void do_RCP( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = 1.0F / arg0[0];  
   PUFF(result);
}

static void do_RSQ( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = INV_SQRTF(FABSF(arg0[0]));
   PUFF(result);
}


static void do_SGE( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] >= arg1[0]) ? 1.0F : 0.0F;
   result[1] = (arg0[1] >= arg1[1]) ? 1.0F : 0.0F;
   result[2] = (arg0[2] >= arg1[2]) ? 1.0F : 0.0F;
   result[3] = (arg0[3] >= arg1[3]) ? 1.0F : 0.0F;
}


static void do_SLT( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] < arg1[0]) ? 1.0F : 0.0F;
   result[1] = (arg0[1] < arg1[1]) ? 1.0F : 0.0F;
   result[2] = (arg0[2] < arg1[2]) ? 1.0F : 0.0F;
   result[3] = (arg0[3] < arg1[3]) ? 1.0F : 0.0F;
}

static void do_SUB( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = arg0[0] - arg1[0];
   result[1] = arg0[1] - arg1[1];
   result[2] = arg0[2] - arg1[2];
   result[3] = arg0[3] - arg1[3];
}


static void do_XPD( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->File[0][op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];
   GLfloat tmp[3];

   tmp[0] = arg0[1] * arg1[2] - arg0[2] * arg1[1];
   tmp[1] = arg0[2] * arg1[0] - arg0[0] * arg1[2];
   tmp[2] = arg0[0] * arg1[1] - arg0[1] * arg1[0];

   /* Need a temporary to be correct in the case where result == arg0
    * or result == arg1.
    */
   result[0] = tmp[0];
   result[1] = tmp[1];
   result[2] = tmp[2];
}

static void do_NOP( struct arb_vp_machine *m, union instruction op ) 
{
}

/* Some useful debugging functions:
 */
static void print_mask( GLuint mask )
{
   _mesa_printf(".");
   if (mask & WRITEMASK_X) _mesa_printf("x");
   if (mask & WRITEMASK_Y) _mesa_printf("y");
   if (mask & WRITEMASK_Z) _mesa_printf("z");
   if (mask & WRITEMASK_W) _mesa_printf("w");
}

static void print_reg( GLuint file, GLuint reg )
{
   static const char *reg_file[] = {
      "REG",
      "LOCAL_PARAM",
      "ENV_PARAM",
      "STATE_VAR",
   };

   if (file == 0) {
      if (reg == REG_RES) 
	 _mesa_printf("RES");
      else if (reg >= REG_ARG0 && reg <= REG_ARG1)
	 _mesa_printf("ARG%d", reg - REG_ARG0);
      else if (reg >= REG_TMP0 && reg <= REG_TMP11)
	 _mesa_printf("TMP%d", reg - REG_TMP0);
      else if (reg >= REG_IN0 && reg <= REG_IN31)
	 _mesa_printf("IN%d", reg - REG_IN0);
      else if (reg >= REG_OUT0 && reg <= REG_OUT14)
	 _mesa_printf("OUT%d", reg - REG_OUT0);
      else if (reg == REG_ADDR)
	 _mesa_printf("ADDR");
      else if (reg == REG_ID)
	 _mesa_printf("ID");
      else
	 _mesa_printf("REG%d", reg);
   }
   else 
      _mesa_printf("%s:%d", reg_file[file], reg);
}


static void print_RSW( union instruction op )
{
   GLuint swz = op.rsw.swz;
   GLuint neg = op.rsw.neg;
   GLuint i;

   _mesa_printf("RSW ");
   print_reg(0, op.rsw.dst);
   _mesa_printf(", ");
   print_reg(op.rsw.file0, op.rsw.idx0);
   _mesa_printf(".");
   for (i = 0; i < 4; i++, swz >>= 3) {
      const char *cswz = "xyzw01";
      if (neg & (1<<i))   
	 _mesa_printf("-");
      _mesa_printf("%c", cswz[swz&0x7]);
   }
   _mesa_printf("\n");
}

static void print_SWZ( union instruction op )
{
   GLuint swz = op.rsw.swz;
   GLuint neg = op.rsw.neg;
   GLuint i;

   _mesa_printf("SWZ ");
   print_reg(0, op.rsw.dst);
   _mesa_printf(", ");
   print_reg(op.rsw.file0, op.rsw.idx0);
   _mesa_printf(".");
   for (i = 0; i < 4; i++, swz >>= 3) {
      const char *cswz = "xyzw01";
      if (neg & (1<<i))   
	 _mesa_printf("-");
      _mesa_printf("%c", cswz[swz&0x7]);
   }
   _mesa_printf("\n");
}


static void print_ALU( union instruction op )
{
   _mesa_printf("%s ", _mesa_opcode_string((enum prog_opcode) op.alu.opcode));
   print_reg(0, op.alu.dst);
   _mesa_printf(", ");
   print_reg(op.alu.file0, op.alu.idx0);
   if (_mesa_num_inst_src_regs((enum prog_opcode) op.alu.opcode) > 1) {
      _mesa_printf(", ");
      print_reg(op.alu.file1, op.alu.idx1);
   }
   _mesa_printf("\n");
}

static void print_MSK( union instruction op )
{
   _mesa_printf("MSK ");
   print_reg(0, op.msk.dst);
   print_mask(op.msk.mask);
   _mesa_printf(", ");
   print_reg(op.msk.file, op.msk.idx);
   _mesa_printf("\n");
}

static void print_NOP( union instruction op )
{
}

void
_tnl_disassem_vba_insn( union instruction op )
{
   switch (op.alu.opcode) {
   case OPCODE_ABS:
   case OPCODE_ADD:
   case OPCODE_DP3:
   case OPCODE_DP4:
   case OPCODE_DPH:
   case OPCODE_DST:
   case OPCODE_EX2:
   case OPCODE_EXP:
   case OPCODE_FLR:
   case OPCODE_FRC:
   case OPCODE_LG2:
   case OPCODE_LIT:
   case OPCODE_LOG:
   case OPCODE_MAX:
   case OPCODE_MIN:
   case OPCODE_MOV:
   case OPCODE_MUL:
   case OPCODE_POW:
   case OPCODE_PRINT:
   case OPCODE_RCP:
   case OPCODE_RSQ:
   case OPCODE_SGE:
   case OPCODE_SLT:
   case OPCODE_SUB:
   case OPCODE_XPD:
      print_ALU(op);
      break;
   case OPCODE_ARA:
   case OPCODE_ARL:
   case OPCODE_ARL_NV:
   case OPCODE_ARR:
   case OPCODE_BRA:
   case OPCODE_CAL:
   case OPCODE_END:
   case OPCODE_MAD:
   case OPCODE_POPA:
   case OPCODE_PUSHA:
   case OPCODE_RCC:
   case OPCODE_RET:
   case OPCODE_SSG:
      print_NOP(op);
      break;
   case OPCODE_SWZ:
      print_SWZ(op);
      break;
   case RSW:
      print_RSW(op);
      break;
   case MSK:
      print_MSK(op);
      break;
   case REL:
      print_ALU(op);
      break;
   default:
      _mesa_problem(NULL, "Bad opcode in _tnl_disassem_vba_insn()");
   }
}


static void (* const opcode_func[MAX_OPCODE+3])(struct arb_vp_machine *, union instruction) = 
{
   do_ABS,
   do_ADD,
   do_NOP,/*ARA*/
   do_NOP,/*ARL*/
   do_NOP,/*ARL_NV*/
   do_NOP,/*ARR*/
   do_NOP,/*BRA*/
   do_NOP,/*CAL*/
   do_NOP,/*CMP*/
   do_NOP,/*COS*/
   do_NOP,/*DDX*/
   do_NOP,/*DDY*/
   do_DP3,
   do_DP4,
   do_DPH,
   do_DST,
   do_NOP,
   do_EX2,
   do_EXP,
   do_FLR,
   do_FRC,
   do_NOP,/*KIL*/
   do_NOP,/*KIL_NV*/
   do_LG2,
   do_LIT,
   do_LOG,
   do_NOP,/*LRP*/
   do_NOP,/*MAD*/
   do_MAX,
   do_MIN,
   do_MOV,
   do_MUL,
   do_NOP,/*PK2H*/
   do_NOP,/*PK2US*/
   do_NOP,/*PK4B*/
   do_NOP,/*PK4UB*/
   do_POW,
   do_NOP,/*POPA*/
   do_PRT,
   do_NOP,/*PUSHA*/
   do_NOP,/*RCC*/
   do_RCP,/*RCP*/
   do_NOP,/*RET*/
   do_NOP,/*RFL*/
   do_RSQ,
   do_NOP,/*SCS*/
   do_NOP,/*SEQ*/
   do_NOP,/*SFL*/
   do_SGE,
   do_NOP,/*SGT*/
   do_NOP,/*SIN*/
   do_NOP,/*SLE*/
   do_SLT,
   do_NOP,/*SNE*/
   do_NOP,/*SSG*/
   do_NOP,/*STR*/
   do_SUB,
   do_SWZ,/*SWZ*/
   do_NOP,/*TEX*/
   do_NOP,/*TXB*/
   do_NOP,/*TXD*/
   do_NOP,/*TXL*/
   do_NOP,/*TXP*/
   do_NOP,/*TXP_NV*/
   do_NOP,/*UP2H*/
   do_NOP,/*UP2US*/
   do_NOP,/*UP4B*/
   do_NOP,/*UP4UB*/
   do_NOP,/*X2D*/
   do_XPD,
   do_RSW,
   do_MSK,
   do_REL,
};

static union instruction *cvp_next_instruction( struct compilation *cp )
{
   union instruction *op = cp->csr++;
   _mesa_bzero(op, sizeof(*op));
   return op;
}

static struct reg cvp_make_reg( GLuint file, GLuint idx )
{
   struct reg reg;
   reg.file = file;
   reg.idx = idx;
   return reg;
}

static struct reg cvp_emit_rel( struct compilation *cp,
				struct reg reg,
				struct reg tmpreg )
{
   union instruction *op = cvp_next_instruction(cp);
   op->alu.opcode = REL;
   op->alu.file0 = reg.file;
   op->alu.idx0 = reg.idx;
   op->alu.dst = tmpreg.idx;
   return tmpreg;
}


static struct reg cvp_load_reg( struct compilation *cp,
				GLuint file,
				GLuint index,
				GLuint rel,
				GLuint tmpidx )
{
   struct reg tmpreg = cvp_make_reg(FILE_REG, tmpidx);
   struct reg reg;

   switch (file) {
   case PROGRAM_TEMPORARY:
      return cvp_make_reg(FILE_REG, REG_TMP0 + index);

   case PROGRAM_INPUT:
      return cvp_make_reg(FILE_REG, REG_IN0 + index);

   case PROGRAM_OUTPUT:
      return cvp_make_reg(FILE_REG, REG_OUT0 + index);

      /* These two aren't populated by the parser?
       */
   case PROGRAM_LOCAL_PARAM: 
      reg = cvp_make_reg(FILE_LOCAL_PARAM, index);
      if (rel) 
	 return cvp_emit_rel(cp, reg, tmpreg);
      else
	 return reg;

   case PROGRAM_ENV_PARAM: 
      reg = cvp_make_reg(FILE_ENV_PARAM, index);
      if (rel) 
	 return cvp_emit_rel(cp, reg, tmpreg);
      else
	 return reg;

   case PROGRAM_STATE_VAR:
      reg = cvp_make_reg(FILE_STATE_PARAM, index);
      if (rel) 
	 return cvp_emit_rel(cp, reg, tmpreg);
      else
	 return reg;

      /* Invalid values:
       */
   case PROGRAM_WRITE_ONLY:
   case PROGRAM_ADDRESS:
   default:
      _mesa_problem(NULL, "Invalid register file %d in cvp_load_reg()");
      assert(0);
      return tmpreg;		/* can't happen */
   }
}

static struct reg cvp_emit_arg( struct compilation *cp,
				const struct prog_src_register *src,
				GLuint arg )
{
   struct reg reg = cvp_load_reg( cp, src->File, src->Index, src->RelAddr, arg );
   union instruction rsw, noop;

   /* Emit any necessary swizzling.  
    */
   _mesa_bzero(&rsw, sizeof(rsw));
   rsw.rsw.neg = src->NegateBase ? WRITEMASK_XYZW : 0;

   /* we're expecting 2-bit swizzles below... */
#if 1 /* XXX THESE ASSERTIONS CURRENTLY FAIL DURING GLEAN TESTS! */
/* hopefully no longer happens? */
   ASSERT(GET_SWZ(src->Swizzle, 0) < 4);
   ASSERT(GET_SWZ(src->Swizzle, 1) < 4);
   ASSERT(GET_SWZ(src->Swizzle, 2) < 4);
   ASSERT(GET_SWZ(src->Swizzle, 3) < 4);
#endif
   rsw.rsw.swz = src->Swizzle;

   _mesa_bzero(&noop, sizeof(noop));
   noop.rsw.neg = 0;
   noop.rsw.swz = SWIZZLE_NOOP;

   if (_mesa_memcmp(&rsw, &noop, sizeof(rsw)) !=0) {
      union instruction *op = cvp_next_instruction(cp);
      struct reg rsw_reg = cvp_make_reg(FILE_REG, REG_ARG0 + arg);
      *op = rsw;
      op->rsw.opcode = RSW;
      op->rsw.file0 = reg.file;
      op->rsw.idx0 = reg.idx;
      op->rsw.dst = rsw_reg.idx;
      return rsw_reg;
   }
   else
      return reg;
}

static GLuint cvp_choose_result( struct compilation *cp,
				 const struct prog_dst_register *dst,
				 union instruction *fixup )
{
   GLuint mask = dst->WriteMask;
   GLuint idx;

   switch (dst->File) {
   case PROGRAM_TEMPORARY:
      idx = REG_TMP0 + dst->Index;
      break;
   case PROGRAM_OUTPUT:
      idx = REG_OUT0 + dst->Index;
      break;
   default:
      assert(0);
      return REG_RES;		/* can't happen */
   }

   /* Optimization: When writing (with a writemask) to an undefined
    * value for the first time, the writemask may be ignored. 
    */
   if (mask != WRITEMASK_XYZW && (cp->reg_active & (1 << idx))) {
      fixup->msk.opcode = MSK;
      fixup->msk.dst = idx;
      fixup->msk.file = FILE_REG;
      fixup->msk.idx = REG_RES;
      fixup->msk.mask = mask;
      cp->reg_active |= 1 << idx;
      return REG_RES;
   }
   else {
      _mesa_bzero(fixup, sizeof(*fixup));
      cp->reg_active |= 1 << idx;
      return idx;
   }
}


static void cvp_emit_inst( struct compilation *cp,
			   const struct prog_instruction *inst )
{
   union instruction *op;
   union instruction fixup;
   struct reg reg[3];
   GLuint result, nr_args, i;

   /* Need to handle SWZ, ARL specially.
    */
   switch (inst->Opcode) {
      /* Split into mul and add:
       */
   case OPCODE_MAD:
      result = cvp_choose_result( cp, &inst->DstReg, &fixup );
      for (i = 0; i < 3; i++) 
	 reg[i] = cvp_emit_arg( cp, &inst->SrcReg[i], REG_ARG0+i );

      op = cvp_next_instruction(cp);
      op->alu.opcode = OPCODE_MUL;
      op->alu.file0 = reg[0].file;
      op->alu.idx0 = reg[0].idx;
      op->alu.file1 = reg[1].file;
      op->alu.idx1 = reg[1].idx;
      op->alu.dst = REG_ARG0;

      op = cvp_next_instruction(cp);
      op->alu.opcode = OPCODE_ADD;
      op->alu.file0 = FILE_REG;
      op->alu.idx0 = REG_ARG0;
      op->alu.file1 = reg[2].file;
      op->alu.idx1 = reg[2].idx;
      op->alu.dst = result;

      if (result == REG_RES) {
	 op = cvp_next_instruction(cp);
	 *op = fixup;
      }
      break;

   case OPCODE_ARL:
      reg[0] = cvp_emit_arg( cp, &inst->SrcReg[0], REG_ARG0 );

      op = cvp_next_instruction(cp);
      op->alu.opcode = OPCODE_FLR;
      op->alu.dst = REG_ADDR;
      op->alu.file0 = reg[0].file;
      op->alu.idx0 = reg[0].idx;
      break;

   case OPCODE_END:
      break;

   case OPCODE_SWZ:
      result = cvp_choose_result( cp, &inst->DstReg, &fixup );
      reg[0] = cvp_load_reg( cp, inst->SrcReg[0].File,
			inst->SrcReg[0].Index, inst->SrcReg[0].RelAddr, REG_ARG0 );
      op = cvp_next_instruction(cp);
      op->rsw.opcode = inst->Opcode;
      op->rsw.file0 = reg[0].file;
      op->rsw.idx0 = reg[0].idx;
      op->rsw.dst = result;
      op->rsw.swz = inst->SrcReg[0].Swizzle;
      op->rsw.neg = inst->SrcReg[0].NegateBase;

      if (result == REG_RES) {
	 op = cvp_next_instruction(cp);
	 *op = fixup;
      }
      break;

   default:
      result = cvp_choose_result( cp, &inst->DstReg, &fixup );
      nr_args = _mesa_num_inst_src_regs(inst->Opcode);
      for (i = 0; i < nr_args; i++) 
	 reg[i] = cvp_emit_arg( cp, &inst->SrcReg[i], REG_ARG0 + i );

      op = cvp_next_instruction(cp);
      op->alu.opcode = inst->Opcode;
      op->alu.file0 = reg[0].file;
      op->alu.idx0 = reg[0].idx;
      op->alu.file1 = reg[1].file;
      op->alu.idx1 = reg[1].idx;
      op->alu.dst = result;

      if (result == REG_RES) {
	 op = cvp_next_instruction(cp);
	 *op = fixup;
      }
      break;
   }
}

static void free_tnl_data( struct gl_vertex_program *program  )
{
   struct tnl_compiled_program *p = (struct tnl_compiled_program *) program->TnlData;
   if (p->compiled_func)
      _mesa_free((void *)p->compiled_func);
   _mesa_free(p);
   program->TnlData = NULL;
}

static void compile_vertex_program( struct gl_vertex_program *program,
				    GLboolean try_codegen )
{ 
   struct compilation cp;
   struct tnl_compiled_program *p = CALLOC_STRUCT(tnl_compiled_program);
   GLuint i;

   if (program->TnlData) 
      free_tnl_data( program );
   
   program->TnlData = p;

   /* Initialize cp.  Note that ctx and VB aren't used in compilation
    * so we don't have to worry about statechanges:
    */
   _mesa_memset(&cp, 0, sizeof(cp));
   cp.csr = p->instructions;

   /* Compile instructions:
    */
   for (i = 0; i < program->Base.NumInstructions; i++) {
      cvp_emit_inst(&cp, &program->Base.Instructions[i]);
   }

   /* Finish up:
    */
   p->nr_instructions = cp.csr - p->instructions;

   /* Print/disassemble:
    */
   if (DISASSEM) {
      for (i = 0; i < p->nr_instructions; i++) {
	 _tnl_disassem_vba_insn(p->instructions[i]);
      }
      _mesa_printf("\n\n");
   }
   
#ifdef USE_SSE_ASM
   if (try_codegen)
      _tnl_sse_codegen_vertex_program(p);
#endif

}




/* ----------------------------------------------------------------------
 * Execution
 */
static void userclip( GLcontext *ctx,
		      GLvector4f *clip,
		      GLubyte *clipmask,
		      GLubyte *clipormask,
		      GLubyte *clipandmask )
{
   GLuint p;

   for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
      if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
	 GLuint nr, i;
	 const GLfloat a = ctx->Transform._ClipUserPlane[p][0];
	 const GLfloat b = ctx->Transform._ClipUserPlane[p][1];
	 const GLfloat c = ctx->Transform._ClipUserPlane[p][2];
	 const GLfloat d = ctx->Transform._ClipUserPlane[p][3];
         GLfloat *coord = (GLfloat *)clip->data;
         GLuint stride = clip->stride;
         GLuint count = clip->count;

	 for (nr = 0, i = 0 ; i < count ; i++) {
	    GLfloat dp = (coord[0] * a + 
			  coord[1] * b +
			  coord[2] * c +
			  coord[3] * d);

	    if (dp < 0) {
	       nr++;
	       clipmask[i] |= CLIP_USER_BIT;
	    }

	    STRIDE_F(coord, stride);
	 }

	 if (nr > 0) {
	    *clipormask |= CLIP_USER_BIT;
	    if (nr == count) {
	       *clipandmask |= CLIP_USER_BIT;
	       return;
	    }
	 }
      }
   }
}


static GLboolean
do_ndc_cliptest(GLcontext *ctx, struct arb_vp_machine *m)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = m->VB;

   /* Cliptest and perspective divide.  Clip functions must clear
    * the clipmask.
    */
   m->ormask = 0;
   m->andmask = CLIP_FRUSTUM_BITS;

   if (tnl->NeedNdcCoords) {
      VB->NdcPtr =
         _mesa_clip_tab[VB->ClipPtr->size]( VB->ClipPtr,
                                            &m->ndcCoords,
                                            m->clipmask,
                                            &m->ormask,
                                            &m->andmask );
   }
   else {
      VB->NdcPtr = NULL;
      _mesa_clip_np_tab[VB->ClipPtr->size]( VB->ClipPtr,
                                            NULL,
                                            m->clipmask,
                                            &m->ormask,
                                            &m->andmask );
   }

   if (m->andmask) {
      /* All vertices are outside the frustum */
      return GL_FALSE;
   }

   /* Test userclip planes.  This contributes to VB->ClipMask.
    */
   if (ctx->Transform.ClipPlanesEnabled && (!ctx->VertexProgram._Enabled ||
      ctx->VertexProgram.Current->IsPositionInvariant)) {
      userclip( ctx,
		VB->ClipPtr,
		m->clipmask,
		&m->ormask,
		&m->andmask );

      if (m->andmask) {
	 return GL_FALSE;
      }
   }

   VB->ClipAndMask = m->andmask;
   VB->ClipOrMask = m->ormask;
   VB->ClipMask = m->clipmask;

   return GL_TRUE;
}


static INLINE void call_func( struct tnl_compiled_program *p,
			      struct arb_vp_machine *m )
{
   p->compiled_func(m);
}

/**
 * Execute the given vertex program.  
 * 
 * TODO: Integrate the t_vertex.c code here, to build machine vertices
 * directly at this point.
 *
 * TODO: Eliminate the VB struct entirely and just use
 * struct arb_vertex_machine.
 */
static GLboolean
run_arb_vertex_program(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
   const struct gl_vertex_program *program;
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct arb_vp_machine *m = ARB_VP_MACHINE(stage);
   struct tnl_compiled_program *p;
   GLuint i, j;
   GLbitfield outputs;

   if (ctx->ShaderObjects._VertexShaderPresent)
      return GL_TRUE;

   program = ctx->VertexProgram._Enabled ? ctx->VertexProgram.Current : NULL;
   if (!program && ctx->_MaintainTnlProgram) {
      program = ctx->_TnlProgram;
   }
   if (!program || program->IsNVProgram)
      return GL_TRUE;   

   if (program->Base.Parameters) {
      _mesa_load_state_parameters(ctx, program->Base.Parameters);
   }   
   
   p = (struct tnl_compiled_program *)program->TnlData;
   assert(p);


   m->nr_inputs = m->nr_outputs = 0;

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      if (program->Base.InputsRead & (1<<i) ||
	  (i == VERT_ATTRIB_POS && program->IsPositionInvariant)) {
	 GLuint j = m->nr_inputs++;
	 m->input[j].idx = i;
	 m->input[j].data = (GLfloat *)m->VB->AttribPtr[i]->data;
	 m->input[j].stride = m->VB->AttribPtr[i]->stride;
	 m->input[j].size = m->VB->AttribPtr[i]->size;
	 ASSIGN_4V(m->File[0][REG_IN0 + i], 0, 0, 0, 1);
      }
   }     

   for (i = 0; i < VERT_RESULT_MAX; i++) {
      if (program->Base.OutputsWritten & (1 << i) ||
	  (i == VERT_RESULT_HPOS && program->IsPositionInvariant)) {
	 GLuint j = m->nr_outputs++;
	 m->output[j].idx = i;
	 m->output[j].data = (GLfloat *)m->attribs[i].data;
      }
   }     


   /* Run the actual program:
    */
   for (m->vtx_nr = 0; m->vtx_nr < VB->Count; m->vtx_nr++) {
      for (j = 0; j < m->nr_inputs; j++) {
	 GLuint idx = REG_IN0 + m->input[j].idx;
	 switch (m->input[j].size) {
	 case 4: m->File[0][idx][3] = m->input[j].data[3];
	 case 3: m->File[0][idx][2] = m->input[j].data[2];
	 case 2: m->File[0][idx][1] = m->input[j].data[1];
	 case 1: m->File[0][idx][0] = m->input[j].data[0];
	 }

	 STRIDE_F(m->input[j].data, m->input[j].stride);
      }


      if (p->compiled_func) {
	 call_func( p, m );
      }
      else {
	 for (j = 0; j < p->nr_instructions; j++) {
	    union instruction inst = p->instructions[j];	 
	    opcode_func[inst.alu.opcode]( m, inst );
	 }
      }

      /* If the program is position invariant, multiply the input position
       * by the MVP matrix and store in the vertex position result register.
       */
      if (program->IsPositionInvariant) {
	 TRANSFORM_POINT( m->File[0][REG_OUT0+0], 
			  ctx->_ModelProjectMatrix.m, 
			  m->File[0][REG_IN0+0]);
      }

      for (j = 0; j < m->nr_outputs; j++) {
	 GLuint idx = REG_OUT0 + m->output[j].idx;
	 m->output[j].data[0] = m->File[0][idx][0];
	 m->output[j].data[1] = m->File[0][idx][1];
	 m->output[j].data[2] = m->File[0][idx][2];
	 m->output[j].data[3] = m->File[0][idx][3];
	 m->output[j].data += 4;
      }

   }

   /* Setup the VB pointers so that the next pipeline stages get
    * their data from the right place (the program output arrays).
    *
    * TODO: 1) Have tnl use these RESULT values for outputs rather
    * than trying to shoe-horn inputs and outputs into one set of
    * values.
    *
    * TODO: 2) Integrate t_vertex.c so that we just go straight ahead
    * and build machine vertices here.
    */
   VB->ClipPtr = &m->attribs[VERT_RESULT_HPOS];
   VB->ClipPtr->count = VB->Count;

   /* XXX There seems to be confusion between using the VERT_ATTRIB_*
    * values vs _TNL_ATTRIB_* tokens here:
    */
   outputs = program->Base.OutputsWritten;
   if (program->IsPositionInvariant) 
      outputs |= (1<<VERT_RESULT_HPOS);

   if (outputs & (1<<VERT_RESULT_COL0)) {
      VB->ColorPtr[0] =
      VB->AttribPtr[VERT_ATTRIB_COLOR0] = &m->attribs[VERT_RESULT_COL0];
   }

   if (outputs & (1<<VERT_RESULT_BFC0)) {
      VB->ColorPtr[1] = &m->attribs[VERT_RESULT_BFC0];
   }

   if (outputs & (1<<VERT_RESULT_COL1)) {
      VB->SecondaryColorPtr[0] =
      VB->AttribPtr[VERT_ATTRIB_COLOR1] = &m->attribs[VERT_RESULT_COL1];
   }

   if (outputs & (1<<VERT_RESULT_BFC1)) {
      VB->SecondaryColorPtr[1] = &m->attribs[VERT_RESULT_BFC1];
   }

   if (outputs & (1<<VERT_RESULT_FOGC)) {
      VB->FogCoordPtr =
      VB->AttribPtr[VERT_ATTRIB_FOG] = &m->attribs[VERT_RESULT_FOGC];
   }

   if (outputs & (1<<VERT_RESULT_PSIZ)) {
      VB->AttribPtr[_TNL_ATTRIB_POINTSIZE] = &m->attribs[VERT_RESULT_PSIZ];
   }

   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      if (outputs & (1<<(VERT_RESULT_TEX0+i))) {
	 VB->TexCoordPtr[i] =
	 VB->AttribPtr[VERT_ATTRIB_TEX0+i] = &m->attribs[VERT_RESULT_TEX0 + i];
      }
   }

#if 0
   for (i = 0; i < VB->Count; i++) {
      printf("Out %d: %f %f %f %f %f %f %f %f\n", i,
	     VEC_ELT(VB->ClipPtr, GLfloat, i)[0],
	     VEC_ELT(VB->ClipPtr, GLfloat, i)[1],
	     VEC_ELT(VB->ClipPtr, GLfloat, i)[2],
	     VEC_ELT(VB->ClipPtr, GLfloat, i)[3],
	     VEC_ELT(VB->AttribPtr[VERT_ATTRIB_TEX0], GLfloat, i)[0],
	     VEC_ELT(VB->AttribPtr[VERT_ATTRIB_TEX0], GLfloat, i)[1],
	     VEC_ELT(VB->AttribPtr[VERT_ATTRIB_TEX0], GLfloat, i)[2],
	     VEC_ELT(VB->AttribPtr[VERT_ATTRIB_TEX0], GLfloat, i)[3]);
   }
#endif

   /* Perform NDC and cliptest operations:
    */
   return do_ndc_cliptest(ctx, m);
}


static void
validate_vertex_program( GLcontext *ctx, struct tnl_pipeline_stage *stage )
{
   struct arb_vp_machine *m = ARB_VP_MACHINE(stage);
   struct gl_vertex_program *program;

   if (ctx->ShaderObjects._VertexShaderPresent)
      return;

   program = (ctx->VertexProgram._Enabled ? ctx->VertexProgram.Current : 0);
   if (!program && ctx->_MaintainTnlProgram) {
      program = ctx->_TnlProgram;
   }

   if (program) {
      if (!program->TnlData)
	 compile_vertex_program( program, m->try_codegen );
      
      /* Grab the state GL state and put into registers:
       */
      m->File[FILE_LOCAL_PARAM] = program->Base.LocalParams;
      m->File[FILE_ENV_PARAM] = ctx->VertexProgram.Parameters;
      /* GL_NV_vertex_programs can't reference GL state */
      if (program->Base.Parameters)
         m->File[FILE_STATE_PARAM] = program->Base.Parameters->ParameterValues;
      else
         m->File[FILE_STATE_PARAM] = NULL;
   }
}







/**
 * Called the first time stage->run is called.  In effect, don't
 * allocate data until the first time the stage is run.
 */
static GLboolean init_vertex_program( GLcontext *ctx,
				      struct tnl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &(tnl->vb);
   struct arb_vp_machine *m;
   const GLuint size = VB->Size;
   GLuint i;

   stage->privatePtr = _mesa_calloc(sizeof(*m));
   m = ARB_VP_MACHINE(stage);
   if (!m)
      return GL_FALSE;

   /* arb_vertex_machine struct should subsume the VB:
    */
   m->VB = VB;

   m->File[0] = (GLfloat(*)[4])ALIGN_MALLOC(REG_MAX * sizeof(GLfloat) * 4, 16);

   /* Initialize regs where necessary:
    */
   ASSIGN_4V(m->File[0][REG_ID], 0, 0, 0, 1);
   ASSIGN_4V(m->File[0][REG_ONES], 1, 1, 1, 1);
   ASSIGN_4V(m->File[0][REG_SWZ], 1, -1, 0, 0);
   ASSIGN_4V(m->File[0][REG_NEG], -1, -1, -1, -1);
   ASSIGN_4V(m->File[0][REG_LIT], 1, 0, 0, 1);
   ASSIGN_4V(m->File[0][REG_LIT2], 1, .5, .2, 1); /* debug value */

   if (_mesa_getenv("MESA_EXPERIMENTAL"))
      m->try_codegen = GL_TRUE;

   /* Allocate arrays of vertex output values */
   for (i = 0; i < VERT_RESULT_MAX; i++) {
      _mesa_vector4f_alloc( &m->attribs[i], 0, size, 32 );
      m->attribs[i].size = 4;
   }

   /* a few other misc allocations */
   _mesa_vector4f_alloc( &m->ndcCoords, 0, size, 32 );
   m->clipmask = (GLubyte *) ALIGN_MALLOC(sizeof(GLubyte)*size, 32 );

   if (ctx->_MaintainTnlProgram)
      _mesa_allow_light_in_model( ctx, GL_FALSE );

   m->fpucntl_rnd_neg = RND_NEG_FPU; /* const value */
   m->fpucntl_restore = RESTORE_FPU; /* const value */

   return GL_TRUE;
}




/**
 * Destructor for this pipeline stage.
 */
static void dtr( struct tnl_pipeline_stage *stage )
{
   struct arb_vp_machine *m = ARB_VP_MACHINE(stage);

   if (m) {
      GLuint i;

      /* free the vertex program result arrays */
      for (i = 0; i < VERT_RESULT_MAX; i++)
         _mesa_vector4f_free( &m->attribs[i] );

      /* free misc arrays */
      _mesa_vector4f_free( &m->ndcCoords );
      ALIGN_FREE( m->clipmask );
      ALIGN_FREE( m->File[0] );

      _mesa_free( m );
      stage->privatePtr = NULL;
   }
}

/**
 * Public description of this pipeline stage.
 */
const struct tnl_pipeline_stage _tnl_arb_vertex_program_stage =
{
   "arb-vertex-program",
   NULL,			/* private_data */
   init_vertex_program,		/* create */
   dtr,				/* destroy */
   validate_vertex_program,	/* validate */
   run_arb_vertex_program	/* run */
};


/**
 * Called via ctx->Driver.ProgramStringNotify() after a new vertex program
 * string has been parsed.
 */
void
_tnl_program_string(GLcontext *ctx, GLenum target, struct gl_program *program)
{
   if (target == GL_VERTEX_PROGRAM_ARB) {
      /* free any existing tnl data hanging off the program */
      struct gl_vertex_program *vprog = (struct gl_vertex_program *) program;
      if (vprog->TnlData) {
         free_tnl_data(vprog);
      }
   }
}
