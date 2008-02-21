/*
 * Copyright 2002-2008 Haiku inc. All rights reserved.
 * Distributed under the terms of the MIT License
 */
#ifndef _INTTYPES_H_
#define _INTTYPES_H_


#include <stdint.h>


typedef struct {
	intmax_t	quot;	/* quotient */
	intmax_t	rem;	/* remainder */
} imaxdiv_t;

#if !defined(__cplusplus) || defined(__STDC_FORMAT_MACROS)
/* fprintf() macros for signed integers */
#	define PRId8			"d"
#	define PRIdLEAST8		"d"
#	define PRIdFAST8		"d"
#	define PRIi8			"i"
#	define PRIiLEAST8		"i"
#	define PRIiFAST8		"i"

#	define PRId16			"d"
#	define PRIdLEAST16		"d"
#	define PRIdFAST16		"d"
#	define PRIi16			"i"
#	define PRIiLEAST16		"i"
#	define PRIiFAST16		"i"

#	define PRId32			"d"
#	define PRIdLEAST32		"d"
#	define PRIdFAST32		"d"
#	define PRIi32			"i"
#	define PRIiLEAST32		"i"
#	define PRIiFAST32		"i"

#	define PRId64			"lld"
#	define PRIdLEAST64		"lld"
#	define PRIdFAST64		"lld"
#	define PRIi64			"lli"
#	define PRIiLEAST64		"lli"
#	define PRIiFAST64		"lli"

#	define PRIdMAX			"lld"
#	define PRIdPTR			"d"
#	define PRIiMAX			"lli"
#	define PRIiPTR			"i"

/* fprintf() macros for unsigned integers */
#	define PRIu8			"u"
#	define PRIuLEAST8		"u"
#	define PRIuFAST8		"u"
#	define PRIo8			"o"
#	define PRIoLEAST8		"o"
#	define PRIoFAST8		"o"
#	define PRIx8			"x"
#	define PRIxLEAST8		"x"
#	define PRIxFAST8		"x"
#	define PRIX8			"X"
#	define PRIXLEAST8		"X"
#	define PRIXFAST8		"X"

#	define PRIu16			"u"
#	define PRIuLEAST16		"u"
#	define PRIuFAST16		"u"
#	define PRIo16			"o"
#	define PRIoLEAST16		"o"
#	define PRIoFAST16		"o"
#	define PRIx16			"x"
#	define PRIxLEAST16		"x"
#	define PRIxFAST16		"x"
#	define PRIX16			"X"
#	define PRIXLEAST16		"X"
#	define PRIXFAST16		"X"

#	define PRIu32			"u"
#	define PRIuLEAST32		"u"
#	define PRIuFAST32		"u"
#	define PRIo32			"o"
#	define PRIoLEAST32		"o"
#	define PRIoFAST32		"o"
#	define PRIx32			"x"
#	define PRIxLEAST32		"x"
#	define PRIxFAST32		"x"
#	define PRIX32			"X"
#	define PRIXLEAST32		"X"
#	define PRIXFAST32		"X"

#	define PRIu64			"llu"
#	define PRIuLEAST64		"llu"
#	define PRIuFAST64		"llu"
#	define PRIo64			"llo"
#	define PRIoLEAST64		"llo"
#	define PRIoFAST64		"llo"
#	define PRIx64			"llx"
#	define PRIxLEAST64		"llx"
#	define PRIxFAST64		"llx"
#	define PRIX64			"llX"
#	define PRIXLEAST64		"llX"
#	define PRIXFAST64		"llX"

#	define PRIuMAX			"llu"
#	define PRIuPTR			"u"
#	define PRIoMAX			"llo"
#	define PRIoPTR			"o"
#	define PRIxMAX			"llx"
#	define PRIxPTR			"x"
#	define PRIXMAX			"llX"
#	define PRIXPTR			"X"

/* fscanf() macros for signed integers */
#	define SCNd8 			"hhd"
#	define SCNdLEAST8 		"hhd"
#	define SCNdFAST8 		"d"
#	define SCNi8 			"hhi"
#	define SCNiLEAST8 		"hhi"
#	define SCNiFAST8 		"i"

#	define SCNd16			"hd"
#	define SCNdLEAST16		"hd"
#	define SCNdFAST16		"d"
#	define SCNi16 			"hi"
#	define SCNiLEAST16		"hi"
#	define SCNiFAST16		"i"

#	define SCNd32 			"d"
#	define SCNdLEAST32		"d"
#	define SCNdFAST32		"d"
#	define SCNi32 			"i"
#	define SCNiLEAST32		"i"
#	define SCNiFAST32		"i"

#	define SCNd64			"lld"
#	define SCNdLEAST64		"lld"
#	define SCNdFAST64		"lld"
#	define SCNi64 			"lli"
#	define SCNiLEAST64		"lli"
#	define SCNiFAST64 		"lli"

#	define SCNdMAX			"lld"
#	define SCNdPTR			"d"
#	define SCNiMAX			"lli"
#	define SCNiPTR			"i"

/* fscanf() macros for unsigned integers */
#	define SCNu8 			"hhu"
#	define SCNuLEAST8 		"hhu"
#	define SCNuFAST8 		"u"
#	define SCNo8 			"hho"
#	define SCNoLEAST8 		"hho"
#	define SCNoFAST8 		"o"
#	define SCNx8 			"hhx"
#	define SCNxLEAST8 		"hhx"
#	define SCNxFAST8 		"x"

#	define SCNu16			"hu"
#	define SCNuLEAST16		"hu"
#	define SCNuFAST16		"u"
#	define SCNo16			"ho"
#	define SCNoLEAST16		"ho"
#	define SCNoFAST16		"o"
#	define SCNx16			"hx"
#	define SCNxLEAST16		"hx"
#	define SCNxFAST16		"x"

#	define SCNu32 			"u"
#	define SCNuLEAST32		"u"
#	define SCNuFAST32		"u"
#	define SCNo32 			"o"
#	define SCNoLEAST32		"o"
#	define SCNoFAST32		"o"
#	define SCNx32 			"x"
#	define SCNxLEAST32		"x"
#	define SCNxFAST32		"x"

#	define SCNu64			"llu"
#	define SCNuLEAST64		"llu"
#	define SCNuFAST64		"llu"
#	define SCNo64			"llo"
#	define SCNoLEAST64		"llo"
#	define SCNoFAST64		"llo"
#	define SCNx64			"llx"
#	define SCNxLEAST64		"llx"
#	define SCNxFAST64		"llx"

#	define SCNuMAX			"llu"
#	define SCNuPTR			"u"
#	define SCNoMAX			"llo"
#	define SCNoPTR			"o"
#	define SCNxMAX			"llx"
#	define SCNxPTR			"x"
#endif /* !defined(__cplusplus) || defined(__STDC_FORMAT_MACROS) */


#ifdef __cplusplus
extern "C" {
#endif

extern intmax_t		imaxabs(intmax_t num);
extern imaxdiv_t	imaxdiv(intmax_t numer, intmax_t denom);

extern intmax_t		strtoimax(const char *string, char **_end, int base);
extern uintmax_t	strtoumax(const char *string, char **_end, int base);
//extern intmax_t		wcstoimax(const __wchar_t *, __wchar_t **, int);
//extern uintmax_t	wcstoumax(const __wchar_t *, __wchar_t **, int);

#ifdef __cplusplus
}
#endif

#endif	/* _INTTYPES_H_ */
