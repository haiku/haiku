/*
 * Copyright 1994-1997 Mark Kilgard, All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Mark Kilgard
 */


#include "glutint.h"
#include "glutbitmap.h"


/* CENTRY */
int APIENTRY 
glutBitmapWidth(GLUTbitmapFont font, int c)
{
  BitmapFontPtr fontinfo;
  const BitmapCharRec *ch;

#ifdef _WIN32
  fontinfo = (BitmapFontPtr) __glutFont(font);
#else
  fontinfo = (BitmapFontPtr) font;
#endif

  if (c < fontinfo->first || c >= fontinfo->first + fontinfo->num_chars)
    return 0;
  ch = fontinfo->ch[c - fontinfo->first];
  if (ch)
    return ch->advance;
  else
    return 0;
}

int APIENTRY 
glutBitmapLength(GLUTbitmapFont font, const unsigned char *string)
{
  int c, length;
  BitmapFontPtr fontinfo;
  const BitmapCharRec *ch;

#ifdef _WIN32
  fontinfo = (BitmapFontPtr) __glutFont(font);
#else
  fontinfo = (BitmapFontPtr) font;
#endif

  length = 0;
  for (; *string != '\0'; string++) {
    c = *string;
    if (c >= fontinfo->first && c < fontinfo->first + fontinfo->num_chars) {
      ch = fontinfo->ch[c - fontinfo->first];
      if (ch)
        length += ch->advance;
    }
  }
  return length;
}

/* ENDCENTRY */
