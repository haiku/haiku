/*
 * Copyright 1994-1997 Mark Kilgard, All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Mark Kilgard
 */


#include "glutint.h"
#include "glutstroke.h"


/* CENTRY */
int APIENTRY 
glutStrokeWidth(GLUTstrokeFont font, int c)
{
  StrokeFontPtr fontinfo;
  const StrokeCharRec *ch;

#if defined(_WIN32)
  fontinfo = (StrokeFontPtr) __glutFont(font);
#else
  fontinfo = (StrokeFontPtr) font;
#endif

  if (c < 0 || c >= fontinfo->num_chars)
    return 0;
  ch = &(fontinfo->ch[c]);
  if (ch)
    return ch->right;
  else
    return 0;
}

int APIENTRY 
glutStrokeLength(GLUTstrokeFont font, const unsigned char *string)
{
  int c, length;
  StrokeFontPtr fontinfo;
  const StrokeCharRec *ch;

#if defined(_WIN32)
  fontinfo = (StrokeFontPtr) __glutFont(font);
#else
  fontinfo = (StrokeFontPtr) font;
#endif

  length = 0;
  for (; *string != '\0'; string++) {
    c = *string;
    if (c >= 0 && c < fontinfo->num_chars) {
      ch = &(fontinfo->ch[c]);
      if (ch)
        length += ch->right;
    }
  }
  return length;
}

/* ENDCENTRY */
