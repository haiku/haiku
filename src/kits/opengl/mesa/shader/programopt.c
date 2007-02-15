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
 * \file  programopt.c 
 * Vertex/Fragment program optimizations and transformations for program
 * options, etc.
 *
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "mtypes.h"
#include "program.h"
#include "programopt.h"
#include "program_instruction.h"


/**
 * This function inserts instructions for coordinate modelview * projection
 * into a vertex program.
 * May be used to implement the position_invariant option.
 */
void
_mesa_insert_mvp_code(GLcontext *ctx, struct gl_vertex_program *vprog)
{
   struct prog_instruction *newInst;
   const GLuint origLen = vprog->Base.NumInstructions;
   const GLuint newLen = origLen + 4;
   GLuint i;

   /*
    * Setup state references for the modelview/projection matrix.
    * XXX we should check if these state vars are already declared.
    */
   static const GLint mvpState[4][5] = {
      { STATE_MATRIX, STATE_MVP, 0, 0, 0 },  /* state.matrix.mvp.row[0] */
      { STATE_MATRIX, STATE_MVP, 0, 1, 1 },  /* state.matrix.mvp.row[1] */
      { STATE_MATRIX, STATE_MVP, 0, 2, 2 },  /* state.matrix.mvp.row[2] */
      { STATE_MATRIX, STATE_MVP, 0, 3, 3 },  /* state.matrix.mvp.row[3] */
   };
   GLint mvpRef[4];

   for (i = 0; i < 4; i++) {
      mvpRef[i] = _mesa_add_state_reference(vprog->Base.Parameters,
                                            mvpState[i]);
   }

   /* Alloc storage for new instructions */
   newInst = _mesa_alloc_instructions(newLen);
   if (!newInst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glProgramString(inserting position_invariant code)");
      return;
   }

   /*
    * Generated instructions:
    * newInst[0] = DP4 result.position.x, mvp.row[0], vertex.position;
    * newInst[1] = DP4 result.position.y, mvp.row[1], vertex.position;
    * newInst[2] = DP4 result.position.z, mvp.row[2], vertex.position;
    * newInst[3] = DP4 result.position.w, mvp.row[3], vertex.position;
    */
   _mesa_init_instructions(newInst, 4);
   for (i = 0; i < 4; i++) {
      newInst[i].Opcode = OPCODE_DP4;
      newInst[i].DstReg.File = PROGRAM_OUTPUT;
      newInst[i].DstReg.Index = VERT_RESULT_HPOS;
      newInst[i].DstReg.WriteMask = (WRITEMASK_X << i);
      newInst[i].SrcReg[0].File = PROGRAM_STATE_VAR;
      newInst[i].SrcReg[0].Index = mvpRef[i];
      newInst[i].SrcReg[0].Swizzle = SWIZZLE_NOOP;
      newInst[i].SrcReg[1].File = PROGRAM_INPUT;
      newInst[i].SrcReg[1].Index = VERT_ATTRIB_POS;
      newInst[i].SrcReg[1].Swizzle = SWIZZLE_NOOP;
   }

   /* Append original instructions after new instructions */
   _mesa_memcpy(newInst + 4, vprog->Base.Instructions,
                origLen * sizeof(struct prog_instruction));

   /* free old instructions */
   _mesa_free(vprog->Base.Instructions);

   /* install new instructions */
   vprog->Base.Instructions = newInst;
   vprog->Base.NumInstructions = newLen;
   vprog->Base.InputsRead |= VERT_BIT_POS;
   vprog->Base.OutputsWritten |= (1 << VERT_RESULT_HPOS);
}



/**
 * Append extra instructions onto the given fragment program to implement
 * the fog mode specified by fprog->FogOption.
 * The fragment.fogcoord input is used to compute the fog blend factor.
 *
 * XXX with a little work, this function could be adapted to add fog code
 * to vertex programs too.
 */
