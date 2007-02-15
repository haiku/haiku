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
 *
 * Authors:
 *    Michal Krol
 */

#include "glheader.h"
#include "imports.h"
#include "macros.h"
#include "shaderobjects.h"
#include "shaderobjects_3dlabs.h"
#include "t_pipeline.h"
#include "slang_utility.h"
#include "slang_link.h"

#if FEATURE_ARB_vertex_shader

typedef struct
{
	GLvector4f outputs[VERT_RESULT_MAX];
	GLvector4f varyings[MAX_VARYING_VECTORS];
	GLvector4f ndc_coords;
	GLubyte *clipmask;
	GLubyte ormask;
	GLubyte andmask;
} arbvs_stage_data;

#define ARBVS_STAGE_DATA(stage) ((arbvs_stage_data *) stage->privatePtr)

static GLboolean construct_arb_vertex_shader (GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;
	arbvs_stage_data *store;
	GLuint size = vb->Size;
	GLuint i;

	stage->privatePtr = _mesa_malloc (sizeof (arbvs_stage_data));
	store = ARBVS_STAGE_DATA(stage);
	if (store == NULL)
		return GL_FALSE;

	for (i = 0; i < VERT_RESULT_MAX; i++)
	{
		_mesa_vector4f_alloc (&store->outputs[i], 0, size, 32);
		store->outputs[i].size = 4;
	}
	for (i = 0; i < MAX_VARYING_VECTORS; i++)
	{
		_mesa_vector4f_alloc (&store->varyings[i], 0, size, 32);
		store->varyings[i].size = 4;
	}
	_mesa_vector4f_alloc (&store->ndc_coords, 0, size, 32);
	store->clipmask = (GLubyte *) ALIGN_MALLOC (size, 32);

	return GL_TRUE;
}

static void destruct_arb_vertex_shader (struct tnl_pipeline_stage *stage)
{
	arbvs_stage_data *store = ARBVS_STAGE_DATA(stage);

	if (store != NULL)
	{
		GLuint i;

		for (i = 0; i < VERT_RESULT_MAX; i++)
			_mesa_vector4f_free (&store->outputs[i]);
		for (i = 0; i < MAX_VARYING_VECTORS; i++)
			_mesa_vector4f_free (&store->varyings[i]);
		_mesa_vector4f_free (&store->ndc_coords);
		ALIGN_FREE (store->clipmask);

		_mesa_free (store);
		stage->privatePtr = NULL;
	}
}

static void validate_arb_vertex_shader (GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
}

static GLvoid fetch_input_float (struct gl2_program_intf **pro, GLuint index, GLuint attr, GLuint i,
	struct vertex_buffer *vb)
{
	const GLubyte *ptr = (const GLubyte *) vb->AttribPtr[attr]->data;
	const GLuint stride = vb->AttribPtr[attr]->stride;
   GLfloat *data = (GLfloat *) (ptr + stride * i);

   (**pro).UpdateFixedAttrib (pro, index, data, 0, sizeof (GLfloat), GL_TRUE);
}

static GLvoid fetch_input_vec3 (struct gl2_program_intf **pro, GLuint index, GLuint attr, GLuint i,
	struct vertex_buffer *vb)
{
	const GLubyte *ptr = (const GLubyte *) vb->AttribPtr[attr]->data;
	const GLuint stride = vb->AttribPtr[attr]->stride;
   GLfloat *data = (GLfloat *) (ptr + stride * i);

   (**pro).UpdateFixedAttrib (pro, index, data, 0, 3 * sizeof (GLfloat), GL_TRUE);
}

static void fetch_input_vec4 (struct gl2_program_intf **pro, GLuint index, GLuint attr, GLuint i,
	struct vertex_buffer *vb)
{
	const GLubyte *ptr = (const GLubyte *) vb->AttribPtr[attr]->data;
	const GLuint size = vb->AttribPtr[attr]->size;
	const GLuint stride = vb->AttribPtr[attr]->stride;
	const GLfloat *data = (const GLfloat *) (ptr + stride * i);
	GLfloat vec[4];

