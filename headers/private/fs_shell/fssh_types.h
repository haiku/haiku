#ifndef _FSSH_TYPES_H
#define _FSSH_TYPES_H

#include <inttypes.h>

typedef uint32_t			fssh_ulong;
typedef volatile int32_t	vint32_t;
typedef volatile int64_t	vint64_t;

#ifdef __x86_64__
typedef uint64_t	fssh_addr_t;
#else
typedef uint32_t	fssh_addr_t;
#endif

typedef int32_t		fssh_dev_t;
typedef int64_t		fssh_ino_t;

typedef uint32_t	fssh_size_t;
typedef int32_t		fssh_ssize_t;
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

#endif	// _FSSH_TYPES_H