void
_mesa_append_fog_code(GLcontext *ctx, struct gl_fragment_program *fprog)
{
   static const GLint fogParamsState[] = { STATE_FOG_PARAMS, 0, 0, 0, 0 };
   static const GLint fogColorState[] = { STATE_FOG_COLOR, 0, 0, 0, 0 };
   struct prog_instruction *newInst, *inst;
   const GLuint origLen = fprog->Base.NumInstructions;
   const GLuint newLen = origLen + 6;
   GLuint i;
   GLint fogParamsRef, fogColorRef; /* state references */
   GLuint colorTemp, fogFactorTemp; /* temporary registerss */
   GLfloat fogVals[4];
   GLuint fogConsts;                /* constant values for EXP, EXP2 mode */

   if (fprog->FogOption == GL_NONE) {
      _mesa_problem(ctx, "_mesa_append_fog_code() called for fragment program"
                    " with FogOption == GL_NONE");
      return;
   }

   /* Alloc storage for new instructions */
   newInst = _mesa_alloc_instructions(newLen);
   if (!newInst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glProgramString(inserting fog_option code)");
      return;
   }

   /* Copy orig instructions into new instruction buffer */
   _mesa_memcpy(newInst, fprog->Base.Instructions,
                origLen * sizeof(struct prog_instruction));

   /* PARAM fogParamsRef = state.fog.params; */
   fogParamsRef
      = _mesa_add_state_reference(fprog->Base.Parameters, fogParamsState);
   /* PARAM fogColorRef = state.fog.color; */
   fogColorRef
      = _mesa_add_state_reference(fprog->Base.Parameters, fogColorState);

   /* TEMP colorTemp; */
   colorTemp = fprog->Base.NumTemporaries++;
   /* TEMP fogFactorTemp; */
   fogFactorTemp = fprog->Base.NumTemporaries++;

   /* PARAM fogVals = { 1/ln(2), 1/sqrt(ln(2), 0, 0 }; */
   fogVals[0] = 1.0 / log(2.0);
   fogVals[1] = 1.0 / SQRTF(log(2.0));
   fogVals[2] = 0.0;
   fogVals[3] = 0.0;
   fogConsts = _mesa_add_unnamed_constant(fprog->Base.Parameters, fogVals, 4);

   /* Scan program to find where result.color is written */
   inst = newInst;
   for (i = 0; i < fprog->Base.NumInstructions; i++) {
      if (inst->Opcode == OPCODE_END)
         break;
      if (inst->DstReg.File == PROGRAM_OUTPUT &&
          inst->DstReg.Index == FRAG_RESULT_COLR) {
         /* change the instruction to write to colorTemp w/ clamping */
         inst->DstReg.File = PROGRAM_TEMPORARY;
         inst->DstReg.Index = colorTemp;
         inst->SaturateMode = SATURATE_ZERO_ONE;
         /* don't break (may be several writes to result.color) */
      }
      inst++;
   }
   assert(inst->Opcode == OPCODE_END); /* we'll overwrite this inst */

   _mesa_init_instructions(inst, 6);

   /* emit instructions to compute fog blending factor */
   if (fprog->FogOption == GL_LINEAR) {
      /* SUB fogFactorTemp.x, fogParamsRef.z, fragment.fogcoord.x; */
      inst->Opcode = OPCODE_SUB;
      inst->DstReg.File = PROGRAM_TEMPORARY;
      inst->DstReg.Index = fogFactorTemp;
      inst->DstReg.WriteMask = WRITEMASK_X;
      inst->SrcReg[0].File = PROGRAM_STATE_VAR;
      inst->SrcReg[0].Index = fogParamsRef;
      inst->SrcReg[0].Swizzle = SWIZZLE_Z;
      inst->SrcReg[1].File = PROGRAM_INPUT;
      inst->SrcReg[1].Index = FRAG_ATTRIB_FOGC;
      inst++;
      /* MUL fogFactorTemp.x, fogFactorTemp, fogParamsRef.w; */
      inst->Opcode = OPCODE_MUL;
      inst->DstReg.File = PROGRAM_TEMPORARY;
      inst->DstReg.Index = fogFactorTemp;
      inst->DstReg.WriteMask = WRITEMASK_X;
      inst->SrcReg[0].File = PROGRAM_TEMPORARY;
      inst->SrcReg[0].Index = fogFactorTemp;
      inst->SrcReg[1].File = PROGRAM_STATE_VAR;
      inst->SrcReg[1].Index = fogParamsRef;
      inst->SrcReg[1].Swizzle = SWIZZLE_W;
      inst++;
   }
   else {
      ASSERT(fprog->FogOption == GL_EXP || fprog->FogOption == GL_EXP2);
      /* MUL fogFactorTemp.x, fogParamsRef.x, fragment.fogcoord; */
      inst->Opcode = OPCODE_MUL;
      inst->DstReg.File = PROGRAM_TEMPORARY;
      inst->DstReg.Index = fogFactorTemp;
      inst->DstReg.WriteMask = WRITEMASK_X;
      inst->SrcReg[0].File = PROGRAM_STATE_VAR;
      inst->SrcReg[0].Index = fogParamsRef;
      inst->SrcReg[0].Swizzle = SWIZZLE_X; /* X=density */
      inst->SrcReg[1].File = PROGRAM_INPUT;
      inst->SrcReg[1].Index = FRAG_ATTRIB_FOGC;
      inst->SrcReg[1].Swizzle = SWIZZLE_X;
      inst++;
      if (fprog->FogOption == GL_EXP2) {
         /* MUL fogFactorTemp.x, fogFactorTemp.x, fogFactorTemp.x; */
         inst->Opcode = OPCODE_MUL;
         inst->DstReg.File = PROGRAM_TEMPORARY;
         inst->DstReg.Index = fogFactorTemp;
         inst->DstReg.WriteMask = WRITEMASK_X;
         inst->SrcReg[0].File = PROGRAM_TEMPORARY;
         inst->SrcReg[0].Index = fogFactorTemp;
         inst->SrcReg[1].File = PROGRAM_TEMPORARY;
         inst->SrcReg[1].Index = fogFactorTemp;
         inst++;
      }
      /* EXP:  MUL fogFactorTemp.x, fogFactorTemp.x, {1/ln(2)}; */
      /* EXP2: MUL fogFactorTemp.x, fogFactorTemp.x, {1/sqrt(ln(2))}; */
      inst->Opcode = OPCODE_MUL;
      inst->DstReg.File = PROGRAM_TEMPORARY;
      inst->DstReg.Index = fogFactorTemp;
      inst->DstReg.WriteMask = WRITEMASK_X;
      inst->SrcReg[0].File = PROGRAM_TEMPORARY;
      inst->SrcReg[0].Index = fogFactorTemp;
      inst->SrcReg[1].File = PROGRAM_CONSTANT;
      inst->SrcReg[1].Index = fogConsts;
      inst->SrcReg[1].Swizzle
         = (fprog->FogOption == GL_EXP) ? SWIZZLE_X : SWIZZLE_Y;
      inst++;
      /* EX2_SAT fogFactorTemp.x, -fogFactorTemp.x; */
      inst->Opcode = OPCODE_EX2;
      inst->DstReg.File = PROGRAM_TEMPORARY;
      inst->DstReg.Index = fogFactorTemp;
      inst->DstReg.WriteMask = WRITEMASK_X;
      inst->SrcReg[0].File = PROGRAM_TEMPORARY;
      inst->SrcReg[0].Index = fogFactorTemp;
      inst->SrcReg[0].NegateBase = GL_TRUE;
      inst->SaturateMode = SATURATE_ZERO_ONE;
      inst++;
   }
   /* LRP result.color.xyz, fogFactorTemp.xxxx, colorTemp, fogColorRef; */
   inst->Opcode = OPCODE_LRP;
   inst->DstReg.File = PROGRAM_OUTPUT;
   inst->DstReg.Index = FRAG_RESULT_COLR;
   inst->DstReg.WriteMask = WRITEMASK_XYZ;
   inst->SrcReg[0].File = PROGRAM_TEMPORARY;
   inst->SrcReg[0].Index = fogFactorTemp;
   inst->SrcReg[0].Swizzle
      = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X);
   inst->SrcReg[1].File = PROGRAM_TEMPORARY;
   inst->SrcReg[1].Index = colorTemp;
   inst->SrcReg[2].File = PROGRAM_STATE_VAR;
   inst->SrcReg[2].Index = fogColorRef;
   inst++;
   /* MOV result.color.w, colorTemp.x;  # copy alpha */
   inst->Opcode = OPCODE_MOV;
   inst->DstReg.File = PROGRAM_OUTPUT;
   inst->DstReg.Index = FRAG_RESULT_COLR;
   inst->DstReg.WriteMask = WRITEMASK_W;
   inst->SrcReg[0].File = PROGRAM_TEMPORARY;
   inst->SrcReg[0].Index = colorTemp;
   inst++;
   /* END; */
   inst->Opcode = OPCODE_END;
   inst++;

   /* free old instructions */
   _mesa_free(fprog->Base.Instructions);

   /* install new instructions */
   fprog->Base.Instructions = newInst;
   fprog->Base.NumInstructions = inst - newInst;
   fprog->Base.InputsRead |= FRAG_BIT_FOGC;
   /* XXX do this?  fprog->FogOption = GL_NONE; */
}