	switch (size)
	{
	case 2:
		vec[0] = data[0];
		vec[1] = data[1];
		vec[2] = 0.0f;
		vec[3] = 1.0f;
		break;
	case 3:
		vec[0] = data[0];
		vec[1] = data[1];
		vec[2] = data[2];
		vec[3] = 1.0f;
		break;
	case 4:
		vec[0] = data[0];
		vec[1] = data[1];
		vec[2] = data[2];
		vec[3] = data[3];
		break;
	}
   (**pro).UpdateFixedAttrib (pro, index, vec, 0, 4 * sizeof (GLfloat), GL_TRUE);
}

static GLvoid
fetch_gen_attrib (struct gl2_program_intf **pro, GLuint index, GLuint i, struct vertex_buffer *vb)
{
   const GLuint attr = _TNL_ATTRIB_GENERIC0 + index;
   const GLubyte *ptr = (const GLubyte *) (vb->AttribPtr[attr]->data);
   const GLuint stride = vb->AttribPtr[attr]->stride;
   const GLfloat *data = (const GLfloat *) (ptr + stride * i);

   (**pro).WriteAttrib (pro, index, data);
}

static GLvoid fetch_output_float (struct gl2_program_intf **pro, GLuint index, GLuint attr, GLuint i,
	arbvs_stage_data *store)
{
   (**pro).UpdateFixedAttrib (pro, index, &store->outputs[attr].data[i], 0, sizeof (GLfloat),
                              GL_FALSE);
}

static void fetch_output_vec4 (struct gl2_program_intf **pro, GLuint index, GLuint attr, GLuint i,
	GLuint offset, arbvs_stage_data *store)
{
   (**pro).UpdateFixedAttrib (pro, index, &store->outputs[attr].data[i], offset,
                              4 * sizeof (GLfloat), GL_FALSE);
}

