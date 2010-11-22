/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _POSIX_CTYPE_H
#define _POSIX_CTYPE_H


namespace BPrivate {
namespace Libroot {


/*
 * The following arrays have 384 elements where the elements at index -128..-2
 * mirror the elements at index 128..255 (to protect against invocations of
 * ctype macros with negative character values).
 * The element at index -1 is a dummy element containing the neutral/identity
 * value used when the array is accessed as in 'isblank(EOF)' (i.e. with
 * index -1).
 */
extern const unsigned short gPosixClassInfo[384];
extern const int gPosixToLowerMap[384];
extern const int gPosixToUpperMap[384];


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _POSIX_CTYPE_H
