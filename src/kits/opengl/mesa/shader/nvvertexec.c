/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
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
 * \file nvvertexec.c
 * Code to execute vertex programs.
 * \author Brian Paul
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "nvvertexec.h"
#include "program_instruction.h"
#include "program.h"
#include "math/m_matrix.h"


static const GLfloat ZeroVec[4] = { 0.0F, 0.0F, 0.0F, 0.0F };


/**
 * Load/initialize the vertex program registers which need to be set
 * per-vertex.
 */
void
_mesa_init_vp_per_vertex_registers(GLcontext *ctx, struct vp_machine *machine)
{
   /* Input registers get initialized from the current vertex attribs */
   MEMCPY(machine->Inputs, ctx->Current.Attrib,
          MAX_VERTEX_PROGRAM_ATTRIBS * 4 * sizeof(GLfloat));

   if (ctx->VertexProgram.Current->IsNVProgram) {
      GLuint i;
      /* Output/result regs are initialized to [0,0,0,1] */
      for (i = 0; i < MAX_NV_VERTEX_PROGRAM_OUTPUTS; i++) {
         ASSIGN_4V(machine->Outputs[i], 0.0F, 0.0F, 0.0F, 1.0F);
      }
      /* Temp regs are initialized to [0,0,0,0] */
      for (i = 0; i < MAX_NV_VERTEX_PROGRAM_TEMPS; i++) {
         ASSIGN_4V(machine->Temporaries[i], 0.0F, 0.0F, 0.0F, 0.0F);
      }
      ASSIGN_4V(machine->AddressReg, 0, 0, 0, 0);
   }
}



/**
 * Copy the 16 elements of a matrix into four consecutive program
 * registers starting at 'pos'.
 */
static void
load_matrix(GLfloat registers[][4], GLuint pos, const GLfloat mat[16])
{
   GLuint i;
   for (i = 0; i < 4; i++) {
      registers[pos + i][0] = mat[0 + i];
      registers[pos + i][1] = mat[4 + i];
      registers[pos + i][2] = mat[8 + i];
      registers[pos + i][3] = mat[12 + i];
   }
}


/**
 * As above, but transpose the matrix.
 */
static void
load_transpose_matrix(GLfloat registers[][4], GLuint pos,
                      const GLfloat mat[16])
{
   MEMCPY(registers[pos], mat, 16 * sizeof(GLfloat));
}


/**
 * Load program parameter registers with tracked matrices (if NV program)
 * or GL state values (if ARB program).
 * This needs to be done per glBegin/glEnd, not per-vertex.
 */
void
_mesa_init_vp_per_primitive_registers(GLcontext *ctx)
{
   if (ctx->VertexProgram.Current->IsNVProgram) {
      GLuint i;

      for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS / 4; i++) {
         /* point 'mat' at source matrix */
         GLmatrix *mat;
         if (ctx->VertexProgram.TrackMatrix[i] == GL_MODELVIEW) {
            mat = ctx->ModelviewMatrixStack.Top;
         }
         else if (ctx->VertexProgram.TrackMatrix[i] == GL_PROJECTION) {
            mat = ctx->ProjectionMatrixStack.Top;
         }
         else if (ctx->VertexProgram.TrackMatrix[i] == GL_TEXTURE) {
            mat = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top;
         }
         else if (ctx->VertexProgram.TrackMatrix[i] == GL_COLOR) {
            mat = ctx->ColorMatrixStack.Top;
         }
         else if (ctx->VertexProgram.TrackMatrix[i]==GL_MODELVIEW_PROJECTION_NV) {
            /* XXX verify the combined matrix is up to date */
            mat = &ctx->_ModelProjectMatrix;
         }
         else if (ctx->VertexProgram.TrackMatrix[i] >= GL_MATRIX0_NV &&
                  ctx->VertexProgram.TrackMatrix[i] <= GL_MATRIX7_NV) {
            GLuint n = ctx->VertexProgram.TrackMatrix[i] - GL_MATRIX0_NV;
            ASSERT(n < MAX_PROGRAM_MATRICES);
            mat = ctx->ProgramMatrixStack[n].Top;
         }
         else {
            /* no matrix is tracked, but we leave the register values as-is */
            assert(ctx->VertexProgram.TrackMatrix[i] == GL_NONE);
            continue;
         }

         /* load the matrix values into sequential registers */
         if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_IDENTITY_NV) {
            load_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
         }
         else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_INVERSE_NV) {
            _math_matrix_analyse(mat); /* update the inverse */
            ASSERT(!_math_matrix_is_dirty(mat));
            load_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
         }
         else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_TRANSPOSE_NV) {
            load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
         }
         else {
            assert(ctx->VertexProgram.TrackMatrixTransform[i]
                   == GL_INVERSE_TRANSPOSE_NV);
            _math_matrix_analyse(mat); /* update the inverse */
            ASSERT(!_math_matrix_is_dirty(mat));
            load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
         }
      }
   }
   else {
      /* ARB vertex program */
      if (ctx->VertexProgram.Current->Base.Parameters) {
         /* Grab the state GL state and put into registers */
         _mesa_load_state_parameters(ctx,
                                 ctx->VertexProgram.Current->Base.Parameters);
      }
   }
}



