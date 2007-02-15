/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2004-2006  Brian Paul   All Rights Reserved.
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

#ifndef SHADEROBJECTS_H
#define SHADEROBJECTS_H

#include "context.h"

#if FEATURE_ARB_shader_objects

/**
 * gl2 unique interface identifier.
 * Each gl2 interface has its own interface id used for object queries.
 */
enum gl2_uiid
{
   UIID_UNKNOWN,		/* supported by all objects */
   UIID_GENERIC,		/* generic object */
   UIID_CONTAINER,		/* contains generic objects */
   UIID_SHADER,			/* shader object */
   UIID_FRAGMENT_SHADER,	/* fragment shader */
   UIID_VERTEX_SHADER,		/* vertex shader */
   UIID_PROGRAM,		/* program object */
   UIID_3DLABS_SHHANDLE,         /* encapsulates 3DLabs' ShHandle */
   UIID_DEBUG                    /* debug object */
};

struct gl2_unknown_intf
{
   GLvoid (* AddRef) (struct gl2_unknown_intf **);
   GLvoid (* Release) (struct gl2_unknown_intf **);
   struct gl2_unknown_intf **(* QueryInterface) (struct gl2_unknown_intf **, enum gl2_uiid uiid);
};

struct gl2_generic_intf
{
   struct gl2_unknown_intf _unknown;
   GLvoid (* Delete) (struct gl2_generic_intf **);
   GLenum (* GetType) (struct gl2_generic_intf **);
   GLhandleARB (* GetName) (struct gl2_generic_intf **);
   GLboolean (* GetDeleteStatus) (struct gl2_generic_intf **);
   GLvoid (* GetInfoLog) (struct gl2_generic_intf **, GLsizei, GLcharARB *);
   GLsizei (* GetInfoLogLength) (struct gl2_generic_intf **);
};

struct gl2_container_intf
{
   struct gl2_generic_intf _generic;
   GLboolean (* Attach) (struct gl2_container_intf **, struct gl2_generic_intf **);
   GLboolean (* Detach) (struct gl2_container_intf **, struct gl2_generic_intf **);
   GLsizei (* GetAttachedCount) (struct gl2_container_intf **);
   struct gl2_generic_intf **(* GetAttached) (struct gl2_container_intf **, GLuint);
};

struct gl2_shader_intf
{
   struct gl2_generic_intf _generic;
   GLenum (* GetSubType) (struct gl2_shader_intf **);
   GLboolean (* GetCompileStatus) (struct gl2_shader_intf **);
   GLvoid (* SetSource) (struct gl2_shader_intf **, GLcharARB *, GLint *, GLsizei);
   const GLcharARB *(* GetSource) (struct gl2_shader_intf **);
   GLvoid (* Compile) (struct gl2_shader_intf **);
};

struct gl2_program_intf
{
   struct gl2_container_intf _container;
   GLboolean (* GetLinkStatus) (struct gl2_program_intf **);
   GLboolean (* GetValidateStatus) (struct gl2_program_intf **);
   GLvoid (* Link) (struct gl2_program_intf **);
   GLvoid (* Validate) (struct gl2_program_intf **);
   GLvoid (* UpdateFixedUniforms) (struct gl2_program_intf **);
   GLvoid (* UpdateFixedAttrib) (struct gl2_program_intf **, GLuint, GLvoid *, GLuint, GLuint,
                                 GLboolean);
   GLvoid (* UpdateFixedVarying) (struct gl2_program_intf **, GLuint, GLvoid *, GLuint, GLuint,
                                  GLboolean);
   GLvoid (* GetTextureImageUsage) (struct gl2_program_intf **, GLbitfield *);
   GLboolean (* IsShaderPresent) (struct gl2_program_intf **, GLenum);
   GLvoid (* GetActiveUniform) (struct gl2_program_intf **, GLuint index, GLsizei maxLength,
                                GLsizei *length, GLint *size, GLenum *type, GLchar *name);
   GLuint (* GetActiveUniformMaxLength) (struct gl2_program_intf **);
   GLuint (* GetActiveUniformCount) (struct gl2_program_intf **);
   GLint (* GetUniformLocation) (struct gl2_program_intf **, const GLchar *name);
   GLboolean (* WriteUniform) (struct gl2_program_intf **, GLint loc, GLsizei count,
                               const GLvoid *data, GLenum type);
   GLboolean (* ReadUniform) (struct gl2_program_intf **, GLint loc, GLsizei count,
                               GLvoid *data, GLenum type);
   GLvoid (* GetActiveAttrib) (struct gl2_program_intf **, GLuint index, GLsizei maxLength,
                               GLsizei *length, GLint *size, GLenum *type, GLchar *name);
   GLuint (* GetActiveAttribMaxLength) (struct gl2_program_intf **);
   GLuint (* GetActiveAttribCount) (struct gl2_program_intf **);
   GLint (* GetAttribLocation) (struct gl2_program_intf **, const GLchar *name);
   GLvoid (* OverrideAttribBinding) (struct gl2_program_intf **, GLuint, const GLchar *);
   GLvoid (* WriteAttrib) (struct gl2_program_intf **, GLuint, const GLfloat *);
   GLvoid (* UpdateVarying) (struct gl2_program_intf **, GLuint, GLfloat *, GLboolean);
};

