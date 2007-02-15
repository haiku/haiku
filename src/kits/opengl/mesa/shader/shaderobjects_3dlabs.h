/*
 * Mesa 3-D graphics library
 * Version:  6.5
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

#ifndef SHADEROBJECTS_3DLABS_H
#define SHADEROBJECTS_3DLABS_H

#if FEATURE_ARB_shader_objects

extern int _slang_fetch_discard (struct gl2_program_intf **pro, GLboolean *val);

extern GLvoid _slang_exec_fragment_shader (struct gl2_program_intf **pro);

extern GLvoid _slang_exec_vertex_shader (struct gl2_program_intf **pro);

extern GLhandleARB
_mesa_3dlabs_create_shader_object (GLenum);

extern GLhandleARB
_mesa_3dlabs_create_program_object (GLvoid);

extern GLhandleARB
_mesa_3dlabs_create_debug_object (GLvoid);

#endif /* FEATURE_ARB_shader_objects */

extern void
_mesa_init_shaderobjects_3dlabs (GLcontext *ctx);

#endif

