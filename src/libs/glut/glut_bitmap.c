/*
 * Copyright 1994-1997 Mark Kilgard, All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Mark Kilgard
 */


#include "glutint.h"
#include "glutbitmap.h"


void APIENTRY 
glutBitmapCharacter(GLUTbitmapFont font, int c)
{
  const BitmapCharRec *ch;
  BitmapFontPtr fontinfo;
  GLint swapbytes, lsbfirst, rowlength;
  GLint skiprows, skippixels, alignment;

#if defined(_WIN32)
  fontinfo = (BitmapFontPtr) __glutFont(font);
#else
  fontinfo = (BitmapFontPtr) font;
#endif

  if (c < fontinfo->first ||
    c >= fontinfo->first + fontinfo->num_chars)
    return;
  ch = fontinfo->ch[c - fontinfo->first];
  if (ch) {
    /* Save current modes. */
    glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapbytes);
    glGetIntegerv(GL_UNPACK_LSB_FIRST, &lsbfirst);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skiprows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skippixels);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    /* Little endian machines (DEC Alpha for example) could
       benefit from setting GL_UNPACK_LSB_FIRST to GL_TRUE
       instead of GL_FALSE, but this would require changing the
       generated bitmaps too. */
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBitmap(ch->width, ch->height, ch->xorig, ch->yorig,
      ch->advance, 0, ch->bitmap);
    /* Restore saved modes. */
    glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
  }
}
