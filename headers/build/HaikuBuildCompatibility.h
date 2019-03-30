#ifndef HAIKU_BUILD_COMPATIBILITY_H
#define HAIKU_BUILD_COMPATIBILITY_H

/*!
	This header is automatically included in all Haiku applications
	that are built for BeOS or a Haiku host (which might not be compatible
	with the current Haiku source anymore).
	It will make BeOS/Haiku a bit more Haiku compatible, and slightly more
	POSIX compatible, too. It is supposed to keep the BeOS compatibility
	kludges in our source files at a minimum.
*/

#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	define _BE_ERRNO_H_
		// this is what Dano/Zeta is using
#	include <Errors.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <SupportDefs.h>
#include <TypeConstants.h>

#include <string.h>

// no addr_t under standard BeOS
#if !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST) \
	&& !defined(HAIKU_HOST_PLATFORM_HAIKU)
	typedef unsigned long haiku_build_addr_t;
#	define addr_t haiku_build_addr_t
#endif

#if !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST) \
	&& !defined(HAIKU_HOST_PLATFORM_HAIKU)

struct sockaddr_storage {
	uint8	ss_len;			/* total length */
	uint8	ss_family;		/* address family */
	uint8	__ss_pad1[6];	/* align to quad */
	uint64	__ss_pad2;		/* force alignment to 64 bit */
	uint8	__ss_pad3[112];	/* pad to a total of 128 bytes */
};

typedef int socklen_t;

#endif	// !HAIKU_HOST_PLATFORM_HAIKU

#ifndef DEFFILEMODE
#	define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#endif

#ifndef FS_WRITE_FSINFO_NAME
#	define	FS_WRITE_FSINFO_NAME	0x0001
#endif

#ifndef B_CURRENT_TEAM
#	define B_CURRENT_TEAM	0
#endif

#ifndef SYMLOOP_MAX
#	define SYMLOOP_MAX	(16)
#endif

#ifndef B_FIRST_REAL_TIME_PRIORITY
#	define B_FIRST_REAL_TIME_PRIORITY B_REAL_TIME_DISPLAY_PRIORITY
#endif

#if !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST) \
	&& !defined(B_SPINLOCK_INITIALIZER)
#	define B_SPINLOCK_INITIALIZER 0
#endif

#if defined(__GNUC__) && !defined(_PRINTFLIKE)
#	define _PRINTFLIKE(_format_, _args_) \
		__attribute__((format(__printf__, _format_, _args_)))
#endif

#if 0
// NOTE: This is probably only needed on platforms which don't use ELF
// as binary format. So could probably be removed completely.
#if defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST) \
	&& !defined(HAIKU_HOST_PLATFORM_HAIKU)
// BeOS version of BeBuild.h defines this
#	define _IMPEXP_ROOT			__declspec(dllimport)
#	define _IMPEXP_BE			__declspec(dllimport)
#	define _IMPEXP_MEDIA		__declspec(dllimport)
#	define _IMPEXP_TRACKER		__declspec(dllimport)
#	define _IMPEXP_TRANSLATION	__declspec(dllimport)
#	define _IMPEXP_DEVICE		__declspec(dllimport)
#	define _IMPEXP_NET			__declspec(dllimport)
#endif
#endif

#if defined(__cplusplus) && !defined(HAIKU_HOST_PLATFORM_HAIKU)
class BBuffer;
class BBufferConsumer;
class BBufferGroup;
class BContinuousParameter;
class BControllable;
class BFileInterface;
class BMimeType;
class BParameterWeb;
class BRegion;
class BTextView;
class BTranslator;
class BTimeSource;
struct entry_ref;
struct media_node;
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern size_t	strnlen(const char *string, size_t count);

extern size_t	strlcat(char *dest, const char *source, size_t length);
extern size_t   strlcpy(char *dest, const char *source, size_t length);

extern char		*strcasestr(const char *string, const char *searchString);

extern float	roundf(float value);

#ifdef __cplusplus
}
#endif

// These are R1-specific extensions
#if !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST) \
	&& !defined(HAIKU_HOST_PLATFORM_HAIKU)
