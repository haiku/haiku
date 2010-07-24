/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _POSIX_CTYPE_H
#define _POSIX_CTYPE_H


namespace BPrivate {


/*
 * the following arrays have 257 elements where the first is a
 * dummy element (containing the neutral/identity value) used when
 * the array is accessed as in 'isblank(EOF)' (i.e. with index -1).
 */
extern const unsigned short gPosixClassInfo[257];
extern const int gPosixToLowerMap[257];
extern const int gPosixToUpperMap[257];


}	// namespace BPrivate


#endif	// _POSIX_CTYPE_H
