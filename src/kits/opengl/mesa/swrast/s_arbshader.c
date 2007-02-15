/*
 * Mesa 3-D graphics library
 * Version:  6.6
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
 *
 * Authors:
 *    Michal Krol
 */

#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "s_arbshader.h"
#include "s_context.h"
#include "shaderobjects.h"
#include "shaderobjects_3dlabs.h"
#include "slang_utility.h"
#include "slang_link.h"

#if FEATURE_ARB_fragment_shader

void
_swrast_exec_arbshader(GLcontext *ctx, SWspan *span)
{
   struct gl2_program_intf **pro;
   GLuint i;

   ASSERT(span->array->ChanType == GL_FLOAT);

   if (!ctx->ShaderObjects._FragmentShaderPresent)
      return;

   pro = ctx->ShaderObjects.CurrentProgram;
   if (!ctx->ShaderObjects._VertexShaderPresent)
      (**pro).UpdateFixedUniforms(pro);

   for (i = span->start; i < span->end; i++) {
      /* only run shader on active fragments */
      if (span->array->mask[i]) {
         GLfloat vec[4];
         GLuint j;
         GLboolean discard;

         /*
          * Load input attributes
          */
         vec[0] = (GLfloat) span->x + i;
         vec[1] = (GLfloat) span->y;
         vec[2] = (GLfloat) span->array->z[i] / ctx->DrawBuffer->_DepthMaxF;
         vec[3] = span->w + span->dwdx * i;
         (**pro).UpdateFixedVarying(pro, SLANG_FRAGMENT_FIXED_FRAGCOORD, vec,
                                    0, 4 * sizeof(GLfloat), GL_TRUE);

         (**pro).UpdateFixedVarying(pro, SLANG_FRAGMENT_FIXED_COLOR,
                                    span->array->color.sz4.rgba[i],
                                    0, 4 * sizeof(GLfloat), GL_TRUE);

         (**pro).UpdateFixedVarying(pro, SLANG_FRAGMENT_FIXED_SECONDARYCOLOR,
                                    span->array->color.sz4.spec[i],
                                    0, 4 * sizeof(GLfloat), GL_TRUE);

         for (j = 0; j < ctx->Const.MaxTextureCoordUnits; j++) {
            (**pro).UpdateFixedVarying(pro, SLANG_FRAGMENT_FIXED_TEXCOORD,
                                       span->array->texcoords[j][i],
                                       j, 4 * sizeof(GLfloat), GL_TRUE);
         }

         for (j = 0; j < MAX_VARYING_VECTORS; j++) {
            GLuint k;
            for (k = 0; k < VARYINGS_PER_VECTOR; k++) {
               (**pro).UpdateVarying(pro, j * VARYINGS_PER_VECTOR + k,
                                     &span->array->varying[i][j][k],
                                     GL_FALSE);
            }
         }

         _slang_exec_fragment_shader(pro);

         /*
          * Store results
          */
         _slang_fetch_discard(pro, &discard);
         if (discard) {
            span->array->mask[i] = GL_FALSE;
            span->writeAll = GL_FALSE;
         }
         else {
            (**pro).UpdateFixedVarying(pro, SLANG_FRAGMENT_FIXED_FRAGCOLOR,
                                       vec, 0, 4 * sizeof(GLfloat), GL_FALSE);
            COPY_4V(span->array->color.sz4.rgba[i], vec);
            
            (**pro).UpdateFixedVarying(pro, SLANG_FRAGMENT_FIXED_FRAGDEPTH, vec, 0,
                                       sizeof (GLfloat), GL_FALSE);
            if (vec[0] <= 0.0f)
               span->array->z[i] = 0;
            else if (vec[0] >= 1.0f)
               span->array->z[i] = ctx->DrawBuffer->_DepthMax;
            else
               span->array->z[i] = IROUND(vec[0] * ctx->DrawBuffer->_DepthMaxF);
         }
      }
   }
}

#endif /* FEATURE_ARB_fragment_shader */

