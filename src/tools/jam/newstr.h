/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * newstr.h - string manipulation routines
 */

char *newstr( char *string );
char *copystr( char *s );
void freestr( char *s );
void donestr();
