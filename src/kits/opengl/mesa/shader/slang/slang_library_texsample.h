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

#if !defined SLANG_LIBRARY_TEXSAMPLE_H
#define SLANG_LIBRARY_TEXSAMPLE_H

#if defined __cplusplus
extern "C" {
#endif

GLvoid _slang_library_tex1d (GLfloat, GLfloat, GLfloat, GLfloat *);
GLvoid _slang_library_tex2d (GLfloat, GLfloat, GLfloat, GLfloat, GLfloat *);
GLvoid _slang_library_tex3d (GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat *);
GLvoid _slang_library_texcube (GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat *);
GLvoid _slang_library_shad1d (GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat *);
GLvoid _slang_library_shad2d (GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat *);

#ifdef __cplusplus
}
#endif

#endif