#	define B_TRANSLATION_MAKE_VERSION(major, minor, revision) \
		((major << 8) | ((minor << 4) & 0xf0) | (revision & 0x0f))
#	define B_TRANSLATION_MAJOR_VERSION(v) (v >> 8)
#	define B_TRANSLATION_MINOR_VERSION(v) ((v >> 4) & 0xf)
#	define B_TRANSLATION_REVISION_VERSION(v) (v & 0xf)
#	ifndef USING_HAIKU_TYPE_CONSTANTS_H
#		define B_LARGE_ICON_TYPE		'ICON'
#		define B_MINI_ICON_TYPE			'MICN'
#		define B_VECTOR_ICON_TYPE		'VICN'
#		define B_BITMAP_NO_SERVER_LINK	0
#		define B_BITMAP_SCALE_BILINEAR	0
#	endif
#endif	// HAIKU_TARGET_PLATFORM_LIBBE_TEST

#if !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST) \
	&& !defined(HAIKU_HOST_PLATFORM_HAIKU)
#	if !defined(B_NOT_SUPPORTED)
#		define B_NOT_SUPPORTED			(B_ERROR)
#	endif
#	define B_KERNEL_READ_AREA			0
#	define B_KERNEL_WRITE_AREA			0
#endif

#include <limits.h>

#ifndef INT32_MAX
#	define INT32_MAX INT_MAX
#endif

#ifndef INT64_MAX
#	define INT64_MAX LONGLONG_MAX
#endif

#if !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST) \
	&& !defined(HAIKU_HOST_PLATFORM_HAIKU)
#	define	B_MPEG_2_AUDIO_LAYER_1 (enum mpeg_id)0x201
#	define	B_MPEG_2_AUDIO_LAYER_2 (enum mpeg_id)0x202
#	define	B_MPEG_2_AUDIO_LAYER_3 (enum mpeg_id)0x203
#	define	B_MPEG_2_VIDEO (enum mpeg_id)0x211
#	define	B_MPEG_2_5_AUDIO_LAYER_1 (enum mpeg_id)0x301
#	define	B_MPEG_2_5_AUDIO_LAYER_2 (enum mpeg_id)0x302
#	define	B_MPEG_2_5_AUDIO_LAYER_3 (enum mpeg_id)0x303
#endif

// TODO: experimental API (keep in sync with Accelerant.h)
#if !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST) \
	&& !defined(HAIKU_HOST_PLATFORM_HAIKU)
typedef struct {
	uint32	version;
	char	vendor[128];
	char	name[128];
	char	serial_number[128];
	uint32	product_id;
	struct {
		uint16	week;
		uint16	year;
	}		produced;
	float	width;
	float	height;
	uint32	min_horizontal_frequency;	// in kHz
	uint32	max_horizontal_frequency;
	uint32	min_vertical_frequency;		// in Hz
	uint32	max_vertical_frequency;
	uint32	max_pixel_clock;			// in kHz
} monitor_info;
#endif // !HAIKU_HOST_PLATFORM_HAIKU


#if !defined(B_HAIKU_32_BIT) && !defined(B_HAIKU_64_BIT)
#	ifdef HAIKU_HOST_PLATFORM_64_BIT
#		define B_HAIKU_64_BIT	1
#	else
#		define B_HAIKU_32_BIT	1
#	endif
#endif

#ifndef B_PRId8
#	define	__HAIKU_PRI_PREFIX_32		"l"
#	define	__HAIKU_PRI_PREFIX_64		"ll"
#	define	__HAIKU_PRI_PREFIX_ADDR		"l"

	/* printf()/scanf() format strings for [u]int* types */
