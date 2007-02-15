/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
** $Date: 2006/06/20 15:30:26 $ $Revision: 1.3 $
** $Header: /cvs/mesa/Mesa/src/glu/sgi/libutil/error.c,v 1.3 2006/06/20 15:30:26 brianp Exp $
*/

#include "gluos.h"
#include "gluint.h"
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>


struct token_string
{
   GLuint Token;
   const char *String;
};

static const struct token_string Errors[] = {
   { GL_NO_ERROR, "no error" },
   { GL_INVALID_ENUM, "invalid enumerant" },
   { GL_INVALID_VALUE, "invalid value" },
   { GL_INVALID_OPERATION, "invalid operation" },
   { GL_STACK_OVERFLOW, "stack overflow" },
   { GL_STACK_UNDERFLOW, "stack underflow" },
   { GL_OUT_OF_MEMORY, "out of memory" },
   { GL_TABLE_TOO_LARGE, "table too large" },
#ifdef GL_EXT_framebuffer_object
   { GL_INVALID_FRAMEBUFFER_OPERATION_EXT, "invalid framebuffer operation" },
#endif
   /* GLU */
   { GLU_INVALID_ENUM, "invalid enumerant" },
   { GLU_INVALID_VALUE, "invalid value" },
   { GLU_OUT_OF_MEMORY, "out of memory" },
   { GLU_INCOMPATIBLE_GL_VERSION, "incompatible gl version" },
   { GLU_INVALID_OPERATION, "invalid operation" },
   { ~0, NULL } /* end of list indicator */
};



const GLubyte* GLAPIENTRY
gluErrorString(GLenum errorCode)
{
    int i;
    for (i = 0; Errors[i].String; i++) {
        if (Errors[i].Token == errorCode)
            return (const GLubyte *) Errors[i].String;
    }
    if ((errorCode >= GLU_NURBS_ERROR1) && (errorCode <= GLU_NURBS_ERROR37)) {
	return (const GLubyte *) __gluNURBSErrorString(errorCode - (GLU_NURBS_ERROR1 - 1));
    }
    if ((errorCode >= GLU_TESS_ERROR1) && (errorCode <= GLU_TESS_ERROR8)) {
	return (const GLubyte *) __gluTessErrorString(errorCode - (GLU_TESS_ERROR1 - 1));
    }
    return (const GLubyte *) 0;
}

