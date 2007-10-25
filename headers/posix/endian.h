#ifndef _ENDIAN_H_
#define _ENDIAN_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/


/* Defines architecture dependent endian constants.
 * The constant reflects the byte order, "4" is the most
 * significant byte, "1" the least significant one.
 */

#if defined(__INTEL__)
#	define LITTLE_ENDIAN	1234
#	define BIG_ENDIAN		0
#	define BYTE_ORDER		LITTLE_ENDIAN
#elif defined(__POWERPC__) || defined(__M68K__)
#	define BIG_ENDIAN		4321
#	define LITTLE_ENDIAN	0
#	define BYTE_ORDER		BIG_ENDIAN
#endif

#define __BIG_ENDIAN		BIG_ENDIAN
#define __LITTLE_ENDIAN		LITTLE_ENDIAN
#define __BYTE_ORDER		BYTE_ORDER

#endif	/* _ENDIAN_H_ */
