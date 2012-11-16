/*
 * Copyright 1994-1997 Mark Kilgard, All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Mark Kilgard
 */
#ifndef __glutbitmap_h__
#define __glutbitmap_h__


#include <GL/glut.h>


typedef struct {
  const GLsizei width;
  const GLsizei height;
  const GLfloat xorig;
  const GLfloat yorig;
  const GLfloat advance;
  const GLubyte *bitmap;
} BitmapCharRec, *BitmapCharPtr;

typedef struct {
  const char *name;
  const int num_chars;
  const int first;
  const BitmapCharRec * const *ch;
} BitmapFontRec, *BitmapFontPtr;

typedef void *GLUTbitmapFont;


#endif /* __glutbitmap_h__ */