struct gl2_fragment_shader_intf
{
   struct gl2_shader_intf _shader;
};

struct gl2_vertex_shader_intf
{
   struct gl2_shader_intf _shader;
};

struct gl2_3dlabs_shhandle_intf
{
   struct gl2_unknown_intf _unknown;
   GLvoid *(* GetShHandle) (struct gl2_3dlabs_shhandle_intf **);
};

struct gl2_debug_intf
{
   struct gl2_generic_intf _generic;
   GLvoid (* ClearDebugLog) (struct gl2_debug_intf **, GLenum logType, GLenum shaderType);
   GLvoid (* GetDebugLog) (struct gl2_debug_intf **, GLenum logType, GLenum shaderType,
                           GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
   GLsizei (* GetDebugLogLength) (struct gl2_debug_intf **, GLenum logType, GLenum shaderType);
};


extern void GLAPIENTRY
_mesa_DeleteObjectARB(GLhandleARB obj);

extern GLhandleARB GLAPIENTRY
_mesa_GetHandleARB(GLenum pname);

extern void GLAPIENTRY
_mesa_DetachObjectARB (GLhandleARB, GLhandleARB);

extern GLhandleARB GLAPIENTRY
_mesa_CreateShaderObjectARB (GLenum);

extern void GLAPIENTRY
_mesa_ShaderSourceARB (GLhandleARB, GLsizei, const GLcharARB* *, const GLint *);

extern void  GLAPIENTRY
_mesa_CompileShaderARB (GLhandleARB);

extern GLhandleARB GLAPIENTRY
_mesa_CreateProgramObjectARB (void);

extern void GLAPIENTRY
_mesa_AttachObjectARB (GLhandleARB, GLhandleARB);

extern void GLAPIENTRY
_mesa_LinkProgramARB (GLhandleARB);

extern void GLAPIENTRY
_mesa_UseProgramObjectARB (GLhandleARB);

extern void GLAPIENTRY
_mesa_ValidateProgramARB (GLhandleARB);

extern void GLAPIENTRY
_mesa_Uniform1fARB (GLint, GLfloat);

extern void GLAPIENTRY
_mesa_Uniform2fARB (GLint, GLfloat, GLfloat);

extern void GLAPIENTRY
_mesa_Uniform3fARB (GLint, GLfloat, GLfloat, GLfloat);

extern void GLAPIENTRY
_mesa_Uniform4fARB (GLint, GLfloat, GLfloat, GLfloat, GLfloat);

extern void GLAPIENTRY
_mesa_Uniform1iARB (GLint, GLint);

extern void GLAPIENTRY
_mesa_Uniform2iARB (GLint, GLint, GLint);

extern void GLAPIENTRY
_mesa_Uniform3iARB (GLint, GLint, GLint, GLint);

extern void GLAPIENTRY
_mesa_Uniform4iARB (GLint, GLint, GLint, GLint, GLint);

extern void GLAPIENTRY
_mesa_Uniform1fvARB (GLint, GLsizei, const GLfloat *);

extern void GLAPIENTRY
_mesa_Uniform2fvARB (GLint, GLsizei, const GLfloat *);

extern void GLAPIENTRY
_mesa_Uniform3fvARB (GLint, GLsizei, const GLfloat *);

extern void GLAPIENTRY
_mesa_Uniform4fvARB (GLint, GLsizei, const GLfloat *);

extern void GLAPIENTRY
_mesa_Uniform1ivARB (GLint, GLsizei, const GLint *);

extern void GLAPIENTRY
_mesa_Uniform2ivARB (GLint, GLsizei, const GLint *);

extern void GLAPIENTRY
_mesa_Uniform3ivARB (GLint, GLsizei, const GLint *);

extern void GLAPIENTRY
_mesa_Uniform4ivARB (GLint, GLsizei, const GLint *);

extern void GLAPIENTRY
_mesa_UniformMatrix2fvARB (GLint, GLsizei, GLboolean, const GLfloat *);

extern void GLAPIENTRY
_mesa_UniformMatrix3fvARB (GLint, GLsizei, GLboolean, const GLfloat *);

extern void GLAPIENTRY
_mesa_UniformMatrix4fvARB (GLint, GLsizei, GLboolean, const GLfloat *);

extern void GLAPIENTRY
_mesa_GetObjectParameterfvARB (GLhandleARB, GLenum, GLfloat *);

extern void GLAPIENTRY
_mesa_GetObjectParameterivARB (GLhandleARB, GLenum, GLint *);

extern void GLAPIENTRY
_mesa_GetInfoLogARB (GLhandleARB, GLsizei, GLsizei *, GLcharARB *);

extern void GLAPIENTRY
_mesa_GetAttachedObjectsARB (GLhandleARB, GLsizei, GLsizei *, GLhandleARB *);

extern GLint GLAPIENTRY
_mesa_GetUniformLocationARB (GLhandleARB, const GLcharARB *);

extern void GLAPIENTRY
_mesa_GetActiveUniformARB (GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);

extern void GLAPIENTRY
_mesa_GetUniformfvARB (GLhandleARB, GLint, GLfloat *);

extern void GLAPIENTRY
_mesa_GetUniformivARB (GLhandleARB, GLint, GLint *);

extern void GLAPIENTRY
_mesa_GetShaderSourceARB (GLhandleARB, GLsizei, GLsizei *, GLcharARB *);

#if FEATURE_ARB_vertex_shader

extern void GLAPIENTRY
_mesa_BindAttribLocationARB (GLhandleARB, GLuint, const GLcharARB *);

extern void GLAPIENTRY
_mesa_GetActiveAttribARB (GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);

extern GLint GLAPIENTRY
_mesa_GetAttribLocationARB (GLhandleARB, const GLcharARB *);

#endif /* FEATURE_ARB_vertex_shader */


/* 2.0 */
extern void GLAPIENTRY
_mesa_AttachShader(GLuint program, GLuint shader);

extern GLuint GLAPIENTRY
_mesa_CreateShader(GLenum);

extern GLuint GLAPIENTRY
_mesa_CreateProgram(void);

extern void GLAPIENTRY
_mesa_DeleteProgram(GLuint program);

extern void GLAPIENTRY
_mesa_DeleteShader(GLuint shader);

extern void GLAPIENTRY
_mesa_DetachShader(GLuint program, GLuint shader);

extern void GLAPIENTRY
_mesa_GetAttachedShaders(GLuint program, GLsizei maxCount,
                         GLsizei *count, GLuint *obj);

extern void GLAPIENTRY
_mesa_GetProgramiv(GLuint program, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetProgramInfoLog(GLuint program, GLsizei bufSize,
                        GLsizei *length, GLchar *infoLog);

extern void GLAPIENTRY
_mesa_GetShaderiv(GLuint shader, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetShaderInfoLog(GLuint shader, GLsizei bufSize,
                       GLsizei *length, GLchar *infoLog);

extern GLboolean GLAPIENTRY
_mesa_IsProgram(GLuint program);

extern GLboolean GLAPIENTRY
_mesa_IsShader(GLuint shader);



/* 2.1 */
extern void GLAPIENTRY
_mesa_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);



#endif /* FEATURE_ARB_shader_objects */

extern void
_mesa_init_shaderobjects (GLcontext *ctx);

#endif /* SHADEROBJECTS_H */
