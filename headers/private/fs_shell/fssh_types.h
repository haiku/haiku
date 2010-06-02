/*
 * Copyright 2007-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_TYPES_H
#define _FSSH_TYPES_H


#include <inttypes.h>


typedef volatile int32_t	vint32_t;
typedef volatile int64_t	vint64_t;

#ifdef HAIKU_HOST_PLATFORM_64_BIT
typedef uint64_t	fssh_addr_t;
typedef uint64_t	fssh_phys_addr_t;
typedef uint64_t	fssh_generic_addr_t;
typedef uint64_t	fssh_size_t;
typedef int64_t		fssh_ssize_t;
#else
typedef uint32_t	fssh_addr_t;
typedef uint32_t	fssh_phys_addr_t;
typedef uint32_t	fssh_generic_addr_t;
typedef uint32_t	fssh_size_t;
typedef int32_t		fssh_ssize_t;
#endif
typedef fssh_phys_addr_t	fssh_phys_size_t;
typedef fssh_generic_addr_t	fssh_generic_size_t;

typedef int32_t		fssh_dev_t;
typedef int64_t		fssh_ino_t;

typedef int64_t		fssh_off_t;

typedef int64_t		fssh_bigtime_t;

typedef int32_t		fssh_status_t;
typedef uint32_t	fssh_type_code;

typedef uint32_t	fssh_mode_t;
typedef uint32_t	fssh_nlink_t;
typedef uint32_t	fssh_uid_t;
typedef uint32_t	fssh_gid_t;
typedef int32_t		fssh_pid_t;

#ifndef NULL
#define NULL (0)
#endif


// NOTE: For this to work the __STDC_FORMAT_MACROS feature macro must be
// defined.

/* printf()/scanf() format strings for [u]int* types */
#define FSSH_B_PRId8		PRId8
#define FSSH_B_PRIi8		PRIi8
#define FSSH_B_PRId16		PRId16
#define FSSH_B_PRIi16		PRIi16
#define FSSH_B_PRId32		PRId32
#define FSSH_B_PRIi32		PRIi32
#define FSSH_B_PRId64		PRId64
#define FSSH_B_PRIi64		PRIi64
#define FSSH_B_PRIu8		PRIu8
#define FSSH_B_PRIo8		PRIo8
#define FSSH_B_PRIx8		PRIx8
#define FSSH_B_PRIX8		PRIX8
#define FSSH_B_PRIu16		PRIu16
#define FSSH_B_PRIo16		PRIo16
#define FSSH_B_PRIx16		PRIx16
#define FSSH_B_PRIX16		PRIX16
#define FSSH_B_PRIu32		PRIu32
#define FSSH_B_PRIo32		PRIo32
#define FSSH_B_PRIx32		PRIx32
#define FSSH_B_PRIX32		PRIX32
#define FSSH_B_PRIu64		PRIu64
#define FSSH_B_PRIo64		PRIo64
#define FSSH_B_PRIx64		PRIx64
#define FSSH_B_PRIX64		PRIX64

#define FSSH_B_SCNd8 		SCNd8
#define FSSH_B_SCNi8 		SCNi8
#define FSSH_B_SCNd16		SCNd16
#define FSSH_B_SCNi16 		SCNi16
#define FSSH_B_SCNd32 		SCNd32
#define FSSH_B_SCNi32 		SCNi32
#define FSSH_B_SCNd64		SCNd64
#define FSSH_B_SCNi64 		SCNi64
#define FSSH_B_SCNu8 		SCNu8
#define FSSH_B_SCNo8 		SCNo8
#define FSSH_B_SCNx8 		SCNx8
#define FSSH_B_SCNu16		SCNu16
#define FSSH_B_SCNu16		SCNu16
#define FSSH_B_SCNx16		SCNx16
#define FSSH_B_SCNu32 		SCNu32
#define FSSH_B_SCNo32 		SCNo32
#define FSSH_B_SCNx32 		SCNx32
#define FSSH_B_SCNu64		SCNu64
#define FSSH_B_SCNo64		SCNo64
#define FSSH_B_SCNx64		SCNx64


