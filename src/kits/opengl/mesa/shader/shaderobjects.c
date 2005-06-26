/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 2004  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "shaderobjects.h"
#include "mtypes.h"
#include "context.h"
#include "macros.h"

void GLAPIENTRY
_mesa_DeleteObjectARB(GLhandleARB obj)
{
}

GLhandleARB GLAPIENTRY
_mesa_GetHandleARB(GLenum pname)
{
   return 0; 
}

void GLAPIENTRY
_mesa_DetachObjectARB (GLhandleARB containerObj, GLhandleARB attachedObj)
{
}

GLhandleARB GLAPIENTRY
_mesa_CreateShaderObjectARB (GLenum shaderType)
{
   return 0;
}

void GLAPIENTRY
_mesa_ShaderSourceARB (GLhandleARB shaderObj, GLsizei count,
		       const GLcharARB **string, const GLint *length)
{
}

void  GLAPIENTRY
_mesa_CompileShaderARB (GLhandleARB shaderObj)
{
}

GLhandleARB GLAPIENTRY
_mesa_CreateProgramObjectARB (void)
{
   return 0;
}

void GLAPIENTRY
_mesa_AttachObjectARB (GLhandleARB containerObj, GLhandleARB obj)
{
}

void GLAPIENTRY
_mesa_LinkProgramARB (GLhandleARB programObj)
{
}

#if 0
GLboolean
_mesa_use_program_object( GLcontext *ctx, struct gl_program_object *pobj )
{
   return GL_FALSE;
}
#endif

void GLAPIENTRY
_mesa_UseProgramObjectARB (GLhandleARB programObj)
{
}

void GLAPIENTRY
_mesa_ValidateProgramARB (GLhandleARB programObj)
{
}

void GLAPIENTRY
_mesa_Uniform1fARB (GLint location, GLfloat v0)
{
}

void GLAPIENTRY
_mesa_Uniform2fARB (GLint location, GLfloat v0, GLfloat v1)
{
}

void GLAPIENTRY
_mesa_Uniform3fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
}

void GLAPIENTRY
_mesa_Uniform4fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
}

void GLAPIENTRY
_mesa_Uniform1iARB (GLint location, GLint v0)
{
}

void GLAPIENTRY
_mesa_Uniform2iARB (GLint location, GLint v0, GLint v1)
{
}

void GLAPIENTRY
_mesa_Uniform3iARB (GLint location, GLint v0, GLint v1, GLint v2)
{
}

void GLAPIENTRY
_mesa_Uniform4iARB (GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
}

void GLAPIENTRY
_mesa_Uniform1fvARB (GLint location, GLsizei count, const GLfloat *value)
{
}

void GLAPIENTRY
_mesa_Uniform2fvARB (GLint location, GLsizei count, const GLfloat *value)
{
}

void GLAPIENTRY
_mesa_Uniform3fvARB (GLint location, GLsizei count, const GLfloat *value)
{
}

void GLAPIENTRY
_mesa_Uniform4fvARB (GLint location, GLsizei count, const GLfloat *value)
{
}

void GLAPIENTRY
_mesa_Uniform1ivARB (GLint location, GLsizei count, const GLint *value)
{
}

void GLAPIENTRY
_mesa_Uniform2ivARB (GLint location, GLsizei count, const GLint *value)
{
}

void GLAPIENTRY
_mesa_Uniform3ivARB (GLint location, GLsizei count, const GLint *value)
{
}

void GLAPIENTRY
_mesa_Uniform4ivARB (GLint location, GLsizei count, const GLint *value)
{
}

void GLAPIENTRY
_mesa_UniformMatrix2fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
}

void GLAPIENTRY
_mesa_UniformMatrix3fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
}

void GLAPIENTRY
_mesa_UniformMatrix4fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
}

void GLAPIENTRY
_mesa_GetObjectParameterfvARB (GLhandleARB obj, GLenum pname, GLfloat *params)
{
}

void GLAPIENTRY
_mesa_GetObjectParameterivARB (GLhandleARB obj, GLenum pname, GLint *params)
{
}

void GLAPIENTRY
_mesa_GetInfoLogARB (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog)
{
}

void GLAPIENTRY
_mesa_GetAttachedObjectsARB (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj)
{
}

GLint GLAPIENTRY
_mesa_GetUniformLocationARB (GLhandleARB programObj, const GLcharARB *name)
{
   return -1;
}

void GLAPIENTRY
_mesa_GetActiveUniformARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name)
{
}

void GLAPIENTRY
_mesa_GetUniformfvARB (GLhandleARB programObj, GLint location, GLfloat *params)
{
}

void GLAPIENTRY
_mesa_GetUniformivARB (GLhandleARB programObj, GLint location, GLint *params)
{
}

void GLAPIENTRY
_mesa_GetShaderSourceARB (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source)
{
}

/* GL_ARB_vertex_shader */

void GLAPIENTRY
_mesa_BindAttribLocationARB (GLhandleARB programObj, GLuint index, const GLcharARB *name)
{
}

void GLAPIENTRY
_mesa_GetActiveAttribARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name)
{
}

GLint GLAPIENTRY
_mesa_GetAttribLocationARB (GLhandleARB programObj, const GLcharARB *name)
{
   return 0;
}

void _mesa_init_shaderobjects( GLcontext * ctx )
{
}