/**
 * For debugging.  Dump the current vertex program machine registers.
 */
void
_mesa_dump_vp_state( const struct gl_vertex_program_state *state,
                     const struct vp_machine *machine)
{
   int i;
   _mesa_printf("VertexIn:\n");
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_INPUTS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
                   machine->Inputs[i][0],
                   machine->Inputs[i][1],
                   machine->Inputs[i][2],
                   machine->Inputs[i][3]);
   }
   _mesa_printf("\n");

   _mesa_printf("VertexOut:\n");
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_OUTPUTS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
                  machine->Outputs[i][0],
                  machine->Outputs[i][1],
                  machine->Outputs[i][2],
                  machine->Outputs[i][3]);
   }
   _mesa_printf("\n");

   _mesa_printf("Registers:\n");
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_TEMPS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
                  machine->Temporaries[i][0],
                  machine->Temporaries[i][1],
                  machine->Temporaries[i][2],
                  machine->Temporaries[i][3]);
   }
   _mesa_printf("\n");

   _mesa_printf("Parameters:\n");
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
                  state->Parameters[i][0],
                  state->Parameters[i][1],
                  state->Parameters[i][2],
                  state->Parameters[i][3]);
   }
   _mesa_printf("\n");
}



/**
 * Return a pointer to the 4-element float vector specified by the given
 * source register.
 */
static INLINE const GLfloat *
get_register_pointer( GLcontext *ctx,
                      const struct prog_src_register *source,
                      struct vp_machine *machine,
                      const struct gl_vertex_program *program )
{
   if (source->RelAddr) {
      const GLint reg = source->Index + machine->AddressReg[0];
      ASSERT( (source->File == PROGRAM_ENV_PARAM) || 
        (source->File == PROGRAM_STATE_VAR) );
      if (reg < 0 || reg > MAX_NV_VERTEX_PROGRAM_PARAMS)
         return ZeroVec;
      else if (source->File == PROGRAM_ENV_PARAM)
         return ctx->VertexProgram.Parameters[reg];
      else {
         ASSERT(source->File == PROGRAM_LOCAL_PARAM);
         return program->Base.Parameters->ParameterValues[reg];
      }
   }
   else {
      switch (source->File) {
         case PROGRAM_TEMPORARY:
            ASSERT(source->Index < MAX_NV_VERTEX_PROGRAM_TEMPS);
            return machine->Temporaries[source->Index];
         case PROGRAM_INPUT:
            ASSERT(source->Index < MAX_NV_VERTEX_PROGRAM_INPUTS);
            return machine->Inputs[source->Index];
         case PROGRAM_OUTPUT:
            /* This is only needed for the PRINT instruction */
            ASSERT(source->Index < MAX_NV_VERTEX_PROGRAM_OUTPUTS);
            return machine->Outputs[source->Index];
         case PROGRAM_LOCAL_PARAM:
            ASSERT(source->Index < MAX_PROGRAM_LOCAL_PARAMS);
            return program->Base.LocalParams[source->Index];
         case PROGRAM_ENV_PARAM:
            ASSERT(source->Index < MAX_NV_VERTEX_PROGRAM_PARAMS);
            return ctx->VertexProgram.Parameters[source->Index];
         case PROGRAM_STATE_VAR:
            ASSERT(source->Index < program->Base.Parameters->NumParameters);
            return program->Base.Parameters->ParameterValues[source->Index];
         default:
            _mesa_problem(NULL,
                          "Bad source register file in get_register_pointer");
            return NULL;
      }
   }
   return NULL;
}


