#ifndef HAIKU_BUILD_COMPATIBILITY_H
#define HAIKU_BUILD_COMPATIBILITY_H

/*!
	This header is automatically included in all Haiku applications
	that are built for BeOS.
	It will make BeOS a bit more Haiku compatible, and slightly more
	POSIX compatible, too. It is supposed to keep the BeOS compatibility
	kludges in our source files at a minimum.
*/

#ifdef HAIKU_HOST_PLATFORM_DANO
#	include <be_setup.h>
#	include <be_errors.h>
#	define _ERRORS_H
		// this is what Haiku/BeOS is using
#endif

#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	define _BE_ERRNO_H_
		// this is what Dano/Zeta is using
#	include <Errors.h>
#endif

#include <sys/types.h>
#include <SupportDefs.h>

#include <string.h>

// no addr_t under standard BeOS
typedef unsigned long haiku_build_addr_t;
#define addr_t haiku_build_addr_t

struct sockaddr_storage {
	uint8	ss_len;			/* total length */
	uint8	ss_family;		/* address family */
	uint8	__ss_pad1[6];	/* align to quad */
	uint64	__ss_pad2;		/* force alignment to 64 bit */
	uint8	__ss_pad3[112];	/* pad to a total of 128 bytes */
};

typedef int socklen_t;

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

#if __GNUC__
#	define _PRINTFLIKE(_format_, _args_) \
		__attribute__((format(__printf__, _format_, _args_)))
#endif

#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
// BeOS version of BeBuild.h defines this
#	define _IMPEXP_ROOT			__declspec(dllimport)
#	define _IMPEXP_BE			__declspec(dllimport)
#	define _IMPEXP_MEDIA		__declspec(dllimport)
#	define _IMPEXP_TRACKER		__declspec(dllimport)
#	define _IMPEXP_TRANSLATION	__declspec(dllimport)
#	define _IMPEXP_DEVICE		__declspec(dllimport)
#	define _IMPEXP_NET			__declspec(dllimport)
#endif

#ifdef __cplusplus
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

extern int32	atomic_set(vint32 *value, int32 newValue);
extern int32	atomic_test_and_set(vint32 *value, int32 newValue,
					int32 testAgainst);
extern int32	atomic_get(vint32 *value);
extern int64	atomic_set64(vint64 *value, int64 newValue);
extern int64	atomic_test_and_set64(vint64 *value, int64 newValue,
					int64 testAgainst);
extern int64	atomic_get64(vint64 *value);
extern int64	atomic_add64(vint64 *value, int64 addValue);
extern int64	atomic_and64(vint64 *value, int64 andValue);
extern int64	atomic_or64(vint64 *value, int64 orValue);

extern size_t	strnlen(const char *string, size_t count);

extern size_t	strlcat(char *dest, const char *source, size_t length);
extern size_t   strlcpy(char *dest, const char *source, size_t length);

extern char		*strcasestr(const char *string, const char *searchString);

extern float	roundf(float value);

#ifdef __cplusplus
}
#endif

// These are R1-specific extensions
#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
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
#	endif
#endif	// HAIKU_TARGET_PLATFORM_LIBBE_TEST

#if defined(HAIKU_TARGET_PLATFORM_BEOS) || defined(HAIKU_TARGET_PLATFORM_BONE)
#	define B_REDO						'REDO'
#	define B_BAD_DATA					(B_NOT_ALLOWED + 1)
#	define B_DOCUMENT_BACKGROUND_COLOR	B_PANEL_BACKGROUND_COLOR
#	define B_DOCUMENT_TEXT_COLOR		B_MENU_ITEM_TEXT_COLOR
#endif

#if !defined(HAIKU_TARGET_PLATFORM_HAIKU) && !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST)
#	ifndef HAIKU_HOST_PLATFORM_DANO
#		define B_NOT_SUPPORTED			(B_ERROR)
#	endif
#	define B_KERNEL_READ_AREA			0
#	define B_KERNEL_WRITE_AREA			0
#endif

#if defined(HAIKU_TARGET_PLATFORM_BONE) || defined(HAIKU_TARGET_PLATFORM_DANO)
#	define IF_NAMESIZE IFNAMSIZ
#	define ifc_value ifc_val
#	define IFF_AUTO_CONFIGURED 0
#endif

#ifndef INT64_MAX
#	include <limits.h>
#	define INT64_MAX LONGLONG_MAX
#endif

#define	B_MPEG_2_AUDIO_LAYER_1 (enum mpeg_id)0x201
#define	B_MPEG_2_AUDIO_LAYER_2 (enum mpeg_id)0x202
#define	B_MPEG_2_AUDIO_LAYER_3 (enum mpeg_id)0x203
#define	B_MPEG_2_VIDEO (enum mpeg_id)0x211
#define	B_MPEG_2_5_AUDIO_LAYER_1 (enum mpeg_id)0x301
#define	B_MPEG_2_5_AUDIO_LAYER_2 (enum mpeg_id)0x302
#define	B_MPEG_2_5_AUDIO_LAYER_3 (enum mpeg_id)0x303

// TODO: experimental API (keep in sync with Accelerant.h)
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


#endif	// HAIKU_BUILD_COMPATIBILITY_H

