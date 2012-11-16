/*
 * Copyright 1994-1997 Mark Kilgard, All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Mark Kilgard
 */
#ifndef __glutstroke_h__
#define __glutstroke_h__


#if defined(_WIN32)
#pragma warning (disable:4244)  /* disable bogus conversion warnings */
#pragma warning (disable:4305)  /* VC++ 5.0 version of above warning. */
#endif

typedef struct {
  float x;
  float y;
} CoordRec, *CoordPtr;

typedef struct {
  int num_coords;
  const CoordRec *coord;
} StrokeRec, *StrokePtr;

typedef struct {
  int num_strokes;
  const StrokeRec *stroke;
  float center;
  float right;
} StrokeCharRec, *StrokeCharPtr;

typedef struct {
  const char *name;
  int num_chars;
  const StrokeCharRec *ch;
  float top;
  float bottom;
} StrokeFontRec, *StrokeFontPtr;

typedef void *GLUTstrokeFont;

#endif /* __glutstroke_h__ */
