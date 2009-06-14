#ifndef HAIKU_TYPES_H
#define HAIKU_TYPES_H

#include <stdio.h>
#include <stdlib.h>


typedef signed char int8;
typedef unsigned char uint8;

typedef short int16;
typedef unsigned short uint16;

typedef long int32;
typedef unsigned long uint32;

typedef long long int64;
typedef unsigned long long uint64;

typedef long status_t;

#define B_OK				0
#define B_NO_MEMORY			0x80000000
#define B_BAD_DATA			0x80000001
#define B_ERROR				0x80000002
#define B_ENTRY_NOT_FOUND	0x80000003
#define B_UNSUPPORTED		0x80000004

#define B_HOST_IS_LENDIAN 1


#define WRAP_ERROR(error)	(-(error))


static inline void
debugger(const char* message)
{
	fprintf(stderr, "debugger(): %s\n", message);
	exit(1);
}


#endif	// HAIKU_TYPES_H