/**
 * Fetch a 4-element float vector from the given source register.
 * Apply swizzling and negating as needed.
 */
static INLINE void
fetch_vector4( GLcontext *ctx, 
               const struct prog_src_register *source,
               struct vp_machine *machine,
               const struct gl_vertex_program *program,
               GLfloat result[4] )
{
   const GLfloat *src = get_register_pointer(ctx, source, machine, program);
   ASSERT(src);
   result[0] = src[GET_SWZ(source->Swizzle, 0)];
   result[1] = src[GET_SWZ(source->Swizzle, 1)];
   result[2] = src[GET_SWZ(source->Swizzle, 2)];
   result[3] = src[GET_SWZ(source->Swizzle, 3)];
   if (source->NegateBase) {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
   }
}



/**
 * As above, but only return result[0] element.
 */
static INLINE void
fetch_vector1( GLcontext *ctx,
               const struct prog_src_register *source,
               struct vp_machine *machine,
               const struct gl_vertex_program *program,
               GLfloat result[4] )
{
   const GLfloat *src = get_register_pointer(ctx, source, machine, program);
   ASSERT(src);
   result[0] = src[GET_SWZ(source->Swizzle, 0)];
   if (source->NegateBase) {
      result[0] = -result[0];
   }
}


/**
 * Store 4 floats into a register.
 */
static void
store_vector4( const struct prog_instruction *inst,
               struct vp_machine *machine,
               const GLfloat value[4] )
{
   const struct prog_dst_register *dest = &(inst->DstReg);
   GLfloat *dst;
   switch (dest->File) {
      case PROGRAM_OUTPUT:
         dst = machine->Outputs[dest->Index];
         break;
      case PROGRAM_TEMPORARY:
         dst = machine->Temporaries[dest->Index];
         break;
      case PROGRAM_ENV_PARAM:
         /* Only for VP state programs */
         {
            /* a slight hack */
            GET_CURRENT_CONTEXT(ctx);
            dst = ctx->VertexProgram.Parameters[dest->Index];
         }
         break;
      default:
         _mesa_problem(NULL, "Invalid register file in store_vector4(file=%d)",
                       dest->File);
         return;
   }

   if (dest->WriteMask & WRITEMASK_X)
      dst[0] = value[0];
   if (dest->WriteMask & WRITEMASK_Y)
      dst[1] = value[1];
   if (dest->WriteMask & WRITEMASK_Z)
      dst[2] = value[2];
   if (dest->WriteMask & WRITEMASK_W)
      dst[3] = value[3];
}


/**
 * Set x to positive or negative infinity.
 */
#if defined(USE_IEEE) || defined(_WIN32)
#define SET_POS_INFINITY(x)  ( *((GLuint *) (void *)&x) = 0x7F800000 )
#define SET_NEG_INFINITY(x)  ( *((GLuint *) (void *)&x) = 0xFF800000 )
#elif defined(VMS)
#define SET_POS_INFINITY(x)  x = __MAXFLOAT
#define SET_NEG_INFINITY(x)  x = -__MAXFLOAT
#else
#define SET_POS_INFINITY(x)  x = (GLfloat) HUGE_VAL
#define SET_NEG_INFINITY(x)  x = (GLfloat) -HUGE_VAL
#endif

#define SET_FLOAT_BITS(x, bits) ((fi_type *) (void *) &(x))->i = bits


/**
 * Execute the given vertex program
 */
