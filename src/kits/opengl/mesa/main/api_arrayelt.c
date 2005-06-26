/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "api_arrayelt.h"
#include "context.h"
#include "glapi.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"

typedef void (GLAPIENTRY *array_func)( const void * );

typedef struct {
   const struct gl_client_array *array;
   array_func func;
} AEarray;

typedef void (GLAPIENTRY *attrib_func)( GLuint indx, const void *data );

typedef struct {
   const struct gl_client_array *array;
   attrib_func func;
   GLuint index;
} AEattrib;

typedef struct {
   AEarray arrays[32];
   AEattrib attribs[VERT_ATTRIB_MAX + 1];
   GLuint NewState;
} AEcontext;

#define AE_CONTEXT(ctx) ((AEcontext *)(ctx)->aelt_context)

/*
 * Convert GL_BYTE, GL_UNSIGNED_BYTE, .. GL_DOUBLE into an integer
 * in the range [0, 7].  Luckily these type tokens are sequentially
 * numbered in gl.h, except for GL_DOUBLE.
 */
#define TYPE_IDX(t) ( (t) == GL_DOUBLE ? 7 : (t) & 7 )

static void GLAPIENTRY Color3bv(const GLbyte *v)
{
   GL_CALL(Color3bv)(v);
}

static void GLAPIENTRY Color3ubv(const GLubyte *v)
{
   GL_CALL(Color3ubv)(v);
}

static void GLAPIENTRY Color3sv(const GLshort *v)
{
   GL_CALL(Color3sv)(v);
}

static void GLAPIENTRY Color3usv(const GLushort *v)
{
   GL_CALL(Color3usv)(v);
}

static void GLAPIENTRY Color3iv(const GLint *v)
{
   GL_CALL(Color3iv)(v);
}

static void GLAPIENTRY Color3uiv(const GLuint *v)
{
   GL_CALL(Color3uiv)(v);
}

static void GLAPIENTRY Color3fv(const GLfloat *v)
{
   GL_CALL(Color3fv)(v);
}

static void GLAPIENTRY Color3dv(const GLdouble *v)
{
   GL_CALL(Color3dv)(v);
}

static void GLAPIENTRY Color4bv(const GLbyte *v)
{
   GL_CALL(Color4bv)(v);
}

static void GLAPIENTRY Color4ubv(const GLubyte *v)
{
   GL_CALL(Color4ubv)(v);
}

static void GLAPIENTRY Color4sv(const GLshort *v)
{
   GL_CALL(Color4sv)(v);
}

static void GLAPIENTRY Color4usv(const GLushort *v)
{
   GL_CALL(Color4usv)(v);
}

static void GLAPIENTRY Color4iv(const GLint *v)
{
   GL_CALL(Color4iv)(v);
}

static void GLAPIENTRY Color4uiv(const GLuint *v)
{
   GL_CALL(Color4uiv)(v);
}

static void GLAPIENTRY Color4fv(const GLfloat *v)
{
   GL_CALL(Color4fv)(v);
}

static void GLAPIENTRY Color4dv(const GLdouble *v)
{
   GL_CALL(Color4dv)(v);
}

static const array_func ColorFuncs[2][8] = {
   {
      (array_func) Color3bv,
      (array_func) Color3ubv,
      (array_func) Color3sv,
      (array_func) Color3usv,
      (array_func) Color3iv,
      (array_func) Color3uiv,
      (array_func) Color3fv,
      (array_func) Color3dv,
   },
   {
      (array_func) Color4bv,
      (array_func) Color4ubv,
      (array_func) Color4sv,
      (array_func) Color4usv,
      (array_func) Color4iv,
      (array_func) Color4uiv,
      (array_func) Color4fv,
      (array_func) Color4dv,
   },
};

static void GLAPIENTRY Vertex2sv(const GLshort *v)
{
   GL_CALL(Vertex2sv)(v);
}

static void GLAPIENTRY Vertex2iv(const GLint *v)
{
   GL_CALL(Vertex2iv)(v);
}

static void GLAPIENTRY Vertex2fv(const GLfloat *v)
{
   GL_CALL(Vertex2fv)(v);
}

static void GLAPIENTRY Vertex2dv(const GLdouble *v)
{
   GL_CALL(Vertex2dv)(v);
}

