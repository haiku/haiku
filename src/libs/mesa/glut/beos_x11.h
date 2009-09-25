#ifndef __beos_x11_h__
#define __beos_x11_h__

/* Copyright (c) Nate Robins, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* 
 * Bitmask returned by XParseGeometry().  Each bit tells if the corresponding
 * value (x, y, width, height) was found in the parsed string.
*/
#define NoValue		0x0000
#define XValue  	0x0001
#define YValue		0x0002
#define WidthValue  	0x0004
#define HeightValue  	0x0008
#define AllValues 	0x000F
#define XNegative 	0x0010
#define YNegative 	0x0020

/* Function prototypes. */

extern int DisplayWidth();
extern int DisplayHeight();

extern int XParseGeometry(
  char* string,
  int* x, int* y, 
  unsigned int* width, unsigned int* height);

#endif /* __beos_x11_h__ */
