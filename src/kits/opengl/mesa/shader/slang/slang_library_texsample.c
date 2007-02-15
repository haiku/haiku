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
 * \file slang_library_texsample.c
 * built-in library functions for texture and shadow sampling
 * \author Michal Krol
 */

#include "imports.h"
#include "context.h"
#include "colormac.h"
#include "swrast/s_context.h"
#include "slang_library_texsample.h"

GLvoid _slang_library_tex1d (GLfloat bias, GLfloat s, GLfloat sampler, GLfloat *color)
{
	GET_CURRENT_CONTEXT(ctx);
	SWcontext *swrast = SWRAST_CONTEXT(ctx);
	GLuint unit = (GLuint) sampler;
	GLfloat texcoord[4];
	GLfloat lambda = bias;
	GLchan rgba[4];

	texcoord[0] = s;
	texcoord[1] = 0.0f;
	texcoord[2] = 0.0f;
	texcoord[3] = 1.0f;

	swrast->TextureSample[unit] (ctx, ctx->Texture.Unit[unit]._Current, 1,
		(const GLfloat (*)[4]) texcoord, &lambda, &rgba);
	color[0] = CHAN_TO_FLOAT(rgba[0]);
	color[1] = CHAN_TO_FLOAT(rgba[1]);
	color[2] = CHAN_TO_FLOAT(rgba[2]);
	color[3] = CHAN_TO_FLOAT(rgba[3]);
}

GLvoid _slang_library_tex2d (GLfloat bias, GLfloat s, GLfloat t, GLfloat sampler, GLfloat *color)
{
	GET_CURRENT_CONTEXT(ctx);
	SWcontext *swrast = SWRAST_CONTEXT(ctx);
	GLuint unit = (GLuint) sampler;
	GLfloat texcoord[4];
	GLfloat lambda = bias;
	GLchan rgba[4];

	texcoord[0] = s;
	texcoord[1] = t;
	texcoord[2] = 0.0f;
	texcoord[3] = 1.0f;

	swrast->TextureSample[unit] (ctx, ctx->Texture.Unit[unit]._Current, 1,
		(const GLfloat (*)[4]) texcoord, &lambda, &rgba);
	color[0] = CHAN_TO_FLOAT(rgba[0]);
	color[1] = CHAN_TO_FLOAT(rgba[1]);
	color[2] = CHAN_TO_FLOAT(rgba[2]);
	color[3] = CHAN_TO_FLOAT(rgba[3]);
}

GLvoid _slang_library_tex3d (GLfloat bias, GLfloat s, GLfloat t, GLfloat r, GLfloat sampler,
	GLfloat *color)
{
	GET_CURRENT_CONTEXT(ctx);
	SWcontext *swrast = SWRAST_CONTEXT(ctx);
	GLuint unit = (GLuint) sampler;
	GLfloat texcoord[4];
	GLfloat lambda = bias;
	GLchan rgba[4];

	texcoord[0] = s;
	texcoord[1] = t;
	texcoord[2] = r;
	texcoord[3] = 1.0f;

	swrast->TextureSample[unit] (ctx, ctx->Texture.Unit[unit]._Current, 1,
		(const GLfloat (*)[4]) texcoord, &lambda, &rgba);
	color[0] = CHAN_TO_FLOAT(rgba[0]);
	color[1] = CHAN_TO_FLOAT(rgba[1]);
	color[2] = CHAN_TO_FLOAT(rgba[2]);
	color[3] = CHAN_TO_FLOAT(rgba[3]);
}

GLvoid _slang_library_texcube (GLfloat bias, GLfloat s, GLfloat t, GLfloat r, GLfloat sampler,
	GLfloat *color)
{
	GET_CURRENT_CONTEXT(ctx);
	SWcontext *swrast = SWRAST_CONTEXT(ctx);
	GLuint unit = (GLuint) sampler;
	GLfloat texcoord[4];
	GLfloat lambda = bias;
	GLchan rgba[4];

	texcoord[0] = s;
	texcoord[1] = t;
	texcoord[2] = r;
	texcoord[3] = 1.0f;

	swrast->TextureSample[unit] (ctx, ctx->Texture.Unit[unit]._Current, 1,
		(const GLfloat (*)[4]) texcoord, &lambda, &rgba);
	color[0] = CHAN_TO_FLOAT(rgba[0]);
	color[1] = CHAN_TO_FLOAT(rgba[1]);
	color[2] = CHAN_TO_FLOAT(rgba[2]);
	color[3] = CHAN_TO_FLOAT(rgba[3]);
}

GLvoid _slang_library_shad1d (GLfloat bias, GLfloat s, GLfloat t, GLfloat r, GLfloat sampler,
	GLfloat *color)
{
	GET_CURRENT_CONTEXT(ctx);
	SWcontext *swrast = SWRAST_CONTEXT(ctx);
	GLuint unit = (GLuint) sampler;
	GLfloat texcoord[4];
	GLfloat lambda = bias;
	GLchan rgba[4];

	texcoord[0] = s;
	texcoord[1] = t;
	texcoord[2] = r;
	texcoord[3] = 1.0f;

	swrast->TextureSample[unit] (ctx, ctx->Texture.Unit[unit]._Current, 1,
		(const GLfloat (*)[4]) texcoord, &lambda, &rgba);
	color[0] = CHAN_TO_FLOAT(rgba[0]);
	color[1] = CHAN_TO_FLOAT(rgba[1]);
	color[2] = CHAN_TO_FLOAT(rgba[2]);
	color[3] = CHAN_TO_FLOAT(rgba[3]);
}

GLvoid _slang_library_shad2d (GLfloat bias, GLfloat s, GLfloat t, GLfloat r, GLfloat sampler,
	GLfloat *color)
{
	GET_CURRENT_CONTEXT(ctx);
	SWcontext *swrast = SWRAST_CONTEXT(ctx);
	GLuint unit = (GLuint) sampler;
	GLfloat texcoord[4];
	GLfloat lambda = bias;
	GLchan rgba[4];

	texcoord[0] = s;
	texcoord[1] = t;
	texcoord[2] = r;
	texcoord[3] = 1.0f;

	swrast->TextureSample[unit] (ctx, ctx->Texture.Unit[unit]._Current, 1,
		(const GLfloat (*)[4]) texcoord, &lambda, &rgba);
	color[0] = CHAN_TO_FLOAT(rgba[0]);
	color[1] = CHAN_TO_FLOAT(rgba[1]);
	color[2] = CHAN_TO_FLOAT(rgba[2]);
	color[3] = CHAN_TO_FLOAT(rgba[3]);
}

