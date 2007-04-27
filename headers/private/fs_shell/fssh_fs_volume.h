/* File System volume functions
 *
 * Copyright 2004-2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_FS_VOLUME_H
#define	_FSSH_FS_VOLUME_H

#include "fssh_os.h"


/* mount flags */
#define FSSH_B_MOUNT_READ_ONLY		1
#define FSSH_B_MOUNT_VIRTUAL_DEVICE	2

/* unmount flags */
#define FSSH_B_FORCE_UNMOUNT		1


#ifdef  __cplusplus
extern "C" {
#endif

extern fssh_dev_t		fssh_fs_mount_volume(const char *where,
							const char *device, const char *filesystem,
							uint32_t flags, const char *parameters);
extern fssh_status_t	fssh_fs_unmount_volume(const char *path,
							uint32_t flags);

#ifdef  __cplusplus
}
#endif

#endif	/* _FSSH_FS_VOLUME_H */