#	define B_PRId8			"d"
#	define B_PRIi8			"i"
#	define B_PRId16			"d"
#	define B_PRIi16			"i"
#	define B_PRId32			__HAIKU_PRI_PREFIX_32 "d"
#	define B_PRIi32			__HAIKU_PRI_PREFIX_32 "i"
#	define B_PRId64			__HAIKU_PRI_PREFIX_64 "d"
#	define B_PRIi64			__HAIKU_PRI_PREFIX_64 "i"
#	define B_PRIu8			"u"
#	define B_PRIo8			"o"
#	define B_PRIx8			"x"
#	define B_PRIX8			"X"
#	define B_PRIu16			"u"
#	define B_PRIo16			"o"
#	define B_PRIx16			"x"
#	define B_PRIX16			"X"
#	define B_PRIu32			__HAIKU_PRI_PREFIX_32 "u"
#	define B_PRIo32			__HAIKU_PRI_PREFIX_32 "o"
#	define B_PRIx32			__HAIKU_PRI_PREFIX_32 "x"
#	define B_PRIX32			__HAIKU_PRI_PREFIX_32 "X"
#	define B_PRIu64			__HAIKU_PRI_PREFIX_64 "u"
#	define B_PRIo64			__HAIKU_PRI_PREFIX_64 "o"
#	define B_PRIx64			__HAIKU_PRI_PREFIX_64 "x"
#	define B_PRIX64			__HAIKU_PRI_PREFIX_64 "X"

#	define B_SCNd8 			"hhd"
#	define B_SCNi8 			"hhi"
#	define B_SCNd16			"hd"
#	define B_SCNi16	 		"hi"
#	define B_SCNd32 		__HAIKU_PRI_PREFIX_32 "d"
#	define B_SCNi32	 		__HAIKU_PRI_PREFIX_32 "i"
#	define B_SCNd64			__HAIKU_PRI_PREFIX_64 "d"
#	define B_SCNi64 		__HAIKU_PRI_PREFIX_64 "i"
#	define B_SCNu8 			"hhu"
#	define B_SCNo8 			"hho"
#	define B_SCNx8 			"hhx"
#	define B_SCNu16			"hu"
#	define B_SCNo16			"ho"
#	define B_SCNx16			"hx"
#	define B_SCNu32 		__HAIKU_PRI_PREFIX_32 "u"
#	define B_SCNo32 		__HAIKU_PRI_PREFIX_32 "o"
#	define B_SCNx32 		__HAIKU_PRI_PREFIX_32 "x"
#	define B_SCNu64			__HAIKU_PRI_PREFIX_64 "u"
#	define B_SCNo64			__HAIKU_PRI_PREFIX_64 "o"
#	define B_SCNx64			__HAIKU_PRI_PREFIX_64 "x"

	/* printf() format strings for some standard types */
	/* size_t */
#	define B_PRIuSIZE		__HAIKU_PRI_PREFIX_ADDR "u"
#	define B_PRIoSIZE		__HAIKU_PRI_PREFIX_ADDR "o"
#	define B_PRIxSIZE		__HAIKU_PRI_PREFIX_ADDR "x"
#	define B_PRIXSIZE		__HAIKU_PRI_PREFIX_ADDR "X"
	/* ssize_t */
#	define B_PRIdSSIZE		__HAIKU_PRI_PREFIX_ADDR "d"
#	define B_PRIiSSIZE		__HAIKU_PRI_PREFIX_ADDR "i"
	/* addr_t */
#	define B_PRIuADDR		__HAIKU_PRI_PREFIX_ADDR "u"
#	define B_PRIoADDR		__HAIKU_PRI_PREFIX_ADDR "o"
#	define B_PRIxADDR		__HAIKU_PRI_PREFIX_ADDR "x"
#	define B_PRIXADDR		__HAIKU_PRI_PREFIX_ADDR "X"
	/* off_t */
#	define B_PRIdOFF		B_PRId64
#	define B_PRIiOFF		B_PRIi64
	/* dev_t */
#	define B_PRIdDEV		B_PRId32
#	define B_PRIiDEV		B_PRIi32
	/* ino_t */
#	define B_PRIdINO		B_PRId64
#	define B_PRIiINO		B_PRIi64
	/* time_t */
#	if defined(__i386__) && !defined(__x86_64__)
#		define B_PRIdTIME		B_PRId32
#		define B_PRIiTIME		B_PRIi32
#	else
#		define B_PRIdTIME		B_PRId64
#		define B_PRIiTIME		B_PRIi64
#	endif
#endif	// !B_PRId8


#endif	// HAIKU_BUILD_COMPATIBILITY_H
