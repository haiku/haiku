/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *
 *  FILE:	glutDstr.cpp
 *
 *	DESCRIPTION:	convert display string into a Be options variable
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include <string.h>
#include <stdlib.h>
#include "glutint.h"
#include "glutState.h"

/***********************************************************
 *	FUNCTION:	glutInitDisplayString
 *
 *	DESCRIPTION:  sets the display string variable
 ***********************************************************/
void APIENTRY 
glutInitDisplayString(const char *string)
{
  if (gState.displayString) {
    free(gState.displayString);
  }
  if (string) {
    gState.displayString = strdup(string);
    if (!gState.displayString)
      __glutFatalError("out of memory.");
  } else
    gState.displayString = NULL;
}

/***********************************************************
 *	FUNCTION:	__glutConvertDisplayModeFromString
 *
 *	DESCRIPTION:  converts the current display mode into a BGLView
 *		display mode, printing warnings as appropriate.
 *
 *	PARAMETERS:	 if options is non-NULL, the current display mode is
 *		returned in it.
 *
 *	RETURNS:	1 if the current display mode is possible, else 0
 ***********************************************************/
int __glutConvertDisplayModeFromString(unsigned long *options) {
	ulong newoptions = 0;
	
	char *word = strtok(gState.displayString, " \t");
	do {
		char *cstr = strpbrk(word, "=><!~");
		if(cstr)
			*cstr = '\0';
		// this is the most minimal possible parser.  scan for
		// options that we support, and add them to newoptions
		// this will certainly cause it to accept things that we
		// don't actually support, but if we don't support it, the
		// program's probably not going to work anyway.
		if(!strcmp(word, "alpha")) {
			newoptions |= BGL_ALPHA;
		} else if((!strcmp(word, "acc")) || (!strcmp(word, "acca"))) {
			newoptions |= BGL_ACCUM;
		} else if(!strcmp(word, "depth")) {
			newoptions |= BGL_DEPTH;
		} else if(!strcmp(word, "double")) {
			newoptions |= BGL_DOUBLE;
		} else if(!strcmp(word, "stencil")) {
			newoptions |= BGL_STENCIL;
		}
	} while((word = strtok(0, " \t")) != 0);

	if (options)
		*options = newoptions;

	return 1;	// assume we support it
}