static GLboolean run_arb_vertex_shader (GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;
	arbvs_stage_data *store = ARBVS_STAGE_DATA(stage);
	struct gl2_program_intf **pro;
	GLsizei i, j;

	if (!ctx->ShaderObjects._VertexShaderPresent)
		return GL_TRUE;

	pro = ctx->ShaderObjects.CurrentProgram;
	(**pro).UpdateFixedUniforms (pro);

	for (i = 0; i < vb->Count; i++)
	{
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_VERTEX, _TNL_ATTRIB_POS, i, vb);
		fetch_input_vec3 (pro, SLANG_VERTEX_FIXED_NORMAL, _TNL_ATTRIB_NORMAL, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_COLOR, _TNL_ATTRIB_COLOR0, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_SECONDARYCOLOR, _TNL_ATTRIB_COLOR1, i, vb);
		fetch_input_float (pro, SLANG_VERTEX_FIXED_FOGCOORD, _TNL_ATTRIB_FOG, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_MULTITEXCOORD0, _TNL_ATTRIB_TEX0, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_MULTITEXCOORD1, _TNL_ATTRIB_TEX1, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_MULTITEXCOORD2, _TNL_ATTRIB_TEX2, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_MULTITEXCOORD3, _TNL_ATTRIB_TEX3, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_MULTITEXCOORD4, _TNL_ATTRIB_TEX4, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_MULTITEXCOORD5, _TNL_ATTRIB_TEX5, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_MULTITEXCOORD6, _TNL_ATTRIB_TEX6, i, vb);
		fetch_input_vec4 (pro, SLANG_VERTEX_FIXED_MULTITEXCOORD7, _TNL_ATTRIB_TEX7, i, vb);
      for (j = 0; j < MAX_VERTEX_ATTRIBS; j++)
         fetch_gen_attrib (pro, j, i, vb);

		_slang_exec_vertex_shader (pro);

		fetch_output_vec4 (pro, SLANG_VERTEX_FIXED_POSITION, VERT_RESULT_HPOS, i, 0, store);
		fetch_output_vec4 (pro, SLANG_VERTEX_FIXED_FRONTCOLOR, VERT_RESULT_COL0, i, 0, store);
		fetch_output_vec4 (pro, SLANG_VERTEX_FIXED_FRONTSECONDARYCOLOR, VERT_RESULT_COL1, i, 0, store);
		fetch_output_float (pro, SLANG_VERTEX_FIXED_FOGFRAGCOORD, VERT_RESULT_FOGC, i, store);
		for (j = 0; j < 8; j++)
			fetch_output_vec4 (pro, SLANG_VERTEX_FIXED_TEXCOORD, VERT_RESULT_TEX0 + j, i, j, store);
		fetch_output_float (pro, SLANG_VERTEX_FIXED_POINTSIZE, VERT_RESULT_PSIZ, i, store);
		fetch_output_vec4 (pro, SLANG_VERTEX_FIXED_BACKCOLOR, VERT_RESULT_BFC0, i, 0, store);
		fetch_output_vec4 (pro, SLANG_VERTEX_FIXED_BACKSECONDARYCOLOR, VERT_RESULT_BFC1, i, 0, store);
		/* XXX: fetch output SLANG_VERTEX_FIXED_CLIPVERTEX */

		for (j = 0; j < MAX_VARYING_VECTORS; j++)
		{
			GLuint k;

			for (k = 0; k < VARYINGS_PER_VECTOR; k++)
			{
				(**pro).UpdateVarying (pro, j * VARYINGS_PER_VECTOR + k,
					&store->varyings[j].data[i][k], GL_TRUE);
			}
		}
	}

	vb->ClipPtr = &store->outputs[VERT_RESULT_HPOS];
	vb->ClipPtr->count = vb->Count;

	vb->ColorPtr[0] = &store->outputs[VERT_RESULT_COL0];
	vb->AttribPtr[VERT_ATTRIB_COLOR0] = vb->ColorPtr[0];
	vb->ColorPtr[1] = &store->outputs[VERT_RESULT_BFC0];

	vb->SecondaryColorPtr[0] =
	vb->AttribPtr[VERT_ATTRIB_COLOR1] = &store->outputs[VERT_RESULT_COL1];

	vb->SecondaryColorPtr[1] = &store->outputs[VERT_RESULT_BFC1];

	for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
		vb->TexCoordPtr[i] =
		vb->AttribPtr[VERT_ATTRIB_TEX0 + i] = &store->outputs[VERT_RESULT_TEX0 + i];
        }

	vb->FogCoordPtr =
	vb->AttribPtr[VERT_ATTRIB_FOG] = &store->outputs[VERT_RESULT_FOGC];

	vb->AttribPtr[_TNL_ATTRIB_POINTSIZE] = &store->outputs[VERT_RESULT_PSIZ];

	for (i = 0; i < MAX_VARYING_VECTORS; i++) {
		vb->VaryingPtr[i] = &store->varyings[i];
		vb->AttribPtr[_TNL_ATTRIB_GENERIC0 + i] = vb->VaryingPtr[i];
        }

	store->ormask = 0;
	store->andmask = CLIP_FRUSTUM_BITS;

	if (tnl->NeedNdcCoords)
	{
		vb->NdcPtr = _mesa_clip_tab[vb->ClipPtr->size] (vb->ClipPtr, &store->ndc_coords,
			store->clipmask, &store->ormask, &store->andmask);
	}
	else
	{
		vb->NdcPtr = NULL;
		_mesa_clip_np_tab[vb->ClipPtr->size] (vb->ClipPtr, NULL, store->clipmask, &store->ormask,
			&store->andmask);
	}

	if (store->andmask)
		return GL_FALSE;

	vb->ClipAndMask = store->andmask;
	vb->ClipOrMask = store->ormask;
	vb->ClipMask = store->clipmask;

	return GL_TRUE;
}

const struct tnl_pipeline_stage _tnl_arb_vertex_shader_stage = {
	"ARB_vertex_shader",
	NULL,
	construct_arb_vertex_shader,
	destruct_arb_vertex_shader,
	validate_arb_vertex_shader,
	run_arb_vertex_shader
};

#endif /* FEATURE_ARB_vertex_shader */