static void GLAPIENTRY Vertex3sv(const GLshort *v)
{
   GL_CALL(Vertex3sv)(v);
}

static void GLAPIENTRY Vertex3iv(const GLint *v)
{
   GL_CALL(Vertex3iv)(v);
}

static void GLAPIENTRY Vertex3fv(const GLfloat *v)
{
   GL_CALL(Vertex3fv)(v);
}

static void GLAPIENTRY Vertex3dv(const GLdouble *v)
{
   GL_CALL(Vertex3dv)(v);
}

static void GLAPIENTRY Vertex4sv(const GLshort *v)
{
   GL_CALL(Vertex4sv)(v);
}

static void GLAPIENTRY Vertex4iv(const GLint *v)
{
   GL_CALL(Vertex4iv)(v);
}

static void GLAPIENTRY Vertex4fv(const GLfloat *v)
{
   GL_CALL(Vertex4fv)(v);
}

static void GLAPIENTRY Vertex4dv(const GLdouble *v)
{
   GL_CALL(Vertex4dv)(v);
}

static const array_func VertexFuncs[3][8] = {
   {
      0,
      0,
      (array_func) Vertex2sv,
      0,
      (array_func) Vertex2iv,
      0,
      (array_func) Vertex2fv,
      (array_func) Vertex2dv,
   },
   {
      0,
      0,
      (array_func) Vertex3sv,
      0,
      (array_func) Vertex3iv,
      0,
      (array_func) Vertex3fv,
      (array_func) Vertex3dv,
   },
   {
      0,
      0,
      (array_func) Vertex4sv,
      0,
      (array_func) Vertex4iv,
      0,
      (array_func) Vertex4fv,
      (array_func) Vertex4dv,
   },
};

static void GLAPIENTRY Indexubv(const GLubyte *c)
{
   GL_CALL(Indexubv)(c);
}

static void GLAPIENTRY Indexsv(const GLshort *c)
{
   GL_CALL(Indexsv)(c);
}

static void GLAPIENTRY Indexiv(const GLint *c)
{
   GL_CALL(Indexiv)(c);
}

static void GLAPIENTRY Indexfv(const GLfloat *c)
{
   GL_CALL(Indexfv)(c);
}

static void GLAPIENTRY Indexdv(const GLdouble *c)
{
   GL_CALL(Indexdv)(c);
}

static const array_func IndexFuncs[8] = {
   0,
   (array_func) Indexubv,
   (array_func) Indexsv,
   0,
   (array_func) Indexiv,
   0,
   (array_func) Indexfv,
   (array_func) Indexdv,
};

static void GLAPIENTRY Normal3bv(const GLbyte *v)
{
   GL_CALL(Normal3bv)(v);
}

static void GLAPIENTRY Normal3sv(const GLshort *v)
{
   GL_CALL(Normal3sv)(v);
}

static void GLAPIENTRY Normal3iv(const GLint *v)
{
   GL_CALL(Normal3iv)(v);
}

static void GLAPIENTRY Normal3fv(const GLfloat *v)
{
   GL_CALL(Normal3fv)(v);
}

static void GLAPIENTRY Normal3dv(const GLdouble *v)
{
   GL_CALL(Normal3dv)(v);
}

static const array_func NormalFuncs[8] = {
   (array_func) Normal3bv,
   0,
   (array_func) Normal3sv,
   0,
   (array_func) Normal3iv,
   0,
   (array_func) Normal3fv,
   (array_func) Normal3dv,
};

/* Wrapper functions in case glSecondaryColor*EXT doesn't exist */
static void GLAPIENTRY SecondaryColor3bvEXT(const GLbyte *c)
{
   GL_CALL(SecondaryColor3bvEXT)(c);
}

static void GLAPIENTRY SecondaryColor3ubvEXT(const GLubyte *c)
{
   GL_CALL(SecondaryColor3ubvEXT)(c);
}

static void GLAPIENTRY SecondaryColor3svEXT(const GLshort *c)
{
   GL_CALL(SecondaryColor3svEXT)(c);
}

static void GLAPIENTRY SecondaryColor3usvEXT(const GLushort *c)
{
   GL_CALL(SecondaryColor3usvEXT)(c);
}

static void GLAPIENTRY SecondaryColor3ivEXT(const GLint *c)
{
   GL_CALL(SecondaryColor3ivEXT)(c);
}