/* printf() format strings for some standard types */
/* addr_t, size_t, ssize_t */
#ifdef HAIKU_HOST_PLATFORM_64_BIT
#	define FSSH_B_PRIuADDR	FSSH_B_PRIu64
#	define FSSH_B_PRIoADDR	FSSH_B_PRIo64
#	define FSSH_B_PRIxADDR	FSSH_B_PRIx64
#	define FSSH_B_PRIXADDR	FSSH_B_PRIX64
#	define FSSH_B_PRIuSIZE	FSSH_B_PRIu64
#	define FSSH_B_PRIoSIZE	FSSH_B_PRIo64
#	define FSSH_B_PRIxSIZE	FSSH_B_PRIx64
#	define FSSH_B_PRIXSIZE	FSSH_B_PRIX64
#	define FSSH_B_PRIdSSIZE	FSSH_B_PRId64
#	define FSSH_B_PRIiSSIZE	FSSH_B_PRIi64
#else
#	define FSSH_B_PRIuADDR	FSSH_B_PRIu32
#	define FSSH_B_PRIoADDR	FSSH_B_PRIo32
#	define FSSH_B_PRIxADDR	FSSH_B_PRIx32
#	define FSSH_B_PRIXADDR	FSSH_B_PRIX32
#	define FSSH_B_PRIuSIZE	FSSH_B_PRIu32
#	define FSSH_B_PRIoSIZE	FSSH_B_PRIo32
#	define FSSH_B_PRIxSIZE	FSSH_B_PRIx32
#	define FSSH_B_PRIXSIZE	FSSH_B_PRIX32
#	define FSSH_B_PRIdSSIZE	FSSH_B_PRId32
#	define FSSH_B_PRIiSSIZE	FSSH_B_PRIi32
#endif
/* phys_addr_t */
#ifdef HAIKU_HOST_PLATFORM_64_BIT
#	define FSSH_B_PRIuPHYSADDR	FSSH_B_PRIu64
#	define FSSH_B_PRIoPHYSADDR	FSSH_B_PRIo64
#	define FSSH_B_PRIxPHYSADDR	FSSH_B_PRIx64
#	define FSSH_B_PRIXPHYSADDR	FSSH_B_PRIX64
#else
#	define FSSH_B_PRIuPHYSADDR	FSSH_B_PRIu32
#	define FSSH_B_PRIoPHYSADDR	FSSH_B_PRIo32
#	define FSSH_B_PRIxPHYSADDR	FSSH_B_PRIx32
#	define FSSH_B_PRIXPHYSADDR	FSSH_B_PRIX32
#endif
/* generic_addr_t */
#define FSSH_B_PRIuGENADDR	FSSH_B_PRIuPHYSADDR
#define FSSH_B_PRIoGENADDR	FSSH_B_PRIoPHYSADDR
#define FSSH_B_PRIxGENADDR	FSSH_B_PRIxPHYSADDR
#define FSSH_B_PRIXGENADDR	FSSH_B_PRIXPHYSADDR
/* off_t */
#define FSSH_B_PRIdOFF		FSSH_B_PRId64
#define FSSH_B_PRIiOFF		FSSH_B_PRIi64
/* dev_t */
#define FSSH_B_PRIdDEV		FSSH_B_PRId32
#define FSSH_B_PRIiDEV		FSSH_B_PRIi32
/* ino_t */
#define FSSH_B_PRIdINO		FSSH_B_PRId64
#define FSSH_B_PRIiINO		FSSH_B_PRIi64
/* time_t */
#define FSSH_B_PRIdTIME		FSSH_B_PRId32
#define FSSH_B_PRIiTIME		FSSH_B_PRIi32


/* limits */
#ifdef HAIKU_HOST_PLATFORM_64_BIT
#	define FSSH_SIZE_MAX		UINT64_MAX
#	define FSSH_SSIZE_MAX		INT64_MAX
#else
#	define FSSH_SIZE_MAX		UINT32_MAX
#	define FSSH_SSIZE_MAX		INT32_MAX
#endif


#endif	// _FSSH_TYPES_H