void
_mesa_exec_vertex_program(GLcontext *ctx,
                          struct vp_machine *machine,
                          const struct gl_vertex_program *program)
{
   const struct prog_instruction *inst;

   ctx->_CurrentProgram = GL_VERTEX_PROGRAM_ARB; /* or NV, doesn't matter */

   /* If the program is position invariant, multiply the input position
    * by the MVP matrix and store in the vertex position result register.
    */
   if (ctx->VertexProgram.Current->IsPositionInvariant) {
      TRANSFORM_POINT( machine->Outputs[VERT_RESULT_HPOS], 
                       ctx->_ModelProjectMatrix.m, 
                       machine->Inputs[VERT_ATTRIB_POS]);

      /* XXX: This could go elsewhere */
      ctx->VertexProgram.Current->Base.OutputsWritten |= VERT_BIT_POS;
   }

   for (inst = program->Base.Instructions; ; inst++) {

      if (ctx->VertexProgram.CallbackEnabled &&
          ctx->VertexProgram.Callback) {
         ctx->VertexProgram.CurrentPosition = inst->StringPos;
         ctx->VertexProgram.Callback(program->Base.Target,
                                     ctx->VertexProgram.CallbackData);
      }

      switch (inst->Opcode) {
         case OPCODE_MOV:
            {
               GLfloat t[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_LIT:
            {
               const GLfloat epsilon = 1.0F / 256.0F; /* per NV spec */
               GLfloat t[4], lit[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               t[0] = MAX2(t[0], 0.0F);
               t[1] = MAX2(t[1], 0.0F);
               t[3] = CLAMP(t[3], -(128.0F - epsilon), (128.0F - epsilon));
               lit[0] = 1.0;
               lit[1] = t[0];
               lit[2] = (t[0] > 0.0) ? (GLfloat) _mesa_pow(t[1], t[3]) : 0.0F;
               lit[3] = 1.0;
               store_vector4( inst, machine, lit );
            }
            break;
         case OPCODE_RCP:
            {
               GLfloat t[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, t );
               if (t[0] != 1.0F)
                  t[0] = 1.0F / t[0];  /* div by zero is infinity! */
               t[1] = t[2] = t[3] = t[0];
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_RSQ:
            {
               GLfloat t[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, t );
               t[0] = INV_SQRTF(FABSF(t[0]));
               t[1] = t[2] = t[3] = t[0];
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_EXP:
            {
               GLfloat t[4], q[4], floor_t0;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, t );
               floor_t0 = FLOORF(t[0]);
               if (floor_t0 > FLT_MAX_EXP) {
                  SET_POS_INFINITY(q[0]);
                  SET_POS_INFINITY(q[2]);
               }
               else if (floor_t0 < FLT_MIN_EXP) {
                  q[0] = 0.0F;
                  q[2] = 0.0F;
               }
               else {
#ifdef USE_IEEE
                  GLint ii = (GLint) floor_t0;
                  ii = (ii < 23) + 0x3f800000;
                  SET_FLOAT_BITS(q[0], ii);
                  q[0] = *((GLfloat *) (void *)&ii);
#else
                  q[0] = (GLfloat) pow(2.0, floor_t0);
#endif
                  q[2] = (GLfloat) (q[0] * LOG2(q[1]));
               }
               q[1] = t[0] - floor_t0;
               q[3] = 1.0F;
               store_vector4( inst, machine, q );
            }
            break;
         case OPCODE_LOG:
            {
               GLfloat t[4], q[4], abs_t0;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, t );
               abs_t0 = FABSF(t[0]);
               if (abs_t0 != 0.0F) {
                  /* Since we really can't handle infinite values on VMS
                   * like other OSes we'll use __MAXFLOAT to represent
                   * infinity.  This may need some tweaking.
                   */
#ifdef VMS
                  if (abs_t0 == __MAXFLOAT)
#else
                  if (IS_INF_OR_NAN(abs_t0))
#endif
                  {
                     SET_POS_INFINITY(q[0]);
                     q[1] = 1.0F;
                     SET_POS_INFINITY(q[2]);
                  }
                  else {
                     int exponent;
                     GLfloat mantissa = FREXPF(t[0], &exponent);
                     q[0] = (GLfloat) (exponent - 1);
                     q[1] = (GLfloat) (2.0 * mantissa); /* map [.5, 1) -> [1, 2) */
                     q[2] = (GLfloat) (q[0] + LOG2(q[1]));
                  }
                  }
               else {
                  SET_NEG_INFINITY(q[0]);
                  q[1] = 1.0F;
                  SET_NEG_INFINITY(q[2]);
               }
               q[3] = 1.0;
               store_vector4( inst, machine, q );
            }
            break;
         case OPCODE_MUL:
            {
               GLfloat t[4], u[4], prod[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               prod[0] = t[0] * u[0];
               prod[1] = t[1] * u[1];
               prod[2] = t[2] * u[2];
               prod[3] = t[3] * u[3];
               store_vector4( inst, machine, prod );
            }
            break;
         case OPCODE_ADD:
            {
               GLfloat t[4], u[4], sum[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               sum[0] = t[0] + u[0];
               sum[1] = t[1] + u[1];
               sum[2] = t[2] + u[2];
               sum[3] = t[3] + u[3];
               store_vector4( inst, machine, sum );
            }
            break;
         case OPCODE_DP3:
            {
               GLfloat t[4], u[4], dot[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               dot[0] = t[0] * u[0] + t[1] * u[1] + t[2] * u[2];
               dot[1] = dot[2] = dot[3] = dot[0];
               store_vector4( inst, machine, dot );
            }
            break;
         case OPCODE_DP4:
            {
               GLfloat t[4], u[4], dot[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               dot[0] = t[0] * u[0] + t[1] * u[1] + t[2] * u[2] + t[3] * u[3];
               dot[1] = dot[2] = dot[3] = dot[0];
               store_vector4( inst, machine, dot );
            }
            break;
         case OPCODE_DST:
            {
               GLfloat t[4], u[4], dst[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               dst[0] = 1.0F;
               dst[1] = t[1] * u[1];
               dst[2] = t[2];
               dst[3] = u[3];
               store_vector4( inst, machine, dst );
            }
            break;
         case OPCODE_MIN:
            {
               GLfloat t[4], u[4], min[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               min[0] = (t[0] < u[0]) ? t[0] : u[0];
               min[1] = (t[1] < u[1]) ? t[1] : u[1];
               min[2] = (t[2] < u[2]) ? t[2] : u[2];
               min[3] = (t[3] < u[3]) ? t[3] : u[3];
               store_vector4( inst, machine, min );
            }
            break;
         case OPCODE_MAX:
            {
               GLfloat t[4], u[4], max[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               max[0] = (t[0] > u[0]) ? t[0] : u[0];
               max[1] = (t[1] > u[1]) ? t[1] : u[1];
               max[2] = (t[2] > u[2]) ? t[2] : u[2];
               max[3] = (t[3] > u[3]) ? t[3] : u[3];
               store_vector4( inst, machine, max );
            }
            break;
         case OPCODE_SLT:
            {
               GLfloat t[4], u[4], slt[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               slt[0] = (t[0] < u[0]) ? 1.0F : 0.0F;
               slt[1] = (t[1] < u[1]) ? 1.0F : 0.0F;
               slt[2] = (t[2] < u[2]) ? 1.0F : 0.0F;
               slt[3] = (t[3] < u[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, slt );
            }
            break;
         case OPCODE_SGE:
            {
               GLfloat t[4], u[4], sge[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               sge[0] = (t[0] >= u[0]) ? 1.0F : 0.0F;
               sge[1] = (t[1] >= u[1]) ? 1.0F : 0.0F;
               sge[2] = (t[2] >= u[2]) ? 1.0F : 0.0F;
               sge[3] = (t[3] >= u[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, sge );
            }
            break;
         case OPCODE_MAD:
            {
               GLfloat t[4], u[4], v[4], sum[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, v );
               sum[0] = t[0] * u[0] + v[0];
               sum[1] = t[1] * u[1] + v[1];
               sum[2] = t[2] * u[2] + v[2];
               sum[3] = t[3] * u[3] + v[3];
               store_vector4( inst, machine, sum );
            }
            break;
         case OPCODE_ARL:
            {
               GLfloat t[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               machine->AddressReg[0] = (GLint) FLOORF(t[0]);
            }
            break;
         case OPCODE_DPH:
            {
               GLfloat t[4], u[4], dot[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               dot[0] = t[0] * u[0] + t[1] * u[1] + t[2] * u[2] + u[3];
               dot[1] = dot[2] = dot[3] = dot[0];
               store_vector4( inst, machine, dot );
            }
            break;
         case OPCODE_RCC:
            {
               GLfloat t[4], u;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, t );
               if (t[0] == 1.0F)
                  u = 1.0F;
               else
                  u = 1.0F / t[0];
               if (u > 0.0F) {
                  if (u > 1.884467e+019F) {
                     u = 1.884467e+019F;  /* IEEE 32-bit binary value 0x5F800000 */
                  }
                  else if (u < 5.42101e-020F) {
                     u = 5.42101e-020F;   /* IEEE 32-bit binary value 0x1F800000 */
                  }
               }
               else {
                  if (u < -1.884467e+019F) {
                     u = -1.884467e+019F; /* IEEE 32-bit binary value 0xDF800000 */
                  }
                  else if (u > -5.42101e-020F) {
                     u = -5.42101e-020F;  /* IEEE 32-bit binary value 0x9F800000 */
                  }
               }
               t[0] = t[1] = t[2] = t[3] = u;
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_SUB: /* GL_NV_vertex_program1_1 */
            {
               GLfloat t[4], u[4], sum[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               sum[0] = t[0] - u[0];
               sum[1] = t[1] - u[1];
               sum[2] = t[2] - u[2];
               sum[3] = t[3] - u[3];
               store_vector4( inst, machine, sum );
            }
            break;
         case OPCODE_ABS: /* GL_NV_vertex_program1_1 */
            {
               GLfloat t[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               if (t[0] < 0.0)  t[0] = -t[0];
               if (t[1] < 0.0)  t[1] = -t[1];
               if (t[2] < 0.0)  t[2] = -t[2];
               if (t[3] < 0.0)  t[3] = -t[3];
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_FLR: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               t[0] = FLOORF(t[0]);
               t[1] = FLOORF(t[1]);
               t[2] = FLOORF(t[2]);
               t[3] = FLOORF(t[3]);
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_FRC: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               t[0] = t[0] - FLOORF(t[0]);
               t[1] = t[1] - FLOORF(t[1]);
               t[2] = t[2] - FLOORF(t[2]);
               t[3] = t[3] - FLOORF(t[3]);
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_EX2: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, t );
               t[0] = t[1] = t[2] = t[3] = (GLfloat)_mesa_pow(2.0, t[0]);
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_LG2: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, t );
               t[0] = t[1] = t[2] = t[3] = LOG2(t[0]);
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_POW: /* GL_ARB_vertex_program */
            {
               GLfloat t[4], u[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector1( ctx, &inst->SrcReg[1], machine, program, u );
               t[0] = t[1] = t[2] = t[3] = (GLfloat)_mesa_pow(t[0], u[0]);
               store_vector4( inst, machine, t );
            }
            break;
         case OPCODE_XPD: /* GL_ARB_vertex_program */
            {
               GLfloat t[4], u[4], cross[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, u );
               cross[0] = t[1] * u[2] - t[2] * u[1];
               cross[1] = t[2] * u[0] - t[0] * u[2];
               cross[2] = t[0] * u[1] - t[1] * u[0];
               store_vector4( inst, machine, cross );
            }
            break;
         case OPCODE_SWZ: /* GL_ARB_vertex_program */
            {
               const struct prog_src_register *source = &inst->SrcReg[0];
               const GLfloat *src = get_register_pointer(ctx, source,
                                                         machine, program);
               GLfloat result[4];
               GLuint i;

               /* do extended swizzling here */
               for (i = 0; i < 4; i++) {
                  const GLuint swz = GET_SWZ(source->Swizzle, i);
                  if (swz == SWIZZLE_ZERO)
                     result[i] = 0.0;
                  else if (swz == SWIZZLE_ONE)
                     result[i] = 1.0;
                  else {
                     ASSERT(swz >= 0);
                     ASSERT(swz <= 3);
                     result[i] = src[swz];
                  }
                  if (source->NegateBase & (1 << i))
                     result[i] = -result[i];
               }
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_PRINT:
            if (inst->SrcReg[0].File) {
               GLfloat t[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, t );
               _mesa_printf("%s%g, %g, %g, %g\n",
                            (char *) inst->Data, t[0], t[1], t[2], t[3]);
            }
            else {
               _mesa_printf("%s\n", (char *) inst->Data);
            }
            break;
         case OPCODE_END:
            ctx->_CurrentProgram = 0;
            return;
         default:
            /* bad instruction opcode */
            _mesa_problem(ctx, "Bad VP Opcode in _mesa_exec_vertex_program");
            ctx->_CurrentProgram = 0;
            return;
      } /* switch */
   } /* for */

   ctx->_CurrentProgram = 0;
}


/**
 * Execute a vertex state program.
 * \sa _mesa_ExecuteProgramNV
 */
void
_mesa_exec_vertex_state_program(GLcontext *ctx,
                                struct gl_vertex_program *vprog,
                                const GLfloat *params)
{
   struct vp_machine machine;
   _mesa_init_vp_per_vertex_registers(ctx, &machine);
   _mesa_init_vp_per_primitive_registers(ctx);
   COPY_4V(machine.Inputs[VERT_ATTRIB_POS], params);
   _mesa_exec_vertex_program(ctx, &machine, vprog);
}