static void GLAPIENTRY SecondaryColor3uivEXT(const GLuint *c)
{
   GL_CALL(SecondaryColor3uivEXT)(c);
}

static void GLAPIENTRY SecondaryColor3fvEXT(const GLfloat *c)
{
   GL_CALL(SecondaryColor3fvEXT)(c);
}

static void GLAPIENTRY SecondaryColor3dvEXT(const GLdouble *c)
{
   GL_CALL(SecondaryColor3dvEXT)(c);
}

static const array_func SecondaryColorFuncs[8] = {
   (array_func) SecondaryColor3bvEXT,
   (array_func) SecondaryColor3ubvEXT,
   (array_func) SecondaryColor3svEXT,
   (array_func) SecondaryColor3usvEXT,
   (array_func) SecondaryColor3ivEXT,
   (array_func) SecondaryColor3uivEXT,
   (array_func) SecondaryColor3fvEXT,
   (array_func) SecondaryColor3dvEXT,
};


/* Again, wrapper functions in case glSecondaryColor*EXT doesn't exist */
static void GLAPIENTRY FogCoordfvEXT(const GLfloat *f)
{
   GL_CALL(FogCoordfvEXT)(f);
}

static void GLAPIENTRY FogCoorddvEXT(const GLdouble *f)
{
   GL_CALL(FogCoorddvEXT)(f);
}

static const array_func FogCoordFuncs[8] = {
   0,
   0,
   0,
   0,
   0,
   0,
   (array_func) FogCoordfvEXT,
   (array_func) FogCoorddvEXT
};

/**********************************************************************/

/* GL_BYTE attributes */

