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
 * \file slang_analyse.c
 * slang assembly code analysis
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_analyse.h"
#include "slang_utility.h"

GLboolean _slang_analyse_texture_usage (slang_program *prog)
{
	GLuint i, count = 0;

	_slang_texture_usages_dtr (&prog->texture_usage);
	_slang_texture_usages_ctr (&prog->texture_usage);

	/*
	 * We could do a full code analysis to find out which uniforms are actually used.
	 * For now, we are very conservative and extract them from uniform binding table, which
	 * in turn also do not come from code analysis.
	 */

	for (i = 0; i < prog->uniforms.count; i++)
	{
		slang_uniform_binding *b = &prog->uniforms.table[i];

		if (b->address[SLANG_SHADER_FRAGMENT] != ~0 && !slang_export_data_quant_struct (b->quant))
		{
			switch (slang_export_data_quant_type (b->quant))
			{
			case GL_SAMPLER_1D_ARB:
			case GL_SAMPLER_2D_ARB:
			case GL_SAMPLER_3D_ARB:
			case GL_SAMPLER_CUBE_ARB:
			case GL_SAMPLER_1D_SHADOW_ARB:
			case GL_SAMPLER_2D_SHADOW_ARB:
				count++;
				break;
			}
		}
	}

	if (count == 0)
		return GL_TRUE;
	prog->texture_usage.table = (slang_texture_usage *) slang_alloc_malloc (
		count * sizeof (slang_texture_usage));
	if (prog->texture_usage.table == NULL)
		return GL_FALSE;
	prog->texture_usage.count = count;

	for (count = i = 0; i < prog->uniforms.count; i++)
	{
		slang_uniform_binding *b = &prog->uniforms.table[i];

		if (b->address[SLANG_SHADER_FRAGMENT] != ~0 && !slang_export_data_quant_struct (b->quant))
		{
			switch (slang_export_data_quant_type (b->quant))
			{
			case GL_SAMPLER_1D_ARB:
			case GL_SAMPLER_2D_ARB:
			case GL_SAMPLER_3D_ARB:
			case GL_SAMPLER_CUBE_ARB:
			case GL_SAMPLER_1D_SHADOW_ARB:
			case GL_SAMPLER_2D_SHADOW_ARB:
				prog->texture_usage.table[count].quant = b->quant;
				prog->texture_usage.table[count].frag_address = b->address[SLANG_SHADER_FRAGMENT];
				count++;
				break;
			}
		}
	}

	return GL_TRUE;
}

