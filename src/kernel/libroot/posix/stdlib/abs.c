/*
 *  Copyright (c) 2002, OpenBeOS Project.
 *  All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *
 *  abs.c:
 *  implements the standard C library functions:
 *    abs, labs
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */


// ToDo: these are supposed to be declared in <stdlib.h>
// - - - - - - - - - - - - - - - 
int  abs(int);
long labs(long);
// - - - - - - - - - - - - - - -


int
abs(int i)
{
	return ((i < 0) ? -i : i);
}


long
labs(long i)
{
	return ((i < 0) ? -i : i);
}