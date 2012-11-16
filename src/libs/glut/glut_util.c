/*
 * Copyright 1994-1997 Mark Kilgard, All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Mark Kilgard
 */


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "glutint.h"


/* strdup is actually not a standard ANSI C or POSIX routine
   so implement a private one for GLUT.  OpenVMS does not have a
   strdup; Linux's standard libc doesn't declare strdup by default
   (unless BSD or SVID interfaces are requested). */
char *
__glutStrdup(const char *string)
{
  char *copy;

  copy = (char*) malloc(strlen(string) + 1);
  if (copy == NULL)
    return NULL;
  strcpy(copy, string);
  return copy;
}

void
__glutWarning(const char *format,...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "GLUT: Warning in %s: ",
    __glutProgramName ? __glutProgramName : "(unamed)");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);
}

/* CENTRY */
void APIENTRY
glutReportErrors(void)
{
  GLenum error;

  while ((error = glGetError()) != GL_NO_ERROR)
    __glutWarning("GL error: %s", gluErrorString(error));
}
/* ENDCENTRY */

void
__glutFatalError(const char *format,...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "GLUT: Fatal Error in %s: ",
    __glutProgramName ? __glutProgramName : "(unamed)");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);
  exit(1);
}

void
__glutFatalUsage(const char *format,...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "GLUT: Fatal API Usage in %s: ",
    __glutProgramName ? __glutProgramName : "(unamed)");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);
  abort();
}
