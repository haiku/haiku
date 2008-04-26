#ifndef B_OK

#include <windows.h>
#include <winnt.h>
//#include <semaphore.h>
//#include <pthread.h>
//#include <sys/utsname.h>

#define B_OK				0
#define B_ENTRY_NOT_FOUND	ENOENT
#define B_PATH_NAME_LENGTH	1024
#define B_FILE_NAME_LENGTH	256
#define B_OS_NAME_LENGTH	32
#define B_INTERRUPTED		1

#define BEOS_ENOENT			2147508227
#define BEOS_EACCES			2147483650
#define BEOS_ENOMEM			2147483648
#define BEOS_EINVAL			0

#ifndef true
#define true				1
#define false				0
#endif

#define WSTAT_MODE			0x0001
#define WSTAT_UID			0x0002
#define WSTAT_GID			0x0004
#define WSTAT_SIZE			0x0008
#define WSTAT_ATIME			0x0010
#define WSTAT_MTIME			0x0020
#define WSTAT_CRTIME		0x0040

#define B_COMMON_SETTINGS_DIRECTORY		".\\"
#define B_COMMON_SYSTEM_DIRECTORY		".\\"

#define ENOTSUP				1

#define lstat(f, s)			stat(f, s)
#define S_ISDIR(m)			((m) & _S_IFDIR)

typedef char int8;
typedef long int32;
typedef LONGLONG int64;
typedef unsigned char uint8;
typedef unsigned long uint32;
typedef ULONGLONG uint64;

typedef uint64 vnode_id;
typedef uint64 beos_ino_t;
typedef uint64 beos_off_t;
typedef unsigned long beos_dev_t;
typedef HANDLE sem_id;
typedef HANDLE thread_id;

#define LITTLE_ENDIAN

#ifdef LITTLE_ENDIAN
#define B_LENDIAN_TO_HOST_INT32(x)		(x)
#define B_LENDIAN_TO_HOST_INT64(x)		(x)
#define B_HOST_TO_LENDIAN_INT32(x)		(x)
#define B_HOST_TO_LENDIAN_INT64(x)		(x)
#else
#define B_LENDIAN_TO_HOST_INT32(x)		btSwapInt32(x)
#define B_LENDIAN_TO_HOST_INT64(x)		btSwapInt64(x)
#define B_HOST_TO_LENDIAN_INT32(x)		btSwapInt32(x)
#define B_HOST_TO_LENDIAN_INT64(x)		btSwapInt64(x)
#endif

uint32 btSwapInt32(uint32 num);
uint64 btSwapInt64(uint64 num);


enum {
	B_ANY_TYPE 					= 'ANYT',
	B_BOOL_TYPE 				= 'BOOL',
	B_CHAR_TYPE 				= 'CHAR',
	B_COLOR_8_BIT_TYPE 			= 'CLRB',
	B_DOUBLE_TYPE 				= 'DBLE',
	B_FLOAT_TYPE 				= 'FLOT',
	B_GRAYSCALE_8_BIT_TYPE		= 'GRYB',
	B_INT64_TYPE 				= 'LLNG',
	B_INT32_TYPE 				= 'LONG',
	B_INT16_TYPE 				= 'SHRT',
	B_INT8_TYPE 				= 'BYTE',
	B_MESSAGE_TYPE				= 'MSGG',
	B_MESSENGER_TYPE			= 'MSNG',
	B_MIME_TYPE					= 'MIME',
	B_MONOCHROME_1_BIT_TYPE 	= 'MNOB',
	B_OBJECT_TYPE 				= 'OPTR',
	B_OFF_T_TYPE 				= 'OFFT',
	B_PATTERN_TYPE 				= 'PATN',
	B_POINTER_TYPE 				= 'PNTR',
	B_POINT_TYPE 				= 'BPNT',
	B_RAW_TYPE 					= 'RAWT',
	B_RECT_TYPE 				= 'RECT',
	B_REF_TYPE 					= 'RREF',
	B_RGB_32_BIT_TYPE 			= 'RGBB',
	B_RGB_COLOR_TYPE 			= 'RGBC',
	B_SIZE_T_TYPE	 			= 'SIZT',
	B_SSIZE_T_TYPE	 			= 'SSZT',
	B_STRING_TYPE 				= 'CSTR',
	B_TIME_TYPE 				= 'TIME',
	B_UINT64_TYPE				= 'ULLG',
	B_UINT32_TYPE				= 'ULNG',
	B_UINT16_TYPE 				= 'USHT',
	B_UINT8_TYPE 				= 'UBYT',
	B_MEDIA_PARAMETER_TYPE		= 'BMCT',
	B_MEDIA_PARAMETER_WEB_TYPE	= 'BMCW',
	B_MEDIA_PARAMETER_GROUP_TYPE= 'BMCG',

	/* deprecated, do not use */
	B_ASCII_TYPE 				= 'TEXT'	/* use B_STRING_TYPE instead */
};


typedef struct fs_info
{
	beos_dev_t	dev;
	beos_ino_t	root;
	uint32		flags;
//	beos_off_t	block_size;
	uint32		block_size;
	beos_off_t	io_size;
//	beos_off_t	total_blocks;
//	beos_off_t	free_blocks;
	uint32		total_blocks;
	uint32		free_blocks;
	beos_off_t	total_nodes;
	beos_off_t	free_nodes;
	char		device_name[128];
	char		volume_name[B_FILE_NAME_LENGTH];
	char		fsh_name[B_OS_NAME_LENGTH];
} fs_info;

typedef struct beos_stat
{
	uint32		st_dev;
	uint32		st_nlink;
	uint32		st_uid;
	uint32		st_gid;
	uint32		st_size;
	uint32		st_blksize;
	uint32		st_rdev;
	uint32		st_ino;
	uint32		st_mode;
	uint32		st_atime;
	uint32		st_mtime;
	uint32		st_ctime;
} beos_stat;

#endif