static void GLAPIENTRY VertexAttrib1NbvNV(GLuint index, const GLbyte *v)
{
   GL_CALL(VertexAttrib1fNV)(index, BYTE_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1bvNV(GLuint index, const GLbyte *v)
{
   GL_CALL(VertexAttrib1fNV)(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2NbvNV(GLuint index, const GLbyte *v)
{
   GL_CALL(VertexAttrib2fNV)(index, BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2bvNV(GLuint index, const GLbyte *v)
{
   GL_CALL(VertexAttrib2fNV)(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3NbvNV(GLuint index, const GLbyte *v)
{
   GL_CALL(VertexAttrib3fNV)(index, BYTE_TO_FLOAT(v[0]),
			     BYTE_TO_FLOAT(v[1]),
			     BYTE_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3bvNV(GLuint index, const GLbyte *v)
{
   GL_CALL(VertexAttrib3fNV)(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4NbvNV(GLuint index, const GLbyte *v)
{
   GL_CALL(VertexAttrib4fNV)(index, BYTE_TO_FLOAT(v[0]),
			     BYTE_TO_FLOAT(v[1]),
			     BYTE_TO_FLOAT(v[2]),
			     BYTE_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4bvNV(GLuint index, const GLbyte *v)
{
   GL_CALL(VertexAttrib4fNV)(index, v[0], v[1], v[2], v[3]);
}

/* GL_UNSIGNED_BYTE attributes */

static void GLAPIENTRY VertexAttrib1NubvNV(GLuint index, const GLubyte *v)
{
   GL_CALL(VertexAttrib1fNV)(index, UBYTE_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1ubvNV(GLuint index, const GLubyte *v)
{
   GL_CALL(VertexAttrib1fNV)(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2NubvNV(GLuint index, const GLubyte *v)
{
   GL_CALL(VertexAttrib2fNV)(index, UBYTE_TO_FLOAT(v[0]),
			     UBYTE_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2ubvNV(GLuint index, const GLubyte *v)
{
   GL_CALL(VertexAttrib2fNV)(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3NubvNV(GLuint index, const GLubyte *v)
{
   GL_CALL(VertexAttrib3fNV)(index, UBYTE_TO_FLOAT(v[0]),
			     UBYTE_TO_FLOAT(v[1]),
			     UBYTE_TO_FLOAT(v[2]));
}
static void GLAPIENTRY VertexAttrib3ubvNV(GLuint index, const GLubyte *v)
{
   GL_CALL(VertexAttrib3fNV)(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4NubvNV(GLuint index, const GLubyte *v)
{
   GL_CALL(VertexAttrib4fNV)(index, UBYTE_TO_FLOAT(v[0]),
                                     UBYTE_TO_FLOAT(v[1]),
                                     UBYTE_TO_FLOAT(v[2]),
                                     UBYTE_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4ubvNV(GLuint index, const GLubyte *v)
{
   GL_CALL(VertexAttrib4fNV)(index, v[0], v[1], v[2], v[3]);
}

/* GL_SHORT attributes */

static void GLAPIENTRY VertexAttrib1NsvNV(GLuint index, const GLshort *v)
{
   GL_CALL(VertexAttrib1fNV)(index, SHORT_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1svNV(GLuint index, const GLshort *v)
{
   GL_CALL(VertexAttrib1fNV)(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2NsvNV(GLuint index, const GLshort *v)
{
   GL_CALL(VertexAttrib2fNV)(index, SHORT_TO_FLOAT(v[0]),
			     SHORT_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2svNV(GLuint index, const GLshort *v)
{
   GL_CALL(VertexAttrib2fNV)(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3NsvNV(GLuint index, const GLshort *v)
{
   GL_CALL(VertexAttrib3fNV)(index, SHORT_TO_FLOAT(v[0]),
			     SHORT_TO_FLOAT(v[1]),
			     SHORT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3svNV(GLuint index, const GLshort *v)
{
   GL_CALL(VertexAttrib3fNV)(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4NsvNV(GLuint index, const GLshort *v)
{
   GL_CALL(VertexAttrib4fNV)(index, SHORT_TO_FLOAT(v[0]),
			     SHORT_TO_FLOAT(v[1]),
			     SHORT_TO_FLOAT(v[2]),
			     SHORT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4svNV(GLuint index, const GLshort *v)
{
   GL_CALL(VertexAttrib4fNV)(index, v[0], v[1], v[2], v[3]);
}

/* GL_UNSIGNED_SHORT attributes */

static void GLAPIENTRY VertexAttrib1NusvNV(GLuint index, const GLushort *v)
{
   GL_CALL(VertexAttrib1fNV)(index, USHORT_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1usvNV(GLuint index, const GLushort *v)
{
   GL_CALL(VertexAttrib1fNV)(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2NusvNV(GLuint index, const GLushort *v)
{
   GL_CALL(VertexAttrib2fNV)(index, USHORT_TO_FLOAT(v[0]),
			     USHORT_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2usvNV(GLuint index, const GLushort *v)
{
   GL_CALL(VertexAttrib2fNV)(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3NusvNV(GLuint index, const GLushort *v)
{
   GL_CALL(VertexAttrib3fNV)(index, USHORT_TO_FLOAT(v[0]),
			     USHORT_TO_FLOAT(v[1]),
			     USHORT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3usvNV(GLuint index, const GLushort *v)
{
   GL_CALL(VertexAttrib3fNV)(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4NusvNV(GLuint index, const GLushort *v)
{
   GL_CALL(VertexAttrib4fNV)(index, USHORT_TO_FLOAT(v[0]),
			     USHORT_TO_FLOAT(v[1]),
			     USHORT_TO_FLOAT(v[2]),
			     USHORT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4usvNV(GLuint index, const GLushort *v)
{
   GL_CALL(VertexAttrib4fNV)(index, v[0], v[1], v[2], v[3]);
}

/* GL_INT attributes */

static void GLAPIENTRY VertexAttrib1NivNV(GLuint index, const GLint *v)
{
   GL_CALL(VertexAttrib1fNV)(index, INT_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1ivNV(GLuint index, const GLint *v)
{
   GL_CALL(VertexAttrib1fNV)(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2NivNV(GLuint index, const GLint *v)
{
   GL_CALL(VertexAttrib2fNV)(index, INT_TO_FLOAT(v[0]),
			     INT_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2ivNV(GLuint index, const GLint *v)
{
   GL_CALL(VertexAttrib2fNV)(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3NivNV(GLuint index, const GLint *v)
{
   GL_CALL(VertexAttrib3fNV)(index, INT_TO_FLOAT(v[0]),
			     INT_TO_FLOAT(v[1]),
			     INT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3ivNV(GLuint index, const GLint *v)
{
   GL_CALL(VertexAttrib3fNV)(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4NivNV(GLuint index, const GLint *v)
{
   GL_CALL(VertexAttrib4fNV)(index, INT_TO_FLOAT(v[0]),
			     INT_TO_FLOAT(v[1]),
			     INT_TO_FLOAT(v[2]),
			     INT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4ivNV(GLuint index, const GLint *v)
{
   GL_CALL(VertexAttrib4fNV)(index, v[0], v[1], v[2], v[3]);
}

/* GL_UNSIGNED_INT attributes */

static void GLAPIENTRY VertexAttrib1NuivNV(GLuint index, const GLuint *v)
{
   GL_CALL(VertexAttrib1fNV)(index, UINT_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1uivNV(GLuint index, const GLuint *v)
{
   GL_CALL(VertexAttrib1fNV)(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2NuivNV(GLuint index, const GLuint *v)
{
   GL_CALL(VertexAttrib2fNV)(index, UINT_TO_FLOAT(v[0]),
			     UINT_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2uivNV(GLuint index, const GLuint *v)
{
   GL_CALL(VertexAttrib2fNV)(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3NuivNV(GLuint index, const GLuint *v)
{
   GL_CALL(VertexAttrib3fNV)(index, UINT_TO_FLOAT(v[0]),
			     UINT_TO_FLOAT(v[1]),
			     UINT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3uivNV(GLuint index, const GLuint *v)
{
   GL_CALL(VertexAttrib3fNV)(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4NuivNV(GLuint index, const GLuint *v)
{
   GL_CALL(VertexAttrib4fNV)(index, UINT_TO_FLOAT(v[0]),
			     UINT_TO_FLOAT(v[1]),
			     UINT_TO_FLOAT(v[2]),
			     UINT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4uivNV(GLuint index, const GLuint *v)
{
   GL_CALL(VertexAttrib4fNV)(index, v[0], v[1], v[2], v[3]);
}

/* GL_FLOAT attributes */

static void GLAPIENTRY VertexAttrib1fvNV(GLuint index, const GLfloat *v)
{
   GL_CALL(VertexAttrib1fvNV)(index, v);
}

static void GLAPIENTRY VertexAttrib2fvNV(GLuint index, const GLfloat *v)
{
   GL_CALL(VertexAttrib2fvNV)(index, v);
}

static void GLAPIENTRY VertexAttrib3fvNV(GLuint index, const GLfloat *v)
{
   GL_CALL(VertexAttrib3fvNV)(index, v);
}

static void GLAPIENTRY VertexAttrib4fvNV(GLuint index, const GLfloat *v)
{
   GL_CALL(VertexAttrib4fvNV)(index, v);
}

/* GL_DOUBLE attributes */

static void GLAPIENTRY VertexAttrib1dvNV(GLuint index, const GLdouble *v)
{
   GL_CALL(VertexAttrib1dvNV)(index, v);
}

static void GLAPIENTRY VertexAttrib2dvNV(GLuint index, const GLdouble *v)
{
   GL_CALL(VertexAttrib2dvNV)(index, v);
}

static void GLAPIENTRY VertexAttrib3dvNV(GLuint index, const GLdouble *v)
{
   GL_CALL(VertexAttrib3dvNV)(index, v);
}

static void GLAPIENTRY VertexAttrib4dvNV(GLuint index, const GLdouble *v)
{
   GL_CALL(VertexAttrib4dvNV)(index, v);
}


/*
 * Array [size][type] of VertexAttrib functions
 */
static attrib_func AttribFuncsNV[2][4][8] = {
   {
      /* non-normalized */
      {
         /* size 1 */
         (attrib_func) VertexAttrib1bvNV,
         (attrib_func) VertexAttrib1ubvNV,
         (attrib_func) VertexAttrib1svNV,
         (attrib_func) VertexAttrib1usvNV,
         (attrib_func) VertexAttrib1ivNV,
         (attrib_func) VertexAttrib1uivNV,
         (attrib_func) VertexAttrib1fvNV,
         (attrib_func) VertexAttrib1dvNV
      },
      {
         /* size 2 */
         (attrib_func) VertexAttrib2bvNV,
         (attrib_func) VertexAttrib2ubvNV,
         (attrib_func) VertexAttrib2svNV,
         (attrib_func) VertexAttrib2usvNV,
         (attrib_func) VertexAttrib2ivNV,
         (attrib_func) VertexAttrib2uivNV,
         (attrib_func) VertexAttrib2fvNV,
         (attrib_func) VertexAttrib2dvNV
      },
      {
         /* size 3 */
         (attrib_func) VertexAttrib3bvNV,
         (attrib_func) VertexAttrib3ubvNV,
         (attrib_func) VertexAttrib3svNV,
         (attrib_func) VertexAttrib3usvNV,
         (attrib_func) VertexAttrib3ivNV,
         (attrib_func) VertexAttrib3uivNV,
         (attrib_func) VertexAttrib3fvNV,
         (attrib_func) VertexAttrib3dvNV
      },
      {
         /* size 4 */
         (attrib_func) VertexAttrib4bvNV,
         (attrib_func) VertexAttrib4ubvNV,
         (attrib_func) VertexAttrib4svNV,
         (attrib_func) VertexAttrib4usvNV,
         (attrib_func) VertexAttrib4ivNV,
         (attrib_func) VertexAttrib4uivNV,
         (attrib_func) VertexAttrib4fvNV,
         (attrib_func) VertexAttrib4dvNV
      }
   },
   {
      /* normalized (except for float/double) */
      {
         /* size 1 */
         (attrib_func) VertexAttrib1NbvNV,
         (attrib_func) VertexAttrib1NubvNV,
         (attrib_func) VertexAttrib1NsvNV,
         (attrib_func) VertexAttrib1NusvNV,
         (attrib_func) VertexAttrib1NivNV,
         (attrib_func) VertexAttrib1NuivNV,
         (attrib_func) VertexAttrib1fvNV,
         (attrib_func) VertexAttrib1dvNV
      },
      {
         /* size 2 */
         (attrib_func) VertexAttrib2NbvNV,
         (attrib_func) VertexAttrib2NubvNV,
         (attrib_func) VertexAttrib2NsvNV,
         (attrib_func) VertexAttrib2NusvNV,
         (attrib_func) VertexAttrib2NivNV,
         (attrib_func) VertexAttrib2NuivNV,
         (attrib_func) VertexAttrib2fvNV,
         (attrib_func) VertexAttrib2dvNV
      },
      {
         /* size 3 */
         (attrib_func) VertexAttrib3NbvNV,
         (attrib_func) VertexAttrib3NubvNV,
         (attrib_func) VertexAttrib3NsvNV,
         (attrib_func) VertexAttrib3NusvNV,
         (attrib_func) VertexAttrib3NivNV,
         (attrib_func) VertexAttrib3NuivNV,
         (attrib_func) VertexAttrib3fvNV,
         (attrib_func) VertexAttrib3dvNV
      },
      {
         /* size 4 */
         (attrib_func) VertexAttrib4NbvNV,
         (attrib_func) VertexAttrib4NubvNV,
         (attrib_func) VertexAttrib4NsvNV,
         (attrib_func) VertexAttrib4NusvNV,
         (attrib_func) VertexAttrib4NivNV,
         (attrib_func) VertexAttrib4NuivNV,
         (attrib_func) VertexAttrib4fvNV,
         (attrib_func) VertexAttrib4dvNV
      }
   }
};

static void GLAPIENTRY EdgeFlagv(const GLboolean *flag)
{
   GL_CALL(EdgeFlagv)(flag);
}

/**********************************************************************/


GLboolean _ae_create_context( GLcontext *ctx )
{
   if (ctx->aelt_context)
      return GL_TRUE;

   ctx->aelt_context = MALLOC( sizeof(AEcontext) );
   if (!ctx->aelt_context)
      return GL_FALSE;

   AE_CONTEXT(ctx)->NewState = ~0;
   return GL_TRUE;
}


void _ae_destroy_context( GLcontext *ctx )
{
   if ( AE_CONTEXT( ctx ) ) {
      FREE( ctx->aelt_context );
      ctx->aelt_context = 0;
   }
}


/**
 * Make a list of per-vertex functions to call for each glArrayElement call.
 * These functions access the array data (i.e. glVertex, glColor, glNormal,
 * etc).
 * Note: this may be called during display list construction.
 */
static void _ae_update_state( GLcontext *ctx )
{
   AEcontext *actx = AE_CONTEXT(ctx);
   AEarray *aa = actx->arrays;
   AEattrib *at = actx->attribs;
   GLuint i;

   /* conventional vertex arrays */
  if (ctx->Array.Index.Enabled) {
      aa->array = &ctx->Array.Index;
      aa->func = IndexFuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }
   if (ctx->Array.EdgeFlag.Enabled) {
      aa->array = &ctx->Array.EdgeFlag;
      aa->func = (array_func) EdgeFlagv;
      aa++;
   }
   if (ctx->Array.Normal.Enabled) {
      aa->array = &ctx->Array.Normal;
      aa->func = NormalFuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }
   if (ctx->Array.Color.Enabled) {
      aa->array = &ctx->Array.Color;
      aa->func = ColorFuncs[aa->array->Size-3][TYPE_IDX(aa->array->Type)];
      aa++;
   }
   if (ctx->Array.SecondaryColor.Enabled) {
      aa->array = &ctx->Array.SecondaryColor;
      aa->func = SecondaryColorFuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }
   if (ctx->Array.FogCoord.Enabled) {
      aa->array = &ctx->Array.FogCoord;
      aa->func = FogCoordFuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }
   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      if (ctx->Array.TexCoord[i].Enabled) {
         /* NOTE: we use generic glVertexAttrib functions here.
          * If we ever de-alias conventional/generic vertex attribs this
          * will have to change.
          */
         struct gl_client_array *attribArray = &ctx->Array.TexCoord[i];
         at->array = attribArray;
         at->func = AttribFuncsNV[at->array->Normalized][at->array->Size-1][TYPE_IDX(at->array->Type)];
         at->index = VERT_ATTRIB_TEX0 + i;
         at++;
      }
   }

   /* generic vertex attribute arrays */
   for (i = 1; i < VERT_ATTRIB_MAX; i++) {  /* skip zero! */
      if (ctx->Array.VertexAttrib[i].Enabled) {
         struct gl_client_array *attribArray = &ctx->Array.VertexAttrib[i];
         at->array = attribArray;
         /* Note: we can't grab the _glapi_Dispatch->VertexAttrib1fvNV
          * function pointer here (for float arrays) since the pointer may
          * change from one execution of _ae_loopback_array_elt() to
          * the next.  Doing so caused UT to break.
          */
         at->func = AttribFuncsNV[at->array->Normalized][at->array->Size-1][TYPE_IDX(at->array->Type)];
         at->index = i;
         at++;
      }
   }

   /* finally, vertex position */
   if (ctx->Array.VertexAttrib[0].Enabled) {
      /* Use glVertex(v) instead of glVertexAttrib(0, v) to be sure it's
       * issued as the last (proviking) attribute).
       */
      aa->array = &ctx->Array.VertexAttrib[0];
      assert(aa->array->Size >= 2); /* XXX fix someday? */
      aa->func = VertexFuncs[aa->array->Size-2][TYPE_IDX(aa->array->Type)];
      aa++;
   }
   else if (ctx->Array.Vertex.Enabled) {
      aa->array = &ctx->Array.Vertex;
      aa->func = VertexFuncs[aa->array->Size-2][TYPE_IDX(aa->array->Type)];
      aa++;
   }

   ASSERT(at - actx->attribs <= VERT_ATTRIB_MAX);
   ASSERT(aa - actx->arrays < 32);
   at->func = NULL;  /* terminate the list */
   aa->func = NULL;  /* terminate the list */

   actx->NewState = 0;
}


/**
 * Called via glArrayElement() and glDrawArrays().
 * Issue the glNormal, glVertex, glColor, glVertexAttrib, etc functions
 * for all enabled vertex arrays (for elt-th element).
 * Note: this may be called during display list construction.
 */
void GLAPIENTRY _ae_loopback_array_elt( GLint elt )
{
   GET_CURRENT_CONTEXT(ctx);
   const AEcontext *actx = AE_CONTEXT(ctx);
   const AEarray *aa;
   const AEattrib *at;

   if (actx->NewState)
      _ae_update_state( ctx );

   /* generic attributes */
   for (at = actx->attribs; at->func; at++) {
      const GLubyte *src = at->array->BufferObj->Data
                         + (unsigned long) at->array->Ptr
                         + elt * at->array->StrideB;
      at->func( at->index, src );
   }

   /* conventional arrays */
   for (aa = actx->arrays; aa->func ; aa++) {
      const GLubyte *src = aa->array->BufferObj->Data
                         + (unsigned long) aa->array->Ptr
                         + elt * aa->array->StrideB;
      aa->func( src );
   }
}


void _ae_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   AE_CONTEXT(ctx)->NewState |= new_state;
}
